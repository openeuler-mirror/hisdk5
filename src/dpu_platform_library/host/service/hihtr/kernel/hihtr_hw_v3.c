/*
 * Copyright (c) 2016-2017 Hisilicon Limited.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <rdma/ib_cache.h>
#include <linux/pci.h>
#include <linux/stat.h>
#include <uapi/linux/pci_regs.h>
#include "hihtr_cmdq.h"
#include "hihtr_hw_v3.h"
#include "hihtr_device.h"
#include "hinic5_srv_nic.h"
#include <net/addrconf.h>
#include "ossl_knl.h"
#include "hihtr_hem.h"
#include "hihtr_common.h"
#include "hihtr_cmd.h"
#include "hihtr_dca.h"
#include "hihtr_hw_v3_qp.h"
#include "hihtr_hw_v3_cq.h"
#include "hihtr_hw_v3_eq.h"
#include "hihtr_hw_v3_post.h"
#include "hihtr_hw_v3_srq.h"
#include "dfx/hihtr_dfx.h"
#include "roce_mpu_cmd.h"
#include "hihtr_debugfs.h"
#include <linux/kprobes.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/ptrace.h>
#if defined(RDMA_HAVE_XARRAY)
	#include <linux/xarray.h>
#else
	#include <linux/radix-tree.h>
#endif

static uint inline_type = 0;
module_param(inline_type, uint, 0444);
MODULE_PARM_DESC(inline_type, "set inline close/rq_inline/cqe_inline(0/1/2), default is 0");

static uint qp_mtt_buf_pg_sz = 0;
module_param(qp_mtt_buf_pg_sz, uint, 0444);
MODULE_PARM_DESC(qp_mtt_buf_pg_sz, "qp buffer page size of mtr, value is in [0, 4], default(0)");

static uint pbl_buf_pg_sz = 0;
module_param(pbl_buf_pg_sz, uint, 0444);
MODULE_PARM_DESC(pbl_buf_pg_sz, "mr buffer page size is 2**(pbl_buf_pg_sz+12)");

static uint cqe_mtt_buf_pg_sz = 0;
module_param(cqe_mtt_buf_pg_sz, uint, 0444);
MODULE_PARM_DESC(cqe_mtt_buf_pg_sz, "qp buffer page size of mtr, value is in [0, 4], default is 0");

static uint hop_num = 2;
module_param(hop_num, uint, 0444);
MODULE_PARM_DESC(hop_num, "hop num for qp buffer size, value is in [0, 2], default is 2");
/*-----------------------------------common utitilies ------------------------------------------------------------*/
#define HR_OPC_MAP(ib_key, hr_key) \
		[IB_WR_ ## ib_key] = 1 + HIHTR_V3_WQE_OP_ ## hr_key

static const u32 hihtr_op_code[] = {
	HR_OPC_MAP(RDMA_WRITE,			RDMA_WRITE),
	HR_OPC_MAP(RDMA_WRITE_WITH_IMM,		RDMA_WRITE_WITH_IMM),
	HR_OPC_MAP(SEND,			SEND),
	HR_OPC_MAP(SEND_WITH_IMM,		SEND_WITH_IMM),
	HR_OPC_MAP(RDMA_READ,			RDMA_READ),
	HR_OPC_MAP(ATOMIC_CMP_AND_SWP,		ATOM_CMP_AND_SWAP),
	HR_OPC_MAP(ATOMIC_FETCH_AND_ADD,	ATOM_FETCH_AND_ADD),
	HR_OPC_MAP(SEND_WITH_INV,		SEND_WITH_INV),
	HR_OPC_MAP(LOCAL_INV,			LOCAL_INV),
	HR_OPC_MAP(MASKED_ATOMIC_CMP_AND_SWP,	ATOM_MSK_CMP_AND_SWAP),
	HR_OPC_MAP(MASKED_ATOMIC_FETCH_AND_ADD,	ATOM_MSK_FETCH_AND_ADD),
	HR_OPC_MAP(REG_MR,			FAST_REG_PMR),
};

static int calc_wr_sge_num(const struct ib_send_wr *wr,
				    unsigned int *sge_len, unsigned int *valid_num_sge)
{
	unsigned int cnt_valid = 0;
	unsigned long long len = 0;
	int i;

	for (i = 0; i < wr->num_sge; i++) {
		if (likely(wr->sg_list[i].length)) {
			len += wr->sg_list[i].length;
			cnt_valid++;
		}
	}
	if (unlikely(len > HIHTR_INVALID_SGE_LENGTH)) {
		return -EINVAL;
	}

	*sge_len = (unsigned int)len;
	*valid_num_sge = cnt_valid;
	return 0;
}

static __le32 get_immtdata(const struct ib_send_wr *wr)
{
	switch (wr->opcode) {
	case IB_WR_SEND_WITH_IMM:
	case IB_WR_RDMA_WRITE_WITH_IMM:
		return cpu_to_le32(be32_to_cpu(wr->ex.imm_data));
	default:
		return 0;
	}
}

static int set_frmr_seg(struct hihtr_v3_rc_send_wqe *rc_sq_wqe,
			 const struct ib_reg_wr *wr)
{
	struct hihtr_wqe_frmr_seg *fseg =
		(void *)rc_sq_wqe + sizeof(struct hihtr_v3_rc_send_wqe);
	struct hihtr_mr *mr = to_hr_mr(wr->mr);
	u64 pbl_ba;
	int ret = 0;

	hr_reg_clear(fseg, FRMR_ZBVA);
	hr_reg_clear(fseg, FRMR_BLOCK_SIZE);

	/* Use ib_access_flags to set FRMR access permissions */
	hr_reg_write_bool(fseg, FRMR_BIND_EN, wr->access & IB_ACCESS_MW_BIND);
	hr_reg_write_bool(fseg, FRMR_ATOMIC,
			  wr->access & IB_ACCESS_REMOTE_ATOMIC);
	hr_reg_write_bool(fseg, FRMR_RR, wr->access & IB_ACCESS_REMOTE_READ);
	hr_reg_write_bool(fseg, FRMR_RW, wr->access & IB_ACCESS_REMOTE_WRITE);
	hr_reg_write_bool(fseg, FRMR_LW, wr->access & IB_ACCESS_LOCAL_WRITE);

	/* Data structure reuse may lead to confusion */
	pbl_ba = mr->pbl_mtr.hem_cfg.root_ba;
	rc_sq_wqe->msg_len = cpu_to_le32(lower_32_bits(pbl_ba));
	rc_sq_wqe->inv_key = cpu_to_le32(upper_32_bits(pbl_ba));

	rc_sq_wqe->byte_16 = cpu_to_le32(wr->mr->length & HIHTR_WQE_MR_LENGTH_L_MASK);
	rc_sq_wqe->byte_20 = cpu_to_le32(wr->mr->length >> HIHTR_WQE_MR_LENGTH_H_OFFSET);
	rc_sq_wqe->rkey = cpu_to_le32(wr->key);
	rc_sq_wqe->va = cpu_to_le64(wr->mr->iova);

	hr_reg_write(fseg, FRMR_PBL_SIZE, mr->npages);
	hr_reg_write(fseg, FRMR_PBL_BUF_PG_SZ,
		     to_hr_hw_page_shift(mr->pbl_mtr.hem_cfg.buf_pg_shift));
	hr_reg_clear(fseg, FRMR_BLK_MODE);
	return ret;
}

static u32 to_hr_opcode(u32 ib_opcode)
{
	if (ib_opcode >= ARRAY_SIZE(hihtr_op_code))
		return HIHTR_V3_WQE_OP_MASK;

	return hihtr_op_code[ib_opcode] ? hihtr_op_code[ib_opcode] - 1 :
					     HIHTR_V3_WQE_OP_MASK;
}

static int set_rc_opcode(struct hihtr_dev *hr_dev,
			 struct hihtr_v3_rc_send_wqe *rc_sq_wqe,
			 const struct ib_send_wr *wr)
{
	u32 ib_op = wr->opcode;
	int ret = 0;

	rc_sq_wqe->immtdata = get_immtdata(wr);

	switch (ib_op) {
	case IB_WR_RDMA_READ:
	case IB_WR_RDMA_WRITE:
	case IB_WR_RDMA_WRITE_WITH_IMM:
		rc_sq_wqe->rkey = cpu_to_le32(rdma_wr(wr)->rkey);
		rc_sq_wqe->va = cpu_to_le64(rdma_wr(wr)->remote_addr);
		break;
	case IB_WR_SEND:
	case IB_WR_SEND_WITH_IMM:
		break;
	case IB_WR_ATOMIC_CMP_AND_SWP:
	case IB_WR_ATOMIC_FETCH_AND_ADD:
		rc_sq_wqe->rkey = cpu_to_le32(atomic_wr(wr)->rkey);
		rc_sq_wqe->va = cpu_to_le64(atomic_wr(wr)->remote_addr);
		break;
	case IB_WR_REG_MR:
		if (HIHTR_GET_PCI_REVISION(hr_dev) >= PCI_REVISION_ID_HIP09) {
			ret = hihtr_check_mr_access(&hr_dev->ib_dev, reg_wr(wr)->access);
			if (ret) {
				dev_alert(hr_dev->dev, "mr access is not valid, errno = %d\n", ret);
				break;
			}
			ret = set_frmr_seg(rc_sq_wqe, reg_wr(wr));
		} else
			ret = -EOPNOTSUPP;
		break;
	case IB_WR_SEND_WITH_INV:
		rc_sq_wqe->inv_key = cpu_to_le32(wr->ex.invalidate_rkey);
		break;
	case IB_WR_LOCAL_INV:
		/*
		 * Hardware does not support LOCAL_INV opcode.
		 * Map to NOP WQE which hardware executes and produces a CQE.
		 * The invalidate_rkey is saved in qp->sq.inv_rkey[] by the
		 * post_send path so that poll_cq can retrieve it and perform
		 * MR invalidation via the slow path.
		 */
		rc_sq_wqe->inv_key = cpu_to_le32(wr->ex.invalidate_rkey);
		hr_reg_write(rc_sq_wqe, RC_SEND_WQE_OPCODE, HIHTR_V3_WQE_OP_NOP);
		return 0;
	default:
		ret = -EINVAL;
	}

	if (unlikely(ret))
		return ret;

	hr_reg_write(rc_sq_wqe, RC_SEND_WQE_OPCODE, to_hr_opcode(ib_op));

	return ret;
}

static inline void fill_dca_fields(struct hihtr_qp *hr_qp,
				   struct hihtr_v3_rc_send_wqe *wqe)
{
	hr_reg_write(wqe, RC_SEND_WQE_SQPN_L, hr_qp->qpn);
	hr_reg_write(wqe, RC_SEND_WQE_SQPN_H,
		     hr_qp->qpn >> V3_RC_SEND_WQE_BYTE_4_SQPN_L_W);
}

static inline void set_data_seg_v3(struct hihtr_v3_wqe_data_seg *dseg,
				   struct ib_sge *sg)
{
	dseg->lkey = cpu_to_le32(sg->lkey);
	dseg->addr = cpu_to_le64(sg->addr);
	dseg->len  = cpu_to_le32(sg->length);
}

static void set_extend_sge(struct hihtr_qp *qp, struct ib_sge *sge,
			   unsigned int *sge_ind, unsigned int cnt)
{
	struct hihtr_v3_wqe_data_seg *dseg;
	unsigned int idx = *sge_ind;

	while (cnt > 0) {
		dseg = hihtr_get_extend_sge(qp, idx & (qp->sge.sge_cnt - 1));
		if (likely(sge->length)) {
			set_data_seg_v3(dseg, sge);
			idx++;
			cnt--;
		}
		sge++;
	}

	*sge_ind = idx;
}

static int fill_ext_sge_inl_data(struct hihtr_qp *qp,
				 const struct ib_send_wr *wr,
				 unsigned int *sge_idx, u32 msg_len)
{
	struct ib_device *ibdev = &(to_hr_dev(qp->ibqp.device))->ib_dev;
	unsigned int left_len_in_pg;
	unsigned int idx = *sge_idx;
	unsigned int i = 0;
	unsigned int len;
	void *addr;
	void *dseg;

	if (msg_len > qp->sq.ext_sge_cnt * HIHTR_SGE_SIZE) {
		ibdev_err(ibdev,
			  "no enough extended sge space for inline data.\n");
		return -EINVAL;
	}

	dseg = hihtr_get_extend_sge(qp, idx & (qp->sge.sge_cnt - 1));
	left_len_in_pg = hr_hw_page_align((uintptr_t)dseg) - (uintptr_t)dseg;
	len = wr->sg_list[0].length;
	addr = (void *)(unsigned long)(wr->sg_list[0].addr);

	/* When copying data to extended sge space, the left length in page may
	 * not long enough for current user's sge. So the data should be
	 * splited into several parts, one in the first page, and the others in
	 * the subsequent pages.
	 */
	while (1) {
		if (len <= left_len_in_pg) {
			memcpy(dseg, addr, len);

			idx += len / HIHTR_SGE_SIZE;

			i++;
			if (i >= wr->num_sge)
				break;

			left_len_in_pg -= len;
			len = wr->sg_list[i].length;
			addr = (void *)(unsigned long)(wr->sg_list[i].addr);
			dseg += len;
		} else {
			memcpy(dseg, addr, left_len_in_pg);

			len -= left_len_in_pg;
			addr += left_len_in_pg;
			idx += left_len_in_pg / HIHTR_SGE_SIZE;
			dseg = hihtr_get_extend_sge(qp, idx & (qp->sge.sge_cnt - 1));
			left_len_in_pg = 1 << HIHTR_HW_PAGE_SHIFT;
		}
	}

	*sge_idx = idx;

	return 0;
}

static bool check_inl_data_len(struct hihtr_qp *qp, unsigned int len)
{
	struct hihtr_dev *hr_dev = to_hr_dev(qp->ibqp.device);
	int mtu = ib_mtu_enum_to_int(qp->path_mtu);
	if (len > qp->max_inline_data || len > mtu) {
		ibdev_err(&hr_dev->ib_dev,
			"invalid length of data, data len = %u, max inline len = %u, path mtu = %d.\n",
			len, qp->max_inline_data, mtu);
		return false;
	}

	return true;
}

static int set_rc_inl(struct hihtr_qp *qp, const struct ib_send_wr *wr,
		      struct hihtr_v3_rc_send_wqe *rc_sq_wqe,
		      unsigned int *sge_idx)
{
	struct hihtr_dev *hr_dev = to_hr_dev(qp->ibqp.device);
	u32 msg_len = le32_to_cpu(rc_sq_wqe->msg_len);
	struct ib_device *ibdev = &hr_dev->ib_dev;
	unsigned int curr_idx = *sge_idx;
	void *dseg = rc_sq_wqe;
	unsigned int i;
	int ret;

	if (unlikely(wr->opcode == IB_WR_RDMA_READ)) {
		ibdev_err(ibdev, "invalid inline parameters!\n");
		return -EINVAL;
	}

	if (!check_inl_data_len(qp, msg_len))
		return -EINVAL;

	dseg += sizeof(struct hihtr_v3_rc_send_wqe);

	if (msg_len <= HIHTR_V3_MAX_RC_INL_INN_SZ) {
		hr_reg_clear(rc_sq_wqe, RC_SEND_WQE_INL_TYPE);

		for (i = 0; i < wr->num_sge; i++) {
			memcpy(dseg, ((void *)wr->sg_list[i].addr),
			       wr->sg_list[i].length);
			dseg += wr->sg_list[i].length;
		}
	} else {
		hr_reg_enable(rc_sq_wqe, RC_SEND_WQE_INL_TYPE);

		ret = fill_ext_sge_inl_data(qp, wr, &curr_idx, msg_len);
		if (ret)
			return ret;

		hr_reg_write(rc_sq_wqe, RC_SEND_WQE_SGE_NUM, curr_idx - *sge_idx);
	}

	*sge_idx = curr_idx;

	return 0;
}

static int set_rwqe_data_seg(struct ib_qp *ibqp, const struct ib_send_wr *wr,
			     struct hihtr_v3_rc_send_wqe *rc_sq_wqe,
			     unsigned int *sge_ind,
			     unsigned int valid_num_sge)
{
	struct hihtr_v3_wqe_data_seg *dseg =
		(void *)rc_sq_wqe + sizeof(struct hihtr_v3_rc_send_wqe);
	struct hihtr_qp *qp = to_hr_qp(ibqp);
	int j = 0;
	int i;

	hr_reg_write(rc_sq_wqe, RC_SEND_WQE_MSG_START_SGE_IDX,
		     (*sge_ind) & (qp->sge.sge_cnt - 1));

	hr_reg_write(rc_sq_wqe, RC_SEND_WQE_INLINE,
		     !!(wr->send_flags & IB_SEND_INLINE));
	if (wr->send_flags & IB_SEND_INLINE)
		return set_rc_inl(qp, wr, rc_sq_wqe, sge_ind);

	if (valid_num_sge <= HIHTR_SGE_IN_WQE) {
		for (i = 0; i < wr->num_sge; i++) {
			if (likely(wr->sg_list[i].length)) {
				set_data_seg_v3(dseg, wr->sg_list + i);
				dseg++;
			}
		}
	} else {
		for (i = 0; i < wr->num_sge && j < HIHTR_SGE_IN_WQE; i++) {
			if (likely(wr->sg_list[i].length)) {
				set_data_seg_v3(dseg, wr->sg_list + i);
				dseg++;
				j++;
			}
		}

		set_extend_sge(qp, wr->sg_list + i, sge_ind,
			       valid_num_sge - HIHTR_SGE_IN_WQE);
	}

	hr_reg_write(rc_sq_wqe, RC_SEND_WQE_SGE_NUM, valid_num_sge);

	return 0;
}

static int set_atomic_seg(struct hihtr_dev *hr_dev,
			  const struct ib_send_wr *wr,
			  struct hihtr_v3_rc_send_wqe *rc_sq_wqe,
			  unsigned int valid_num_sge, u32 msg_len)
{
	struct hihtr_v3_wqe_data_seg *dseg =
		(void *)rc_sq_wqe + sizeof(struct hihtr_v3_rc_send_wqe);
	struct hihtr_wqe_atomic_seg *aseg =
		(void *)dseg + sizeof(struct hihtr_v3_wqe_data_seg);

	if (msg_len != ATOMIC_WR_LEN) {
		ibdev_err(&hr_dev->ib_dev, "invalid atomic wr len, len = %u.\n",
			  msg_len);
		return -EINVAL;
	}

	set_data_seg_v3(dseg, wr->sg_list);

	if (wr->opcode == IB_WR_ATOMIC_CMP_AND_SWP) {
		aseg->fetchadd_swap_data = cpu_to_le64(atomic_wr(wr)->swap);
		aseg->cmp_data = cpu_to_le64(atomic_wr(wr)->compare_add);
	} else {
		aseg->fetchadd_swap_data =
			cpu_to_le64(atomic_wr(wr)->compare_add);
		aseg->cmp_data = 0;
	}

	hr_reg_write(rc_sq_wqe, RC_SEND_WQE_SGE_NUM, valid_num_sge);

	return 0;
}

int set_rc_wqe(struct hihtr_qp *qp,
			     const struct ib_send_wr *wr,
			     void *wqe, unsigned int *sge_idx,
			     unsigned int owner_bit, unsigned int *msg_len)
{
	struct hihtr_dev *hr_dev = to_hr_dev(qp->ibqp.device);
	struct hihtr_v3_rc_send_wqe *rc_sq_wqe = wqe;
	unsigned int curr_idx = *sge_idx;
	unsigned int valid_num_sge;
	int ret;

	*msg_len = 0;
	ret = calc_wr_sge_num(wr, msg_len, &valid_num_sge);
	if (unlikely(ret)) {
		return ret;
	}

	rc_sq_wqe->msg_len = cpu_to_le32(*msg_len);

	ret = set_rc_opcode(hr_dev, rc_sq_wqe, wr);
	if (unlikely(ret))
		return ret;

	// fence改用SO强保序标识完成
	hr_reg_write(rc_sq_wqe, RC_SEND_WQE_SO,
		     (wr->send_flags & IB_SEND_FENCE) ? 1 : 0);

	hr_reg_write(rc_sq_wqe, RC_SEND_WQE_SE,
		     (wr->send_flags & IB_SEND_SOLICITED) ? 1 : 0);

	if (wr->opcode == IB_WR_LOCAL_INV) {
		hr_reg_write(rc_sq_wqe, RC_SEND_WQE_CQE, 1);
	} else {
		hr_reg_write(rc_sq_wqe, RC_SEND_WQE_CQE,
			     (wr->send_flags & IB_SEND_SIGNALED) ? 1 : 0);
	}

	if (wr->opcode == IB_WR_ATOMIC_CMP_AND_SWP ||
	    wr->opcode == IB_WR_ATOMIC_FETCH_AND_ADD) {
		ret = set_atomic_seg(hr_dev, wr, rc_sq_wqe, valid_num_sge, *msg_len);
    } else if (wr->opcode != IB_WR_REG_MR) {
        ret = set_rwqe_data_seg(&qp->ibqp, wr, rc_sq_wqe,
					&curr_idx, valid_num_sge);
    }

	if (qp->en_flags & HIHTR_QP_CAP_DYNAMIC_CTX_ATTACH)
		fill_dca_fields(qp, rc_sq_wqe);

	/*
	 * The pipeline can sequentially post all valid WQEs into WQ buffer,
	 * including new WQEs waiting for the doorbell to update the PI again.
	 * Therefore, the owner bit of WQE MUST be updated after all fields
	 * and extSGEs have been written into DDR instead of cache.
	 */
	if (qp->en_flags & HIHTR_QP_CAP_OWNER_DB)
		dma_wmb();

	*sge_idx = curr_idx;
	hr_reg_write(rc_sq_wqe, RC_SEND_WQE_OWNER, owner_bit);

	return ret;
}

static int set_ud_opcode(struct hihtr_v3_ud_send_wqe *ud_sq_wqe,
			 const struct ib_send_wr *wr)
{
	u32 ib_op = wr->opcode;

	if (ib_op != IB_WR_SEND && ib_op != IB_WR_SEND_WITH_IMM)
		return -EINVAL;

	ud_sq_wqe->immtdata = get_immtdata(wr);

	hr_reg_write(ud_sq_wqe, UD_SEND_WQE_OPCODE, to_hr_opcode(ib_op));

	return 0;
}

static int fill_ud_av(struct hihtr_v3_ud_send_wqe *ud_sq_wqe,
		      struct hihtr_ah *ah)
{
#ifdef HIHTR_DRIVER
	struct ib_device *ib_dev = ah->ibah.device;
	struct hihtr_dev *hr_dev = to_hr_dev(ib_dev);
#endif

	hr_reg_write(ud_sq_wqe, UD_SEND_WQE_UDPSPN, ah->av.udp_sport);
	hr_reg_write(ud_sq_wqe, UD_SEND_WQE_HOPLIMIT, ah->av.hop_limit);
	hr_reg_write(ud_sq_wqe, UD_SEND_WQE_TCLASS, ah->av.tclass);
	hr_reg_write(ud_sq_wqe, UD_SEND_WQE_FLOW_LABEL, ah->av.flowlabel);

	if (WARN_ON(ah->av.sl > MAX_SERVICE_LEVEL))
		return -EINVAL;

	hr_reg_write(ud_sq_wqe, UD_SEND_WQE_SL, ah->av.sl);

	ud_sq_wqe->sgid_index = ah->av.gid_index;

	memcpy(ud_sq_wqe->dmac, ah->av.mac, ETH_ALEN);
	memcpy(ud_sq_wqe->dgid, ah->av.dgid, GID_LEN_V2);

	if (HIHTR_GET_PCI_REVISION(hr_dev) >= PCI_REVISION_ID_HIP09)
		return 0;

	hr_reg_write(ud_sq_wqe, UD_SEND_WQE_VLAN_EN, ah->av.vlan_en);
	hr_reg_write(ud_sq_wqe, UD_SEND_WQE_VLAN, ah->av.vlan_id);

	return 0;
}

int set_ud_wqe(struct hihtr_qp *qp,
			     const struct ib_send_wr *wr,
			     void *wqe, unsigned int *sge_idx,
			     unsigned int owner_bit)
{
	struct hihtr_dev *hr_dev = to_hr_dev(qp->ibqp.device);
	struct hihtr_ah *ah = to_hr_ah(ud_wr(wr)->ah);
	struct hihtr_v3_ud_send_wqe *ud_sq_wqe = wqe;
	unsigned int curr_idx = *sge_idx;
	unsigned int valid_num_sge;
	u32 msg_len = 0;
	int ret;

	ret = calc_wr_sge_num(wr, &msg_len, &valid_num_sge);
	if (unlikely(ret)) {
		return ret;
	}

	ret = set_ud_opcode(ud_sq_wqe, wr);
	if (WARN_ON(ret))
		return ret;

	ud_sq_wqe->msg_len = cpu_to_le32(msg_len);

	hr_reg_write(ud_sq_wqe, UD_SEND_WQE_CQE,
		     !!(wr->send_flags & IB_SEND_SIGNALED));
	hr_reg_write(ud_sq_wqe, UD_SEND_WQE_SE,
		     !!(wr->send_flags & IB_SEND_SOLICITED));

	hr_reg_write(ud_sq_wqe, UD_SEND_WQE_PD, to_hr_pd(qp->ibqp.pd)->pdn);
	hr_reg_write(ud_sq_wqe, UD_SEND_WQE_SGE_NUM, valid_num_sge);
	hr_reg_write(ud_sq_wqe, UD_SEND_WQE_MSG_START_SGE_IDX,
		     curr_idx & (qp->sge.sge_cnt - 1));

	ud_sq_wqe->qkey = cpu_to_le32(ud_wr(wr)->remote_qkey & 0x80000000 ?
			  qp->qkey : ud_wr(wr)->remote_qkey);
	hr_reg_write(ud_sq_wqe, UD_SEND_WQE_DQPN, ud_wr(wr)->remote_qpn);

	ret = fill_ud_av(ud_sq_wqe, ah);
	if (ret)
		return ret;

	if (hr_dev->mac_type == HNAE5_MAC_ROH && qp->ibqp.qp_type == IB_QPT_GSI)
		ud_sq_wqe->dmac[0] = 0xFF;

	qp->sl = to_hr_ah(ud_wr(wr)->ah)->av.sl;

	set_extend_sge(qp, wr->sg_list, &curr_idx, valid_num_sge);

	/*
	 * The pipeline can sequentially post all valid WQEs into WQ buffer,
	 * including new WQEs waiting for the doorbell to update the PI again.
	 * Therefore, the owner bit of WQE MUST be updated after all fields
	 * and extSGEs have been written into DDR instead of cache.
	 */
	if (qp->en_flags & HIHTR_QP_CAP_OWNER_DB)
		dma_wmb();

	*sge_idx = curr_idx;
	hr_reg_write(ud_sq_wqe, UD_SEND_WQE_OWNER, owner_bit);

	return 0;
}

static int load_func_res_caps(struct hihtr_dev *hr_dev, bool is_vf)
{
	struct hihtr_cmq_desc desc[3];
	struct hihtr_cmq_req *r_a = (struct hihtr_cmq_req *)desc[0].data;
	struct hihtr_cmq_req *r_b = (struct hihtr_cmq_req *)desc[1].data;
	struct hihtr_caps *caps = &hr_dev->caps;
	enum hihtr_opcode_type opcode;
	u32 func_num;
	int ret;

	if (is_vf) {
		opcode = HIHTR_OPC_QUERY_VF_RES;
		func_num = 1;
	} else {
		opcode = HIHTR_OPC_QUERY_PF_RES;
		func_num = hr_dev->func_num;
	}

	hihtr_cmq_setup_basic_desc(&desc[HIHTR_CMD_DESC_NUM_0], opcode, true);
	desc[0].flag |= cpu_to_le16(HIHTR_CMD_FLAG_NEXT);
	hihtr_cmq_setup_basic_desc(&desc[HIHTR_CMD_DESC_NUM_1], opcode, true);
	desc[1].flag |= cpu_to_le16(HIHTR_CMD_FLAG_NEXT);
	hihtr_cmq_setup_basic_desc(&desc[HIHTR_CMD_DESC_NUM_2], opcode, true);

	ret = hihtr5_cmq_send(hr_dev, desc, HIHTR_CMD_DESC_NUM_3); // 1872 cmq仅支持1, 3, 5 个weqnum
	if (ret)
		return ret;

	caps->qpc_bt_num = hr_reg_read(r_a, FUNC_RES_A_QPC_BT_NUM) / func_num;
	caps->srqc_bt_num = hr_reg_read(r_a, FUNC_RES_A_SRQC_BT_NUM) / func_num;
	caps->cqc_bt_num = hr_reg_read(r_a, FUNC_RES_A_CQC_BT_NUM) / func_num;
	caps->mpt_bt_num = hr_reg_read(r_a, FUNC_RES_A_MPT_BT_NUM) / func_num;
	caps->eqc_bt_num = hr_reg_read(r_a, FUNC_RES_A_EQC_BT_NUM) / func_num;
	caps->smac_bt_num = hr_reg_read(r_b, FUNC_RES_B_SMAC_NUM) / func_num;
	caps->sgid_bt_num = hr_reg_read(r_b, FUNC_RES_B_SGID_NUM) / func_num;
	caps->sccc_bt_num = hr_reg_read(r_b, FUNC_RES_B_SCCC_BT_NUM) / func_num;

	/* 当func为vf时，cmdq直接返回sl num;
	 * 当func为pf时，cmdq返回该func被分配的queue num。
	 */
	if (is_vf) {
		caps->sl_num = hr_reg_read(r_b, FUNC_RES_V_QID_NUM);
		caps->gmv_bt_num = hr_reg_read(r_b, FUNC_RES_V_GMV_BT_NUM) /
					       func_num;
	} else {
		caps->sl_num = (hr_reg_read(r_b, FUNC_RES_B_QID_NUM) - HIHTR_COS_MIN) / func_num;
		caps->gmv_bt_num = hr_reg_read(r_b, FUNC_RES_B_GMV_BT_NUM) /
					       func_num;
	}
	return 0;
}

static int query_func_resource_caps(struct hihtr_dev *hr_dev, bool is_vf)
{
	struct device *dev = hr_dev->dev;
	int ret;

	ret = load_func_res_caps(hr_dev, is_vf);
	if (ret) {
		dev_err(dev, "failed to load res caps, ret = %d (%s).\n", ret,
			is_vf ? "vf" : "pf");
		return ret;
	}

	return ret;
}

static int load_pf_timer_res_caps(struct hihtr_dev *hr_dev)
{
	struct hihtr_cmq_desc desc;
	struct hihtr_cmq_req *req = (struct hihtr_cmq_req *)desc.data;
	struct hihtr_caps *caps = &hr_dev->caps;
	int ret;

	hihtr_cmq_setup_basic_desc(&desc, HIHTR_OPC_QUERY_PF_TIMER_RES,
				      true);

	ret = hihtr5_cmq_send(hr_dev, &desc, 1);
	if (ret)
		return ret;

	caps->qpc_timer_bt_num = hr_reg_read(req, PF_TIMER_RES_QPC_ITEM_NUM);
	caps->cqc_timer_bt_num = hr_reg_read(req, PF_TIMER_RES_CQC_ITEM_NUM);
	dev_info(hr_dev->dev, "qpc, 0x%x, cqc 0x%x\r\n", caps->qpc_timer_bt_num, caps->cqc_timer_bt_num); // TODO: 后续删除
	return 0;
}

static int hihtr_query_pf_resource(struct hihtr_dev *hr_dev)
{
	struct device *dev = hr_dev->dev;
	int ret;

	ret = query_func_resource_caps(hr_dev, false);
	if (ret)
		return ret;

	ret = load_pf_timer_res_caps(hr_dev);
	if (ret)
		dev_err(dev, "failed to load pf timer resource, ret = %d.\n",
			ret);

	return ret;
}

static void v3_calc_pg_sz(u32 obj_num, u32 obj_size, u32 hop_num, u32 ctx_bt_num,
		       u32 *buf_page_size, u32 *bt_page_size, u32 hem_type)
{
	u64 obj_per_chunk;
	u64 bt_chunk_size = PAGE_SIZE;
	u64 buf_chunk_size = PAGE_SIZE;
	u64 obj_per_chunk_default = buf_chunk_size / obj_size;

	*buf_page_size = 0;
	*bt_page_size = 0;

	switch (hop_num) {
	case HOP_NUM_3:
		obj_per_chunk = ctx_bt_num * (bt_chunk_size / BA_BYTE_LEN) *
				(bt_chunk_size / BA_BYTE_LEN) *
				(bt_chunk_size / BA_BYTE_LEN) *
				 obj_per_chunk_default;
		break;
	case HOP_NUM_2:
		obj_per_chunk = ctx_bt_num * (bt_chunk_size / BA_BYTE_LEN) *
				(bt_chunk_size / BA_BYTE_LEN) *
				 obj_per_chunk_default;
		break;
	case HOP_NUM_1:
		obj_per_chunk = ctx_bt_num * (bt_chunk_size / BA_BYTE_LEN) *
				obj_per_chunk_default;
		break;
	case HIHTR_HOP_NUM_0:
		obj_per_chunk = ctx_bt_num * obj_per_chunk_default;
		break;
	default:
		pr_err("table %u not support hop_num = %u!\n", hem_type,
		       hop_num);
		return;
	}

	if (hem_type >= HEM_TYPE_MTT)
		*bt_page_size = ilog2(DIV_ROUND_UP(obj_num, obj_per_chunk));
	else
		*buf_page_size = ilog2(DIV_ROUND_UP(obj_num, obj_per_chunk));
}

static void v3_set_hem_page_size(struct hihtr_dev *hr_dev)
{
	struct hihtr_caps *caps = &hr_dev->caps;

	/* EQ */
	caps->eqe_ba_pg_sz = 0;
	caps->eqe_buf_pg_sz = 0;

	/* Link Table */
	caps->llm_buf_pg_sz = 0;

	/* MR */
	caps->mpt_ba_pg_sz = 0;
	caps->mpt_buf_pg_sz = 0;
	caps->pbl_ba_pg_sz = HIHTR_BA_PG_SZ_SUPPORTED_16K;
	caps->pbl_buf_pg_sz = pbl_buf_pg_sz;
	v3_calc_pg_sz(caps->num_mtpts, caps->mtpt_entry_sz, caps->mpt_hop_num,
		   caps->mpt_bt_num, &caps->mpt_buf_pg_sz, &caps->mpt_ba_pg_sz,
		   HEM_TYPE_MTPT);

	/* QP */
	caps->qpc_ba_pg_sz = 0;
	caps->qpc_buf_pg_sz = 0;
	caps->qpc_timer_ba_pg_sz = 0;
	caps->qpc_timer_buf_pg_sz = 0;
	caps->sccc_ba_pg_sz = 0;
	caps->sccc_buf_pg_sz = 0;
	caps->mtt_ba_pg_sz = 0;
	caps->mtt_buf_pg_sz = qp_mtt_buf_pg_sz;
	if (qp_mtt_buf_pg_sz < 0 || qp_mtt_buf_pg_sz > HIHTR_QP_MTT_BUF_PG_SZ_MAX) {
		dev_alert(hr_dev->dev, "qp_mtt_buf_pg_sz is not a valid value, fallback to be default value 0\n");
		caps->mtt_buf_pg_sz = 0;
	}
	v3_calc_pg_sz(caps->num_qps, caps->qpc_sz, caps->qpc_hop_num,
		   caps->qpc_bt_num, &caps->qpc_buf_pg_sz, &caps->qpc_ba_pg_sz,
		   HEM_TYPE_QPC);

	v3_calc_pg_sz(caps->num_qps, caps->sccc_sz, caps->sccc_hop_num,
			caps->sccc_bt_num, &caps->sccc_buf_pg_sz,
			&caps->sccc_ba_pg_sz, HEM_TYPE_SCCC);

	/* CQ */
	caps->cqc_ba_pg_sz = 0;
	caps->cqc_buf_pg_sz = 0;
	caps->cqc_timer_ba_pg_sz = 0;
	caps->cqc_timer_buf_pg_sz = 0;
	caps->cqe_ba_pg_sz = HIHTR_BA_PG_SZ_SUPPORTED_256K;
	caps->cqe_buf_pg_sz = cqe_mtt_buf_pg_sz;
	if (cqe_mtt_buf_pg_sz < 0 || cqe_mtt_buf_pg_sz > HIHTR_QP_MTT_BUF_PG_SZ_MAX) {
		dev_alert(hr_dev->dev, "cqe_mtt_buf_pg_sz is not a valid value, fallback to be default value 0\n");
		caps->cqe_buf_pg_sz = 0;
	}
	v3_calc_pg_sz(caps->num_cqs, caps->cqc_entry_sz, caps->cqc_hop_num,
		   caps->cqc_bt_num, &caps->cqc_buf_pg_sz, &caps->cqc_ba_pg_sz,
		   HEM_TYPE_CQC);
	v3_calc_pg_sz(caps->max_cqes, caps->cqe_sz, caps->cqe_hop_num,
		   1, &caps->cqe_buf_pg_sz, &caps->cqe_ba_pg_sz, HEM_TYPE_CQE);

	/* SRQ */
	if (caps->flags & HIHTR_CAP_FLAG_SRQ) {
		caps->srqc_ba_pg_sz = 0;
		caps->srqc_buf_pg_sz = 0;
		caps->srqwqe_ba_pg_sz = 0;
		caps->srqwqe_buf_pg_sz = 0;
		caps->idx_ba_pg_sz = 0;
		caps->idx_buf_pg_sz = 0;
		v3_calc_pg_sz(caps->num_srqs, caps->srqc_entry_sz,
			   caps->srqc_hop_num, caps->srqc_bt_num,
			   &caps->srqc_buf_pg_sz, &caps->srqc_ba_pg_sz,
			   HEM_TYPE_SRQC);
		v3_calc_pg_sz(caps->num_srqwqe_segs, caps->mtt_entry_sz,
			   caps->srqwqe_hop_num, 1, &caps->srqwqe_buf_pg_sz,
			   &caps->srqwqe_ba_pg_sz, HEM_TYPE_SRQWQE);
		v3_calc_pg_sz(caps->num_idx_segs, caps->idx_entry_sz,
			   caps->idx_hop_num, 1, &caps->idx_buf_pg_sz,
			   &caps->idx_ba_pg_sz, HEM_TYPE_IDX);
	}

	/* GMV */
	caps->gmv_ba_pg_sz = 0;
	caps->gmv_buf_pg_sz = 0;
}

/* Apply all loaded caps before setting to hardware */
static void v3_apply_func_caps(struct hihtr_dev *hr_dev)
{
	struct hihtr_caps *caps = &hr_dev->caps;

	/* The following configurations don't need to be got from firmware. */
	caps->qpc_timer_entry_sz = HIHTR_V2_QPC_TIMER_ENTRY_SZ;
	caps->cqc_timer_entry_sz = HIHTR_V2_CQC_TIMER_ENTRY_SZ;
	caps->mtt_entry_sz = HIHTR_V2_MTT_ENTRY_SZ;

	caps->pbl_hop_num = HIHTR_PBL_HOP_NUM;
	caps->qpc_timer_hop_num = HIHTR_HOP_NUM_0;
	caps->cqc_timer_hop_num = HIHTR_HOP_NUM_0;

	caps->reserved_xrcds = HIHTR_V2_RSV_XRCD_NUM;

	caps->num_mtt_segs = HIHTR_V2_MAX_MTT_SEGS;
	caps->num_srqwqe_segs = HIHTR_V2_MAX_SRQWQE_SEGS;
	caps->num_idx_segs = HIHTR_V2_MAX_IDX_SEGS;

	if (!caps->num_comp_vectors)
		caps->num_comp_vectors = min_t(u32, caps->eqc_bt_num - 1,
				 0);

	if (HIHTR_GET_PCI_REVISION(hr_dev) >= PCI_REVISION_ID_HIP09) {
		caps->eqe_hop_num = HIHTR_V3_EQE_HOP_NUM;
		caps->ceqe_size = HIHTR_V3_EQE_SIZE;
		caps->aeqe_size = HIHTR_V3_EQE_SIZE;

		/* The following configurations will be overwritten */
		caps->qpc_sz = HIHTR_V3_QPC_SZ;
		caps->cqe_sz = HIHTR_V3_CQE_SIZE;
		caps->sccc_sz = HIHTR_V3_SCCC_SZ;

		/* The following configurations are not got from firmware */
		caps->gmv_entry_sz = HIHTR_V3_GMV_ENTRY_SZ;

		caps->gmv_hop_num = HIHTR_HOP_NUM_0;
		caps->gid_table_len[0] = caps->gmv_bt_num *
					(HIHTR_HW_PAGE_SIZE / caps->gmv_entry_sz);

		caps->gmv_entry_num = caps->gmv_bt_num * (HIHTR_HW_PAGE_SIZE  /
							  caps->gmv_entry_sz);
	} else {
		u32 func_num = max_t(u32, 1, hr_dev->func_num);

		caps->eqe_hop_num = HIHTR_V2_EQE_HOP_NUM;
		caps->ceqe_size = HIHTR_CEQE_SIZE;
		caps->aeqe_size = HIHTR_AEQE_SIZE;
		caps->gid_table_len[0] /= func_num;
	}

	if (hr_dev->is_vf) {
		caps->default_aeq_arm_st = 0x3;
		caps->default_ceq_arm_st = 0x3;
		caps->default_ceq_max_cnt = 0x1;
		caps->default_ceq_period = 0x10;
		caps->default_aeq_max_cnt = 0x1;
		caps->default_aeq_period = 0x10;
	}

	v3_set_hem_page_size(hr_dev);
}

static int config_vf_hem_resource(struct hihtr_dev *hr_dev, int vf_id)
{
	struct hihtr_cmq_desc desc[HIHTR_CMD_DESC_NUM_3];
	struct hihtr_cmq_req *r_a = (struct hihtr_cmq_req *)desc[HIHTR_CMD_DESC_NUM_0].data;
	struct hihtr_cmq_req *r_b = (struct hihtr_cmq_req *)desc[HIHTR_CMD_DESC_NUM_1].data;
	enum hihtr_opcode_type opcode = HIHTR_OPC_ALLOC_VF_RES;
	struct hihtr_caps *caps = &hr_dev->caps;

	hihtr_cmq_setup_basic_desc(&desc[HIHTR_CMD_DESC_NUM_0], opcode, false);
	desc[0].flag |= cpu_to_le16(HIHTR_CMD_FLAG_NEXT);
	hihtr_cmq_setup_basic_desc(&desc[HIHTR_CMD_DESC_NUM_1], opcode, false);
	desc[1].flag |= cpu_to_le16(HIHTR_CMD_FLAG_NEXT);
	hihtr_cmq_setup_basic_desc(&desc[HIHTR_CMD_DESC_NUM_2], opcode, false);

	hr_reg_write(r_a, FUNC_RES_A_VF_ID, vf_id);
	hr_reg_write(r_a, FUNC_RES_A_QPC_BT_NUM, caps->qpc_bt_num);
	hr_reg_write(r_a, FUNC_RES_A_QPC_BT_IDX, vf_id * caps->qpc_bt_num);
	hr_reg_write(r_a, FUNC_RES_A_SRQC_BT_NUM, caps->srqc_bt_num);
	hr_reg_write(r_a, FUNC_RES_A_SRQC_BT_IDX, vf_id * caps->srqc_bt_num);
	hr_reg_write(r_a, FUNC_RES_A_CQC_BT_NUM, caps->cqc_bt_num);
	hr_reg_write(r_a, FUNC_RES_A_CQC_BT_IDX, vf_id * caps->cqc_bt_num);
	hr_reg_write(r_a, FUNC_RES_A_MPT_BT_NUM, caps->mpt_bt_num);
	hr_reg_write(r_a, FUNC_RES_A_MPT_BT_IDX, vf_id * caps->mpt_bt_num);
	hr_reg_write(r_a, FUNC_RES_A_EQC_BT_NUM, caps->eqc_bt_num);
	hr_reg_write(r_a, FUNC_RES_A_EQC_BT_IDX, vf_id * caps->eqc_bt_num);
	hr_reg_write(r_b, FUNC_RES_V_QID_NUM, caps->sl_num);
	hr_reg_write(r_b, FUNC_RES_B_QID_IDX, vf_id * caps->sl_num);
	hr_reg_write(r_b, FUNC_RES_B_SCCC_BT_NUM, caps->sccc_bt_num);
	hr_reg_write(r_b, FUNC_RES_B_SCCC_BT_IDX, vf_id * caps->sccc_bt_num);

	if (HIHTR_GET_PCI_REVISION(hr_dev) >= PCI_REVISION_ID_HIP09) {
		hr_reg_write(r_b, FUNC_RES_V_GMV_BT_NUM, caps->gmv_bt_num);
		hr_reg_write(r_b, FUNC_RES_B_GMV_BT_IDX,
			     vf_id * caps->gmv_bt_num);
	} else {
		hr_reg_write(r_b, FUNC_RES_B_SGID_NUM, caps->sgid_bt_num);
		hr_reg_write(r_b, FUNC_RES_B_SGID_IDX,
			     vf_id * caps->sgid_bt_num);
		hr_reg_write(r_b, FUNC_RES_B_SMAC_NUM, caps->smac_bt_num);
		hr_reg_write(r_b, FUNC_RES_B_SMAC_IDX,
			     vf_id * caps->smac_bt_num);
	}

	return hihtr5_cmq_send(hr_dev, desc, HIHTR_CMD_DESC_NUM_3);
}

static int hihtr_alloc_vf_resource(struct hihtr_dev *hr_dev)
{
	u32 func_num = max_t(u32, 1, hr_dev->func_num);
	u32 vf_id;
	int ret;

	for (vf_id = 0; vf_id < func_num; vf_id++) {
		ret = config_vf_hem_resource(hr_dev, vf_id);
		if (ret) {
			dev_err(hr_dev->dev,
				"failed to config vf-%u hem res, ret = %d.\n",
				vf_id, ret);
			return ret;
		}
	}

	return 0;
}

static int hihtr_query_vf_resource(struct hihtr_dev *hr_dev)
{
	return query_func_resource_caps(hr_dev, true);
}

static int hihtr_v3_set_bt(struct hihtr_dev *hr_dev)
{
	struct hihtr_cmq_desc desc;
	struct hihtr_cmq_req *req = (struct hihtr_cmq_req *)desc.data;
	struct hihtr_caps *caps = &hr_dev->caps;

	hihtr_cmq_setup_basic_desc(&desc, HIHTR_OPC_CFG_BT_ATTR, false);

	hr_reg_write(req, CFG_BT_ATTR_QPC_BA_PGSZ,
		     caps->qpc_ba_pg_sz + PG_SHIFT_OFFSET);
	hr_reg_write(req, CFG_BT_ATTR_QPC_BUF_PGSZ,
		     caps->qpc_buf_pg_sz + PG_SHIFT_OFFSET);
	hr_reg_write(req, CFG_BT_ATTR_QPC_HOPNUM,
		     to_hr_hem_hopnum(caps->qpc_hop_num, caps->num_qps));

	hr_reg_write(req, CFG_BT_ATTR_SRQC_BA_PGSZ,
		     caps->srqc_ba_pg_sz + PG_SHIFT_OFFSET);
	hr_reg_write(req, CFG_BT_ATTR_SRQC_BUF_PGSZ,
		     caps->srqc_buf_pg_sz + PG_SHIFT_OFFSET);
	hr_reg_write(req, CFG_BT_ATTR_SRQC_HOPNUM,
		     to_hr_hem_hopnum(caps->srqc_hop_num, caps->num_srqs));

	hr_reg_write(req, CFG_BT_ATTR_CQC_BA_PGSZ,
		     caps->cqc_ba_pg_sz + PG_SHIFT_OFFSET);
	hr_reg_write(req, CFG_BT_ATTR_CQC_BUF_PGSZ,
		     caps->cqc_buf_pg_sz + PG_SHIFT_OFFSET);
	hr_reg_write(req, CFG_BT_ATTR_CQC_HOPNUM,
		     to_hr_hem_hopnum(caps->cqc_hop_num, caps->num_cqs));

	hr_reg_write(req, CFG_BT_ATTR_MPT_BA_PGSZ,
		     caps->mpt_ba_pg_sz + PG_SHIFT_OFFSET);
	hr_reg_write(req, CFG_BT_ATTR_MPT_BUF_PGSZ,
		     caps->mpt_buf_pg_sz + PG_SHIFT_OFFSET);
	hr_reg_write(req, CFG_BT_ATTR_MPT_HOPNUM,
		     to_hr_hem_hopnum(caps->mpt_hop_num, caps->num_mtpts));

	hr_reg_write(req, CFG_BT_ATTR_SCCC_BA_PGSZ,
		     caps->sccc_ba_pg_sz + PG_SHIFT_OFFSET);
	hr_reg_write(req, CFG_BT_ATTR_SCCC_BUF_PGSZ,
		     caps->sccc_buf_pg_sz + PG_SHIFT_OFFSET);
	hr_reg_write(req, CFG_BT_ATTR_SCCC_HOPNUM,
		     to_hr_hem_hopnum(caps->sccc_hop_num, caps->num_qps));

	return hihtr5_cmq_send(hr_dev, &desc, 1);
}

static int hihtr_query_func_info(struct hihtr_dev *hr_dev)
{
	struct hihtr_cmq_desc desc;
	int ret;

	if (HIHTR_GET_PCI_REVISION(hr_dev) == PCI_REVISION_ID_HIP08) {
		hr_dev->func_num = 1;
		return 0;
	}

	hihtr_cmq_setup_basic_desc(&desc, HIHTR_OPC_QUERY_FUNC_INFO,
				      true);
	ret = hihtr5_cmq_send(hr_dev, &desc, 1);
	if (ret) {
		hr_dev->func_num = 1;
		return ret;
	}

	hr_dev->func_num = le32_to_cpu(desc.func_info.own_func_num);
	hr_dev->cong_algo_tmpl_id = le32_to_cpu(desc.func_info.own_mac_id);

	return 0;
}

static int hihtr_hw_v3_query_type_counter(struct hihtr_dev *hr_dev, int roce_wqebb_num,
				u64 *stats, int *num_counters, int counter_num, enum hihtr_opcode_type opcode)
{
#define CNT_PER_DESC 8
    struct hihtr_cmq_desc *desc;
    int bd_idx, cnt_idx;
    __le32 *cnt_data;
    int i;
    int ret;
    desc = kvcalloc(roce_wqebb_num, sizeof(*desc), GFP_KERNEL);
    if (!desc)
        return ENOMEM;

    for (i = 0; i < roce_wqebb_num; i++) {
        hihtr_cmq_setup_basic_desc(&desc[i], opcode, true);
		if (i != roce_wqebb_num - 1)
            desc[i].flag |= cpu_to_le16(HIHTR_CMD_FLAG_NEXT);
	}

    ret = hihtr5_cmq_send(hr_dev, desc, roce_wqebb_num);
    if (ret) {
        ibdev_err(&hr_dev->ib_dev,
			  "failed to get counter, ret = %d.\n", ret);
		goto err_out;
    }

    for (i = 0; i < roce_wqebb_num * CNT_PER_DESC && i < counter_num; i++) {
        bd_idx = i / CNT_PER_DESC;
        cnt_data = (__le32 *)&desc[bd_idx];
        cnt_idx = i % CNT_PER_DESC;
		stats[i + *num_counters] = le32_to_cpu(cnt_data[cnt_idx]);
	}
	*num_counters += i;

err_out:
	kvfree(desc);
	return ret;
}

STATIC int hihtr_hw_v3_query_counter(struct hihtr_dev *hr_dev,
					u64 *stats, u32 port, int *num_counters)
{
#define ROCE_WQEBB_RX_NUM 3
#define ROCE_HW_RX_CNT_NUM 19
#define ROCE_WQEBB_TX_NUM 5
#define ROCE_HW_TX_CNT_NUM 34
#define ROCE_WQEBB_ERR_NUM 1
#define ROCE_HW_ERR_CNT_NUM 8
#define ROCE_WQEBB_OTHER_NUM 1
#define ROCE_HW_OTHER_CNT_NUM 6
    int ret;

 	if (port > hr_dev->caps.num_ports)
        return -EINVAL;

	*num_counters = 0;
    ret = hihtr_hw_v3_query_type_counter(hr_dev, ROCE_WQEBB_RX_NUM, stats, num_counters,
								ROCE_HW_RX_CNT_NUM, HIHTR_OPC_QUERY_HW_RX_COUNTER);
    if (ret) {
        dev_err(hr_dev->dev, "failed to query rx counter, ret = %d\n", ret);
        return ret;
    }

    ret = hihtr_hw_v3_query_type_counter(hr_dev, ROCE_WQEBB_TX_NUM, stats, num_counters,
								ROCE_HW_TX_CNT_NUM, HIHTR_OPC_QUERY_HW_TX_COUNTER);
    if (ret) {
        dev_err(hr_dev->dev, "failed to query tx counter, ret = %d\n", ret);
        return ret;
    }

    ret = hihtr_hw_v3_query_type_counter(hr_dev, ROCE_WQEBB_ERR_NUM, stats, num_counters,
								ROCE_HW_ERR_CNT_NUM, HIHTR_OPC_QUERY_HW_ERR_COUNTER);
    if (ret) {
        dev_err(hr_dev->dev, "failed to query err counter, ret = %d\n", ret);
        return ret;
    }

    ret = hihtr_hw_v3_query_type_counter(hr_dev, ROCE_WQEBB_OTHER_NUM, stats, num_counters,
								ROCE_HW_OTHER_CNT_NUM, HIHTR_OPC_QUERY_HW_OTHER_COUNTER);
    if (ret) {
        dev_err(hr_dev->dev, "failed to query other counter, ret = %d\n", ret);
        return ret;
    }

    return ret;
}

static int hihtr_hw_v3_query_dp_counter(struct hihtr_dev *hr_dev, int roce_wqebb_num,
				u32 *stats, int *num_counters, int counter_num, enum hihtr_opcode_type opcode)
{
#define U32_CNT_PER_DESC 8
#define U64_CNT_PER_DESC 4

    struct hihtr_cmq_desc *desc;
    int bd_idx, cnt_idx, i, ret;
    __le32 *cnt_data;
	u64* txdma_stats = NULL;
	__le64 *txdma_cnt_data;

    desc = kvcalloc(roce_wqebb_num, sizeof(*desc), GFP_KERNEL);
    if (!desc)
        return ENOMEM;

    for (i = 0; i < roce_wqebb_num; i++) {
        hihtr_cmq_setup_basic_desc(&desc[i], opcode, true);
		if (i != roce_wqebb_num - 1)
            desc[i].flag |= cpu_to_le16(HIHTR_CMD_FLAG_NEXT);
	}

    ret = hihtr5_cmq_send(hr_dev, desc, roce_wqebb_num);
    if (ret) {
        ibdev_err(&hr_dev->ib_dev, "failed to get counter, ret = %d.\n", ret);
		goto err_out;
    }

	/* TXDMA数据为64位，需要单独处理 */
	if (opcode == HIHTR_OPC_QUERY_TXDMA_COUNTER) {
		// 获取首地址
		txdma_stats = (u64*)&stats[*num_counters];
		for (i = 0; i < roce_wqebb_num * U64_CNT_PER_DESC && i < counter_num / (U32_CNT_PER_DESC / U64_CNT_PER_DESC); i++) {
			bd_idx = i / U64_CNT_PER_DESC;
			txdma_cnt_data = (__le64 *)&desc[bd_idx];
			cnt_idx = i % U64_CNT_PER_DESC;
			txdma_stats[i] = txdma_cnt_data[cnt_idx];
		}
		*num_counters += (i * (U32_CNT_PER_DESC / U64_CNT_PER_DESC));
	} else {
		for (i = 0; i < roce_wqebb_num * CNT_PER_DESC && i < counter_num; i++) {
			bd_idx = i / CNT_PER_DESC;
			cnt_data = (__le32 *)&desc[bd_idx];
			cnt_idx = i % CNT_PER_DESC;
			stats[i + *num_counters] = le32_to_cpu(cnt_data[cnt_idx]);
		}
		*num_counters += i;
	}
err_out:
	kvfree(desc);
	return ret;
}

STATIC int hihtr_hw_v3_query_dp_pkt_counter(struct hihtr_dev *hr_dev,
					struct hihtr_data_path_pkt_cnt *dp_pkt_cnt, int *num_counters)
{
#define ROCE_WQEBB_RDMA_RX_NUM 3
#define ROCE_RDMA_RX_CNT_NUM 24
#define ROCE_WQEBB_RDMA_TX_NUM 3
#define ROCE_RDMA_TX_CNT_NUM 24
#define ROCE_WQEBB_TXDMA_NUM 5
#define ROCE_TXDMA_CNT_NUM 32
#define ROCE_WQEBB_IPSU_NUM 5
#define ROCE_IPSU_CNT_NUM 32
#define ROCE_WQEBB_CPB_IN_NUM 3
#define ROCE_CPB_IN_CNT_NUM 19
#define ROCE_WQEBB_CPB_OUT_NUM 3
#define ROCE_CPB_OUT_CNT_NUM 24
#define ROCE_WQEBB_PE_NUM 3
#define ROCE_PE_CNT_NUM 22

    int ret;
	*num_counters = 0;

    ret = hihtr_hw_v3_query_dp_counter(hr_dev, ROCE_WQEBB_RDMA_RX_NUM, (u32*)dp_pkt_cnt, num_counters,
								ROCE_RDMA_RX_CNT_NUM, HIHTR_OPC_QUERY_RDMA_RX_COUNTER);
    if (ret) {
        dev_err(hr_dev->dev, "failed to query rdma rx counter, ret = %d\n", ret);
        return ret;
    }
    ret = hihtr_hw_v3_query_dp_counter(hr_dev, ROCE_WQEBB_TX_NUM, (u32*)dp_pkt_cnt, num_counters,
								ROCE_RDMA_TX_CNT_NUM, HIHTR_OPC_QUERY_RDMA_TX_COUNTER);
    if (ret) {
        dev_err(hr_dev->dev, "failed to query rdma tx counter, ret = %d\n", ret);
        return ret;
    }

	ret = hihtr_hw_v3_query_dp_counter(hr_dev, ROCE_WQEBB_TXDMA_NUM, (u32*)dp_pkt_cnt, num_counters,
								ROCE_TXDMA_CNT_NUM, HIHTR_OPC_QUERY_TXDMA_COUNTER);
    if (ret) {
        dev_err(hr_dev->dev, "failed to query txdma counter, ret = %d\n", ret);
		return ret;
    }

    ret = hihtr_hw_v3_query_dp_counter(hr_dev, ROCE_WQEBB_IPSU_NUM, (u32*)dp_pkt_cnt, num_counters,
								ROCE_IPSU_CNT_NUM, HIHTR_OPC_QUERY_IPSU_COUNTER);
    if (ret) {
        dev_err(hr_dev->dev, "failed to query ipsu counter, ret = %d\n", ret);
        return ret;
    }

    ret = hihtr_hw_v3_query_dp_counter(hr_dev, ROCE_WQEBB_CPB_IN_NUM, (u32*)dp_pkt_cnt, num_counters,
								ROCE_CPB_IN_CNT_NUM, HIHTR_OPC_QUERY_CPB_IN_COUNTER);
    if (ret) {
        dev_err(hr_dev->dev, "failed to query cpb in counter, ret = %d\n", ret);
        return ret;
    }

    ret = hihtr_hw_v3_query_dp_counter(hr_dev, ROCE_WQEBB_CPB_OUT_NUM, (u32*)dp_pkt_cnt, num_counters,
								ROCE_CPB_OUT_CNT_NUM, HIHTR_OPC_QUERY_CPB_OUT_COUNTER);
    if (ret) {
        dev_err(hr_dev->dev, "failed to query cpb out counter, ret = %d\n", ret);
        return ret;
    }

    ret = hihtr_hw_v3_query_dp_counter(hr_dev, ROCE_WQEBB_PE_NUM, (u32*)dp_pkt_cnt, num_counters,
								ROCE_PE_CNT_NUM, HIHTR_OPC_QUERY_PE_COUNTER);
    if (ret) {
        dev_err(hr_dev->dev, "failed to query pe counter, ret = %d\n", ret);
        return ret;
    }

    return ret;
}

STATIC int hihtr_hw_v3_query_scc(struct hihtr_dev *hr_dev, void *buffer)
{
	struct hihtr_cmq_desc desc;
	int ret;

	if (HIHTR_GET_PCI_REVISION(hr_dev) <= PCI_REVISION_ID_HIP08)
		return -EOPNOTSUPP;

	hihtr_cmq_setup_basic_desc(&desc, HIHTR_OPC_QUERY_SCC_CTRL_INFO_CMD, true);
	ret = hihtr5_cmq_send(hr_dev, &desc, 1);
	if (ret)
		return ret;

	memcpy(buffer, desc.data, sizeof(struct scc_info));

	return ret;
}

STATIC int hihtr_hw_v3_set_scc(struct hihtr_dev *hr_dev,
	u32 scc_enable, u32 algo_id, u32 template_id, u32 scc_mask)
{
    struct hihtr_cmq_desc desc;
    struct hihtr_set_scc *set_scc = (struct hihtr_set_scc *)desc.data;
	int ret;

	if (HIHTR_GET_PCI_REVISION(hr_dev) <= PCI_REVISION_ID_HIP08 || hr_dev->is_vf)
		return -EOPNOTSUPP;

	hihtr_cmq_setup_basic_desc(&desc, HIHTR_OPC_SET_SCC_CMD, false);
	set_scc->scc_enable = scc_mask & SCC_ENABLE_MASK ? scc_enable : 0;
	set_scc->scc_algo_id = scc_mask & SCC_ALGO_MASK ? algo_id : 0;
	set_scc->scc_template_id = scc_mask & SCC_TEMPLATE_MASK ? template_id : 0;
	set_scc->scc_mask = scc_mask;
    ret = hihtr5_cmq_send(hr_dev, &desc, 1);
    if (ret) {
        ibdev_err(&hr_dev->ib_dev,
			  "failed to set scc, ret = %d.\n", ret);
    }

    return ret;
}

static int hihtr_config_global_param(struct hihtr_dev *hr_dev)
{
	struct hihtr_cmq_desc desc;
	struct hihtr_cmq_req *req = (struct hihtr_cmq_req *)desc.data;
	u32 clock_cycles_of_1us;

	hihtr_cmq_setup_basic_desc(&desc, HIHTR_OPC_CFG_GLOBAL_PARAM,
				      false);

	if (HIHTR_GET_PCI_REVISION(hr_dev) == PCI_REVISION_ID_HIP08)
		clock_cycles_of_1us = HIHTR_1NS_CFG;
	else
		clock_cycles_of_1us = HIHTR_1US_CFG;

	hr_reg_write(req, CFG_GLOBAL_PARAM_1US_CYCLES, clock_cycles_of_1us);
	hr_reg_write(req, CFG_GLOBAL_PARAM_UDP_PORT, ROCE_V3_UDP_DPORT);

	return hihtr5_cmq_send(hr_dev, &desc, 1);
}

static void hihtr_set_pf_caps_a(struct hihtr_caps *caps, struct hihtr_query_pf_caps_a *resp_a)
{
	caps->local_ca_ack_delay     = resp_a->local_ca_ack_delay;
	caps->max_sq_sg		     = le16_to_cpu(resp_a->max_sq_sg);
	caps->max_sq_inline	     = le16_to_cpu(resp_a->max_sq_inline);
	caps->max_rq_sg		     = le16_to_cpu(resp_a->max_rq_sg);
	caps->max_rq_sg = roundup_pow_of_two(caps->max_rq_sg);
	caps->max_extend_sg	     = le32_to_cpu(resp_a->max_extend_sg);
	caps->num_qpc_timer	     = le16_to_cpu(resp_a->num_qpc_timer);
	caps->max_srq_sges	     = le16_to_cpu(resp_a->max_srq_sges);
	caps->max_srq_sges = roundup_pow_of_two(caps->max_srq_sges);
	caps->num_aeq_vectors	     = resp_a->num_aeq_vectors;
	caps->num_other_vectors	     = resp_a->num_other_vectors;
	caps->max_sq_desc_sz	     = resp_a->max_sq_desc_sz;
	caps->max_rq_desc_sz	     = resp_a->max_rq_desc_sz;
	caps->max_srq_desc_sz	     = resp_a->max_srq_desc_sz;
	caps->cqe_sz		     = resp_a->cqe_sz;
	caps->max_dwqe_resource = resp_a->dwqe_resource_num;
	caps->free_dwqe_resource = resp_a->dwqe_resource_num;
}

static void hihtr_set_pf_caps_b(struct hihtr_caps *caps, struct hihtr_query_pf_caps_b *resp_b)
{
	int ctx_hop_num;
	int pbl_hop_num;

	caps->mtpt_entry_sz	     = resp_b->mtpt_entry_sz;
	caps->irrl_entry_sz	     = resp_b->irrl_entry_sz;
	caps->trrl_entry_sz	     = resp_b->trrl_entry_sz;
	caps->cqc_entry_sz	     = resp_b->cqc_entry_sz;
	caps->srqc_entry_sz	     = resp_b->srqc_entry_sz;
	caps->idx_entry_sz	     = resp_b->idx_entry_sz;
	caps->sccc_sz		     = resp_b->sccc_sz;
	caps->max_mtu		     = resp_b->max_mtu;
	caps->qpc_sz		     = le16_to_cpu(resp_b->qpc_sz);
	caps->min_cqes		     = resp_b->min_cqes;
	caps->min_wqes		     = resp_b->min_wqes;
	caps->page_size_cap	     = le32_to_cpu(resp_b->page_size_cap);
	caps->pkey_table_len[0]	     = resp_b->pkey_table_len;
	caps->phy_num_uars	     = resp_b->phy_num_uars;
	ctx_hop_num		     = resp_b->ctx_hop_num;
	pbl_hop_num		     = resp_b->pbl_hop_num;
	caps->qpc_hop_num = ctx_hop_num;
	caps->sccc_hop_num = ctx_hop_num;
	caps->srqc_hop_num = ctx_hop_num;
	caps->cqc_hop_num = ctx_hop_num;
	caps->mpt_hop_num = ctx_hop_num;
	caps->mtt_hop_num = pbl_hop_num;
	caps->cqe_hop_num = pbl_hop_num;
	caps->srqwqe_hop_num = pbl_hop_num;
	caps->idx_hop_num = pbl_hop_num;
}

static void hihtr_set_pf_caps_c(struct hihtr_caps *caps, struct hihtr_query_pf_caps_c *resp_c)
{
	caps->num_pds = 1 << hr_reg_read(resp_c, PF_CAPS_C_NUM_PDS);
	caps->flags = hr_reg_read(resp_c, PF_CAPS_C_CAP_FLAGS);
	caps->num_cqs = 1 << hr_reg_read(resp_c, PF_CAPS_C_NUM_CQS);
	caps->gid_table_len[0] = hr_reg_read(resp_c, PF_CAPS_C_MAX_GID);
	caps->max_cqes = 1 << hr_reg_read(resp_c, PF_CAPS_C_CQ_DEPTH);
	caps->num_xrcds = 1 << hr_reg_read(resp_c, PF_CAPS_C_NUM_XRCDS);
	caps->num_mtpts = 1 << hr_reg_read(resp_c, PF_CAPS_C_NUM_MRWS);
	caps->num_qps = (1 << hr_reg_read(resp_c, PF_CAPS_C_NUM_QPS));
	caps->max_pi = 1 << le16_to_cpu(resp_c->max_pi_shift);
	caps->max_qp_init_rdma = hr_reg_read(resp_c, PF_CAPS_C_MAX_ORD);
	caps->max_qp_dest_rdma = caps->max_qp_init_rdma;
	caps->max_sq_wqes = 1 << le16_to_cpu(resp_c->sq_depth);
	caps->max_rq_wqes = 1 << le16_to_cpu(resp_c->rq_depth);
}

static void hihtr_set_pf_caps_d(struct hihtr_caps *caps, struct hihtr_query_pf_caps_d *resp_d)
{
	struct hihtr_dev *hr_dev = container_of(caps, struct hihtr_dev, caps);
	caps->num_srqs = 1 << hr_reg_read(resp_d, PF_CAPS_D_NUM_SRQS);
	caps->congest_type = hr_reg_read(resp_d, PF_CAPS_D_CONG_TYPE);
	caps->max_srq_wrs = 1 << le16_to_cpu(resp_d->srq_depth);
	caps->ceqe_depth = 1 << hr_reg_read(resp_d, PF_CAPS_D_CEQ_DEPTH);
	caps->num_comp_vectors = hr_reg_read(resp_d, PF_CAPS_D_NUM_CEQS);
	caps->aeqe_depth = 1 << hr_reg_read(resp_d, PF_CAPS_D_AEQ_DEPTH);
	caps->default_congest_type = hr_reg_read(resp_d, PF_CAPS_D_DEFAULT_ALG);
	caps->default_aeq_arm_st = hr_reg_read(resp_d, PF_CAPS_D_AEQ_ARM_ST);
	caps->default_ceq_arm_st = hr_reg_read(resp_d, PF_CAPS_D_CEQ_ARM_ST);
	caps->reserved_pds = hr_reg_read(resp_d, PF_CAPS_D_RSV_PDS);
	caps->num_uars = 1 << hr_reg_read(resp_d, PF_CAPS_D_NUM_UARS);
	caps->reserved_qps = hr_reg_read(resp_d, PF_CAPS_D_RSV_QPS);
	caps->reserved_uars = hr_reg_read(resp_d, PF_CAPS_D_RSV_UARS);
	caps->resize_cq_hw_cq_depth = 1 << hr_reg_read(resp_d, PF_CAPS_D_RESIZE_CQ_DEPTH);
	caps->wqe_sq_hop_num = hop_num;
	caps->wqe_sge_hop_num = hop_num;
	caps->wqe_rq_hop_num = hop_num;
	if (hop_num < 0 || hop_num > HOP_NUM_2) {
		dev_alert(hr_dev->dev, "hop_num is not a valid value, fallback to be default value 0\n");
		caps->wqe_sq_hop_num = 0;
		caps->wqe_sge_hop_num = 0;
		caps->wqe_rq_hop_num = 0;
	}
	caps->flags |= le16_to_cpu(resp_d->cap_flags_ex) <<
		       HIHTR_CAP_FLAGS_EX_SHIFT;
#ifdef HIHTR_DRIVER_NOT_SUPPORT_XRC
	caps->flags &= ~HIHTR_CAP_FLAG_XRC;
#endif
}

static void hihtr_set_pf_caps_e(struct hihtr_caps *caps, struct hihtr_query_pf_caps_e *resp_e)
{
	caps->reserved_mrws = hr_reg_read(resp_e, PF_CAPS_E_RSV_MRWS);
	caps->chunk_sz = 1 << hr_reg_read(resp_e, PF_CAPS_E_CHUNK_SIZE_SHIFT);
	caps->reserved_cqs = hr_reg_read(resp_e, PF_CAPS_E_RSV_CQS);
	caps->reserved_srqs = hr_reg_read(resp_e, PF_CAPS_E_RSV_SRQS);
	caps->reserved_lkey = hr_reg_read(resp_e, PF_CAPS_E_RSV_LKEYS);
	caps->default_ceq_max_cnt = le16_to_cpu(resp_e->ceq_max_cnt);
	caps->default_ceq_period = le16_to_cpu(resp_e->ceq_period);
	caps->default_aeq_max_cnt = le16_to_cpu(resp_e->aeq_max_cnt);
	caps->default_aeq_period = le16_to_cpu(resp_e->aeq_period);
}

static int hihtr_query_caps(struct hihtr_dev *hr_dev)
{
	struct hihtr_cmq_desc desc[HIHTR_QUERY_PF_CAPS_CMD_NUM];
	struct hihtr_caps *caps = &hr_dev->caps;
	enum hihtr_opcode_type cmd;
	int ret;
	int i;

	cmd = hr_dev->is_vf ? HIHTR_OPC_QUERY_VF_CAPS_NUM :
	      HIHTR_OPC_QUERY_PF_CAPS_NUM;

	for (i = 0; i < HIHTR_QUERY_PF_CAPS_CMD_NUM; i++) {
		hihtr_cmq_setup_basic_desc(&desc[i], cmd, true);
		if (i < (HIHTR_QUERY_PF_CAPS_CMD_NUM - 1))
			desc[i].flag |= cpu_to_le16(HIHTR_CMD_FLAG_NEXT);
		else
			desc[i].flag &= ~cpu_to_le16(HIHTR_CMD_FLAG_NEXT);
	}

	ret = hihtr5_cmq_send(hr_dev, desc, HIHTR_QUERY_PF_CAPS_CMD_NUM);
	if (ret)
		return ret;

	hihtr_set_pf_caps_a(caps, (struct hihtr_query_pf_caps_a *)&desc[HIHTR_CMD_DESC_NUM_0]);
	hihtr_set_pf_caps_b(caps, (struct hihtr_query_pf_caps_b *)&desc[HIHTR_CMD_DESC_NUM_1]);
	hihtr_set_pf_caps_c(caps, (struct hihtr_query_pf_caps_c *)&desc[HIHTR_CMD_DESC_NUM_2]);
	hihtr_set_pf_caps_d(caps, (struct hihtr_query_pf_caps_d *)&desc[HIHTR_CMD_DESC_NUM_3]);
	hihtr_set_pf_caps_e(caps, (struct hihtr_query_pf_caps_e *)&desc[HIHTR_CMD_DESC_NUM_4]);
	dev_info(hr_dev->dev, "caps->num_qps:0x%x, caps->max_pi:0x%x\n", caps->num_qps, caps->max_pi);

	if (caps->flags & HIHTR_CAP_FLAG_RESIZE_CQ) {
		caps->resize_cq_hw_cq_depth = max((u32)roundup_pow_of_two(hr_dev->caps.min_cqes), hr_dev->caps.resize_cq_hw_cq_depth);
		caps->resize_cq_hw_cq_depth = min(hr_dev->caps.max_cqes, hr_dev->caps.resize_cq_hw_cq_depth);
	}

	if (!(caps->page_size_cap & PAGE_SIZE))
		caps->page_size_cap = HIHTR_V3_PAGE_SIZE_SUPPORTED;
	if (hr_dev->is_vf) {
		caps->max_dwqe_resource = 0;
		caps->free_dwqe_resource = 0;
	}
	spin_lock_init(&(hr_dev->dwqe_resource_lock));
	dev_info(hr_dev->dev, "max_dwqe_resource:0x%x, hw_cq_depth:0x%x.\n", caps->max_dwqe_resource, caps->resize_cq_hw_cq_depth);
	return 0;
}

static int config_hem_entry_size(struct hihtr_dev *hr_dev, u32 type, u32 val)
{
	struct hihtr_cmq_desc desc;
	struct hihtr_cmq_req *req = (struct hihtr_cmq_req *)desc.data;

	hihtr_cmq_setup_basic_desc(&desc, HIHTR_OPC_CFG_ENTRY_SIZE,
				      false);

	hr_reg_write(req, CFG_HEM_ENTRY_SIZE_TYPE, type);
	hr_reg_write(req, CFG_HEM_ENTRY_SIZE_VALUE, val);

	return hihtr5_cmq_send(hr_dev, &desc, 1);
}

static int hihtr_config_entry_size(struct hihtr_dev *hr_dev)
{
	struct hihtr_caps *caps = &hr_dev->caps;
	int ret;

	if (HIHTR_GET_PCI_REVISION(hr_dev) == PCI_REVISION_ID_HIP08)
		return 0;

	ret = config_hem_entry_size(hr_dev, HIHTR_CFG_QPC_SIZE,
				    caps->qpc_sz);
	if (ret) {
		dev_err(hr_dev->dev, "failed to cfg qpc sz, ret = %d.\n", ret);
		return ret;
	}

	ret = config_hem_entry_size(hr_dev, HIHTR_CFG_SCCC_SIZE,
				caps->sccc_sz);
	if (ret)
		dev_err(hr_dev->dev, "failed to cfg sccc sz, ret = %d.\n", ret);
	return ret;
}

static int hihtr_set_perx_round_bit(struct hihtr_dev *hr_dev, u32 flag)
{
	int ret;
	struct hihtr_v3_priv *priv = hr_dev->priv;
	u16 out_size = 0;
	ret = hihtr_v3_msg_to_mgmt_sync(priv->lld_dev->hwdev, ROCE_MPU_CMD_CFG_PERX_IDX, &flag,
	    sizeof(flag), NULL, &out_size);
	if (ret) {
		dev_err(hr_dev->dev, "failed to cfg terx round bit ret:%d\n", ret);
	}
	return ret;
}

/* PPF hwdev saved during driver probe, used by hihtr_stop_flow which
 * runs in FLR context where hinic5_get_ppf_dev() is unavailable.
 */
static void *hihtr_ppf_hwdev;

static int hihtr_stop_flow(u16 func_id)
{
	int ret;
	roce_stop_flow_cmd_s buf_in;
	void *hwdev = READ_ONCE(hihtr_ppf_hwdev);

	if (!hwdev) {
		pr_err("hihtr_stop_flow: ppf hwdev not available\n");
		return -ENODEV;
	}

	buf_in.func_id = func_id;
	ret = hihtr_v3_msg_to_mgmt_sync(hwdev, ROCE_MPU_CMD_STOP_FLOW, &buf_in, sizeof(buf_in), NULL, NULL);
	if (ret) {
		pr_err("failed to cfg terx round bit ret:%d\n", ret);
	}
	return ret;
}

static int hihtr_v3_pf_profile(struct hihtr_dev *hr_dev)
{
	struct device *dev = hr_dev->dev;
	int ret;

	ret = hihtr_query_func_info(hr_dev);
	if (ret) {
		dev_err(dev, "failed to query func info, ret = %d.\n", ret);
		return ret;
	}
	dev_info(dev, "hihtr_query_func_info, func_num:%d, cong_algo_tmpl_id:%d\n",
		hr_dev->func_num, hr_dev->cong_algo_tmpl_id); // TODO 后续删除

	ret = hihtr_config_global_param(hr_dev);
	if (ret) {
		dev_err(dev, "failed to config global param, ret = %d.\n", ret);
		return ret;
	}

	ret = hihtr_query_caps(hr_dev);
	if (ret) {
		dev_warn(dev, "failed to query PF caps, ret = %d.\n", ret);
		return ret;
	}

	ret = hihtr_query_pf_resource(hr_dev);
	if (ret) {
		dev_err(dev, "failed to query pf resource, ret = %d.\n", ret);
		return ret;
	}

	v3_apply_func_caps(hr_dev);

	ret = hihtr_alloc_vf_resource(hr_dev);
	if (ret) {
		dev_err(dev, "failed to alloc vf resource, ret = %d.\n", ret);
		return ret;
	}

	ret = hihtr_v3_set_bt(hr_dev);
	if (ret) {
		dev_err(dev, "failed to config BA table, ret = %d.\n", ret);
		return ret;
	}

	/* Configure the size of QPC, SCCC, etc. */
	hihtr_config_entry_size(hr_dev);
	if (ret) {
		dev_err(dev, "failed to config entrt size, ret = %d.\n", ret);
		return ret;
	}
	ret = hihtr_set_perx_round_bit(hr_dev, 1);
	if (ret)
		dev_err(dev, "failed to config petx round bit, ret = %d.\n", ret);
	return ret;
}

/*
 * Description  : hihtr_roce 驱动 vf相关配置设置
 * Input        : hihtr device
 * Output       : NULL
 * Return Value : ret: 0 means success, other means fail
 */
STATIC int hihtr_v3_vf_profile(struct hihtr_dev *hr_dev)
{
	struct device *dev = hr_dev->dev;
	int ret;

	dev_info(dev, "hihtr_v3_vf_profile\n");
	hr_dev->func_num = 1;

	ret = hihtr_query_caps(hr_dev);
	if (ret) {
		dev_warn(dev, "failed to query VF caps, ret = %d.\n", ret);
		return ret;
	}

	ret = hihtr_query_vf_resource(hr_dev);
	if (ret) {
		dev_err(dev, "failed to query VF resource, ret = %d.\n", ret);
		return ret;
	}

	v3_apply_func_caps(hr_dev);

	ret = hihtr_v3_set_bt(hr_dev);
	if (ret)
		dev_err(dev, "failed to config VF BA table, ret = %d.\n", ret);

	ret = hihtr_set_perx_round_bit(hr_dev, 1);
	if (ret)
		dev_err(dev, "failed to config petx round bit, ret = %d.\n", ret);
	return ret;
}

static int hihtr_cmq_query_hw_info(struct hihtr_dev *hr_dev)
{
	struct hihtr_query_version *resp;
	struct hihtr_cmq_desc desc;
	int ret;

	hihtr_cmq_setup_basic_desc(&desc, HIHTR_OPC_QUERY_HW_VER, true);
	ret = hihtr5_cmq_send(hr_dev, &desc, 1);
	if (ret)
		return ret;

	resp = (struct hihtr_query_version *)desc.data;
	hr_dev->hw_rev = le16_to_cpu(resp->rocee_hw_version);
	if (hr_dev->pci_dev) {
		hr_dev->vendor_id = hr_dev->pci_dev->vendor;
	} else {
		hr_dev->vendor_id = 0;
	}

	return 0;
}

static int hihtr_query_fw_ver(struct hihtr_dev *hr_dev)
{
	struct hihtr_query_fw_info *resp;
	struct hihtr_cmq_desc desc;
	int ret;

	hihtr_cmq_setup_basic_desc(&desc, HIHTR_QUERY_FW_VER, true);
	ret = hihtr5_cmq_send(hr_dev, &desc, 1);
	if (ret)
		return ret;

	resp = (struct hihtr_query_fw_info *)desc.data;
	hr_dev->caps.fw_ver = (u64)(le32_to_cpu(resp->fw_ver));

	return 0;
}

static int hihtr_enable_imp_func_cmdq(struct hihtr_dev *hr_dev)
{
	struct hihtr_cmq_desc desc;
	int ret;

	hihtr_cmq_setup_basic_desc(&desc, HIHTR_OPC_ENABLE_FUNC_CMDQ, false);
	ret = hihtr5_cmq_send(hr_dev, &desc, 1);
	return ret;
}

STATIC int hihtr_v3_profile(struct hihtr_dev *hr_dev)
{
	struct device *dev = hr_dev->dev;
	int ret;

	ret = hihtr_enable_imp_func_cmdq(hr_dev);
	if (ret) {
		dev_err(dev, "failed to enable imp func cmdq, ret = %d.\n", ret);
		return ret;
	}

	ret = hihtr_cmq_query_hw_info(hr_dev);
	if (ret) {
		dev_err(dev, "failed to query hardware info, ret = %d.\n", ret);
		return ret;
	}

	ret = hihtr_query_fw_ver(hr_dev);
	if (ret) {
		dev_err(dev, "failed to query firmware info, ret = %d.\n", ret);
		return ret;
	}

	if (hr_dev->pci_dev)
		hr_dev->vendor_part_id = hr_dev->pci_dev->device;
	hr_dev->sys_image_guid = be64_to_cpu(hr_dev->ib_dev.node_guid);
	dev_info(dev, "sys_image_guid %llu\r\n", hr_dev->sys_image_guid);
	dev_info(dev, "isvf %d\r\n", hr_dev->is_vf);
	if (hr_dev->is_vf) {
		ret = hihtr_v3_vf_profile(hr_dev);
		return ret;
	} else
		return hihtr_v3_pf_profile(hr_dev);
}

static void copy_gid(void *dest, const union ib_gid *gid)
{
#define GID_SIZE 4
	const union ib_gid *src = gid;
	__le32 (*p)[GID_SIZE] = dest;
	int i;

	if (!gid)
		src = &zgid;

	for (i = 0; i < GID_SIZE; i++)
		(*p)[i] = cpu_to_le32(*(u32 *)&src->raw[i * sizeof(u32)]);
}

static int config_sgid_table(struct hihtr_dev *hr_dev,
			     int gid_index, const union ib_gid *gid,
			     enum hihtr_sgid_type sgid_type)
{
	struct hihtr_cmq_desc desc;
	struct hihtr_cfg_sgid_tb *sgid_tb =
				    (struct hihtr_cfg_sgid_tb *)desc.data;

	hihtr_cmq_setup_basic_desc(&desc, HIHTR_OPC_CFG_SGID_TB, false);

	hr_reg_write(sgid_tb, CFG_SGID_TB_TABLE_IDX, gid_index);
	hr_reg_write(sgid_tb, CFG_SGID_TB_VF_SGID_TYPE, sgid_type);

	copy_gid(&sgid_tb->vf_sgid_l, gid);

	return hihtr5_cmq_send(hr_dev, &desc, 1);
}

STATIC int config_gmv_table(struct hihtr_dev *hr_dev,
			    int gid_index, const union ib_gid *gid,
			    enum hihtr_sgid_type sgid_type,
			    const struct ib_gid_attr *attr)
{
	struct hihtr_cmq_desc desc[HIHTR_CMD_DESC_NUM_3];
	struct hihtr_cfg_gmv_tb_a *tb_a =
				(struct hihtr_cfg_gmv_tb_a *)desc[HIHTR_CMD_DESC_NUM_0].data;
	struct hihtr_cfg_gmv_tb_b *tb_b =
				(struct hihtr_cfg_gmv_tb_b *)desc[HIHTR_CMD_DESC_NUM_1].data;

	u16 vlan_id = VLAN_CFI_MASK;
	u8 mac[ETH_ALEN] = {};
	int ret;

	if (gid) {
		u8 *mac_head = &mac[0];
		ret = rdma_read_gid_l2_fields(attr, &vlan_id, mac_head);
		if (ret)
			return ret;
	}

	hihtr_cmq_setup_basic_desc(&desc[HIHTR_CMD_DESC_NUM_0], HIHTR_OPC_CFG_GMV_TBL, false);
	desc[0].flag |= cpu_to_le16(HIHTR_CMD_FLAG_NEXT);
	hihtr_cmq_setup_basic_desc(&desc[HIHTR_CMD_DESC_NUM_1], HIHTR_OPC_CFG_GMV_TBL, false);
	desc[1].flag |= cpu_to_le16(HIHTR_CMD_FLAG_NEXT);
	hihtr_cmq_setup_basic_desc(&desc[HIHTR_CMD_DESC_NUM_2], HIHTR_OPC_CFG_GMV_TBL, false);

	copy_gid(&tb_a->vf_sgid_l, gid);

	hr_reg_write(tb_a, GMV_TB_A_VF_SGID_TYPE, sgid_type);
	hr_reg_write(tb_a, GMV_TB_A_VF_VLAN_EN, vlan_id < VLAN_CFI_MASK);
	hr_reg_write(tb_a, GMV_TB_A_VF_VLAN_ID, vlan_id);
	hr_reg_write(tb_a, GMV_TB_A_SMAC_H, *(u16 *)mac);

	tb_b->vf_smac_l = cpu_to_le32(*(u32 *)&mac[HIHTR_VF_SMAC_IDX]);
	hr_reg_write(tb_b, GMV_TB_B_SGID_IDX, gid_index);

	return hihtr5_cmq_send(hr_dev, desc, HIHTR_CMD_DESC_NUM_3);

	return 0;
}

STATIC int hihtr_v3_set_gid(struct hihtr_dev *hr_dev, int gid_index,
			       const union ib_gid *gid,
			       const struct ib_gid_attr *attr)
{
	enum hihtr_sgid_type sgid_type = GID_TYPE_FLAG_ROCE_V2_IPV4; // 1872不支持roce V1
	int ret;

	if (gid) {
		if (attr->gid_type == IB_GID_TYPE_ROCE_UDP_ENCAP) {
			if (ipv6_addr_v4mapped((void *)gid))
				sgid_type = GID_TYPE_FLAG_ROCE_V2_IPV4;
			else
				sgid_type = GID_TYPE_FLAG_ROCE_V2_IPV6;
		} else {
			dev_info(hr_dev->dev, "invalid gid_type 0x%x\r\n", attr->gid_type);
			return -EPERM;
		}
	}

	dev_info(hr_dev->dev, "pci_revision 0x%x, sgid_type 0x%x\r\n", HIHTR_GET_PCI_REVISION(hr_dev), sgid_type);
	if (HIHTR_GET_PCI_REVISION(hr_dev) >= PCI_REVISION_ID_HIP09)
		ret = config_gmv_table(hr_dev, gid_index, gid, sgid_type, attr);
	else
		ret = config_sgid_table(hr_dev, gid_index, gid, sgid_type);

	if (ret)
		ibdev_err(&hr_dev->ib_dev, "failed to set gid, gid_index = %d ret = %d!\n",
			  gid_index, ret);

	return ret;
}

static int hihtr_v3_get_reset_page(struct hihtr_dev *hr_dev)
{
	hr_dev->reset_page = alloc_page(GFP_KERNEL | __GFP_ZERO);
	if (!hr_dev->reset_page)
		return -ENOMEM;

	hr_dev->reset_kaddr = vmap(&hr_dev->reset_page, 1, VM_MAP, PAGE_KERNEL);
	if (!hr_dev->reset_kaddr)
		goto err_with_vmap;
	return 0;

err_with_vmap:
	put_page(hr_dev->reset_page);
	return -ENOMEM;
}

static void hihtr_set_mac_type(struct hihtr_dev *hr_dev)
{
	if (HIHTR_GET_PCI_REVISION(hr_dev) == PCI_REVISION_ID_HIP08)
		return;
	hr_dev->mac_type = HNAE5_MAC_ETH; // 由于没有HCCS模块, 因此不支持ROH
}

static int alloc_ctx_table(struct hihtr_dev *hr_dev, struct hihtr_hem_table *table, unsigned int *cnt)
{
	unsigned int ctx_entry_num;
	int ret;

	ctx_entry_num = table->num_hem * table->table_chunk_size / table->obj_size;
	for (*cnt = 0; *cnt < ctx_entry_num; ++*cnt) {
		ret = hihtr_table_get(hr_dev, table, *cnt);
		if (ret) {
			dev_err(hr_dev->dev, "table %u get failed\n", table->type);
			table->obj_count = 0;
			return -ENOMEM;
		}
	}
	table->obj_count = *cnt;
	return 0;
}

static int get_hem_ctx_table(struct hihtr_dev *hr_dev)
{
	unsigned int qpc_count;
	unsigned int srqc_count;
	unsigned int cqc_count;
	unsigned int scc_ctx_count;
	unsigned int mtpt_count;
	int ret;
	unsigned int i;

	ret = alloc_ctx_table(hr_dev, &hr_dev->mr_table.mtpt_table, &mtpt_count);
	if (ret)
		goto err_mtpt_failed;

	ret = alloc_ctx_table(hr_dev, &hr_dev->qp_table.qp_table, &qpc_count);
	if (ret)
		goto err_qpc_failed;

	ret = alloc_ctx_table(hr_dev, &hr_dev->cq_table.table, &cqc_count);
	if (ret)
		goto err_cqc_failed;

	if (hr_dev->caps.flags & HIHTR_CAP_FLAG_SRQ) {
		ret = alloc_ctx_table(hr_dev, &hr_dev->srq_table.table, &srqc_count);
		if (ret)
			goto err_srqc_failed;
	}

	ret = alloc_ctx_table(hr_dev, &hr_dev->qp_table.sccc_table, &scc_ctx_count);
	if (ret)
		goto err_scc_ctx_failed;

	return 0;

err_scc_ctx_failed:
	for (i = 0; i < scc_ctx_count; ++i)
		hihtr_table_put(hr_dev, &hr_dev->qp_table.sccc_table, i);

err_srqc_failed:
	if (hr_dev->caps.flags & HIHTR_CAP_FLAG_SRQ) {
		for (i = 0; i < srqc_count; ++i)
			hihtr_table_put(hr_dev, &hr_dev->srq_table.table, i);
	}

err_cqc_failed:
	for (i = 0; i < cqc_count; ++i)
		hihtr_table_put(hr_dev, &hr_dev->cq_table.table, i);

err_qpc_failed:
	for (i = 0; i < qpc_count; ++i)
		hihtr_table_put(hr_dev, &hr_dev->qp_table.qp_table, i);

err_mtpt_failed:
	for (i = 0; i < mtpt_count; ++i)
		hihtr_table_put(hr_dev, &hr_dev->mr_table.mtpt_table, i);

	return ret;
}

static void destroy_ctx_table(struct hihtr_dev *hr_dev, struct hihtr_hem_table *table)
{
	unsigned int ctx_entry_num;
	unsigned int i;

	ctx_entry_num = table->num_hem * table->table_chunk_size / table->obj_size;
	for (i = 0; i < ctx_entry_num; ++i) {
		hihtr_table_put(hr_dev, table, i);
	}
}

static void put_hem_ctx_table(struct hihtr_dev *hr_dev)
{
	if (hr_dev->caps.flags & HIHTR_CAP_FLAG_SRQ) {
		destroy_ctx_table(hr_dev, &hr_dev->srq_table.table);
	}

	destroy_ctx_table(hr_dev, &hr_dev->cq_table.table);
	destroy_ctx_table(hr_dev, &hr_dev->qp_table.qp_table);
	destroy_ctx_table(hr_dev, &hr_dev->mr_table.mtpt_table);
	destroy_ctx_table(hr_dev, &hr_dev->qp_table.sccc_table);
}

static int get_hem_table(struct hihtr_dev *hr_dev)
{
	unsigned int qpc_timer_count;
	unsigned int cqc_tiemr_count;
	unsigned int gmv_count;
	int ret;
	unsigned int i;

	/* Alloc memory for source address table buffer space chunk */
	for (gmv_count = 0; gmv_count < hr_dev->caps.gmv_entry_num;
	     gmv_count++) {
		ret = hihtr_table_get(hr_dev, &hr_dev->gmv_table, gmv_count);
		if (ret)
			goto err_gmv_failed;
	}

	// 增加qpc、srqc、cqc的ctx资源申请，增加mpt和sccc资源申请
	ret = get_hem_ctx_table(hr_dev);
	if (ret)
		goto err_gmv_failed;

	if (hr_dev->is_vf)
		return 0;

	/* Alloc memory for QPC Timer buffer space chunk */
	for (qpc_timer_count = 0; qpc_timer_count < hr_dev->caps.qpc_timer_bt_num;
	     qpc_timer_count++) {
		ret = hihtr_table_get(hr_dev, &hr_dev->qpc_timer_table,
					 qpc_timer_count);
		if (ret) {
			dev_err(hr_dev->dev, "QPC Timer get failed\n");
			goto err_qpc_timer_failed;
		}
	}

	/* Alloc memory for CQC Timer buffer space chunk */
	for (cqc_tiemr_count = 0; cqc_tiemr_count < hr_dev->caps.cqc_timer_bt_num;
	     cqc_tiemr_count++) {
		ret = hihtr_table_get(hr_dev, &hr_dev->cqc_timer_table,
					 cqc_tiemr_count);
		if (ret) {
			dev_err(hr_dev->dev, "CQC Timer get failed\n");
			goto err_cqc_timer_failed;
		}
	}

	return 0;

err_cqc_timer_failed:
	for (i = 0; i < cqc_tiemr_count; i++)
		hihtr_table_put(hr_dev, &hr_dev->cqc_timer_table, i);

err_qpc_timer_failed:
	for (i = 0; i < qpc_timer_count; i++)
		hihtr_table_put(hr_dev, &hr_dev->qpc_timer_table, i);

	put_hem_ctx_table(hr_dev);

err_gmv_failed:
	for (i = 0; i < gmv_count; i++)
		hihtr_table_put(hr_dev, &hr_dev->gmv_table, i);

	return ret;
}

static void hihtr_v3_put_reset_page(struct hihtr_dev *hr_dev)
{
	if (hr_dev->reset_kaddr != NULL) {
		vunmap(hr_dev->reset_kaddr);
		hr_dev->reset_kaddr = NULL;
	}

	if (hr_dev->reset_page != NULL) {
		put_page(hr_dev->reset_page);
		hr_dev->reset_page = NULL;
	}
}

STATIC int hihtr_v3_init(struct hihtr_dev *hr_dev)
{
	int ret;

	ret = hihtr_v3_get_reset_page(hr_dev);
	if (ret) {
		dev_err(hr_dev->dev, "reset state init failed, ret = %d.\n", ret);
		return ret;
	}

	hihtr_set_mac_type(hr_dev);
	dev_info(hr_dev->dev, "set mac type 0x%x\n", hr_dev->mac_type); // TODO: 后续删除

	ret = get_hem_table(hr_dev);
	if (ret) {
		dev_err(hr_dev->dev, "failed to get hem table, ret = %d.\n", ret);
		goto err_clear_extdb_failed;
	}

	return 0;

err_clear_extdb_failed:
	hihtr_v3_put_reset_page(hr_dev);
	return ret;
}

static bool check_device_is_in_reset(struct hihtr_dev *hr_dev)
{
	// TODO: 待补充
	return hr_dev->is_reset;
}

static void __hihtr_function_clear(struct hihtr_dev *hr_dev, int vf_id)
{
	struct hihtr_cmq_desc desc;
	struct hihtr_func_clear *req = (struct hihtr_func_clear *)desc.data;
	int ret = 0;

	if (check_device_is_in_reset(hr_dev))
		goto out;

	hihtr_cmq_setup_basic_desc(&desc, HIHTR_OPC_FUNC_CLEAR, false);
	req->rst_funcid = cpu_to_le32(vf_id);
	// 发起一次 flr
	ret = hihtr5_cmq_send(hr_dev, &desc, 1);
	if (ret) {
		dev_err(hr_dev->dev, "Func clear write failed, ret = %d.\n",
			 ret);
		goto out;
	}
	if (vf_id == 0) {
		hr_dev->is_reset = true;
	}
	return;
out:
	// TODO , 待补充
	return;
}

static void free_dip_list(struct hihtr_dev *hr_dev)
{
	struct hihtr_dip *hr_dip;
	struct hihtr_dip *tmp;
	unsigned long flags;

	spin_lock_irqsave(&hr_dev->dip_list_lock, flags);

	list_for_each_entry_safe(hr_dip, tmp, &hr_dev->dip_list, node) {
		list_del(&hr_dip->node);
		kfree(hr_dip);
	}

	spin_unlock_irqrestore(&hr_dev->dip_list_lock, flags);
}

static int hihtr_free_vf_resource(struct hihtr_dev *hr_dev, int vf_id)
{
	enum hihtr_opcode_type opcode = HIHTR_OPC_ALLOC_VF_RES;
	struct hihtr_cmq_desc desc[3];
	struct hihtr_cmq_req *req_a;

	req_a = (struct hihtr_cmq_req *)desc[0].data;
	hihtr_cmq_setup_basic_desc(&desc[HIHTR_CMD_DESC_NUM_0], opcode, false);
	desc[0].flag |= cpu_to_le16(HIHTR_CMD_FLAG_NEXT);
	hihtr_cmq_setup_basic_desc(&desc[HIHTR_CMD_DESC_NUM_1], opcode, false);
	desc[1].flag |= cpu_to_le16(HIHTR_CMD_FLAG_NEXT);
	hihtr_cmq_setup_basic_desc(&desc[HIHTR_CMD_DESC_NUM_2], opcode, false);

	hr_reg_write(req_a, FUNC_RES_A_VF_ID, vf_id);
    dev_info(hr_dev->dev, " free_vf %d resource \n", vf_id); // TODO  待删除
	return hihtr5_cmq_send(hr_dev, desc, HIHTR_CMD_DESC_NUM_3);
}

static void hihtr_function_clear(struct hihtr_dev *hr_dev)
{
	int ret;
	int i;

	if (hr_dev->cmd.state == HIHTR_CMDQ_STATE_FATAL_ERR)
		return;

	hihtr_set_perx_round_bit(hr_dev, 0);
	for (i = hr_dev->func_num - 1; i >= 0; i--) {
		__hihtr_function_clear(hr_dev, i);

		if (i == 0)
			continue;

		ret = hihtr_free_vf_resource(hr_dev, i);
		if (ret)
			ibdev_err(&hr_dev->ib_dev,
				  "failed to free vf resource, vf_id = %d, ret = %d.\n",
				  i, ret);
	}
}

STATIC void hihtr_v3_exit(struct hihtr_dev *hr_dev)
{
	hihtr_function_clear(hr_dev);

	hihtr_v3_put_reset_page(hr_dev);

	if (HIHTR_GET_PCI_REVISION(hr_dev) == PCI_REVISION_ID_HIP09
		|| HIHTR_GET_PCI_REVISION(hr_dev) == PCI_REVISION_ID_HIP10) {
		free_dip_list(hr_dev);
	}
}

static int v3_post_mbox(struct hihtr_dev *hr_dev,
			struct hihtr_mbox_msg *mbox_msg)
{
    dev_info(hr_dev->dev, " v3_post_mbox \n");
    return 0;
}

static int v3_poll_mbox_done(struct hihtr_dev *hr_dev)
{
    dev_info(hr_dev->dev, " v3_post_mbox \n");
	return 0;
}

static u32 check_aedev_reset_status(struct hihtr_dev *hr_dev)
{
    dev_info(hr_dev->dev, " check_aedev_reset_status \n");
    return CMD_RST_PRC_OTHERS;
}

STATIC bool v3_chk_mbox_is_avail(struct hihtr_dev *hr_dev, bool *busy)
{
	u32 status;

	if (hr_dev->is_reset)
		status = CMD_RST_PRC_SUCCESS;
	else
		status = check_aedev_reset_status(hr_dev); /* TODO:待适配 priv->handle */

	*busy = (status == CMD_RST_PRC_EBUSY);

	return status == CMD_RST_PRC_OTHERS;
}

STATIC int hihtr_v3_set_mac(struct hihtr_dev *hr_dev, u8 phy_port,
			       const u8 *addr)
{
	struct hihtr_cmq_desc desc;
	struct hihtr_cfg_smac_tb *smac_tb =
				    (struct hihtr_cfg_smac_tb *)desc.data;
	u16 reg_smac_h;
	u32 reg_smac_l;

	hihtr_cmq_setup_basic_desc(&desc, HIHTR_OPC_CFG_SMAC_TB, false);

	reg_smac_l = *(u32 *)(&addr[0]);
	reg_smac_h = *(u16 *)(&addr[HIHTR_SMAC_ADDR_IDX]);

	hr_reg_write(smac_tb, CFG_SMAC_TB_IDX, phy_port);
	hr_reg_write(smac_tb, CFG_SMAC_TB_VF_SMAC_H, reg_smac_h);
	smac_tb->vf_smac_l = cpu_to_le32(reg_smac_l);

	return hihtr5_cmq_send(hr_dev, &desc, 1);
}

static int set_mtpt_pbl(struct hihtr_dev *hr_dev,
			struct hihtr_v3_mpt_entry *mpt_entry,
			struct hihtr_mr *mr)
{
	u64 pages[HIHTR_V2_MAX_INNER_MTPT_NUM] = { 0 };
	struct ib_device *ibdev = &hr_dev->ib_dev;
	dma_addr_t pbl_ba;
	int i, count;

	count = hihtr_mtr_find(hr_dev, &mr->pbl_mtr, 0, pages,
				  min_t(int, ARRAY_SIZE(pages), mr->npages),
				  &pbl_ba);
	if (count < 1) {
		ibdev_err(ibdev, "failed to find PBL mtr, count = %d.\n",
			  count);
		return -ENOBUFS;
	}

	/* Aligned to the hardware address access unit */
	for (i = 0; i < count; i++)
		pages[i] >>= HIHTR_PAGE_ALIGN_OFFSET;

	mpt_entry->pbl_size = cpu_to_le32(mr->npages);
	mpt_entry->pbl_ba_l = cpu_to_le32(pbl_ba >> HIHTR_PBL_BA_L_OFFSET);
	hr_reg_write(mpt_entry, MPT_PBL_BA_H, upper_32_bits(pbl_ba >> HIHTR_PBL_BA_L_OFFSET));

	mpt_entry->pa0_l = cpu_to_le32(lower_32_bits(pages[0]));
	hr_reg_write(mpt_entry, MPT_PA0_H, upper_32_bits(pages[0]));

	mpt_entry->pa1_l = cpu_to_le32(lower_32_bits(pages[1]));
	hr_reg_write(mpt_entry, MPT_PA1_H, upper_32_bits(pages[1]));
	hr_reg_write(mpt_entry, MPT_PBL_BUF_PG_SZ,
		     to_hr_hw_page_shift(mr->pbl_mtr.hem_cfg.buf_pg_shift));

	return 0;
}

STATIC int hihtr_v3_write_mtpt(struct hihtr_dev *hr_dev,
				  void *mb_buf, struct hihtr_mr *mr)
{
	struct hihtr_v3_mpt_entry *mpt_entry;

	mpt_entry = mb_buf;
	memset(mpt_entry, 0, sizeof(*mpt_entry));

	hr_reg_write(mpt_entry, MPT_ST, V2_MPT_ST_VALID);
	hr_reg_write(mpt_entry, MPT_PD, mr->pd);
	hr_reg_enable(mpt_entry, MPT_R_INV_EN);
	hr_reg_enable(mpt_entry, MPT_L_INV_EN);

	hr_reg_write_bool(mpt_entry, MPT_BIND_EN,
			  mr->access & IB_ACCESS_MW_BIND);
	hr_reg_write_bool(mpt_entry, MPT_ATOMIC_EN,
			  mr->access & IB_ACCESS_REMOTE_ATOMIC);
	hr_reg_write_bool(mpt_entry, MPT_RR_EN,
			  mr->access & IB_ACCESS_REMOTE_READ);
	hr_reg_write_bool(mpt_entry, MPT_RW_EN,
			  mr->access & IB_ACCESS_REMOTE_WRITE);
	hr_reg_write_bool(mpt_entry, MPT_LW_EN,
			  mr->access & IB_ACCESS_LOCAL_WRITE);

	mpt_entry->len_l = cpu_to_le32(lower_32_bits(mr->size));
	mpt_entry->len_h = cpu_to_le32(upper_32_bits(mr->size));
	mpt_entry->lkey = cpu_to_le32(mr->key);
	mpt_entry->va_l = cpu_to_le32(lower_32_bits(mr->iova));
	mpt_entry->va_h = cpu_to_le32(upper_32_bits(mr->iova));

	if (mr->type != MR_TYPE_MR)
		hr_reg_enable(mpt_entry, MPT_PA);

	if (mr->type == MR_TYPE_DMA)
		return 0;

	if (mr->pbl_hop_num != HIHTR_HOP_NUM_0)
		hr_reg_write(mpt_entry, MPT_PBL_HOP_NUM, mr->pbl_hop_num);

	hr_reg_write(mpt_entry, MPT_PBL_BA_PG_SZ,
		     to_hr_hw_page_shift(mr->pbl_mtr.hem_cfg.ba_pg_shift));
	hr_reg_enable(mpt_entry, MPT_INNER_PA_VLD);

	return set_mtpt_pbl(hr_dev, mpt_entry, mr);
}

STATIC int hihtr_v3_rereg_write_mtpt(struct hihtr_dev *hr_dev,
					struct hihtr_mr *mr, int flags,
					void *mb_buf)
{
	struct hihtr_v3_mpt_entry *mpt_entry = mb_buf;
	u32 mr_access_flags = mr->access;
	int ret = 0;

	hr_reg_write(mpt_entry, MPT_ST, V2_MPT_ST_VALID);
	hr_reg_write(mpt_entry, MPT_PD, mr->pd);

	if (flags & IB_MR_REREG_ACCESS) {
		hr_reg_write(mpt_entry, MPT_BIND_EN,
			     (mr_access_flags & IB_ACCESS_MW_BIND ? 1 : 0));
		hr_reg_write(mpt_entry, MPT_ATOMIC_EN,
			     mr_access_flags & IB_ACCESS_REMOTE_ATOMIC ? 1 : 0);
		hr_reg_write(mpt_entry, MPT_RR_EN,
			     mr_access_flags & IB_ACCESS_REMOTE_READ ? 1 : 0);
		hr_reg_write(mpt_entry, MPT_RW_EN,
			     mr_access_flags & IB_ACCESS_REMOTE_WRITE ? 1 : 0);
		hr_reg_write(mpt_entry, MPT_LW_EN,
			     mr_access_flags & IB_ACCESS_LOCAL_WRITE ? 1 : 0);
	}

	if (flags & IB_MR_REREG_TRANS) {
		mpt_entry->va_l = cpu_to_le32(lower_32_bits(mr->iova));
		mpt_entry->va_h = cpu_to_le32(upper_32_bits(mr->iova));
		mpt_entry->len_l = cpu_to_le32(lower_32_bits(mr->size));
		mpt_entry->len_h = cpu_to_le32(upper_32_bits(mr->size));

		ret = set_mtpt_pbl(hr_dev, mpt_entry, mr);
	}

	return ret;
}

STATIC int hihtr_v3_frmr_write_mtpt(struct hihtr_dev *hr_dev,
				       void *mb_buf, struct hihtr_mr *mr)
{
	struct ib_device *ibdev = &hr_dev->ib_dev;
	struct hihtr_v3_mpt_entry *mpt_entry;
	dma_addr_t pbl_ba = 0;

	mpt_entry = mb_buf;
	memset(mpt_entry, 0, sizeof(*mpt_entry));

	if (hihtr_mtr_find(hr_dev, &mr->pbl_mtr, 0, NULL, 0, &pbl_ba) < 0) {
		ibdev_err(ibdev, "failed to find frmr mtr.\n");
		return -ENOBUFS;
	}

	hr_reg_write(mpt_entry, MPT_ST, V2_MPT_ST_FREE);
	hr_reg_write(mpt_entry, MPT_PD, mr->pd);

	hr_reg_enable(mpt_entry, MPT_RA_EN);
	hr_reg_enable(mpt_entry, MPT_R_INV_EN);
	hr_reg_enable(mpt_entry, MPT_L_INV_EN);

	hr_reg_enable(mpt_entry, MPT_FRE);
	hr_reg_clear(mpt_entry, MPT_MR_MW);
	hr_reg_enable(mpt_entry, MPT_BPD);
	hr_reg_clear(mpt_entry, MPT_PA);

	hr_reg_write(mpt_entry, MPT_PBL_HOP_NUM, 1);
	hr_reg_write(mpt_entry, MPT_PBL_BA_PG_SZ,
		     to_hr_hw_page_shift(mr->pbl_mtr.hem_cfg.ba_pg_shift));
	hr_reg_write(mpt_entry, MPT_PBL_BUF_PG_SZ,
		     to_hr_hw_page_shift(mr->pbl_mtr.hem_cfg.buf_pg_shift));

	mpt_entry->pbl_size = cpu_to_le32(mr->npages);

	mpt_entry->pbl_ba_l = cpu_to_le32(lower_32_bits(pbl_ba >> HIHTR_PBL_BA_L_OFFSET));
	hr_reg_write(mpt_entry, MPT_PBL_BA_H, upper_32_bits(pbl_ba >> HIHTR_PBL_BA_L_OFFSET));

	return 0;
}

STATIC int hihtr_v3_mw_write_mtpt(void *mb_buf, struct hihtr_mw *mw)
{
	struct hihtr_v3_mpt_entry *mpt_entry;

	mpt_entry = mb_buf;
	memset(mpt_entry, 0, sizeof(*mpt_entry));

	hr_reg_write(mpt_entry, MPT_ST, V2_MPT_ST_FREE);
	hr_reg_write(mpt_entry, MPT_PD, mw->pdn);

	hr_reg_enable(mpt_entry, MPT_R_INV_EN);
	hr_reg_enable(mpt_entry, MPT_L_INV_EN);
	hr_reg_enable(mpt_entry, MPT_LW_EN);

	hr_reg_enable(mpt_entry, MPT_MR_MW);
	hr_reg_enable(mpt_entry, MPT_BPD);
	hr_reg_clear(mpt_entry, MPT_PA);
	hr_reg_write(mpt_entry, MPT_BQP,
		     mw->ibmw.type == IB_MW_TYPE_1 ? 0 : 1);

	mpt_entry->lkey = cpu_to_le32(mw->rkey);

	hr_reg_write(mpt_entry, MPT_PBL_HOP_NUM,
		     mw->pbl_hop_num == HIHTR_HOP_NUM_0 ? 0 :
							     mw->pbl_hop_num);
	hr_reg_write(mpt_entry, MPT_PBL_BA_PG_SZ,
		     mw->pbl_ba_pg_sz + PG_SHIFT_OFFSET);
	hr_reg_write(mpt_entry, MPT_PBL_BUF_PG_SZ,
		     mw->pbl_buf_pg_sz + PG_SHIFT_OFFSET);

	return 0;
}

static int get_op_for_set_hem(struct hihtr_dev *hr_dev, u32 type,
			      u32 step_idx, u8 *mbox_cmd)
{
	u8 cmd;

	switch (type) {
	case HEM_TYPE_QPC:
		cmd = HIHTR_CMD_WRITE_QPC_BT0;
		break;
	case HEM_TYPE_MTPT:
		cmd = HIHTR_CMD_WRITE_MPT_BT0;
		break;
	case HEM_TYPE_CQC:
		cmd = HIHTR_CMD_WRITE_CQC_BT0;
		break;
	case HEM_TYPE_SRQC:
		cmd = HIHTR_CMD_WRITE_SRQC_BT0;
		break;
	case HEM_TYPE_SCCC:
		cmd = HIHTR_CMD_WRITE_SCCC_BT0;
		break;
	case HEM_TYPE_QPC_TIMER:
		cmd = HIHTR_CMD_WRITE_QPC_TIMER_BT0;
		break;
	case HEM_TYPE_CQC_TIMER:
		cmd = HIHTR_CMD_WRITE_CQC_TIMER_BT0;
		break;
	default:
		dev_warn(hr_dev->dev, "failed to check hem type %u.\n", type);
		return -EINVAL;
	}

	*mbox_cmd = cmd + step_idx;

	return 0;
}

static int config_gmv_ba_to_hw(struct hihtr_dev *hr_dev, unsigned long obj,
			       dma_addr_t base_addr)
{
    struct hihtr_cmq_desc desc;
	struct hihtr_cmq_req *req = (struct hihtr_cmq_req *)desc.data;
	u32 idx = obj / (HIHTR_HW_PAGE_SIZE / hr_dev->caps.gmv_entry_sz);
	u64 addr = to_hr_hw_page_addr(base_addr);

	hihtr_cmq_setup_basic_desc(&desc, HIHTR_OPC_CFG_GMV_BT, false);

	hr_reg_write(req, CFG_GMV_BT_BA_L, lower_32_bits(addr));
	hr_reg_write(req, CFG_GMV_BT_BA_H, upper_32_bits(addr));
	hr_reg_write(req, CFG_GMV_BT_IDX, idx);

	return hihtr5_cmq_send(hr_dev, &desc, 1);
}

static int config_hem_ba_to_hw(struct hihtr_dev *hr_dev,
			       dma_addr_t base_addr, u8 cmd, unsigned long tag)
{
	return hihtr_cmq_mbox(hr_dev, base_addr, 0, cmd, tag);
}

STATIC int set_hem_to_hw(struct hihtr_dev *hr_dev, int obj,
			 dma_addr_t base_addr, u32 hem_type, u32 step_idx)
{
	int ret;
	u8 cmd;

	if (unlikely(hem_type == HEM_TYPE_GMV))
		return config_gmv_ba_to_hw(hr_dev, obj, base_addr);

	if (unlikely(hem_type == HEM_TYPE_SCCC && step_idx))
		return 0;

	// bt0在硬件侧，bt1和2在内存。所以省略硬件的写bt1和bt2的表项命令，可大幅加快驱动加载速度
	if (step_idx > 0)
		return 0;

	ret = get_op_for_set_hem(hr_dev, hem_type, step_idx, &cmd);
	if (ret < 0)
		return ret;

	return config_hem_ba_to_hw(hr_dev, base_addr, cmd, obj);
}

STATIC int hihtr_v3_set_hem(struct hihtr_dev *hr_dev,
			       struct hihtr_hem_table *table, int obj,
			       u32 step_idx)
{
	struct hihtr_hem_iter iter;
	struct hihtr_hem_mhop mhop;
	struct hihtr_hem *hem;
	unsigned long mhop_obj = obj;
	int i, j, k;
	int ret = 0;
	u64 hem_idx = 0;
	u64 l1_idx = 0;
	u64 bt_ba = 0;
	u32 chunk_ba_num;
	u32 hop_num;

	if (!hihtr_check_whether_mhop(hr_dev, table->type))
		return 0;

	hihtr_calc_hem_mhop(hr_dev, table, &mhop_obj, &mhop);
	i = mhop.l0_idx;
	j = mhop.l1_idx;
	k = mhop.l2_idx;
	hop_num = mhop.hop_num;
	chunk_ba_num = mhop.bt_chunk_size / sizeof(void *);

	if (hop_num == HOP_NUM_2) {
		hem_idx = i * chunk_ba_num * chunk_ba_num + j * chunk_ba_num +
			  k;
		l1_idx = i * chunk_ba_num + j;
	} else if (hop_num == HOP_NUM_1) {
		hem_idx = i * chunk_ba_num + j;
	} else if (hop_num == HIHTR_HOP_NUM_0) {
		hem_idx = i;
	}

	if (table->type == HEM_TYPE_SCCC)
		obj = mhop.l0_idx;

	if (check_whether_last_step(hop_num, step_idx)) {
		hem = table->hem[hem_idx];
		for (hihtr_hem_first(hem, &iter);
		     !hihtr_hem_last(&iter); hihtr_hem_next(&iter)) {
			bt_ba = hihtr_hem_addr(&iter);
			ret = set_hem_to_hw(hr_dev, obj, bt_ba, table->type,
					    step_idx);
		}
	} else {
		if (step_idx == 0)
			bt_ba = table->bt_l0_dma_addr[i];
		else if (step_idx == 1 && hop_num == HOP_NUM_2)
			bt_ba = table->bt_l1_dma_addr[l1_idx];

		ret = set_hem_to_hw(hr_dev, obj, bt_ba, table->type, step_idx);
	}

	return ret;
}

STATIC int hihtr_v3_clear_hem(struct hihtr_dev *hr_dev,
				 struct hihtr_hem_table *table,
				 int tag, u32 step_idx)
{
	struct hihtr_cmd_mailbox *mailbox;
	struct device *dev = hr_dev->dev;
	u8 cmd = 0xff;
	int ret;

	if (!hihtr_check_whether_mhop(hr_dev, table->type))
		return 0;

	switch (table->type) {
		case HEM_TYPE_QPC:
			cmd = HIHTR_CMD_DESTROY_QPC_BT0;
			break;
		case HEM_TYPE_MTPT:
			cmd = HIHTR_CMD_DESTROY_MPT_BT0;
			break;
		case HEM_TYPE_CQC:
			cmd = HIHTR_CMD_DESTROY_CQC_BT0;
			break;
		case HEM_TYPE_SRQC:
			cmd = HIHTR_CMD_DESTROY_SRQC_BT0;
			break;
		case HEM_TYPE_SCCC:
		case HEM_TYPE_QPC_TIMER:
		case HEM_TYPE_CQC_TIMER:
		case HEM_TYPE_GMV:
			return 0;
		default:
			dev_warn(dev, "table %u not to be destroyed by mailbox!\n",
				table->type);
			return 0;
	}

	cmd += step_idx;

	mailbox = hihtr_alloc_cmd_mailbox(hr_dev);
	if (IS_ERR(mailbox))
		return PTR_ERR(mailbox);

	ret = hihtr_cmq_mbox(hr_dev, 0, mailbox->dma, cmd, tag);

	hihtr_free_cmd_mailbox(hr_dev, mailbox);
	return ret;
}

STATIC int hihtr_v3_qp_flow_control_init(struct hihtr_dev *hr_dev,
					    struct hihtr_qp *hr_qp)
{
	struct ib_device *ibdev = &hr_dev->ib_dev;
	struct hihtr_sccc_clr_done *resp;
	struct hihtr_sccc_clr *clr;
	struct hihtr_cmq_desc desc;
	int ret, i;

	if (HIHTR_GET_PCI_REVISION(hr_dev) >= PCI_REVISION_ID_HIP09)
		return 0;

	mutex_lock(&hr_dev->qp_table.scc_mutex);

	/* set scc ctx clear done flag */
	hihtr_cmq_setup_basic_desc(&desc, HIHTR_OPC_RESET_SCCC, false);
	ret =  hihtr5_cmq_send(hr_dev, &desc, 1);
	if (ret) {
		ibdev_err(ibdev, "failed to reset SCC ctx, ret = %d.\n", ret);
		goto out;
	}

	/* clear scc context */
	hihtr_cmq_setup_basic_desc(&desc, HIHTR_OPC_CLR_SCCC, false);
	clr = (struct hihtr_sccc_clr *)desc.data;
	clr->qpn = cpu_to_le32(hr_qp->qpn);
	ret =  hihtr5_cmq_send(hr_dev, &desc, 1);
	if (ret) {
		ibdev_err(ibdev, "failed to clear SCC ctx, ret = %d.\n", ret);
		goto out;
	}

	/* query scc context clear is done or not */
	resp = (struct hihtr_sccc_clr_done *)desc.data;
	for (i = 0; i <= HIHTR_CMQ_SCC_CLR_DONE_CNT; i++) {
		hihtr_cmq_setup_basic_desc(&desc,
					      HIHTR_OPC_QUERY_SCCC, true);
		ret = hihtr5_cmq_send(hr_dev, &desc, 1);
		if (ret) {
			ibdev_err(ibdev, "failed to query clr cmq, ret = %d\n",
				  ret);
			goto out;
		}

		if (resp->clr_done)
			goto out;
	}

	ibdev_err(ibdev, "Query SCC clr done flag overtime.\n");
	ret = -ETIMEDOUT;

out:
	mutex_unlock(&hr_dev->qp_table.scc_mutex);
	return ret;
}

STATIC int hihtr_v3_query_mpt(struct hihtr_dev *hr_dev, u32 key,
				 void *buffer)
{
	struct hihtr_v3_mpt_entry *context;
	struct hihtr_cmd_mailbox *mailbox;
	int ret;

	mailbox = hihtr_alloc_cmd_mailbox(hr_dev);
	if (IS_ERR(mailbox))
		return PTR_ERR(mailbox);

	context = mailbox->buf;
	ret = hihtr_cmq_mbox(hr_dev, 0, mailbox->dma, HIHTR_CMD_QUERY_MPT,
				key_to_hw_index(key));
	if (ret) {
		ibdev_err(&hr_dev->ib_dev,
			  "failed to process cmd when querying MPT, ret = %d.\n",
			  ret);
		goto err_mailbox;
	}

	memcpy(buffer, context, sizeof(*context));

err_mailbox:
	hihtr_free_cmd_mailbox(hr_dev, mailbox);

	return ret;
}

STATIC int hihtr_v3_set_scc_temp(struct hihtr_dev *hr_dev, void *buffer)
{
	struct hihtr_cmq_desc desc;
	struct hihtr_scc_temp *temp_info = (struct hihtr_scc_temp *)desc.data;
	struct hihtr_dfx_scc_buf *inbuf = (struct hihtr_dfx_scc_buf *)buffer;
	int ret, i;

	if (HIHTR_GET_PCI_REVISION(hr_dev) <= PCI_REVISION_ID_HIP08 || hr_dev->is_vf)
		return -EOPNOTSUPP;

	hihtr_cmq_setup_basic_desc(&desc, HIHTR_OPC_SET_SCC_TEMP_CMD, false);
	temp_info->template_id = inbuf->template_id;
	for (i = 0; i < SCC_TEMPLATE_PARAMS_NUM; i++) {
		temp_info->temp_param[i] = inbuf->temp_param[i];
	}

	ret = hihtr5_cmq_send(hr_dev, &desc, 1);
	if (ret)
		dev_err(hr_dev->dev,
				"failed to configure scc param, opcode: 0x%x, ret = %d.\n",
				le16_to_cpu(desc.opcode), ret);
	return ret;
}

STATIC int hihtr_v3_query_scc_temp(struct hihtr_dev *hr_dev, void *buffer)
{
	struct hihtr_cmq_desc desc;
	struct hihtr_scc_temp *temp_info = (struct hihtr_scc_temp *)desc.data;
	struct hihtr_dfx_scc_buf *outbuf = (struct hihtr_dfx_scc_buf *)buffer;
	int ret, i;

	if (HIHTR_GET_PCI_REVISION(hr_dev) <= PCI_REVISION_ID_HIP08 || hr_dev->is_vf)
		return -EOPNOTSUPP;

	hihtr_cmq_setup_basic_desc(&desc, HIHTR_OPC_QUERY_SCC_TEMP_CMD, true);
	temp_info->template_id = outbuf->template_id;
	ret = hihtr5_cmq_send(hr_dev, &desc, 1);
	if (ret) {
		dev_err(hr_dev->dev,
				"failed to query scc param, opcode: 0x%x, ret = %d.\n",
				le16_to_cpu(desc.opcode), ret);
		return ret;
	}

	for (i = 0; i < SCC_TEMPLATE_PARAMS_NUM; i++) {
		outbuf->temp_param[i] = temp_info->temp_param[i];
	}

	return 0;
}

STATIC int hihtr_v3_modify_cnp_tx_period(struct hihtr_dev *hr_dev, u32 tx_period)
{
	struct hihtr_cmq_desc desc;
	struct hihtr_cmq_req *req = (struct hihtr_cmq_req *)desc.data;
	int ret;

	hihtr_cmq_setup_basic_desc(&desc, HIHTR_OPC_SET_CNP_TX_PERIOD, false);
	hr_reg_write(req, CFG_CNP_TX_PERIOD, tx_period);
	ret = hihtr5_cmq_send(hr_dev, &desc, 1);
	if (ret) {
		dev_err(hr_dev->dev, "failed to set cnp_tx_period, ret = %d\n", ret);
	}

	return ret;
}

STATIC int hihtr_v3_query_cnp_tx_period(struct hihtr_dev *hr_dev, u32 *tx_period)
{
	struct hihtr_cmq_desc desc;
	struct hihtr_cmq_req *req = (struct hihtr_cmq_req *)desc.data;
	int ret;

	hihtr_cmq_setup_basic_desc(&desc, HIHTR_OPC_QUERY_CNP_TX_PERIOD, true);
	ret = hihtr5_cmq_send(hr_dev, &desc, 1);
	if (ret) {
		dev_err(hr_dev->dev, "failed to query cnp_tx_period, ret = %d\n", ret);
		return ret;
	}

	*tx_period = hr_reg_read(req, CFG_CNP_TX_PERIOD);
	return 0;
}

/* hihtr_hw_v3大部分继承v2的方法，v3特殊的函数单独注册 */
#if defined(RDMA_HAVE_IB_DEVICE_OPS)
static const struct ib_device_ops hihtr_v3_dev_srq_ops = {
#else
static const struct ib_device hihtr_v3_dev_srq_ops = {
#endif
	.modify_srq = hihtr_v3_modify_srq,
	.post_srq_recv = hihtr_v3_post_srq_recv,
	.query_srq = hihtr_v3_query_srq,
};

/* hihtr_hw_v3大部分继承v2的方法，v3特殊的函数单独注册 */
#if defined(RDMA_HAVE_IB_DEVICE_OPS)
static const struct ib_device_ops hihtr_v3_dev_ops = {
#else
static const struct ib_device hihtr_v3_dev_ops = {
#endif
	.destroy_qp = hihtr_v3_destroy_qp,
	.modify_cq = hihtr_v3_modify_cq,
	.poll_cq = hihtr_v3_poll_cq,
	.post_recv = hihtr_v3_post_recv,
	.post_send = hihtr_v3_post_send,
	.query_qp = hihtr_v3_query_qp,
	.req_notify_cq = hihtr_v3_req_notify_cq,
};

/* hihtr_hw_v3大部分继承v2的方法，v3特殊的函数单独注册 */
static const struct hihtr_hw hihtr_hw_v3 = {
	.cmq_init = NULL,
	.cmq_exit = NULL,
	.hw_profile = hihtr_v3_profile,
	.hw_init = hihtr_v3_init, /* todo:待适配 */
	.hw_exit = hihtr_v3_exit, /* todo:待适配 */
	.post_mbox = v3_post_mbox,
	.poll_mbox_done = v3_poll_mbox_done,
	.chk_mbox_avail = v3_chk_mbox_is_avail,
	.set_gid = hihtr_v3_set_gid,
	.set_mac = hihtr_v3_set_mac,
	.write_mtpt = hihtr_v3_write_mtpt,
	.rereg_write_mtpt = hihtr_v3_rereg_write_mtpt,
	.frmr_write_mtpt = hihtr_v3_frmr_write_mtpt,
	.mw_write_mtpt = hihtr_v3_mw_write_mtpt,
	.write_cqc = hihtr_v3_write_cqc,
	.set_hem = hihtr_v3_set_hem,
	.clear_hem = hihtr_v3_clear_hem,
	.set_dca_buf = hihtr_v3_set_dca_buf,
	.chk_dca_buf_inactive = hihtr_v3_chk_dca_buf_inactive,
	.modify_qp = hihtr_v3_modify_qp,
	.qp_flow_control_init = hihtr_v3_qp_flow_control_init,
	.init_eq = hihtr_v3_init_eq_table,
	.cleanup_eq = hihtr_v3_cleanup_eq_table,
	.write_srqc = hihtr_v3_write_srqc,
	.query_srqc = hihtr_v3_query_srqc,
	.query_aeqc = hihtr_v3_query_aeqc,
	.query_cqc = hihtr_v3_query_cqc,
	.query_qpc = hihtr_v3_query_qpc,
	.query_mpt = hihtr_v3_query_mpt,
	.query_sccc = hihtr_v3_query_sccc,
	.modify_eq = hihtr_v3_modify_eq,
	.modify_cnp_tx_period = hihtr_v3_modify_cnp_tx_period,
	.query_cnp_tx_period = hihtr_v3_query_cnp_tx_period,
	.get_cos = hihtr_hw_v3_get_cos,
	.hihtr_dev_ops = &hihtr_v3_dev_ops,
	.hihtr_dev_srq_ops = &hihtr_v3_dev_srq_ops,
	.query_hw_counter = hihtr_hw_v3_query_counter,
	.query_hw_dp_pkt_counter = hihtr_hw_v3_query_dp_pkt_counter,
	.set_scc = hihtr_hw_v3_set_scc,
	.query_scc = hihtr_hw_v3_query_scc,
	.set_scc_temp = hihtr_v3_set_scc_temp,
	.query_scc_temp = hihtr_v3_query_scc_temp,
};

static inline bool hihtr_func_is_vf(void *hwdev)
{
    return hinic5_func_type(hwdev) == TYPE_VF;
}

void hihtr_hw_v3_get_cfg(struct hihtr_dev *hr_dev,
	struct hinic5_lld_dev *lld_dev)
{
	struct hihtr_v3_priv *priv = hr_dev->priv;
	void __iomem *db_base = NULL; // 内核态doorbell的基址 virtual
	u64 udb_base;      // 用户态doorbell的基址 phy
	u64 udb_dweq_base; // 用户态direct weq的基址 phy 跟udb_base在同一页db
	int ret;

	if (lld_dev->dev_type == HINIC5_DEVICE_T_PCI) {
		hr_dev->pci_dev = to_pci_dev(lld_dev->dev);
	} else {
		hr_dev->pci_dev = NULL;
	}

	hr_dev->is_vf = hihtr_func_is_vf(lld_dev->hwdev);
	hr_dev->dev = lld_dev->dev;
	hr_dev->hw = &hihtr_hw_v3;

	ret = hinic5_alloc_db_addr(lld_dev->hwdev, &db_base, NULL);
	if (ret) {
		dev_info(hr_dev->dev, "Failed to allocate kernel doorbell");
	}
	ret = hinic5_alloc_db_phy_addr(lld_dev->hwdev, &udb_base, &udb_dweq_base);
	if (ret) {
		dev_info(hr_dev->dev, "Failed to allocate user doorbell");
	}
	hr_dev->reg_base = (u8 *)db_base;
	hr_dev->mem_base = (u8 *)udb_base;
	dev_info(hr_dev->dev, "k_db 0x%p, u_db 0x%llx, udb_dweq 0x%llx, is_vf:%d\n",
		db_base, udb_base, udb_dweq_base, hr_dev->is_vf);
	hr_dev->caps.num_ports = 1; // TODO: B502仅支持单port, B503适配
	hr_dev->iboe.netdevs[0] = hinic5_get_netdev_by_lld(lld_dev);
	hr_dev->iboe.phy_port[0] = 0;
	addrconf_addr_eui48((u8 *)&hr_dev->ib_dev.node_guid,
			    hr_dev->iboe.netdevs[0]->dev_addr);

	/* cmd issue mode: 0 is poll, 1 is event */
	hr_dev->cmd_mod = 0;
	hr_dev->loop_idc = 0;
	hr_dev->inline_type = inline_type;
	/* hinic5_lld_dev挂到hihtr_dev上 */
	priv->lld_dev = lld_dev;
}

static void enable_atomic_ops(struct hinic5_lld_dev *lld_dev)
{
    u16 devctl2;
	struct pci_dev *pdev = NULL;

	if (lld_dev->dev_type == HINIC5_DEVICE_T_UB) {
		dev_warn(lld_dev->dev, "AtomicOps not supported for UB devices\n");
		return;
	}

	pdev = to_pci_dev(lld_dev->dev);
    // 读取 Device Control 2 Register (PCIe Cap + 0x08)
    pcie_capability_read_word(pdev, PCI_EXP_DEVCTL2, &devctl2);

    if (devctl2 & PCI_DEVCTL2_ATOMICOP_REQ_EN) {
        dev_info(lld_dev->dev, "AtomicOps already enabled in DevCtl2\n");
        return;
    }

    // 启用 AtomicOp Requester
    devctl2 |= PCI_DEVCTL2_ATOMICOP_REQ_EN;
    pcie_capability_write_word(pdev, PCI_EXP_DEVCTL2, devctl2);

    // 验证
    pcie_capability_read_word(pdev, PCI_EXP_DEVCTL2, &devctl2);
    if (devctl2 & PCI_DEVCTL2_ATOMICOP_REQ_EN) {
        dev_info(lld_dev->dev, "AtomicOps enabled successfully via DevCtl2\n");
    } else {
        dev_warn(lld_dev->dev, "Failed to enable AtomicOps\n");
    }

    return;
}

bool roce_func_is_ppf(void *hwdev)
{
	return hinic5_func_type(hwdev) == TYPE_PPF;
}

#if defined(__x86_64__) && !defined(HIHTR_DT)
static void get_cpu_feature(void)
{
	if (cpu_feature_enabled(X86_FEATURE_AVX512F)) {
        pr_info("Hihtr: CPU supports AVX-512F. Enabling AVF512 for DWQE.\n");
    } else {
        pr_info("MyDriver: CPU lacks AVX-512F. Use normal copy instruction.\n");
    }
}
#endif

#if defined(RDMA_HAVE_XARRAY)
static DEFINE_XARRAY(hihtr_flr_func_id_map);
#else
static RADIX_TREE(hihtr_flr_func_id_map, GFP_KERNEL);
static DEFINE_SPINLOCK(hihtr_flr_func_id_map_lock);
#endif

/* roce 驱动加载主入口 */
int hihtr_hw_v3_probe(struct hinic5_lld_dev *lld_dev, void **uld_dev, char *uld_dev_name)
{
	struct hihtr_dev *hr_dev;
	int ret;
#ifndef RDMA_HAVE_XARRAY
	unsigned long flags;
	void *entry = NULL;
	unsigned long index = 0;
#endif
	pr_info("In hihtr_hw_v3_probe\n");

	/* return 0(false) if the hwdev don't support ROCE, make sure it probe success */
	if (hinic5_support_hihtr(lld_dev->hwdev) == false) {
		pr_info("%s Not support RoCE, func: %d", dev_name(lld_dev->dev), hinic5_global_func_id(lld_dev->hwdev));
		ret = -EINVAL;
		goto err_check;
	}

#if defined(__x86_64__) && !defined(HIHTR_DT)
	get_cpu_feature();
#endif

#if defined(IB_VERBS_HAVE__IB_ALLOC_DEVICE)
	hr_dev = ib_alloc_device(hihtr_dev, ib_dev);
#else
	hr_dev = (struct hihtr_dev *)ib_alloc_device(sizeof(struct hihtr_dev));
#endif
	if (!hr_dev) {
		return -ENOMEM;
	}
	dev_info(hr_dev->dev, "After ib_alloc_device\n");

	hr_dev->priv = kzalloc(sizeof(struct hihtr_v3_priv), GFP_KERNEL);
	if (!hr_dev->priv) {
		ret = -ENOMEM;
		goto error_failed_kzalloc;
	}

	/* 获取固件、PCIe配置信息 */
	hihtr_hw_v3_get_cfg(hr_dev, lld_dev);

	/* Register PCI device → func_id mapping for FLR flow-stop lookups.
	 * Only PCI-type devices have a pci_dev; skip non-PCI (UBUS) devices.
	 * Store (func_id + 1) to avoid xarray NULL sentinel collision with
	 * valid func_id 0.
	 */
	if (hr_dev->pci_dev) {
		u16 _func_id = hinic5_global_func_id(lld_dev->hwdev);

#if defined(RDMA_HAVE_XARRAY)
		xa_store(&hihtr_flr_func_id_map, (unsigned long)hr_dev->pci_dev,
			xa_mk_value(STORE_ENCODE_FLR_FUNC_ID_VAL(_func_id)), GFP_KERNEL);
#else
		entry = (void *)(unsigned long)(STORE_ENCODE_FLR_FUNC_ID_VAL(_func_id));
		index = (unsigned long)hr_dev->pci_dev;

		if (radix_tree_preload(GFP_KERNEL)) {
			dev_err(&hr_dev->pci_dev->dev, "radix_tree_preload failed\n");
			return -ENOMEM;
		}

		spin_lock_irqsave(&hihtr_flr_func_id_map_lock, flags);
		radix_tree_delete(&hihtr_flr_func_id_map, index);
		if (radix_tree_insert(&hihtr_flr_func_id_map, index, entry)) {
			dev_err(&hr_dev->pci_dev->dev, "radix_tree_insert failed\n");
		}
		spin_unlock_irqrestore(&hihtr_flr_func_id_map_lock, flags);

		radix_tree_preload_end();
#endif
		dev_info(hr_dev->dev, "FLR func_id mapping registered: pdev=%p func_id=%u\n",
			 hr_dev->pci_dev, _func_id);
	}

	if (roce_func_is_ppf(lld_dev->hwdev) == true) {
		WRITE_ONCE(hihtr_ppf_hwdev, lld_dev->hwdev);
		ret = hinic5_stateful_init(lld_dev->hwdev);
		if (ret != 0) {
			dev_err(hr_dev->dev, "%s Failed to init stateful resourse in ppf, func: %d",
					dev_name(lld_dev->dev), hinic5_global_func_id(lld_dev->hwdev));
			goto error_failed_roce_init;
		}
	}

	/* 使能atomic,vf继承pf */
	if (!hr_dev->is_vf)
		enable_atomic_ops(lld_dev);

	ret = hihtr_init(hr_dev, uld_dev_name);
	if (ret) {
		dev_err(hr_dev->dev, "hihtr_init failed! %d\n", ret);
		goto error_failed_free_hinic;
	}

	dev_info(hr_dev->dev, "state %d\n", hr_dev->state);
	hr_dev->state = HIHTR_DEVICE_STATE_INITED;
	dev_info(hr_dev->dev, "hr_dev caps 0x%llu\r\n", hr_dev->caps.flags);
	*uld_dev = hr_dev;
	dev_info(hr_dev->dev, "func %d probe success\n", hinic5_global_func_id(lld_dev->hwdev));
	return 0;


error_failed_free_hinic:
	hinic5_stateful_deinit(lld_dev->hwdev);

error_failed_roce_init:
	kfree(hr_dev->priv);

error_failed_kzalloc:
	ib_dealloc_device(&hr_dev->ib_dev);
err_check:
    *uld_dev = NULL;

	return ret;
}

static void hihtr_hw_v3_do_remove(struct hinic5_lld_dev *lld_dev, struct hihtr_dev *hr_dev, u16 glb_func_id)
{
	hr_dev->state = HIHTR_DEVICE_STATE_UNINIT;
	hihtr_handle_device_err(hr_dev);

	hihtr_exit(hr_dev);

	if (roce_func_is_ppf(lld_dev->hwdev) == true) {
		WRITE_ONCE(hihtr_ppf_hwdev, NULL);
		hinic5_stateful_deinit(lld_dev->hwdev);
	}

	// 释放doorbell
	hinic5_free_db_addr(lld_dev->hwdev, hr_dev->reg_base, NULL); // 释放内核态 doorbell page
	hinic5_free_db_phy_addr(lld_dev->hwdev, (u64)hr_dev->mem_base,
	    ((u64)hr_dev->mem_base + ROCE_V3_DB_DWEQ_OFFSET)); // 释放用户态对应的doorbell page

	kfree(hr_dev->priv);
	ib_dealloc_device(&hr_dev->ib_dev);
}

void hihtr_hw_v3_remove(struct hinic5_lld_dev *lld_dev, void *uld_dev)
{
#ifdef __UBUS_DRIVER__
	// 6.6内核下，驱动卸载仍有问题，需要进一步适配
	return;
#endif
	struct hihtr_dev *hr_dev = NULL;
	char* device_name = NULL;
	u16 glb_func_id;
	if ((uld_dev == NULL) || (lld_dev == NULL)) {
		pr_err("input param is NULL");
	}

	if (!lld_dev->hwdev) {
		pr_err("hwdev is NULL\n");
		return;
	}

	if (hinic5_support_hihtr(lld_dev->hwdev) == false) {
		pr_err("Not support RoCE\n");
		return;
	}

	hr_dev = (struct hihtr_dev*)uld_dev;
	device_name = hr_dev->ib_dev.name;
	glb_func_id = hr_dev->func_num;
	dev_info(hr_dev->dev, "Remove start, func_id(%d), name(%s), pci(%s)",
        hr_dev->func_num, device_name, dev_name(lld_dev->dev));

	// TODO: 需分析是否参考1823 roce5_g_rdev_remove适配
	hihtr_hw_v3_do_remove(lld_dev, hr_dev, glb_func_id);
    return;
}

void hihtr_hw_v3_event(struct hinic5_lld_dev *lld_dev, void *uld_dev, struct hinic5_event_info *event)
{
    /* todo */
    return;
}

int hihtr_adm_dfx_get_dev_type(struct hihtr_dev *hr_dev, const void *buf_in, u32 in_size, void *buf_out, u32 *out_size)
{
    union hihtr_dfx_common_outbuf *outbuf = (union hihtr_dfx_common_outbuf *)buf_out;

    // TODO 待实现查询设备名的逻辑
    if (in_size < sizeof(struct hihtr_dfx_common_inbuf)) {
        dev_err(hr_dev->dev, "Input parameter(in_size: %dB) is shorter than input buffer(allocated: %dB)",
                (int)in_size, (int)sizeof(struct hihtr_dfx_common_inbuf));
        return -ENOMEM;
    }

    outbuf->dev_type = 7; // 7 跟工具侧ROCE_DEV_TYPE_HI1872_V100对应, TODO: 待整改
    *out_size = (u32)sizeof(union hihtr_dfx_common_outbuf);
    return 0;
}

static hihtr_adm_cmd_proc_t g_hihtr_adm_cmd_proc[] = {
#ifdef __ROCE_DFX__
    { HIHTR_CMD_GET_QPC, hihtr_adm_dfx_query },
    { HIHTR_CMD_GET_CQC, hihtr_adm_dfx_query },
    { HIHTR_CMD_GET_SRQC, hihtr_adm_dfx_query },
    { HIHTR_CMD_GET_AEQC, hihtr_adm_dfx_query },
    { HIHTR_CMD_GET_MPT, hihtr_adm_dfx_query },
    { HIHTR_CMD_GET_GID, hihtr_adm_dfx_query },
    { HIHTR_CMD_GET_QPC_CQC_PI_CI, hihtr_adm_dfx_query },
    { HIHTR_CMD_GET_QP_COUNT, hihtr_adm_dfx_query },
	{ HIHTR_CMD_GET_HW_COUNT, hihtr_adm_dfx_query },
    { HIHTR_CMD_GET_DEV_TYPE, hihtr_adm_dfx_get_dev_type },
	{ HIHTR_CMD_GET_SPECIFICATIONS, hihtr_adm_dfx_query },
	{ HIHTR_CMD_SET_BYPASS, hihtr_dfx_cmd_set_bypass },
	{ HIHTR_CMD_QUERY_BYPASS, hihtr_dfx_cmd_query_bypass },
	{ HIHTR_CMD_SET_SCC_CTRL_INFO, hihtr_dfx_cmd_set_scc },
	{ HIHTR_CMD_QUERY_SCC_CTRL_INFO, hihtr_dfx_cmd_query_scc },
	{ HIHTR_CMD_SET_SCC_TEMPLATE_INFO, hihtr_dfx_cmd_set_scc_template },
	{ HIHTR_CMD_QUERY_SCC_TEMPLATE_INFO, hihtr_dfx_cmd_query_scc_template },
	{ HIHTR_CMD_GET_DRIVER_COUNT, hihtr_adm_dfx_query },
	{ HIHTR_CMD_GET_SCC_CTX, hihtr_adm_dfx_query },
	{ HIHTR_CMD_GET_DATA_PATH_MOD_PKT_CNT, hihtr_adm_dfx_query },
	{ HIHTR_CMD_ORDER_SET, hihtr_dfx_set_so_type },
	{ HIHTR_CMD_ORDER_QUERY, hihtr_dfx_query_so_type },
	/* 以下命令待适配 */
#endif /* __ROCE_DFX__ */
};

int hihtr_hw_v3_ioctl(void *uld_dev, u32 cmd, const void *buf_in, u32 in_size, void *buf_out, u32 *out_size)
{
    struct hihtr_dev *hr_dev = (struct hihtr_dev *)uld_dev;
	const hihtr_adm_generic_cmd_proc_t *generic_array =
		(const hihtr_adm_generic_cmd_proc_t *)g_hihtr_adm_cmd_proc;
    int elem_count = sizeof(g_hihtr_adm_cmd_proc) / sizeof(g_hihtr_adm_cmd_proc[0]);
    hihtr_adm_func_t hihtr_adm_func;
	struct hihtr_v3_priv *priv = hr_dev->priv;

    if (uld_dev == NULL || buf_in == NULL || buf_out == NULL || out_size == NULL) {
        dev_err(hr_dev->dev, "Input params is null");
        return -EINVAL;
    }

    if (hinic5_support_hihtr(priv->lld_dev->hwdev) == false) {
        dev_err(hr_dev->dev, "%s Not support RoCE.", dev_name(hr_dev->dev));
        return -EINVAL;
    }

    /* 检查是否为root用户 */
    if (!capable(CAP_SYS_ADMIN)) {
        dev_err(hr_dev->dev, "Invalid permission");
        return -EPERM;  /* 如果不是root，返回错误 */
    }

    if (cmd >= HIHTR_CMD_INVALID) {
        dev_err(hr_dev->dev, "Not support this type(%d)", cmd);
        return -EINVAL;
    } else if (cmd == GET_DRV_VERSION) {
        return hihtr_get_drv_version(buf_out, out_size, hr_dev);
    }

    hihtr_adm_func = (hihtr_adm_func_t)hihtr_adm_generic_find_cmd_func(generic_array, elem_count, cmd);
    if (hihtr_adm_func == NULL) {
        dev_err(hr_dev->dev, "Not support this type(%d)", cmd);
        return -EINVAL;
    }

    return hihtr_adm_func(hr_dev, buf_in, in_size, buf_out, out_size);
}

struct hinic5_uld_info hihtr_hw_v3_uld_info = {
    .probe = hihtr_hw_v3_probe,
    .remove = hihtr_hw_v3_remove,
    .suspend = NULL,
    .resume = NULL,
    .event = hihtr_hw_v3_event,
    .ioctl = hihtr_hw_v3_ioctl,
};

struct hinic5_uld_info *hihtr_hw_v3_uld_get(void)
{
    return &hihtr_hw_v3_uld_info;
}

static struct file *xrc_domain_fix_file;

#define HIHTR_PCI_VENDOR_ID_HUAWEI	0x19e5

typedef int (*pci_reset_fn_t)(struct pci_dev *pdev);
static pci_reset_fn_t real_pci_reset_function;

#define HIHTR_FLR_ACTIVE	2

#if defined(RDMA_HAVE_XARRAY)
static DEFINE_XARRAY(hihtr_flr_active);
#else
static RADIX_TREE(hihtr_flr_active, GFP_KERNEL);
static DEFINE_SPINLOCK(hihtr_flr_lock);
#endif

static bool hihtr_is_htr_device(struct pci_dev *pdev)
{
	u16 vendor, device;

	if (!pdev) {
		return false;
	}

	vendor = pdev->vendor;
	device = pdev->device;

	if (vendor != HIHTR_PCI_VENDOR_ID_HUAWEI)
		return false;

	if (device == HIHTR_PF_DEVICE_ID || device == HIHTR_VF_DEVICE_ID)
		return true;

	return false;
}

#if defined(RDMA_HAVE_XARRAY)
static void hihtr_flr_stop_flow(struct pci_dev *pdev)
{
	u16 func_id;
	void *entry = xa_load(&hihtr_flr_func_id_map, (unsigned long)pdev);

	if (!entry) {
		dev_warn(&pdev->dev, "func_id not found, skip stop_flow\n");
		return;
	}

	func_id = (u16)STORE_DECODE_FLR_FUNC_ID_VAL(xa_to_value(entry));
	hihtr_stop_flow(func_id);
	msleep(200);
}
#else
static void hihtr_flr_stop_flow(struct pci_dev *pdev)
{
	u16 func_id;
	unsigned long flags;
	void *entry;

    spin_lock_irqsave(&hihtr_flr_func_id_map_lock, flags);
    entry = radix_tree_lookup(&hihtr_flr_func_id_map, (unsigned long)pdev);
    spin_unlock_irqrestore(&hihtr_flr_func_id_map_lock, flags);

	if (!entry) {
		dev_warn(&pdev->dev, "func_id not found, skip stop_flow\n");
		return;
	}

	func_id = (u16)STORE_DECODE_FLR_FUNC_ID_VAL(((unsigned long)entry));
	hihtr_stop_flow(func_id);
	msleep(200);
}
#endif

static int hihtr_flr_hijack(struct pci_dev *pdev)
{
	int ret;

	dev_info(&pdev->dev, "stopping flow... before flr\n");

	hihtr_flr_stop_flow(pdev);

	if (real_pci_reset_function)
		ret = real_pci_reset_function(pdev);
	else
		ret = -ENOSYS;

#if defined(RDMA_HAVE_XARRAY)
	xa_erase(&hihtr_flr_active, (unsigned long)pdev);
#else
	radix_tree_delete(&hihtr_flr_active, (unsigned long)pdev);
#endif
	return ret;
}

static void hihtr_flr_post_handler(struct kprobe *kp, struct pt_regs *regs,
				   unsigned long flags)
{
	/* Intentionally empty */
}

static struct pci_dev *hihtr_get_pci_dev_arg(struct pt_regs *regs)
{
#if defined(CONFIG_ARM64)
	return (struct pci_dev *)regs->regs[0];
#else
	return (struct pci_dev *)regs->di;
#endif
}

static void hihtr_flr_redirect_to_hijack(struct pt_regs *regs)
{
	instruction_pointer_set(regs, (unsigned long)hihtr_flr_hijack);
}

#if defined(RDMA_HAVE_XARRAY)
static int hihtr_flr_pre_handler(struct kprobe *p, struct pt_regs *regs)
{
	struct pci_dev *pdev;
	void *old;

	pdev = hihtr_get_pci_dev_arg(regs);

	if (!hihtr_is_htr_device(pdev))
		return 0;

	dev_info(&pdev->dev, "FLR triggered\n");

	old = xa_cmpxchg(&hihtr_flr_active, (unsigned long)pdev, NULL, xa_mk_value(HIHTR_FLR_ACTIVE), GFP_NOWAIT);

	if (unlikely(xa_is_err(old))) {
		dev_warn(&pdev->dev, "xa_cmpxchg failed with error %d, skipping interception\n", xa_err(old));
		return 0;
	}

	if (old != NULL && xa_is_value(old) && xa_to_value(old) == HIHTR_FLR_ACTIVE) {
		dev_info(&pdev->dev, "re-entry, already claimed, passing through\n");
		return 0;
	}

	hihtr_flr_redirect_to_hijack(regs);

	return 1;
}
#else
static int hihtr_flr_pre_handler(struct kprobe *p, struct pt_regs *regs)
{
	struct pci_dev *pdev;
	unsigned long index;
	unsigned long flags;
	void *existing;
	int ret;

	pdev = hihtr_get_pci_dev_arg(regs);
	index = (unsigned long)pdev;

	if (!hihtr_is_htr_device(pdev))
		return 0;

	dev_info(&pdev->dev, "FLR triggered\n");
	ret = radix_tree_preload(GFP_NOWAIT);
	if (ret) {
		dev_warn(&pdev->dev, "radix_tree_preload failed, skipping interception\n");
		return 0;
	}

	spin_lock_irqsave(&hihtr_flr_lock, flags);
	existing = radix_tree_lookup(&hihtr_flr_active, index);

	if (existing != NULL) {
		spin_unlock_irqrestore(&hihtr_flr_lock, flags);
		radix_tree_preload_end();
		dev_info(&pdev->dev, "re-entry, already claimed, passing through\n");
		return 0;
	}

	ret = radix_tree_insert(&hihtr_flr_active, index, (void *)(unsigned long)(HIHTR_FLR_ACTIVE));

	spin_unlock_irqrestore(&hihtr_flr_lock, flags);
	radix_tree_preload_end();

	if (ret < 0) {
		dev_warn(&pdev->dev, "radix_tree_insert failed with error %d, skipping interception\n", ret);
		return 0;
	}

	hihtr_flr_redirect_to_hijack(regs);
	return 1;
}
#endif

static struct kprobe hihtr_flr_kp = {
	.symbol_name	= "__pci_reset_function_locked",
	.pre_handler	= hihtr_flr_pre_handler,
	.post_handler	= hihtr_flr_post_handler,
};
static bool flr_kp_registered;

/*
 * Description  : hihtr_roce 驱动 insmod 接口
 * Input        : void
 * Output       : NULL
 * Return Value : ret: 0 means success, other means fail
 */
int hihtr_hw_v3_init(void)
{
	int ret;

	pr_info("hihtr v3 init start!\n");
	/* 为了规避shutdown os时，卸载FS和清理IB资源的并发bug，持有一个tmpfs的文件，确保FS卸载流程阻塞
	 * 保证清理IB资源过程中不会因为并发bug，抛出空指针
	 */
	xrc_domain_fix_file = filp_open("/tmp/xrc_domain_fs_fix", O_CREAT | O_RDWR | O_TRUNC,
					S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (IS_ERR(xrc_domain_fix_file)) {
		pr_warn("XRC domain FS fix workaround disabled (ret=%ld), shutdown concurrency risk exists.\n",
			PTR_ERR(xrc_domain_fix_file));
		xrc_domain_fix_file = NULL;
	}

	ret = hinic5_register_uld(SERVICE_T_HIHTR, hihtr_hw_v3_uld_get());
	if (ret != 0) {
		pr_err("Failed to register hihtr V3 uld. ret(%d)\n", ret);
		if (xrc_domain_fix_file)
			filp_close(xrc_domain_fix_file, NULL);
		return ret;
	}

	real_pci_reset_function = (pci_reset_fn_t)
		kallsyms_lookup_name(hihtr_flr_kp.symbol_name);
	if (!real_pci_reset_function) {
		pr_warn("hihtr: failed to resolve %s, FLR interception disabled\n", hihtr_flr_kp.symbol_name);
		goto out;
	}

	ret = register_kprobe(&hihtr_flr_kp);
	if (ret < 0) {
		pr_warn("hihtr: register_kprobe(__pci_reset_function_locked) failed: %d, "
			"FLR interception disabled\n", ret);
		goto out;
	}
	flr_kp_registered = true;

out:
	pr_info("hihtr v3 init succ!\n");
	return 0;
}

/*
 * Description  : hihtr_roce 驱动 rmmod 接口
 * Input        : void
 * Output       : NULL
 * Return Value : void
 */
void hihtr_hw_v3_exit(void)
{
	if (flr_kp_registered)
		unregister_kprobe(&hihtr_flr_kp);

#if defined(RDMA_HAVE_XARRAY)
	xa_destroy(&hihtr_flr_active);
	xa_destroy(&hihtr_flr_func_id_map);
#else
	INIT_RADIX_TREE(&hihtr_flr_active, GFP_KERNEL);
	INIT_RADIX_TREE(&hihtr_flr_func_id_map, GFP_KERNEL);
#endif
	hinic5_unregister_uld(SERVICE_T_HIHTR);
	if (xrc_domain_fix_file)
		filp_close(xrc_domain_fix_file, NULL);
	pr_info("hihtr v3 exit succ!\n");
}

module_init(hihtr_hw_v3_init);
module_exit(hihtr_hw_v3_exit);