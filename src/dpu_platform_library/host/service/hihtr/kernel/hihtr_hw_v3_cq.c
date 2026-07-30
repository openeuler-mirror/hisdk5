/*
 * Copyright (c) 2016 Hisilicon Limited.
 * Copyright (c) 2007, 2008 Mellanox Technologies. All rights reserved.
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
#include <linux/acpi.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <rdma/ib_addr.h>
#include <rdma/ib_mad.h>
#include <rdma/ib_user_verbs.h>
#include <rdma/ib_cache.h>
#include <rdma/uverbs_ioctl.h>

#include "hihtr_common.h"
#include "hihtr_device.h"
#include "hihtr_hem.h"
#include "hihtr_dca.h"
#include "hihtr_debugfs.h"
#include "hihtr_cmdq.h"
#include "hihtr_cmd.h"
#include "hihtr_hw_v3_cq.h"
#include "hihtr_hw_v3_qp.h"
#include "hihtr_hw_v3.h"
#include "shared/hihtr_shared.h"

/* ******************__hihtr_v3_cq_clean start **************** */
void *get_cqe_v3(struct hihtr_cq *hr_cq, int n)
{
	return hihtr_buf_offset(hr_cq->mtr.kmem, n * hr_cq->cqe_size);
}

void *get_sw_cqe_v3(struct hihtr_cq *hr_cq, unsigned int n)
{
	struct hihtr_v3_cqe *cqe = NULL;
	u32 cq_real_depth = hr_cq->cq_depth;
	if (hr_cq->resize_cq_en == HIHTR_CQ_RESIZE_CQ_ENABLE) {
		cq_real_depth = hr_cq->resize_cq_hw_cq_depth;
	}
	cqe = get_cqe_v3(hr_cq, n & (cq_real_depth - 1));

    /* Get cqe when Owner bit is Conversely with the MSB of cons_idx */
    return (hr_reg_read(cqe, CQE_OWNER) ^ !!(n & cq_real_depth)) ? cqe : NULL;
}

void hihtr_free_srq_wqe(struct hihtr_srq *srq, u32 wqe_index)
{
	/* always called with interrupts disabled. */
	spin_lock(&srq->lock);

	bitmap_clear(srq->idx_que.bitmap, wqe_index, 1);
	srq->idx_que.tail++;

	spin_unlock(&srq->lock);
}


void update_cq_db(struct hihtr_dev *hr_dev,
				struct hihtr_cq *hr_cq)
{
	if (likely(hr_cq->flags & HIHTR_CQ_FLAG_RECORD_DB)) {
		*hr_cq->set_ci_db = hr_cq->cons_index & V3_CQ_DB_CONS_IDX_M;
	} else {
		struct hihtr_v3_db cq_db = {0};

		hr_reg_write(&cq_db, DB_Q_NUM, hr_cq->cqn);
		hr_reg_write(&cq_db, DB_CN, HIHTR_V3_DB_CQ_CN);
		hr_reg_write(&cq_db, DB_COS, HIHTR_COS_MIN);
		hr_reg_write(&cq_db, DB_SRV, HIHTR_V3_DB_CQ_SRV);
		hr_reg_write(&cq_db, DB_CQ_CI, hr_cq->cons_index);
		// TODO: notify字段默认填0
		hr_reg_write(&cq_db, DB_CQ_CMD_SN, 1);

		// cq的door bell直接敲到reg base即可
		dev_info(hr_dev->dev, "cq ring db 0x%x, 0x%x\r\n", cq_db.data[0], cq_db.data[1]); // TODO: 后续删除
		hihtr_write64(hr_dev, (__le32 *)&cq_db, hr_dev->reg_base);
	}
}

void __hihtr_v3_cq_clean(struct hihtr_cq *hr_cq, u32 qpn,
				   struct hihtr_srq *srq)
{
	struct hihtr_dev *hr_dev = to_hr_dev(hr_cq->ib_cq.device);
	struct hihtr_v3_cqe *cqe, *dest;
	u32 prod_index;
	int nfreed = 0;
	int wqe_index;
	u8 owner_bit;
	u32 cq_mask = hr_cq->cq_depth - 1;

	if (hr_cq->resize_cq_en == HIHTR_CQ_RESIZE_CQ_ENABLE) {
		cq_mask = hr_cq->resize_cq_hw_cq_depth;
	}
	for (prod_index = hr_cq->cons_index; get_sw_cqe_v3(hr_cq, prod_index);
	     ++prod_index) {
		if (prod_index > hr_cq->cons_index + cq_mask)
			break;
	}

	/*
	 * Now backwards through the CQ, removing CQ entries
	 * that match our QP by overwriting them with next entries.
	 */
	while ((int) --prod_index - (int) hr_cq->cons_index >= 0) {
		cqe = get_cqe_v3(hr_cq, prod_index & cq_mask);
		if (hr_reg_read(cqe, CQE_LCL_QPN) == qpn) {
			if (srq && hr_reg_read(cqe, CQE_S_R)) {
				wqe_index = hr_reg_read(cqe, CQE_WQE_IDX);
				hihtr_free_srq_wqe(srq, wqe_index);
			}
			++nfreed;
		} else if (nfreed) {
			dest = get_cqe_v3(hr_cq, (prod_index + nfreed) &
					  cq_mask);
			owner_bit = hr_reg_read(dest, CQE_OWNER);
			memcpy(dest, cqe, hr_cq->cqe_size);
			hr_reg_write(dest, CQE_OWNER, owner_bit);
		}
	}

	if (nfreed) {
		hr_cq->cons_index += nfreed;
		wmb();
		update_cq_db(hr_dev, hr_cq);
	}
}

void hihtr_v3_cq_clean(struct hihtr_cq *cq, unsigned int qpn,
				 struct hihtr_srq *srq)
{
	spin_lock_irq(&cq->lock);
	__hihtr_v3_cq_clean(cq, qpn, srq);
	spin_unlock_irq(&cq->lock);
}

int hihtr_v3_query_cqc(struct hihtr_dev *hr_dev, u32 cqn,
				 void *buffer)
{
	struct hihtr_v3_cq_context *context;
	struct hihtr_cmd_mailbox *mailbox;
	int ret;
	int count = hr_dev->cq_table.table.obj_count;

	if (cqn >= count) {
		return -EINVAL;
	}
    dev_info(hr_dev->dev, "hihtr_v3_query_cqc\n");
	mailbox = hihtr_alloc_cmd_mailbox(hr_dev);
	if (IS_ERR(mailbox))
		return PTR_ERR(mailbox);

	context = mailbox->buf;
	ret = hihtr_cmq_mbox(hr_dev, 0, mailbox->dma,
				HIHTR_CMD_QUERY_CQC, cqn);
	if (ret) {
		ibdev_err(&hr_dev->ib_dev,
			  "failed to process cmd when querying CQ, ret = %d.\n",
			  ret);
		goto err_mailbox;
	}

	memcpy(buffer, context, sizeof(*context));

err_mailbox:
	hihtr_free_cmd_mailbox(hr_dev, mailbox);

	return ret;
}

void hihtr_v3_write_cqc(struct hihtr_dev *hr_dev,
				  struct hihtr_cq *hr_cq, void *mb_buf,
				  u64 *mtts, dma_addr_t dma_handle)
{
    struct hihtr_v3_cq_context *cq_context;

	cq_context = mb_buf;
	memset(cq_context, 0, sizeof(*cq_context));

	hr_reg_write(cq_context, CQC_CQ_ST, V2_CQ_STATE_VALID);
	hr_reg_write(cq_context, CQC_ARM_ST, NO_ARMED);
	if (hr_cq->resize_cq_en == HIHTR_CQ_RESIZE_CQ_ENABLE) {
		hr_reg_write(cq_context, CQC_SHIFT, ilog2(hr_cq->resize_cq_hw_cq_depth));
	} else {
		hr_reg_write(cq_context, CQC_SHIFT, ilog2(hr_cq->cq_depth));
	}
	hr_reg_write(cq_context, CQC_CEQN, hr_cq->vector);
	hr_reg_write(cq_context, CQC_CQN, hr_cq->cqn);

	if (hr_cq->cqe_size == HIHTR_V3_CQE_SIZE)
		hr_reg_write(cq_context, CQC_CQE_SIZE, CQE_SIZE_64B);

	if (hr_dev->caps.flags & HIHTR_CAP_FLAG_STASH)
		hr_reg_enable(cq_context, CQC_STASH);

	hr_reg_write(cq_context, CQC_CQE_CUR_BLK_ADDR_L,
		     to_hr_hw_page_addr(mtts[0]));
	hr_reg_write(cq_context, CQC_CQE_CUR_BLK_ADDR_H,
		     upper_32_bits(to_hr_hw_page_addr(mtts[0])));
	hr_reg_write(cq_context, CQC_CQE_HOP_NUM, hr_dev->caps.cqe_hop_num ==
		     HIHTR_HOP_NUM_0 ? 0 : hr_dev->caps.cqe_hop_num);
	hr_reg_write(cq_context, CQC_CQE_NEX_BLK_ADDR_L,
		     to_hr_hw_page_addr(mtts[1]));
	hr_reg_write(cq_context, CQC_CQE_NEX_BLK_ADDR_H,
		     upper_32_bits(to_hr_hw_page_addr(mtts[1])));
	hr_reg_write(cq_context, CQC_CQE_BAR_PG_SZ,
		     to_hr_hw_page_shift(hr_cq->mtr.hem_cfg.ba_pg_shift));
	hr_reg_write(cq_context, CQC_CQE_BUF_PG_SZ,
		     to_hr_hw_page_shift(hr_cq->mtr.hem_cfg.buf_pg_shift));
	hr_reg_write(cq_context, CQC_CQE_BA_L, dma_handle >> CQC_CQE_BA_L_SHIFT);
	hr_reg_write(cq_context, CQC_CQE_BA_H, (dma_handle >> CQC_CQE_BA_H_SHIFT));
	hr_reg_write_bool(cq_context, CQC_DB_RECORD_EN,
			  hr_cq->flags & HIHTR_CQ_FLAG_RECORD_DB);
	hr_reg_write(cq_context, CQC_CQE_DB_RECORD_ADDR_L,
		     ((u32)hr_cq->db.dma) >> CQC_CQE_DB_RECORD_ADDR_L_SHIFT);
	hr_reg_write(cq_context, CQC_CQE_DB_RECORD_ADDR_H,
		     hr_cq->db.dma >> CQC_CQE_DB_RECORD_ADDR_H_SHIFT);
	hr_reg_write(cq_context, CQC_CQ_MAX_CNT,
		     HIHTR_V2_CQ_DEFAULT_BURST_NUM);
	hr_reg_write(cq_context, CQC_CQ_PERIOD,
		     HIHTR_V2_CQ_DEFAULT_INTERVAL);
}

static bool check_dca_detach_enable(struct hihtr_qp *hr_qp)
{
	return hr_qp->en_flags & HIHTR_QP_CAP_DYNAMIC_CTX_DETACH;
}

static int sw_comp(struct hihtr_qp *hr_qp, struct hihtr_wq *wq,
		   int num_entries, struct ib_wc *wc)
{
	unsigned int left;
	int npolled = 0;

	left = wq->head - wq->tail;
	if (left == 0)
		return 0;

	left = min_t(unsigned int, (unsigned int)num_entries, left);
	while (npolled < left) {
		wc->wr_id = wq->wrid[wq->tail & (wq->wqe_cnt - 1)];
		wc->status = IB_WC_WR_FLUSH_ERR;
		wc->vendor_err = 0;
		wc->qp = &hr_qp->ibqp;

		wq->tail++;
		wc++;
		npolled++;
	}

	return npolled;
}

static int hihtr_v3_sw_poll_cq(struct hihtr_cq *hr_cq, int num_entries,
				  struct ib_wc *wc)
{
	struct hihtr_qp *hr_qp;
	int npolled = 0;

	list_for_each_entry(hr_qp, &hr_cq->sq_list, sq_node) {
		npolled += sw_comp(hr_qp, &hr_qp->sq,
				   num_entries - npolled, wc + npolled);
		if (npolled >= num_entries)
			goto out;
	}

	list_for_each_entry(hr_qp, &hr_cq->rq_list, rq_node) {
		npolled += sw_comp(hr_qp, &hr_qp->rq,
				   num_entries - npolled, wc + npolled);
		if (npolled >= num_entries)
			goto out;
	}

out:
	return npolled;
}

static void get_cqe_status(struct hihtr_dev *hr_dev, struct hihtr_qp *qp,
			   struct hihtr_cq *cq, struct hihtr_v3_cqe *cqe,
			   struct ib_wc *wc)
{
	static const struct {
		u32 cqe_status;
		enum ib_wc_status wc_status;
	} map[] = {
		{ HIHTR_CQE_V2_SUCCESS, IB_WC_SUCCESS },
		{ HIHTR_CQE_V2_LOCAL_LENGTH_ERR, IB_WC_LOC_LEN_ERR },
		{ HIHTR_CQE_V2_LOCAL_QP_OP_ERR, IB_WC_LOC_QP_OP_ERR },
		{ HIHTR_CQE_V2_LOCAL_PROT_ERR, IB_WC_LOC_PROT_ERR },
		{ HIHTR_CQE_V2_WR_FLUSH_ERR, IB_WC_WR_FLUSH_ERR },
		{ HIHTR_CQE_V2_MW_BIND_ERR, IB_WC_MW_BIND_ERR },
		{ HIHTR_CQE_V2_BAD_RESP_ERR, IB_WC_BAD_RESP_ERR },
		{ HIHTR_CQE_V2_LOCAL_ACCESS_ERR, IB_WC_LOC_ACCESS_ERR },
		{ HIHTR_CQE_V2_REMOTE_INVAL_REQ_ERR, IB_WC_REM_INV_REQ_ERR },
		{ HIHTR_CQE_V2_REMOTE_ACCESS_ERR, IB_WC_REM_ACCESS_ERR },
		{ HIHTR_CQE_V2_REMOTE_OP_ERR, IB_WC_REM_OP_ERR },
		{ HIHTR_CQE_V2_TRANSPORT_RETRY_EXC_ERR,
		  IB_WC_RETRY_EXC_ERR },
		{ HIHTR_CQE_V2_RNR_RETRY_EXC_ERR, IB_WC_RNR_RETRY_EXC_ERR },
		{ HIHTR_CQE_V2_REMOTE_ABORT_ERR, IB_WC_REM_ABORT_ERR },
		{ HIHTR_CQE_V2_GENERAL_ERR, IB_WC_GENERAL_ERR}
	};

	u32 cqe_status = hr_reg_read(cqe, CQE_STATUS);
	int i;

	wc->status = IB_WC_GENERAL_ERR;
	wc->vendor_err = hr_reg_read(cqe, CQE_SUB_STATUS);
	for (i = 0; i < ARRAY_SIZE(map); i++)
		if (cqe_status == map[i].cqe_status) {
			wc->status = map[i].wc_status;
			break;
		}

	if (likely(wc->status == IB_WC_SUCCESS ||
		   wc->status == IB_WC_WR_FLUSH_ERR ||
		   (wc->status == IB_WC_GENERAL_ERR && wc->vendor_err == SGE_LENGTH_ERR)))
		return;

	ibdev_err_ratelimited(&hr_dev->ib_dev, "error cqe status 0x%x: substatus 0x%x\n",
			      cqe_status, wc->vendor_err);
	print_hex_dump(KERN_DEBUG, "", DUMP_PREFIX_NONE, 16, 4, cqe,
		       cq->cqe_size, false);

	flush_cqe(hr_dev, qp);
}

#define HR_WC_OP_MAP(hr_key, ib_key) \
		[HIHTR_V3_WQE_OP_ ## hr_key] = 1 + IB_WC_ ## ib_key

static const u32 wc_send_op_map[] = {
	HR_WC_OP_MAP(SEND,			SEND),
	HR_WC_OP_MAP(SEND_WITH_INV,		SEND),
	HR_WC_OP_MAP(SEND_WITH_IMM,		SEND),
	HR_WC_OP_MAP(RDMA_READ,			RDMA_READ),
	HR_WC_OP_MAP(RDMA_WRITE,		RDMA_WRITE),
	HR_WC_OP_MAP(RDMA_WRITE_WITH_IMM,	RDMA_WRITE),
	HR_WC_OP_MAP(LOCAL_INV,			LOCAL_INV),
	HR_WC_OP_MAP(ATOM_CMP_AND_SWAP,		COMP_SWAP),
	HR_WC_OP_MAP(ATOM_FETCH_AND_ADD,	FETCH_ADD),
	HR_WC_OP_MAP(ATOM_MSK_CMP_AND_SWAP,	MASKED_COMP_SWAP),
	HR_WC_OP_MAP(ATOM_MSK_FETCH_AND_ADD,	MASKED_FETCH_ADD),
	HR_WC_OP_MAP(FAST_REG_PMR,		REG_MR),
	HR_WC_OP_MAP(BIND_MW,			REG_MR),
};

static int to_ib_wc_send_op(u32 hr_opcode)
{
	if (hr_opcode >= ARRAY_SIZE(wc_send_op_map))
		return -EINVAL;

	return wc_send_op_map[hr_opcode] ? wc_send_op_map[hr_opcode] - 1 :
					   -EINVAL;
}

static const u32 wc_recv_op_map[] = {
	HR_WC_OP_MAP(RDMA_WRITE_WITH_IMM,		WITH_IMM),
	HR_WC_OP_MAP(SEND,				RECV),
	HR_WC_OP_MAP(SEND_WITH_IMM,			WITH_IMM),
	HR_WC_OP_MAP(SEND_WITH_INV,			RECV),
};

static int to_ib_wc_recv_op(u32 hr_opcode)
{
	if (hr_opcode >= ARRAY_SIZE(wc_recv_op_map))
		return -EINVAL;

	return wc_recv_op_map[hr_opcode] ? wc_recv_op_map[hr_opcode] - 1 :
					   -EINVAL;
}

static void fill_send_wc(struct ib_wc *wc, struct hihtr_v3_cqe *cqe)
{
	u32 hr_opcode;
	int ib_opcode;
	u32 status;

	wc->wc_flags = 0;
	status = hr_reg_read(cqe, CQE_STATUS);
	if (status != HIHTR_CQE_V2_SUCCESS) {
		return;
	}

	hr_opcode = hr_reg_read(cqe, CQE_OPCODE);
	switch (hr_opcode) {
		case HIHTR_V3_WQE_OP_RDMA_READ:
			wc->byte_len = le32_to_cpu(cqe->byte_cnt);
			break;
		case HIHTR_V3_WQE_OP_SEND_WITH_IMM:
		case HIHTR_V3_WQE_OP_RDMA_WRITE_WITH_IMM:
			wc->wc_flags |= IB_WC_WITH_IMM;
			break;
		case HIHTR_V3_WQE_OP_ATOM_CMP_AND_SWAP:
		case HIHTR_V3_WQE_OP_ATOM_FETCH_AND_ADD:
		case HIHTR_V3_WQE_OP_ATOM_MSK_CMP_AND_SWAP:
		case HIHTR_V3_WQE_OP_ATOM_MSK_FETCH_AND_ADD:
			wc->byte_len = HIHTR_V3_ATOMIC_DATA_LENGTH;
			break;
		default:
			break;
	}

	ib_opcode = to_ib_wc_send_op(hr_opcode);
	if (ib_opcode < 0)
		wc->status = IB_WC_GENERAL_ERR;
	else
		wc->opcode = ib_opcode;
}

static void fill_recv_wc_from_opcode(struct ib_wc *wc, struct hihtr_v3_cqe *cqe)
{
	u32 hr_opcode = 0;
	int ib_opcode;

	hr_opcode = hr_reg_read(cqe, CQE_OPCODE);
	switch (hr_opcode) {
		case HIHTR_V2_OPCODE_RDMA_WRITE_IMM:
		case HIHTR_V2_OPCODE_SEND_WITH_IMM:
		case HIHTR_V2_OPCODE_PERSISTENCE_WRITE_WITH_IMM:
			wc->wc_flags = IB_WC_WITH_IMM;
			wc->ex.imm_data = cpu_to_be32(le32_to_cpu(cqe->immtdata));
			break;
		case HIHTR_V2_OPCODE_SEND_WITH_INV:
			wc->wc_flags = IB_WC_WITH_INVALIDATE;
			wc->ex.invalidate_rkey = le32_to_cpu(cqe->rkey);
			break;
		default:
			wc->wc_flags = 0;
	}

	ib_opcode = to_ib_wc_recv_op(hr_opcode);
	if (ib_opcode < 0)
		wc->status = IB_WC_GENERAL_ERR;
	else
		wc->opcode = ib_opcode;
}

static int fill_recv_wc(struct ib_wc *wc, struct hihtr_v3_cqe *cqe)
{
	u32 status;

	status = hr_reg_read(cqe, CQE_STATUS);
	// status异常opcode字段无效，不再根据opcode进行相关置位
	if (status == HIHTR_CQE_V2_SUCCESS) {
		fill_recv_wc_from_opcode(wc, cqe);
	} else {
		wc->wc_flags = 0;
	}
	wc->byte_len = le32_to_cpu(cqe->byte_cnt);
	wc->sl = hr_reg_read(cqe, CQE_SL);
	wc->src_qp = hr_reg_read(cqe, CQE_RMT_QPN);
	wc->slid = 0;
	wc->wc_flags |= hr_reg_read(cqe, CQE_GRH) ? IB_WC_GRH : 0;
	wc->port_num = hr_reg_read(cqe, CQE_PORTN);
	wc->pkey_index = 0;

	if (hr_reg_read(cqe, CQE_VID_VLD)) {
		wc->vlan_id = hr_reg_read(cqe, CQE_VID);
		wc->wc_flags |= IB_WC_WITH_VLAN;
	} else {
		wc->vlan_id = 0xffff;
	}

	wc->network_hdr_type = hr_reg_read(cqe, CQE_PORT_TYPE);

	return 0;
}

void hihtr_inv_work_handler(struct work_struct *work)
{
	struct hihtr_inv_work *inv_work = container_of(work, struct hihtr_inv_work, work);
	struct hihtr_dev *hr_dev;
	unsigned long mtpt_idx;
	struct hihtr_v3_mpt_entry context;
	struct hihtr_cmd_mailbox *mailbox;
	int ret;

	/* Early exit if QP is being destroyed. */
	if (smp_load_acquire(&inv_work->done))
		return;

	hr_dev = inv_work->hr_dev;
	mtpt_idx = key_to_hw_index(inv_work->rkey) & (hr_dev->caps.num_mtpts - 1);

	/* Step 1: Query MPT to save current hardware context */
	ret = hr_dev->hw->query_mpt(hr_dev, inv_work->rkey, &context);
	if (ret) {
		dev_err(hr_dev->dev,
			"query MPT failed, rkey=0x%x, ret=%d\n",
			inv_work->rkey, ret);
		smp_store_release(&inv_work->done, true);
		return;
	}

	/* Step 2: Create MPT with status=FREE (re-enable MR key for reuse) */
	mailbox = hihtr_alloc_cmd_mailbox(hr_dev);
	if (IS_ERR(mailbox)) {
		dev_err(hr_dev->dev,
			"alloc mailbox failed, rkey=0x%x\n", inv_work->rkey);
		smp_store_release(&inv_work->done, true);
		return;
	}

	/* Copy saved context to DMA buffer, then modify MPT_ST to FREE */
	memcpy(mailbox->buf, &context, sizeof(context));
	hr_reg_write(mailbox->buf, MPT_ST, V2_MPT_ST_FREE);

	ret = hihtr_cmq_mbox(hr_dev, mailbox->dma, 0, HIHTR_CMD_CREATE_MPT,
			      mtpt_idx);
	if (ret)
		dev_err(hr_dev->dev,
			"create MPT failed, rkey=0x%x, ret=%d\n",
			inv_work->rkey, ret);

	hihtr_free_cmd_mailbox(hr_dev, mailbox);
	smp_store_release(&inv_work->done, true);
}

static void dfx_stats_count(struct hihtr_dev *hr_dev, struct ib_wc *wc)
{
	hihtr_history_cqe_error_count *err_count = NULL;

	if (wc == NULL || wc->status == IB_WC_SUCCESS || hr_dev->kernel_count_buf == NULL) {
		return;
	}

	err_count = &(hr_dev->kernel_count_buf->cqe_error_count);
	switch (wc->vendor_err) {
		case HIHTR_TRP_SUBTYPE_PSN_ERR:
			atomic64_fetch_add_relaxed(1, (atomic64_t *)&err_count->hihtr_rq_psn_error_count);
			break;
		case HIHTR_TRP_SUBTYPE_RNR:
		case HIHTR_TRP_SUBTYPE_RNR_EXCEED:
			atomic64_fetch_add_relaxed(1, (atomic64_t *)&err_count->hihtr_rq_rnr_error_count);
			break;
		case HIHTR_TRP_SUBTYPE_NAK_INVALID_REQUEST:
		case HIHTR_TRP_SUBTYPE_NAK_REMOTE_ACCESS_ERROR:
		case HIHTR_TRP_SUBTYPE_NAK_REMOTE_OPERATION_ERROR:
		case HIHTR_TRP_SUBTYPE_NAK_INVALID_RD_REQUEST:
		case HIHTR_TRP_SUBTYPE_NAK_PAYLOAD_POISON:
			atomic64_fetch_add_relaxed(1, (atomic64_t *)&err_count->hihtr_rq_nak_error_count);
			break;
		default:
			break;
	}
}

static int hihtr_v3_poll_one(struct hihtr_cq *hr_cq,
				struct hihtr_qp **cur_qp, struct ib_wc *wc,
				bool *ud_len_err_flg)
{
	struct hihtr_dev *hr_dev = to_hr_dev(hr_cq->ib_cq.device);
	struct hihtr_qp *qp = *cur_qp;
	struct hihtr_srq *srq = NULL;
	struct hihtr_v3_cqe *cqe;
	struct hihtr_wq *wq;
	int is_send;
	u16 wqe_idx;
	int ret;

	cqe = get_sw_cqe_v3(hr_cq, hr_cq->cons_index);
	if (!cqe)
		return -EAGAIN;

	++hr_cq->cons_index;
	/* Memory barrier */
	rmb();

	ret = get_cur_qp(hr_cq, cqe, &qp);
	if (ret)
		return ret;

	wc->qp = &qp->ibqp;
	wc->vendor_err = 0;

	wqe_idx = hr_reg_read(cqe, CQE_WQE_IDX);

	is_send = !hr_reg_read(cqe, CQE_S_R);
	if (is_send) {
		wq = &qp->sq;

		/* If sg_signal_bit is set, tail pointer will be updated to
		 * the WQE corresponding to the current CQE.
		 */
		if (qp->sq_signal_bits)
			wq->tail += (wqe_idx - (u16)wq->tail) &
				    (wq->wqe_cnt - 1);

		wc->wr_id = wq->wrid[wq->tail & (wq->wqe_cnt - 1)];
		if (hr_reg_read(cqe, CQE_OPCODE) == HIHTR_V3_WQE_OP_NOP) {
			u32 tail_idx = wq->tail & (wq->wqe_cnt - 1);

			wq->inv_work.hr_dev = hr_dev;
			wq->inv_work.rkey = wq->inv_rkey[tail_idx];
			wq->inv_work.done = false;
			queue_work(hr_dev->irq_workq, &wq->inv_work.work);

			/* Busy-wait for inv_work completion (safe in spinlock) */
			while (!smp_load_acquire(&wq->inv_work.done))
				cpu_relax();

			wc->opcode = IB_WC_LOCAL_INV;
			wc->status = IB_WC_SUCCESS;
			wc->byte_len = 0;
			wc->wc_flags = 0;
			wc->ex.invalidate_rkey = wq->inv_rkey[tail_idx];
		} else {
			fill_send_wc(wc, cqe);
		}

		++wq->tail;
		hr_cq->dfx_cnt[HIHTR_CQ_SEND_CQE]++;
	} else {
		if (qp->ibqp.srq) {
			srq = to_hr_srq(qp->ibqp.srq);
			wc->wr_id = srq->wrid[wqe_idx];
			hihtr_free_srq_wqe(srq, wqe_idx);
		} else {
			wq = &qp->rq;
			wc->wr_id = wq->wrid[wq->tail & (wq->wqe_cnt - 1)];
			++wq->tail;
		}

		ret = fill_recv_wc(wc, cqe);
		hr_cq->dfx_cnt[HIHTR_CQ_RECV_CQE]++;
	}

	get_cqe_status(hr_dev, qp, hr_cq, cqe, wc);
	dfx_stats_count(hr_dev, wc);
	// UD不使用srq，出现length error场景，静默丢弃不上报
	if (wc->status == IB_WC_GENERAL_ERR && wc->vendor_err == SGE_LENGTH_ERR) {
		*ud_len_err_flg = true;
		return -EAGAIN;
	}

	if (unlikely(wc->status != IB_WC_SUCCESS))
		return 0;

	return ret;
}

int hihtr_v3_poll_cq(struct ib_cq *ibcq, int num_entries,
			       struct ib_wc *wc)
{
	struct hihtr_dev *hr_dev = to_hr_dev(ibcq->device);
	struct hihtr_cq *hr_cq = to_hr_cq(ibcq);
	struct hihtr_qp *cur_qp = NULL;
	unsigned long flags;
	int npolled;
	int ret;
	bool ud_len_err_flg = false; // 处理UD静默丢包场景，更新cq db

	spin_lock_irqsave(&hr_cq->lock, flags);

	/*
	 * When the device starts to reset, the state is RST_DOWN. At this time,
	 * there may still be some valid CQEs in the hardware that are not
	 * polled. Therefore, it is not allowed to switch to the software mode
	 * immediately. When the state changes to UNINIT, CQE no longer exists
	 * in the hardware, and then switch to software mode.
	 */
	if (hr_dev->state == HIHTR_DEVICE_STATE_UNINIT) {
		npolled = hihtr_v3_sw_poll_cq(hr_cq, num_entries, wc);
		goto out;
	}

	for (npolled = 0; npolled < num_entries; ++npolled) {
		if ((hr_cq->resize_cq_en == HIHTR_CQ_RESIZE_CQ_ENABLE) && (npolled == hr_cq->cq_depth)) {
			break;
		}
		ret = hihtr_v3_poll_one(hr_cq, &cur_qp, wc + npolled, &ud_len_err_flg);
		if (cur_qp && check_dca_detach_enable(cur_qp))
			dca_detach_qp_buf(hr_dev, cur_qp);
		if (ret)
			break;
	}

	if (npolled || ud_len_err_flg)
		update_cq_db(hr_dev, hr_cq);

out:
	spin_unlock_irqrestore(&hr_cq->lock, flags);
    return npolled;
}

int hihtr_v3_req_notify_cq(struct ib_cq *ibcq,
				     enum ib_cq_notify_flags flags)
{
	struct hihtr_dev *hr_dev = to_hr_dev(ibcq->device);
	struct hihtr_cq *hr_cq = to_hr_cq(ibcq);
	struct hihtr_v3_db cq_db = {0};
	u32 notify_flag;

	/*
	 * flags = 0, then notify_flag : next
	 * flags = 1, then notify flag : solocited
	 */
	notify_flag = (flags & IB_CQ_SOLICITED_MASK) == IB_CQ_SOLICITED ?
		      V2_CQ_DB_REQ_NOT : V2_CQ_DB_REQ_NOT_SOL;

	hr_reg_write(&cq_db, DB_Q_NUM, hr_cq->cqn);
	hr_reg_write(&cq_db, DB_CN, HIHTR_V3_DB_CQ_CN);
	// TODO: cos字段默认填0
	hr_reg_write(&cq_db, DB_SRV, HIHTR_V3_DB_CQ_NOTIFY_SRV);
	hr_reg_write(&cq_db, DB_CQ_CI, hr_cq->cons_index);
	hr_reg_write(&cq_db, DB_CQ_NOTIFY, notify_flag);
	hr_reg_write(&cq_db, DB_CQ_CMD_SN, hr_cq->arm_sn);

	// cq的door bell直接敲到reg base即可
	wmb();
	hihtr_write64(hr_dev, (__le32 *)&cq_db, hr_cq->db_reg);

	return 0;
}

int hihtr_v3_modify_cq(struct ib_cq *cq, u16 cq_count, u16 cq_period)
{
	struct hihtr_dev *hr_dev = to_hr_dev(cq->device);
	struct hihtr_v3_cq_context *cq_context;
	struct hihtr_cq *hr_cq = to_hr_cq(cq);
	struct hihtr_v3_cq_context *cqc_mask;
	struct hihtr_cmd_mailbox *mailbox;
	int ret;

	mailbox = hihtr_alloc_cmd_mailbox(hr_dev);
	if (IS_ERR(mailbox)) {
		ret = PTR_ERR(mailbox);
		goto out;
	}

	cq_context = mailbox->buf;
	cqc_mask = (struct hihtr_v3_cq_context *)mailbox->buf + 1;

	memset(cqc_mask, 0xff, sizeof(*cqc_mask));

	hr_reg_write(cq_context, CQC_CQ_MAX_CNT, cq_count);
	hr_reg_clear(cqc_mask, CQC_CQ_MAX_CNT);

	if (HIHTR_GET_PCI_REVISION(hr_dev) == PCI_REVISION_ID_HIP08) {
		if (cq_period * HIHTR_CLOCK_ADJUST > USHRT_MAX) {
			dev_info(hr_dev->dev,
				 "cq_period(%u) reached the upper limit, adjusted to 65.\n",
				 cq_period);
			cq_period = HIHTR_MAX_CQ_PERIOD_HIP08;
		}
		cq_period *= HIHTR_CLOCK_ADJUST;
	}
	hr_reg_write(cq_context, CQC_CQ_PERIOD, cq_period);
	hr_reg_clear(cqc_mask, CQC_CQ_PERIOD);

	ret = hihtr_cmq_mbox(hr_dev, mailbox->dma, 0,
				HIHTR_CMD_MODIFY_CQC, hr_cq->cqn);
	if (ret)
		ibdev_err(&hr_dev->ib_dev,
			  "failed to process cmd when modifying CQ, ret = %d.\n",
			  ret);
    hihtr_free_cmd_mailbox(hr_dev, mailbox);

out:
	if (ret)
		atomic64_inc(&hr_dev->dfx_cnt[HIHTR_DFX_CQ_MODIFY_ERR_CNT]);

	return ret;
}