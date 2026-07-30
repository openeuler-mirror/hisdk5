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
#include "hihtr_hw_v3_cq.h"
#include "hihtr_hw_v3_qp.h"
bool check_dca_attach_enable(struct hihtr_qp *hr_qp)
{
	return hr_qp->en_flags & HIHTR_QP_CAP_DYNAMIC_CTX_ATTACH;
}

int dca_attach_qp_buf(struct hihtr_dev *hr_dev,
			     struct hihtr_qp *hr_qp)
{
	struct hihtr_dca_attach_attr attr = {};
	unsigned long flags_sq, flags_rq;
	u32 idx;

	spin_lock_irqsave(&hr_qp->sq.lock, flags_sq);
	spin_lock_irqsave(&hr_qp->rq.lock, flags_rq);

	if (hr_qp->sq.wqe_cnt > 0) {
		idx = hr_qp->sq.head & (hr_qp->sq.wqe_cnt - 1);
		attr.sq_offset = idx << hr_qp->sq.wqe_shift;
	}

	if (hr_qp->sge.sge_cnt > 0) {
		idx = hr_qp->next_sge & (hr_qp->sge.sge_cnt - 1);
		attr.sge_offset = idx << hr_qp->sge.sge_shift;
	}

	if (hr_qp->rq.wqe_cnt > 0) {
		idx = hr_qp->rq.head & (hr_qp->rq.wqe_cnt - 1);
		attr.rq_offset = idx << hr_qp->rq.wqe_shift;
	}

	spin_unlock_irqrestore(&hr_qp->rq.lock, flags_rq);
	spin_unlock_irqrestore(&hr_qp->sq.lock, flags_sq);

	return hihtr_dca_attach(hr_dev, hr_qp, &attr);
}

/**
 * @brief 根据用户传入的传输路径相关属性获取服务等级（service level）
 *
 * Hi1872芯片约束RoCE使用的服务等级范围为4~7，且随func数量变化。
 * 具体表现为随着func数量增多，可以使用的服务等级数量减小。
 * @param hr_dev 当前RoCE设备func
 * @param ah_attr 地址句柄属性
 * @param priority 服务等级（优先级）
 * @return int 执行结果
 */
int hihtr_hw_v3_get_cos(struct hihtr_dev *hr_dev,
				const struct rdma_ah_attr *ah_attr, u8 *priority)
{
	struct hihtr_v3_priv *priv = hr_dev->priv;
	struct hinic5_dcb_state dcb_state;
	u16 vlan_id;
	int trust_mode, pri, sl_num;
	int ret;

	sl_num = hr_dev->caps.sl_num;

	/* 通过NIC接口获取DCB状态信息 */
	ret = hinic5_get_dcb_state(priv->lld_dev->hwdev, &dcb_state);
	if (ret) {
		ibdev_err(&hr_dev->ib_dev, "failed to get dcb state\n");
		return ret;
	}

	/* 根据不同的映射模式，获取对应的优先级 */
	trust_mode = dcb_state.trust;
	if (trust_mode == HINIC5_TC_MAP_MODE_PCP) {
		ret = rdma_read_gid_l2_fields(ah_attr->grh.sgid_attr, &vlan_id, NULL);
		if (ret) {
			ibdev_err(&hr_dev->ib_dev, "failed to get vlan id\n");
			return ret;
		}

		if (vlan_id > VLAN_N_VID) {
			*priority = (dcb_state.default_cos | HIHTR_COS_MIN) < HIHTR_COS_MIN + sl_num ?
						(dcb_state.default_cos | HIHTR_COS_MIN) : HIHTR_COS_MIN + sl_num - 1;
			return 0;
		}
		pri = ah_attr->sl;
	} else {
		pri = get_dscp_from_tclass(&ah_attr->grh);
	}

	/* 根据优先级获取对应的cos */
	ret = hinic5_get_cos_by_pri(priv->lld_dev->hwdev, pri, priority);
	if (ret) {
		ibdev_err(&hr_dev->ib_dev, "failed to get cos by pri\n");
		return ret;
	}

	/* 确保cos在4 ~ (4 + sl_num)范围内 */
	*priority = (*priority | HIHTR_COS_MIN) < HIHTR_COS_MIN + sl_num ?
				(*priority | HIHTR_COS_MIN) : HIHTR_COS_MIN + sl_num - 1;
	return 0;
}

int hihtr_v3_qp_modify(struct hihtr_dev *hr_dev,
				 struct hihtr_v3_qp_context *context,
				 struct hihtr_v3_qp_context *qpc_mask,
				 struct hihtr_qp *hr_qp)
{
	struct hihtr_cmd_mailbox *mailbox;
	int qpc_size;
	int ret;

	mailbox = hihtr_alloc_cmd_mailbox(hr_dev);
	if (IS_ERR(mailbox))
		return PTR_ERR(mailbox);

	/* The qpc size of HIP08 is only 256B, which is half of HIP09 */
	qpc_size = hr_dev->caps.qpc_sz;
	memcpy(mailbox->buf, context, qpc_size);
	memcpy(mailbox->buf + qpc_size, qpc_mask, qpc_size);

	ret = hihtr_cmq_mbox(hr_dev, mailbox->dma, 0,
				HIHTR_CMD_MODIFY_QPC, hr_qp->qpn);

	hihtr_free_cmd_mailbox(hr_dev, mailbox);

	return ret;
}

static void set_access_flags(struct hihtr_qp *hr_qp,
			     struct hihtr_v3_qp_context *context,
			     struct hihtr_v3_qp_context *qpc_mask,
			     const struct ib_qp_attr *attr, int attr_mask)
{
	u8 dest_rd_atomic;
	u32 access_flags;

	dest_rd_atomic = (attr_mask & IB_QP_MAX_DEST_RD_ATOMIC) ?
			 attr->max_dest_rd_atomic : hr_qp->resp_depth;

	access_flags = (attr_mask & IB_QP_ACCESS_FLAGS) ?
		       attr->qp_access_flags : hr_qp->atomic_rd_en;

	if (!dest_rd_atomic)
		access_flags &= IB_ACCESS_REMOTE_WRITE;

	hr_reg_write_bool(context, QPC_RRE,
			  access_flags & IB_ACCESS_REMOTE_READ);
	hr_reg_clear(qpc_mask, QPC_RRE);

	hr_reg_write_bool(context, QPC_RWE,
			  access_flags & IB_ACCESS_REMOTE_WRITE);
	hr_reg_clear(qpc_mask, QPC_RWE);

	hr_reg_write_bool(context, QPC_ATE,
			  access_flags & IB_ACCESS_REMOTE_ATOMIC);
	hr_reg_clear(qpc_mask, QPC_ATE);
	hr_reg_write_bool(context, QPC_EXT_ATE,
			  access_flags & IB_ACCESS_REMOTE_ATOMIC);
	hr_reg_clear(qpc_mask, QPC_EXT_ATE);
}

static void set_qpc_wqe_cnt(struct hihtr_qp *hr_qp,
			    struct hihtr_v3_qp_context *context,
			    struct hihtr_v3_qp_context *qpc_mask)
{
	hr_reg_write(context, QPC_SGE_SHIFT,
		     to_hr_hem_entries_shift(hr_qp->sge.sge_cnt,
					     hr_qp->sge.sge_shift));

	hr_reg_write(context, QPC_SQ_SHIFT, ilog2(hr_qp->sq.wqe_cnt));

	hr_reg_write(context, QPC_RQ_SHIFT, ilog2(hr_qp->rq.wqe_cnt));
}

static inline int get_cqn(struct ib_cq *ib_cq)
{
	return ib_cq ? to_hr_cq(ib_cq)->cqn : 0;
}

static inline int get_pdn(struct ib_pd *ib_pd)
{
	return ib_pd ? to_hr_pd(ib_pd)->pdn : 0;
}

static void modify_qp_reset_to_init(struct ib_qp *ibqp,
				    const struct ib_qp_attr *attr,
				    int attr_mask,
				    struct hihtr_v3_qp_context *context,
				    struct hihtr_v3_qp_context *qpc_mask)
{
	struct hihtr_dev *hr_dev = to_hr_dev(ibqp->device);
	struct hihtr_qp *hr_qp = to_hr_qp(ibqp);

	hr_reg_write(context, QPC_TST, to_hr_qp_type(ibqp->qp_type));

	hr_reg_write(context, QPC_PD, get_pdn(ibqp->pd));

	hr_reg_write(context, QPC_RQWS, ilog2(hr_qp->rq.max_gs));

	set_qpc_wqe_cnt(hr_qp, context, qpc_mask);

	/* No VLAN need to set 0xFFF */
	hr_reg_write(context, QPC_VLAN_ID, 0xfff);

	if (ibqp->qp_type == IB_QPT_XRC_TGT) {
		context->qkey_xrcd = cpu_to_le32(hr_qp->xrcdn);

		hr_reg_enable(context, QPC_XRC_QP_TYPE);
	}

	if (hr_qp->en_flags & HIHTR_QP_CAP_RQ_RECORD_DB)
		hr_reg_enable(context, QPC_RQ_RECORD_EN);

	if (hr_qp->en_flags & HIHTR_QP_CAP_OWNER_DB)
		hr_reg_enable(context, QPC_OWNER_MODE);

	hr_reg_write(context, QPC_RQ_DB_RECORD_ADDR_L,
		     lower_32_bits(hr_qp->rdb.dma) >> 1);
	hr_reg_write(context, QPC_RQ_DB_RECORD_ADDR_H,
		     upper_32_bits(hr_qp->rdb.dma));

	hr_reg_write(context, QPC_RX_CQN, get_cqn(ibqp->recv_cq));

	if (ibqp->srq) {
		hr_reg_enable(context, QPC_SRQ_EN);
		hr_reg_write(context, QPC_SRQN, to_hr_srq(ibqp->srq)->srqn);
	}

	hr_reg_enable(context, QPC_FRE);

	hr_reg_write(context, QPC_TX_CQN, get_cqn(ibqp->send_cq));

	if (hr_dev->caps.qpc_sz < HIHTR_V3_QPC_SZ)
		return;

	if (hr_dev->caps.flags & HIHTR_CAP_FLAG_STASH)
		hr_reg_enable(&context->ext, QPCEX_STASH);
}

static void modify_qp_init_to_init(struct ib_qp *ibqp,
				   const struct ib_qp_attr *attr, int attr_mask,
				   struct hihtr_v3_qp_context *context,
				   struct hihtr_v3_qp_context *qpc_mask)
{
	hr_reg_write(context, QPC_TST, to_hr_qp_type(ibqp->qp_type));
	hr_reg_clear(qpc_mask, QPC_TST);

	hr_reg_write(context, QPC_PD, get_pdn(ibqp->pd));
	hr_reg_clear(qpc_mask, QPC_PD);

	hr_reg_write(context, QPC_RX_CQN, get_cqn(ibqp->recv_cq));
	hr_reg_clear(qpc_mask, QPC_RX_CQN);

	hr_reg_write(context, QPC_TX_CQN, get_cqn(ibqp->send_cq));
	hr_reg_clear(qpc_mask, QPC_TX_CQN);

	if (ibqp->srq) {
		hr_reg_enable(context, QPC_SRQ_EN);
		hr_reg_clear(qpc_mask, QPC_SRQ_EN);
		hr_reg_write(context, QPC_SRQN, to_hr_srq(ibqp->srq)->srqn);
		hr_reg_clear(qpc_mask, QPC_SRQN);
	}
}

static int config_qp_rq_buf(struct hihtr_dev *hr_dev,
			    struct hihtr_qp *hr_qp,
			    struct hihtr_v3_qp_context *context,
			    struct hihtr_v3_qp_context *qpc_mask)
{
	u64 mtts[MTT_MIN_COUNT] = { 0 };
	u64 wqe_sge_ba;
	int count;

	/* Search qp buf's mtts */
	count = hihtr_mtr_find(hr_dev, &hr_qp->mtr, hr_qp->rq.wqe_offset,
				  mtts, ARRAY_SIZE(mtts), &wqe_sge_ba);
	if (hr_qp->rq.wqe_cnt && count < 1) {
		ibdev_err(&hr_dev->ib_dev,
			  "failed to find RQ WQE, QPN = 0x%lx.\n", hr_qp->qpn);
		return -EINVAL;
	}

	context->wqe_sge_ba = cpu_to_le32(wqe_sge_ba >> WQE_SGE_BA_SHIFT);
	qpc_mask->wqe_sge_ba = 0;

	/*
	 * In v2 engine, software pass context and context mask to hardware
	 * when modifying qp. If software need modify some fields in context,
	 * we should set all bits of the relevant fields in context mask to
	 * 0 at the same time, else set them to 0x1.
	 */
	hr_reg_write(context, QPC_WQE_SGE_BA_H, wqe_sge_ba >> QPC_WQE_SGE_BA_H_SHIFT);
	hr_reg_clear(qpc_mask, QPC_WQE_SGE_BA_H);

	hr_reg_write(context, QPC_SQ_HOP_NUM,
		     to_hr_hem_hopnum(hr_dev->caps.wqe_sq_hop_num,
				      hr_qp->sq.wqe_cnt));
	hr_reg_clear(qpc_mask, QPC_SQ_HOP_NUM);

	hr_reg_write(context, QPC_SGE_HOP_NUM,
		     to_hr_hem_hopnum(hr_dev->caps.wqe_sge_hop_num,
				      hr_qp->sge.sge_cnt));
	hr_reg_clear(qpc_mask, QPC_SGE_HOP_NUM);

	hr_reg_write(context, QPC_RQ_HOP_NUM,
		     to_hr_hem_hopnum(hr_dev->caps.wqe_rq_hop_num,
				      hr_qp->rq.wqe_cnt));

	hr_reg_clear(qpc_mask, QPC_RQ_HOP_NUM);

	hr_reg_write(context, QPC_WQE_SGE_BA_PG_SZ,
		     to_hr_hw_page_shift(hr_qp->mtr.hem_cfg.ba_pg_shift));
	hr_reg_clear(qpc_mask, QPC_WQE_SGE_BA_PG_SZ);

	hr_reg_write(context, QPC_WQE_SGE_BUF_PG_SZ,
		     to_hr_hw_page_shift(hr_qp->mtr.hem_cfg.buf_pg_shift));
	hr_reg_clear(qpc_mask, QPC_WQE_SGE_BUF_PG_SZ);

	context->rq_cur_blk_addr = cpu_to_le32(to_hr_hw_page_addr(mtts[0]));
	qpc_mask->rq_cur_blk_addr = 0;

	hr_reg_write(context, QPC_RQ_CUR_BLK_ADDR_H,
		     upper_32_bits(to_hr_hw_page_addr(mtts[0])));
	hr_reg_clear(qpc_mask, QPC_RQ_CUR_BLK_ADDR_H);

	/* The rq next block address is only valid for HIP08 QPC. */
	if (HIHTR_GET_PCI_REVISION(hr_dev) == PCI_REVISION_ID_HIP08) {
		context->rq_nxt_blk_addr =
				cpu_to_le32(to_hr_hw_page_addr(mtts[1]));
		qpc_mask->rq_nxt_blk_addr = 0;
		hr_reg_write(context, QPC_RQ_NXT_BLK_ADDR_H,
			     upper_32_bits(to_hr_hw_page_addr(mtts[1])));
		hr_reg_clear(qpc_mask, QPC_RQ_NXT_BLK_ADDR_H);
	}

	return 0;
}

static int config_qp_sq_buf(struct hihtr_dev *hr_dev,
			    struct hihtr_qp *hr_qp,
			    struct hihtr_v3_qp_context *context,
			    struct hihtr_v3_qp_context *qpc_mask)
{
	struct ib_device *ibdev = &hr_dev->ib_dev;
	u64 sge_cur_blk = 0;
	u64 sq_cur_blk = 0;
	int count;

	/* search qp buf's mtts */
	count = hihtr_mtr_find(hr_dev, &hr_qp->mtr, hr_qp->sq.wqe_offset,
				  &sq_cur_blk, 1, NULL);
	if (count < 1) {
		ibdev_err(ibdev, "failed to find QP(0x%lx) SQ buf.\n",
			  hr_qp->qpn);
		return -EINVAL;
	}
	if (hr_qp->sge.sge_cnt > 0) {
		count = hihtr_mtr_find(hr_dev, &hr_qp->mtr,
					  hr_qp->sge.wqe_offset, &sge_cur_blk,
					  1, NULL);
		if (count < 1) {
			ibdev_err(ibdev, "failed to find QP(0x%lx) SGE buf.\n",
				  hr_qp->qpn);
			return -EINVAL;
		}
	}

	/*
	 * In v2 engine, software pass context and context mask to hardware
	 * when modifying qp. If software need modify some fields in context,
	 * we should set all bits of the relevant fields in context mask to
	 * 0 at the same time, else set them to 0x1.
	 */
	hr_reg_write(context, QPC_SQ_CUR_BLK_ADDR_L,
		     lower_32_bits(to_hr_hw_page_addr(sq_cur_blk)));
	hr_reg_write(context, QPC_SQ_CUR_BLK_ADDR_H,
		     upper_32_bits(to_hr_hw_page_addr(sq_cur_blk)));
	hr_reg_clear(qpc_mask, QPC_SQ_CUR_BLK_ADDR_L);
	hr_reg_clear(qpc_mask, QPC_SQ_CUR_BLK_ADDR_H);

	hr_reg_write(context, QPC_SQ_CUR_SGE_BLK_ADDR_L,
		     lower_32_bits(to_hr_hw_page_addr(sge_cur_blk)));
	hr_reg_write(context, QPC_SQ_CUR_SGE_BLK_ADDR_H,
		     upper_32_bits(to_hr_hw_page_addr(sge_cur_blk)));
	hr_reg_clear(qpc_mask, QPC_SQ_CUR_SGE_BLK_ADDR_L);
	hr_reg_clear(qpc_mask, QPC_SQ_CUR_SGE_BLK_ADDR_H);

	hr_reg_write(context, QPC_RX_SQ_CUR_BLK_ADDR_L,
		     lower_32_bits(to_hr_hw_page_addr(sq_cur_blk)));
	hr_reg_write(context, QPC_RX_SQ_CUR_BLK_ADDR_H,
		     upper_32_bits(to_hr_hw_page_addr(sq_cur_blk)));
	hr_reg_clear(qpc_mask, QPC_RX_SQ_CUR_BLK_ADDR_L);
	hr_reg_clear(qpc_mask, QPC_RX_SQ_CUR_BLK_ADDR_H);

	return 0;
}

static inline enum ib_mtu get_mtu(struct ib_qp *ibqp,
				  const struct ib_qp_attr *attr)
{
	if (ibqp->qp_type == IB_QPT_GSI || ibqp->qp_type == IB_QPT_UD)
		return IB_MTU_4096;

	return attr->path_mtu;
}

static int modify_qp_init_to_rtr(struct ib_qp *ibqp,
				 const struct ib_qp_attr *attr, int attr_mask,
				 struct hihtr_v3_qp_context *context,
				 struct hihtr_v3_qp_context *qpc_mask,
				 struct ib_udata *udata)
{
	struct hihtr_ucontext *uctx = NULL;
	struct hihtr_dev *hr_dev = to_hr_dev(ibqp->device);
	struct hihtr_qp *hr_qp = to_hr_qp(ibqp);
	struct ib_device *ibdev = &hr_dev->ib_dev;
	dma_addr_t trrl_ba;
	dma_addr_t irrl_ba;
	enum ib_mtu ib_mtu;
	const u8 *smac;
	u8 lp_pktn_ini;
	u64 *mtts;
	u8 *dmac;
	int mtu;
	int port;
	int ret;

#if !defined(UVERBS_ATTR_BUNDLE_HAVE_DRIVER_UDATA)
	struct ib_uobject *uobject = ibqp->uobject;
	if (uobject) {
		struct ib_ucontext *ib_uctx = uobject->context;
		uctx = to_hr_ucontext(ib_uctx);
	}
#elif !defined(UVERBS_IOCTL_HAVE_RDMA_UDATA_TO_DRV_CONTEXT)
	struct uverbs_attr_bundle *uattr = NULL;

	uattr = udata ? container_of(udata, struct uverbs_attr_bundle, driver_udata) : NULL;
	uctx = udata ? to_hr_ucontext(ib_uverbs_get_ucontext(uattr)) : NULL;
#else
	uctx = rdma_udata_to_drv_context(udata, struct hihtr_ucontext, ibucontext);
#endif

	hr_qp->rq.wqe_offset = hr_qp->rq.offset;
	ret = config_qp_rq_buf(hr_dev, hr_qp, context, qpc_mask);
	if (ret) {
		ibdev_err(ibdev, "failed to config rq buf, ret = %d.\n", ret);
		return ret;
	}

	/* Search IRRL's mtts */
	mtts = hihtr_table_find(hr_dev, &hr_dev->qp_table.irrl_table,
				   hr_qp->qpn, &irrl_ba);
	if (!mtts) {
		ibdev_err(ibdev, "failed to find qp irrl_table.\n");
		return -EINVAL;
	}

	/* Search TRRL's mtts */
	mtts = hihtr_table_find(hr_dev, &hr_dev->qp_table.trrl_table,
				   hr_qp->qpn, &trrl_ba);
	if (!mtts) {
		ibdev_err(ibdev, "failed to find qp trrl_table.\n");
		return -EINVAL;
	}

	if (attr_mask & IB_QP_ALT_PATH) {
		ibdev_err(ibdev, "INIT2RTR attr_mask (0x%x) error.\n",
			  attr_mask);
		return -EINVAL;
	}

	hr_reg_write(context, QPC_TRRL_BA_L, trrl_ba >> QPC_TRRL_BA_L_SHIFT);
	hr_reg_clear(qpc_mask, QPC_TRRL_BA_L);
	context->trrl_ba = cpu_to_le32(trrl_ba >> TRRL_BA_SHIFT);
	qpc_mask->trrl_ba = 0;
	hr_reg_write(context, QPC_TRRL_BA_H, trrl_ba >> QPC_TRRL_BA_H_SHIFT);
	hr_reg_clear(qpc_mask, QPC_TRRL_BA_H);

	if (HIHTR_GET_PCI_REVISION(hr_dev) >= PCI_REVISION_ID_HIP09) {
		if (hr_qp->en_flags & HIHTR_QP_CAP_DYNAMIC_CTX_ATTACH) {
			hr_reg_enable(context, QPC_DCA_MODE);
			hr_reg_clear(qpc_mask, QPC_DCA_MODE);
		}
	} else {
		/* reset IRRL_HEAD */
		hr_reg_clear(qpc_mask, QPC_V2_IRRL_HEAD);
	}

	context->irrl_ba = cpu_to_le32(irrl_ba >> IRRL_BA_SHIFT);
	qpc_mask->irrl_ba = 0;
	hr_reg_write(context, QPC_IRRL_BA_H, irrl_ba >> QPC_IRRL_BA_H_SHIFT);
	hr_reg_clear(qpc_mask, QPC_IRRL_BA_H);

	hr_reg_enable(context, QPC_RMT_E2E);
	hr_reg_clear(qpc_mask, QPC_RMT_E2E);

	hr_reg_write(context, QPC_SIG_TYPE, hr_qp->sq_signal_bits);
	hr_reg_clear(qpc_mask, QPC_SIG_TYPE);

	port = (attr_mask & IB_QP_PORT) ? (attr->port_num - 1) : hr_qp->port;

	smac = (const u8 *)hr_dev->dev_addr[port];
	dmac = (u8 *)attr->ah_attr.roce.dmac;
	/* when dmac equals smac or loop_idc is 1, it should loopback */
	if (ether_addr_equal_unaligned(dmac, smac) ||
	    hr_dev->loop_idc == 0x1) {
		hr_reg_write(context, QPC_LBI, hr_dev->loop_idc);
		hr_reg_clear(qpc_mask, QPC_LBI);
	}

	if (attr_mask & IB_QP_DEST_QPN) {
		hr_reg_write(context, QPC_DQPN, attr->dest_qp_num);
		hr_reg_clear(qpc_mask, QPC_DQPN);
	}

	memcpy(&(context->dmac), dmac, sizeof(u32));
	hr_reg_write(context, QPC_DMAC_H, *((u16 *)(&dmac[QPC_DMAC_H_INDEX])));
	qpc_mask->dmac = 0;
	hr_reg_clear(qpc_mask, QPC_DMAC_H);

	ib_mtu = get_mtu(ibqp, attr);
	hr_qp->path_mtu = ib_mtu;

	mtu = ib_mtu_enum_to_int(ib_mtu);
	if (WARN_ON(mtu <= 0)) {
		ibdev_err(ibdev, "mtu value error, mtu = %d.\n", mtu);
		return -EINVAL;
	}

#define MAX_LP_MSG_LEN 4096
	/* MTU * (2 ^ LP_PKTN_INI) shouldn't be bigger than MAX_LP_MSG_LEN */
	lp_pktn_ini = ilog2(max(mtu, MAX_LP_MSG_LEN) / mtu);

	if (attr_mask & IB_QP_PATH_MTU) {
		hr_reg_write(context, QPC_MTU, ib_mtu);
		hr_reg_clear(qpc_mask, QPC_MTU);
	}

	hr_reg_write(context, QPC_LP_PKTN_INI, lp_pktn_ini);
	hr_reg_clear(qpc_mask, QPC_LP_PKTN_INI);

	/* ACK_REQ_FREQ should be larger than or equal to LP_PKTN_INI */
	hr_reg_write(context, QPC_ACK_REQ_FREQ, lp_pktn_ini);
	hr_reg_clear(qpc_mask, QPC_ACK_REQ_FREQ);

	hr_reg_clear(qpc_mask, QPC_RX_REQ_PSN_ERR);
	hr_reg_clear(qpc_mask, QPC_RX_REQ_MSN);
	hr_reg_clear(qpc_mask, QPC_RX_REQ_LAST_OPTYPE);

	context->rq_rnr_timer = 0;
	qpc_mask->rq_rnr_timer = 0;

	hr_reg_clear(qpc_mask, QPC_TRRL_HEAD_MAX);
	hr_reg_clear(qpc_mask, QPC_TRRL_TAIL_MAX);

	/* rocee send 2^lp_sgen_ini segs every time */
	hr_reg_write(context, QPC_LP_SGEN_INI, QPC_LP_SGEN_INI_VALUE);
	hr_reg_clear(qpc_mask, QPC_LP_SGEN_INI);

	if (udata && (ibqp->qp_type == IB_QPT_RC) &&
	    (uctx->config & HIHTR_RQ_INLINE_FLAGS)) {
		hr_reg_write_bool(context, QPC_RQIE,
				  (hr_dev->inline_type == RQ_INLINE));
		hr_reg_clear(qpc_mask, QPC_RQIE);
	}

	if (udata &&
	    (ibqp->qp_type == IB_QPT_RC || ibqp->qp_type == IB_QPT_XRC_TGT) &&
	    (uctx->config & HIHTR_CQE_INLINE_FLAGS)) {
		hr_reg_write_bool(context, QPC_CQEIE,
				  (hr_dev->inline_type == CQE_INLINE));
		hr_reg_clear(qpc_mask, QPC_CQEIE);

		hr_reg_write(context, QPC_CQEIS, 0);
		hr_reg_clear(qpc_mask, QPC_CQEIS);
	}

	return 0;
}

static int modify_qp_rtr_to_rts(struct ib_qp *ibqp,
				const struct ib_qp_attr *attr, int attr_mask,
				struct hihtr_v3_qp_context *context,
				struct hihtr_v3_qp_context *qpc_mask)
{
	struct hihtr_dev *hr_dev = to_hr_dev(ibqp->device);
	struct hihtr_qp *hr_qp = to_hr_qp(ibqp);
	struct ib_device *ibdev = &hr_dev->ib_dev;
	int ret;

	/* Not support alternate path and path migration */
	if (attr_mask & (IB_QP_ALT_PATH | IB_QP_PATH_MIG_STATE)) {
		ibdev_err(ibdev, "RTR2RTS attr_mask (0x%x)error\n", attr_mask);
		return -EINVAL;
	}

	hr_qp->sq.wqe_offset = hr_qp->sq.offset;
	hr_qp->sge.wqe_offset = hr_qp->sge.offset;
	ret = config_qp_sq_buf(hr_dev, hr_qp, context, qpc_mask);
	if (ret) {
		ibdev_err(ibdev, "failed to config sq buf, ret = %d.\n", ret);
		return ret;
	}

	/*
	 * Set some fields in context to zero, Because the default values
	 * of all fields in context are zero, we need not set them to 0 again.
	 * but we should set the relevant fields of context mask to 0.
	 */
	hr_reg_clear(qpc_mask, QPC_IRRL_SGE_IDX);

	hr_reg_clear(qpc_mask, QPC_RX_ACK_MSN);

	hr_reg_clear(qpc_mask, QPC_ACK_LAST_OPTYPE);
	hr_reg_clear(qpc_mask, QPC_IRRL_PSN_VLD);
	hr_reg_clear(qpc_mask, QPC_IRRL_PSN);

	hr_reg_clear(qpc_mask, QPC_IRRL_TAIL_REAL);

	hr_reg_clear(qpc_mask, QPC_RETRY_MSG_MSN);

	hr_reg_clear(qpc_mask, QPC_RNR_RETRY_FLAG);

	hr_reg_clear(qpc_mask, QPC_CHECK_FLG);

	return 0;
}

#if defined(RDMA_HAVE_XARRAY)
STATIC int get_dip_ctx_idx(struct ib_qp *ibqp, const struct ib_qp_attr *attr,
			   u32 *dip_idx)
{
	const struct ib_global_route *grh = rdma_ah_read_grh(&attr->ah_attr);
	struct hihtr_dev *hr_dev = to_hr_dev(ibqp->device);
	struct hihtr_dip *hr_dip;
	struct xarray *dip_xa = &hr_dev->qp_table.dip_table.dip_xa;
	unsigned long flags;
	const struct xa_limit dip_xa_limit = {
		.min = hr_dev->qp_table.dip_table.next_dip_idx,
		.max = hr_dev->caps.num_qps - 1,
	};
	int ret = 0;

	spin_lock_irqsave(&hr_dev->dip_list_lock, flags);

	list_for_each_entry(hr_dip, &hr_dev->dip_list, node) {
		if (!memcmp(grh->dgid.raw, hr_dip->dgid, GID_LEN_V2)) {
			*dip_idx = hr_dip->dip_idx;
			refcount_inc(&hr_dip->refcount);
			goto out;
		}
	}

	/* If no dgid is found, a new dip and a mapping between dgid and
	 * dip_idx will be created.
	 */
	hr_dip = kzalloc(sizeof(*hr_dip), GFP_ATOMIC);
	if (!hr_dip) {
		ret = -ENOMEM;
		goto out;
	}

	memcpy(hr_dip->dgid, grh->dgid.raw, sizeof(grh->dgid.raw));

	ret = xa_alloc(dip_xa, dip_idx, hr_dip, dip_xa_limit, GFP_KERNEL);
	if (ret) {
		ibdev_err(&hr_dev->ib_dev, "fail to alloc dip idx and store dip entry. ret = %d\n", ret);
		kfree(hr_dip);
		goto out;
	}
	hr_dip->dip_idx = *dip_idx;
	refcount_set(&hr_dip->refcount, 1);
	list_add_tail(&hr_dip->node, &hr_dev->dip_list);

	hr_dev->qp_table.dip_table.next_dip_idx = (*dip_idx + 1) > dip_xa_limit.max
		? hr_dev->caps.reserved_qps : (*dip_idx + 1);

out:
	spin_unlock_irqrestore(&hr_dev->dip_list_lock, flags);
	return ret;
}
#else
STATIC int get_dip_ctx_idx(struct ib_qp *ibqp, const struct ib_qp_attr *attr,
			   u32 *dip_idx)
{
	const struct ib_global_route *grh = rdma_ah_read_grh(&attr->ah_attr);
	struct hihtr_dev *hr_dev = to_hr_dev(ibqp->device);
	struct hihtr_dip *hr_dip;
	unsigned long flags;
	const u32 idx_min = hr_dev->qp_table.dip_table.next_dip_idx;
	const u32 idx_max = hr_dev->caps.num_qps - 1;
	int ret = 0;

	spin_lock_irqsave(&hr_dev->dip_list_lock, flags);

	list_for_each_entry(hr_dip, &hr_dev->dip_list, node) {
		if (!memcmp(grh->dgid.raw, hr_dip->dgid, GID_LEN_V2)) {
			*dip_idx = hr_dip->dip_idx;
			refcount_inc(&hr_dip->refcount);
			goto out;
		}
	}

	/* If no dgid is found, a new dip and a mapping between dgid and
	 * dip_idx will be created.
	 */
	hr_dip = kzalloc(sizeof(*hr_dip), GFP_ATOMIC);
	if (!hr_dip) {
		ret = -ENOMEM;
		goto out;
	}

	memcpy(hr_dip->dgid, grh->dgid.raw, sizeof(grh->dgid.raw));

	*dip_idx = idr_alloc(&hr_dev->qp_table.dip_table.dip_idr, hr_dip, idx_min, idx_max, GFP_KERNEL);
	if (*dip_idx < 0) {
		ibdev_err(&hr_dev->ib_dev, "fail to alloc dip idx and store dip entry. ret = %d\n", *dip_idx);
		kfree(hr_dip);
		ret = *dip_idx;
		goto out;
	}
	hr_dip->dip_idx = *dip_idx;
	refcount_set(&hr_dip->refcount, 1);
	list_add_tail(&hr_dip->node, &hr_dev->dip_list);

	hr_dev->qp_table.dip_table.next_dip_idx = (*dip_idx + 1) > idx_max
		? hr_dev->caps.reserved_qps : (*dip_idx + 1);

out:
	spin_unlock_irqrestore(&hr_dev->dip_list_lock, flags);
	return ret;
}
#endif

enum {
	CONG_DCQCN,
	CONG_WINDOW,
};

enum {
	UNSUPPORT_CONG_LEVEL,
	SUPPORT_CONG_LEVEL,
};

enum {
	CONG_LDCP,
	CONG_HC3,
};

enum {
	DIP_INVALID,
	DIP_VALID,
};

enum {
	WND_LIMIT,
	WND_UNLIMIT,
};

static int check_congest_type(struct ib_qp *ibqp,
			   struct hihtr_congestion_algorithm *cong_alg,
			   u32 *scc_algo_sel)
{
	struct ib_device *ibdev = ibqp->device;

	if (ibqp->qp_type == IB_QPT_UD) {
		ibdev_warn(ibdev,
			"service type UD can only use DCQCN, current cc_algo: %d\n",
			*scc_algo_sel);
		*scc_algo_sel = HIHTR_SCC_ALGO_DCQCN;
	}

	switch (*scc_algo_sel) {
	case HIHTR_SCC_ALGO_DCQCN:
		cong_alg->alg_sel = CONG_DCQCN;
		cong_alg->alg_sub_sel = UNSUPPORT_CONG_LEVEL;
		cong_alg->dip_vld = DIP_INVALID;
		cong_alg->wnd_mode_sel = WND_LIMIT;
		break;
	case HIHTR_SCC_ALGO_LDCP:
		cong_alg->alg_sel = CONG_WINDOW;
		cong_alg->alg_sub_sel = CONG_LDCP;
		cong_alg->dip_vld = DIP_INVALID;
		cong_alg->wnd_mode_sel = WND_UNLIMIT;
		break;
	case HIHTR_SCC_ALGO_DIP:
		cong_alg->alg_sel = CONG_DCQCN;
		cong_alg->alg_sub_sel = UNSUPPORT_CONG_LEVEL;
		cong_alg->dip_vld = DIP_VALID;
		cong_alg->wnd_mode_sel = WND_LIMIT;
		break;
	default:
		cong_alg->alg_sel = CONG_DCQCN;
		cong_alg->alg_sub_sel = UNSUPPORT_CONG_LEVEL;
		cong_alg->dip_vld = DIP_INVALID;
		cong_alg->wnd_mode_sel = WND_LIMIT;
		break;
	}

	return 0;
}

static int fill_congest_field(struct ib_qp *ibqp,
			   const struct ib_qp_attr *attr,
			   struct hihtr_v3_qp_context *context,
			   struct hihtr_v3_qp_context *qpc_mask,
			   struct scc_info *card_scc_info)
{
	const struct ib_global_route *grh = rdma_ah_read_grh(&attr->ah_attr);
	struct hihtr_congestion_algorithm cong_field;
	struct ib_device *ibdev = ibqp->device;
	struct hihtr_qp *hr_qp = to_hr_qp(ibqp);
	u32 scc_algo_sel;
	u32 dip_idx = 0;
	int ret;

	if (HIHTR_GET_PCI_REVISION(to_hr_dev(ibdev)) == PCI_REVISION_ID_HIP08 ||
	    grh->sgid_attr->gid_type == IB_GID_TYPE_ROCE)
		return 0;

	scc_algo_sel = card_scc_info->scc_algo_id;
	ret = check_congest_type(ibqp, &cong_field, &scc_algo_sel);
	if (ret)
		return ret;
	// 为了确保UD正常，需保留默认DCQCN模板0
	hr_reg_write(context, QPC_CONG_ALGO_TMPL_ID, ibqp->qp_type == IB_QPT_UD ?
		0 : card_scc_info->scc_template_id);
	hr_reg_clear(qpc_mask, QPC_CONG_ALGO_TMPL_ID);
	hr_reg_write(&context->ext, QPCEX_CONG_ALG_SEL, cong_field.alg_sel);
	hr_reg_clear(&qpc_mask->ext, QPCEX_CONG_ALG_SEL);
	hr_reg_write(&context->ext, QPCEX_CONG_ALG_SUB_SEL,
		     cong_field.alg_sub_sel);
	hr_reg_clear(&qpc_mask->ext, QPCEX_CONG_ALG_SUB_SEL);
	hr_reg_write(&context->ext, QPCEX_DIP_CTX_IDX_VLD, cong_field.dip_vld);
	hr_reg_clear(&qpc_mask->ext, QPCEX_DIP_CTX_IDX_VLD);
	hr_reg_write(&context->ext, QPCEX_SQ_RQ_NOT_FORBID_EN,
		     cong_field.wnd_mode_sel);
	hr_reg_clear(&qpc_mask->ext, QPCEX_SQ_RQ_NOT_FORBID_EN);

	/* if dip is disabled, there is no need to set dip idx */
	if (cong_field.dip_vld == DIP_INVALID)
		return 0;

	ret = get_dip_ctx_idx(ibqp, attr, &dip_idx);
	if (ret) {
		ibdev_err(ibdev, "failed to fill congest field, ret = %d.\n", ret);
		return ret;
	}

	hr_qp->dip_vld = true;
	hr_qp->dip_idx = dip_idx;
	hr_reg_write(&context->ext, QPCEX_DIP_CTX_IDX, dip_idx);
	hr_reg_write(&qpc_mask->ext, QPCEX_DIP_CTX_IDX, 0);
	return 0;
}

static int hihtr_set_sl(struct ib_qp *ibqp,
			   const struct ib_qp_attr *attr,
			   struct hihtr_v3_qp_context *context,
			   struct hihtr_v3_qp_context *qpc_mask)
{
	const struct rdma_ah_attr *ah_attr = &attr->ah_attr;
	const struct ib_global_route *grh = rdma_ah_read_grh(ah_attr);
	struct hihtr_dev *hr_dev = to_hr_dev(ibqp->device);
	struct hihtr_qp *hr_qp = to_hr_qp(ibqp);
	struct ib_device *ibdev = &hr_dev->ib_dev;
	int ret;

	/* hr_qp.sl字段用于填充Doorbell中的CoS字段 */
	if (grh->sgid_attr->gid_type == IB_GID_TYPE_ROCE_UDP_ENCAP) {
		ret = hihtr_hw_v3_get_cos(hr_dev, ah_attr, &hr_qp->priority);
		if (ret) {
			ibdev_err(ibdev, "failed to get cos, ret = %d.\n", ret);
			return ret;
		}
		hr_qp->sl = hr_qp->priority;
	} else {
		hr_qp->sl = rdma_ah_get_sl(ah_attr);
	}

	if (unlikely(hr_qp->sl > MAX_SERVICE_LEVEL)) {
		ibdev_err(ibdev,
			  "failed to fill QPC, sl (%u) shouldn't be larger than %d.\n",
			  hr_qp->sl, MAX_SERVICE_LEVEL);
		return -EINVAL;
	}

	/* QPC sl字段用于填充VLAN报文头中的pri域段 */
	hr_reg_write(context, QPC_SL, hr_qp->sl);
	hr_reg_clear(qpc_mask, QPC_SL);

	return 0;
}

static int hihtr_v3_set_path(struct ib_qp *ibqp,
				const struct ib_qp_attr *attr,
				int attr_mask,
				struct hihtr_v3_qp_context *context,
				struct hihtr_v3_qp_context *qpc_mask)
{
	const struct ib_global_route *grh = rdma_ah_read_grh(&attr->ah_attr);
	struct hihtr_dev *hr_dev = to_hr_dev(ibqp->device);
	struct hihtr_qp *hr_qp = to_hr_qp(ibqp);
	struct ib_device *ibdev = &hr_dev->ib_dev;
	const struct ib_gid_attr *gid_attr = NULL;
	int is_roce_protocol;
	u16 vlan_id = 0xffff;
	bool is_udp = false;
	u8 ib_port;
	u8 hr_port;
	struct scc_info card_scc_info;
	int ret;

	/*
	 * If free_mr_en of qp is set, it means that this qp comes from
	 * free mr. This qp will perform the loopback operation.
	 * In the loopback scenario, only sl needs to be set.
	 */
	if (hr_qp->free_mr_en) {
		hr_reg_write(context, QPC_SL, rdma_ah_get_sl(&attr->ah_attr));
		hr_reg_clear(qpc_mask, QPC_SL);
		hr_qp->sl = rdma_ah_get_sl(&attr->ah_attr);
		return 0;
	}

	ib_port = (attr_mask & IB_QP_PORT) ? attr->port_num : hr_qp->port + 1;
	hr_port = ib_port - 1;
	is_roce_protocol = (rdma_cap_eth_ah(&hr_dev->ib_dev, ib_port)) &&
                   ((rdma_ah_get_ah_flags(&attr->ah_attr)) & (IB_AH_GRH));
	if (is_roce_protocol) {
		gid_attr = attr->ah_attr.grh.sgid_attr;
		ret = rdma_read_gid_l2_fields(gid_attr, &vlan_id, NULL);
		if (ret)
			return ret;

		is_udp = (gid_attr->gid_type == IB_GID_TYPE_ROCE_UDP_ENCAP);
	}

	/* Only HIP08 needs to set the vlan_en bits in QPC */
	if (vlan_id < VLAN_N_VID &&
	    HIHTR_GET_PCI_REVISION(hr_dev) == PCI_REVISION_ID_HIP08) {
		hr_reg_enable(context, QPC_RQ_VLAN_EN);
		hr_reg_clear(qpc_mask, QPC_RQ_VLAN_EN);
		hr_reg_enable(context, QPC_SQ_VLAN_EN);
		hr_reg_clear(qpc_mask, QPC_SQ_VLAN_EN);
	}

	hr_reg_write(context, QPC_VLAN_ID, vlan_id);
	hr_reg_clear(qpc_mask, QPC_VLAN_ID);

	if (grh->sgid_index >= hr_dev->caps.gid_table_len[hr_port]) {
		ibdev_err(ibdev, "sgid_index(%u) too large. max is %d\n",
			  grh->sgid_index, hr_dev->caps.gid_table_len[hr_port]);
		return -EINVAL;
	}

	if (attr->ah_attr.type != RDMA_AH_ATTR_TYPE_ROCE) {
		ibdev_err(ibdev, "ah attr is not RDMA roce type\n");
		return -EINVAL;
	}

	hr_reg_write(context, QPC_UDPSPN,
		     is_udp ? rdma_get_udp_sport(grh->flow_label, ibqp->qp_num,
						 attr->dest_qp_num) :
				    0);

	hr_reg_clear(qpc_mask, QPC_UDPSPN);

	hr_reg_write(context, QPC_GMV_IDX, grh->sgid_index);

	hr_reg_clear(qpc_mask, QPC_GMV_IDX);

	hr_reg_write(context, QPC_HOPLIMIT, grh->hop_limit);
	hr_reg_clear(qpc_mask, QPC_HOPLIMIT);

	if (hr_dev->hw->query_scc) {
		ret = hr_dev->hw->query_scc(hr_dev, (void*)(&card_scc_info));
		if (ret) {
			ibdev_err(ibdev, "get card scc status failed, ret : %d\n", ret);
			return -EINVAL;
		}
		ret = fill_congest_field(ibqp, attr, context, qpc_mask, &card_scc_info);
		if (ret)
			return ret;
	}

	/* QPC tc字段用于填充IP报文头中DSCP域段 */
	hr_reg_write(context, QPC_TC, get_dscp_from_tclass(&attr->ah_attr.grh));
	hr_reg_clear(qpc_mask, QPC_TC);

	hr_reg_write(context, QPC_FL, grh->flow_label);
	hr_reg_clear(qpc_mask, QPC_FL);
	memcpy(context->dgid, grh->dgid.raw, sizeof(grh->dgid.raw));
	memset(qpc_mask->dgid, 0, sizeof(grh->dgid.raw));

	ret = hihtr_set_sl(ibqp, attr, context, qpc_mask);
	if (ret)
		return ret;

	return 0;
}

static bool check_qp_state(enum ib_qp_state cur_state,
			   enum ib_qp_state new_state)
{
	static const bool sm[][IB_QPS_ERR + 1] = {
		[IB_QPS_RESET] = { [IB_QPS_RESET] = true,
				   [IB_QPS_INIT] = true },
		[IB_QPS_INIT] = { [IB_QPS_RESET] = true,
				  [IB_QPS_INIT] = true,
				  [IB_QPS_RTR] = true,
				  [IB_QPS_ERR] = true },
		[IB_QPS_RTR] = { [IB_QPS_RESET] = true,
				 [IB_QPS_RTS] = true,
				 [IB_QPS_ERR] = true },
		[IB_QPS_RTS] = { [IB_QPS_RESET] = true,
				 [IB_QPS_RTS] = true,
				 [IB_QPS_ERR] = true },
		[IB_QPS_SQD] = {},
		[IB_QPS_SQE] = {},
		[IB_QPS_ERR] = { [IB_QPS_RESET] = true,
				 [IB_QPS_ERR] = true }
	};

	return sm[cur_state][new_state];
}

int hihtr_v3_set_abs_fields(struct ib_qp *ibqp,
				      const struct ib_qp_attr *attr,
				      int attr_mask,
				      enum ib_qp_state cur_state,
				      enum ib_qp_state new_state,
				      struct hihtr_v3_qp_context *context,
				      struct hihtr_v3_qp_context *qpc_mask,
				      struct ib_udata *udata)
{
	struct hihtr_dev *hr_dev = to_hr_dev(ibqp->device);
	int ret = 0;

	if (!check_qp_state(cur_state, new_state)) {
		ibdev_err(&hr_dev->ib_dev, "Illegal state for QP!\n");
		return -EINVAL;
	}

	if (cur_state == IB_QPS_RESET && new_state == IB_QPS_INIT) {
		memset(qpc_mask, 0, hr_dev->caps.qpc_sz);
		modify_qp_reset_to_init(ibqp, attr, attr_mask, context,
					qpc_mask);
	} else if (cur_state == IB_QPS_INIT && new_state == IB_QPS_INIT) {
		modify_qp_init_to_init(ibqp, attr, attr_mask, context,
				       qpc_mask);
	} else if (cur_state == IB_QPS_INIT && new_state == IB_QPS_RTR) {
		ret = modify_qp_init_to_rtr(ibqp, attr, attr_mask, context,
					    qpc_mask, udata);
	} else if (cur_state == IB_QPS_RTR && new_state == IB_QPS_RTS) {
		ret = modify_qp_rtr_to_rts(ibqp, attr, attr_mask, context,
					   qpc_mask);
	}

	return ret;
}

static bool check_qp_timeout_cfg_range(struct hihtr_dev *hr_dev, u8 *timeout)
{
#define QP_ACK_TIMEOUT_MAX_HIP08 20
#define QP_ACK_TIMEOUT_OFFSET 10
#define QP_ACK_TIMEOUT_MAX 31

	if (HIHTR_GET_PCI_REVISION(hr_dev) == PCI_REVISION_ID_HIP08) {
		if (*timeout > QP_ACK_TIMEOUT_MAX_HIP08) {
			ibdev_warn(&hr_dev->ib_dev,
				   "Local ACK timeout shall be 0 to 20.\n");
			return false;
		}
		*timeout += QP_ACK_TIMEOUT_OFFSET;
	} else if (HIHTR_GET_PCI_REVISION(hr_dev) > PCI_REVISION_ID_HIP08) {
		if (*timeout > QP_ACK_TIMEOUT_MAX) {
			ibdev_warn(&hr_dev->ib_dev,
				   "Local ACK timeout shall be 0 to 31.\n");
			return false;
		}
	}

	return true;
}

int hihtr_v3_set_opt_fields(struct ib_qp *ibqp,
				      const struct ib_qp_attr *attr,
				      int attr_mask,
				      struct hihtr_v3_qp_context *context,
				      struct hihtr_v3_qp_context *qpc_mask)
{
	struct hihtr_dev *hr_dev = to_hr_dev(ibqp->device);
	struct hihtr_qp *hr_qp = to_hr_qp(ibqp);
	int ret = 0;
	u8 timeout;

	if (attr_mask & IB_QP_AV) {
		ret = hihtr_v3_set_path(ibqp, attr, attr_mask, context,
					   qpc_mask);
		if (ret)
			return ret;
	}

	if (attr_mask & IB_QP_TIMEOUT) {
		timeout = attr->timeout;
		if (check_qp_timeout_cfg_range(hr_dev, &timeout)) {
			hr_reg_write(context, QPC_AT, timeout);
			hr_reg_clear(qpc_mask, QPC_AT);
		}
	}

	if (attr_mask & IB_QP_RETRY_CNT) {
		hr_reg_write(context, QPC_RETRY_NUM_INIT, attr->retry_cnt);
		hr_reg_clear(qpc_mask, QPC_RETRY_NUM_INIT);

		hr_reg_write(context, QPC_RETRY_CNT, attr->retry_cnt);
		hr_reg_clear(qpc_mask, QPC_RETRY_CNT);
	}

	if (attr_mask & IB_QP_RNR_RETRY) {
		hr_reg_write(context, QPC_RNR_NUM_INIT, attr->rnr_retry);
		hr_reg_clear(qpc_mask, QPC_RNR_NUM_INIT);

		hr_reg_write(context, QPC_RNR_CNT, attr->rnr_retry);
		hr_reg_clear(qpc_mask, QPC_RNR_CNT);
	}

	if (attr_mask & IB_QP_SQ_PSN) {
		hr_reg_write(context, QPC_SQ_CUR_PSN, attr->sq_psn);
		hr_reg_clear(qpc_mask, QPC_SQ_CUR_PSN);

		hr_reg_write(context, QPC_SQ_MAX_PSN, attr->sq_psn);
		hr_reg_clear(qpc_mask, QPC_SQ_MAX_PSN);

		hr_reg_write(context, QPC_RETRY_MSG_PSN_L, attr->sq_psn);
		hr_reg_clear(qpc_mask, QPC_RETRY_MSG_PSN_L);

		hr_reg_write(context, QPC_RETRY_MSG_PSN_H,
			     attr->sq_psn >> RETRY_MSG_PSN_SHIFT);
		hr_reg_clear(qpc_mask, QPC_RETRY_MSG_PSN_H);

		hr_reg_write(context, QPC_RETRY_MSG_FPKT_PSN, attr->sq_psn);
		hr_reg_clear(qpc_mask, QPC_RETRY_MSG_FPKT_PSN);

		hr_reg_write(context, QPC_RX_ACK_EPSN, attr->sq_psn);
		hr_reg_clear(qpc_mask, QPC_RX_ACK_EPSN);
	}

	if ((attr_mask & IB_QP_MAX_DEST_RD_ATOMIC) &&
	     attr->max_dest_rd_atomic) {
		// 规避read trrl表更新问题，默认配置TRRL为最大
		hr_reg_write(context, QPC_RR_MAX,
			     fls(DEFAULT_QP_TRRL_DEPTH - 1));
		hr_reg_clear(qpc_mask, QPC_RR_MAX);
	}

	if ((attr_mask & IB_QP_MAX_QP_RD_ATOMIC) && attr->max_rd_atomic) {
		hr_reg_write(context, QPC_SR_MAX, fls(attr->max_rd_atomic - 1));
		hr_reg_clear(qpc_mask, QPC_SR_MAX);
	}

	if (attr_mask & (IB_QP_ACCESS_FLAGS | IB_QP_MAX_DEST_RD_ATOMIC))
		set_access_flags(hr_qp, context, qpc_mask, attr, attr_mask);

	if (attr_mask & IB_QP_MIN_RNR_TIMER) {
		hr_reg_write(context, QPC_MIN_RNR_TIME,
			    HIHTR_GET_PCI_REVISION(hr_dev) == PCI_REVISION_ID_HIP08 ?
			    HIHTR_RNR_TIMER_10NS : attr->min_rnr_timer);
		hr_reg_clear(qpc_mask, QPC_MIN_RNR_TIME);
	}

	if (attr_mask & IB_QP_RQ_PSN) {
		hr_reg_write(context, QPC_RX_REQ_EPSN, attr->rq_psn);
		hr_reg_clear(qpc_mask, QPC_RX_REQ_EPSN);

		hr_reg_write(context, QPC_RAQ_PSN, attr->rq_psn - 1);
		hr_reg_clear(qpc_mask, QPC_RAQ_PSN);
	}

	if (attr_mask & IB_QP_QKEY) {
		context->qkey_xrcd = cpu_to_le32(attr->qkey);
		qpc_mask->qkey_xrcd = 0;
		hr_qp->qkey = attr->qkey;
	}

	return ret;
}

void hihtr_v3_record_opt_fields(struct ib_qp *ibqp,
					  const struct ib_qp_attr *attr,
					  int attr_mask)
{
	struct hihtr_dev *hr_dev = to_hr_dev(ibqp->device);
	struct hihtr_qp *hr_qp = to_hr_qp(ibqp);

	if (attr_mask & IB_QP_ACCESS_FLAGS)
		hr_qp->atomic_rd_en = attr->qp_access_flags;

	if (attr_mask & IB_QP_MAX_DEST_RD_ATOMIC) {
		hr_qp->resp_depth = attr->max_dest_rd_atomic;
	}

	if (attr_mask & IB_QP_PORT) {
		hr_qp->port = attr->port_num - 1;
		hr_qp->phy_port = hr_dev->iboe.phy_port[hr_qp->port];
	}
}

void v3_set_flushed_fields(struct ib_qp *ibqp,
				  struct hihtr_v3_qp_context *context,
				  struct hihtr_v3_qp_context *qpc_mask)
{
	struct hihtr_qp *hr_qp = to_hr_qp(ibqp);
	unsigned long sq_flag = 0;
	unsigned long rq_flag = 0;

	if (ibqp->qp_type == IB_QPT_XRC_TGT)
		return;

	spin_lock_irqsave(&hr_qp->sq.lock, sq_flag);
	hr_reg_write(context, QPC_SQ_PRODUCER_IDX, hr_qp->sq.head);
	hr_reg_clear(qpc_mask, QPC_SQ_PRODUCER_IDX);
	hr_qp->state = IB_QPS_ERR;
	spin_unlock_irqrestore(&hr_qp->sq.lock, sq_flag);

	if (ibqp->srq || ibqp->qp_type == IB_QPT_XRC_INI) /* no RQ */
		return;

	spin_lock_irqsave(&hr_qp->rq.lock, rq_flag);
	hr_reg_write(context, QPC_RQ_PRODUCER_IDX, hr_qp->rq.head);
	hr_reg_clear(qpc_mask, QPC_RQ_PRODUCER_IDX);
	spin_unlock_irqrestore(&hr_qp->rq.lock, rq_flag);
}

void clear_qp(struct hihtr_qp *hr_qp)
{
	struct ib_qp *ibqp = &hr_qp->ibqp;

	if (ibqp->send_cq)
		hihtr_v3_cq_clean(to_hr_cq(ibqp->send_cq),
				     hr_qp->qpn, NULL);

	if (ibqp->recv_cq  && ibqp->recv_cq != ibqp->send_cq)
		hihtr_v3_cq_clean(to_hr_cq(ibqp->recv_cq),
				     hr_qp->qpn, ibqp->srq ?
				     to_hr_srq(ibqp->srq) : NULL);

	if (hr_qp->en_flags & HIHTR_QP_CAP_RQ_RECORD_DB)
		*hr_qp->rdb.db_record = 0;

	hr_qp->rq.head = 0;
	hr_qp->rq.tail = 0;
	hr_qp->sq.head = 0;
	hr_qp->sq.tail = 0;
	hr_qp->next_sge = 0;
}

int hihtr_v3_modify_qp(struct ib_qp *ibqp,
				 const struct ib_qp_attr *attr,
				 int attr_mask, enum ib_qp_state cur_state,
				 enum ib_qp_state new_state, struct ib_udata *udata)
{
	struct hihtr_dev *hr_dev = to_hr_dev(ibqp->device);
	struct hihtr_qp *hr_qp = to_hr_qp(ibqp);
	struct hihtr_v3_qp_context ctx[2];
	struct hihtr_v3_qp_context *context = ctx;
	struct hihtr_v3_qp_context *qpc_mask = ctx + 1;
	struct ib_device *ibdev = &hr_dev->ib_dev;
	int ret;

	memset(context, 0, hr_dev->caps.qpc_sz);
	memset(qpc_mask, 0xff, hr_dev->caps.qpc_sz);

	ret = hihtr_v3_set_abs_fields(ibqp, attr, attr_mask, cur_state,
					 new_state, context, qpc_mask, udata);
	if (ret) {
		ibdev_err(ibdev, "failed to set abs fields.ret = %d.\n", ret);
		goto out;
	}

	/* When QP state is err, SQ and RQ WQE should be flushed */
	if (new_state == IB_QPS_ERR)
		v3_set_flushed_fields(ibqp, context, qpc_mask);

	/* Configure the optional fields */
	ret = hihtr_v3_set_opt_fields(ibqp, attr, attr_mask, context,
					 qpc_mask);
	if (ret) {
		ibdev_err(ibdev, "failed to set opt fields.ret = %d.\n", ret);
		goto out;
	}

	hr_reg_write_bool(context, QPC_INV_CREDIT,
			  to_hr_qp_type(hr_qp->ibqp.qp_type) == SERV_TYPE_XRC ||
			  ibqp->srq);
	hr_reg_clear(qpc_mask, QPC_INV_CREDIT);

	/* Every status migrate must change state */
	hr_reg_write(context, QPC_QP_ST, new_state);
	hr_reg_clear(qpc_mask, QPC_QP_ST);

	/* SW pass context to HW */
	ret = hihtr_v3_qp_modify(hr_dev, context, qpc_mask, hr_qp);
	if (ret) {
		ibdev_err(ibdev, "failed to modify QP, ret = %d. new state 0x%x\n", ret, new_state);
		goto out;
	}

	hr_qp->state = new_state;

	hihtr_v3_record_opt_fields(ibqp, attr, attr_mask);

	if (new_state == IB_QPS_RESET && !ibqp->uobject)
		clear_qp(hr_qp);

	if (check_dca_attach_enable(hr_qp))
		hihtr_modify_dca(hr_dev, hr_qp, udata);
out:
	return ret;
}

static inline int modify_qp_is_ok(struct hihtr_qp *hr_qp)
{
	return ((hr_qp->ibqp.qp_type == IB_QPT_RC ||
		 hr_qp->ibqp.qp_type == IB_QPT_UD ||
		 hr_qp->ibqp.qp_type == IB_QPT_XRC_INI ||
		 hr_qp->ibqp.qp_type == IB_QPT_XRC_TGT) &&
		hr_qp->state != IB_QPS_RESET);
}

struct roce_fake_db_info {
    u32 fake_rst_cos;
    u32 fake_rst_qpn;
	u32 real_rst_qpn;
};

int hihtr_v3_query_qpc(struct hihtr_dev *hr_dev, u32 qpn, void *buffer);

static int hihtr_v3_send_fake_db(struct hihtr_dev *hr_dev, struct hihtr_qp *hr_qp)
{
	struct hihtr_cmq_desc desc;
	struct hihtr_v3_qp_context context = {};
	struct roce_fake_db_info *db_info = (struct roce_fake_db_info *)desc.data;
	int ret;

	hihtr_cmq_setup_basic_desc(&desc, HIHTR_OPC_SEND_FAKE_DB, false);

	/* 对于UD服务类型，QPC的SL字段不会被驱动修改，而是被硬件刷新 */
	if (hr_qp->ibqp.qp_type == IB_QPT_UD) {
		ret = hihtr_v3_query_qpc(hr_dev, hr_qp->qpn, &context);
		if (ret) {
			ibdev_err(&hr_dev->ib_dev, "failed to query QPC, ret = %d.\n", ret);
			return ret;
		}
		db_info->fake_rst_cos = hr_reg_read(&context, QPC_SL) | HIHTR_COS_MIN;
	} else {
		db_info->fake_rst_cos = hr_qp->sl | HIHTR_COS_MIN;
	}

	db_info->fake_rst_qpn = hr_dev->caps.num_qps - 1;
	db_info->real_rst_qpn = hr_qp->qpn;
	ret = hihtr5_cmq_send(hr_dev, &desc, 1);
	if (ret) {
		dev_err(hr_dev->dev, "failed to send fake db, ret = %d.\n", ret);
	}
	return ret;
}

int hihtr_v3_destroy_qp_common(struct hihtr_dev *hr_dev,
				  struct hihtr_qp *hr_qp,
				  struct ib_udata *udata)
{
	struct ib_device *ibdev = &hr_dev->ib_dev;
	struct hihtr_cq *send_cq, *recv_cq;
	unsigned long flags;
	int ret = 0;

	if (modify_qp_is_ok(hr_qp)) {
		ret = hihtr_v3_send_fake_db(hr_dev, hr_qp);
		if (ret)
			ibdev_err(ibdev, "failed to send fake DB, ret = %d.\n", ret);

		/* Modify qp to reset before destroying qp */
		ret = hihtr_v3_modify_qp(&hr_qp->ibqp, NULL, 0,
					    hr_qp->state, IB_QPS_RESET, udata);
		if (ret)
			ibdev_err(ibdev,
				  "failed to modify QP to RST, ret = %d.\n",
				  ret);
	}

	send_cq = hr_qp->ibqp.send_cq ? to_hr_cq(hr_qp->ibqp.send_cq) : NULL;
	recv_cq = hr_qp->ibqp.recv_cq ? to_hr_cq(hr_qp->ibqp.recv_cq) : NULL;

	spin_lock_irqsave(&hr_dev->qp_list_lock, flags);
	hihtr_lock_cqs(send_cq, recv_cq);

#if defined(ROCE_DESTROY_QP_INT_HAVE_UDATA)
	if (!udata) {
#else
	if (!udata && !hr_qp->ibqp.uobject) {
#endif
		if (recv_cq)
			__hihtr_v3_cq_clean(recv_cq, hr_qp->qpn,
					       (hr_qp->ibqp.srq ?
						to_hr_srq(hr_qp->ibqp.srq) :
						NULL));

		if (send_cq && send_cq != recv_cq)
			__hihtr_v3_cq_clean(send_cq, hr_qp->qpn, NULL);
	}

	hihtr_qp_remove(hr_dev, hr_qp);

	hihtr_unlock_cqs(send_cq, recv_cq);
	spin_unlock_irqrestore(&hr_dev->qp_list_lock, flags);

	return ret;
}

#if defined(ROCE_DESTROY_QP_INT_HAVE_UDATA)
int hihtr_v3_destroy_qp(struct ib_qp *ibqp, struct ib_udata *udata)
{
	struct hihtr_dev *hr_dev = to_hr_dev(ibqp->device);
	struct hihtr_qp *hr_qp = to_hr_qp(ibqp);
	int ret;

	ret = hihtr_v3_destroy_qp_common(hr_dev, hr_qp, udata);
	if (ret)
		ibdev_err(&hr_dev->ib_dev,
			  "failed to destroy QP, QPN = 0x%06lx, ret = %d.\n",
			  hr_qp->qpn, ret);

	hihtr_qp_destroy(hr_dev, hr_qp, udata);
    return 0;
}
#else
int hihtr_v3_destroy_qp(struct ib_qp *ibqp)
{
	struct hihtr_dev *hr_dev = to_hr_dev(ibqp->device);
	struct hihtr_qp *hr_qp = to_hr_qp(ibqp);
	int ret;

	ret = hihtr_v3_destroy_qp_common(hr_dev, hr_qp, NULL);
	if (ret)
		dev_err(hr_dev->dev,
			  "failed to destroy QP, QPN = 0x%06lx, ret = %d.\n",
			  hr_qp->qpn, ret);

	hihtr_qp_destroy(hr_dev, hr_qp, NULL);
    return 0;
}
#endif

int to_ib_qp_st(enum hihtr_v3_qp_state state)
{
	static const enum ib_qp_state map[] = {
		[HIHTR_QP_ST_RST] = IB_QPS_RESET,
		[HIHTR_QP_ST_INIT] = IB_QPS_INIT,
		[HIHTR_QP_ST_RTR] = IB_QPS_RTR,
		[HIHTR_QP_ST_RTS] = IB_QPS_RTS,
		[HIHTR_QP_ST_SQD] = IB_QPS_SQD,
		[HIHTR_QP_ST_SQER] = IB_QPS_SQE,
		[HIHTR_QP_ST_ERR] = IB_QPS_ERR,
		[HIHTR_QP_ST_SQ_DRAINING] = IB_QPS_SQD
	};

	return (state < ARRAY_SIZE(map)) ? map[state] : -1;
}

int hihtr_v3_query_qpc(struct hihtr_dev *hr_dev, u32 qpn,
				 void *buffer)
{
	struct hihtr_cmd_mailbox *mailbox;
	int ret;
	int count = hr_dev->qp_table.qp_table.obj_count;

	if (qpn >= count) {
		return -EINVAL;
	}
	mailbox = hihtr_alloc_cmd_mailbox(hr_dev);
	if (IS_ERR(mailbox))
		return PTR_ERR(mailbox);
	ret = hihtr_cmq_mbox(hr_dev, 0, mailbox->dma, HIHTR_CMD_QUERY_QPC,
				qpn);
	if (ret)
		goto out;

	memcpy(buffer, mailbox->buf, hr_dev->caps.qpc_sz);

out:
	hihtr_free_cmd_mailbox(hr_dev, mailbox);
	return ret;
}

int hihtr_v3_query_qp(struct ib_qp *ibqp, struct ib_qp_attr *qp_attr,
				int qp_attr_mask,
				struct ib_qp_init_attr *qp_init_attr)
{
	struct hihtr_dev *hr_dev = to_hr_dev(ibqp->device);
	struct hihtr_qp *hr_qp = to_hr_qp(ibqp);
	struct hihtr_v3_qp_context context = {};
	struct ib_device *ibdev = &hr_dev->ib_dev;
	int tmp_qp_state;
	int state;
	int ret;

	memset(qp_attr, 0, sizeof(*qp_attr));
	memset(qp_init_attr, 0, sizeof(*qp_init_attr));

	mutex_lock(&hr_qp->mutex);
	if (hr_qp->state == IB_QPS_RESET) {
		qp_attr->qp_state = IB_QPS_RESET;
		ret = 0;
		goto done;
	}
	state = hr_reg_read(&context, QPC_QP_ST);
	ret = hihtr_v3_query_qpc(hr_dev, hr_qp->qpn, &context);
	if (ret) {
		ibdev_err(ibdev, "failed to query QPC, ret = %d.\n", ret);
		ret = -EINVAL;
		goto out;
	}

	state = hr_reg_read(&context, QPC_QP_ST);
	tmp_qp_state = to_ib_qp_st((enum hihtr_v3_qp_state)state);
	if (tmp_qp_state == -1) {
		ibdev_err(ibdev, "Illegal ib_qp_state\n");
		ret = -EINVAL;
		goto out;
	}
	hr_qp->state = (u8)tmp_qp_state;
	qp_attr->qp_state = (enum ib_qp_state)hr_qp->state;
	qp_attr->path_mtu = (enum ib_mtu)hr_reg_read(&context, QPC_MTU);
	qp_attr->path_mig_state = IB_MIG_ARMED;
	qp_attr->ah_attr.type = RDMA_AH_ATTR_TYPE_ROCE;
	if (hr_qp->ibqp.qp_type == IB_QPT_UD)
		qp_attr->qkey = le32_to_cpu(context.qkey_xrcd);

	qp_attr->rq_psn = hr_reg_read(&context, QPC_RX_REQ_EPSN);
	qp_attr->sq_psn = (u32)hr_reg_read(&context, QPC_SQ_CUR_PSN);
	qp_attr->dest_qp_num = hr_reg_read(&context, QPC_DQPN);
	qp_attr->qp_access_flags =
		((hr_reg_read(&context, QPC_RRE)) << V3_QP_RRE_S) |
		((hr_reg_read(&context, QPC_RWE)) << V3_QP_RWE_S) |
		((hr_reg_read(&context, QPC_ATE)) << V3_QP_ATE_S);

	if (hr_qp->ibqp.qp_type == IB_QPT_RC ||
	    hr_qp->ibqp.qp_type == IB_QPT_XRC_INI ||
	    hr_qp->ibqp.qp_type == IB_QPT_XRC_TGT) {
		struct ib_global_route *grh =
			rdma_ah_retrieve_grh(&qp_attr->ah_attr);

		rdma_ah_set_sl(&qp_attr->ah_attr,
			       hr_reg_read(&context, QPC_SL));
		rdma_ah_set_port_num(&qp_attr->ah_attr, hr_qp->port + 1);
		rdma_ah_set_ah_flags(&qp_attr->ah_attr, IB_AH_GRH);
		grh->flow_label = hr_reg_read(&context, QPC_FL);
		grh->sgid_index = hr_reg_read(&context, QPC_GMV_IDX);
		grh->hop_limit = hr_reg_read(&context, QPC_HOPLIMIT);
		grh->traffic_class = hr_reg_read(&context, QPC_TC);

		memcpy(grh->dgid.raw, context.dgid, sizeof(grh->dgid.raw));
	}

	qp_attr->port_num = hr_qp->port + 1;
	qp_attr->sq_draining = 0;
	qp_attr->max_rd_atomic = 1 << hr_reg_read(&context, QPC_SR_MAX);
	qp_attr->max_dest_rd_atomic = 1 << hr_reg_read(&context, QPC_RR_MAX);

	qp_attr->min_rnr_timer = (u8)hr_reg_read(&context, QPC_MIN_RNR_TIME);
	qp_attr->timeout = (u8)hr_reg_read(&context, QPC_AT);
	qp_attr->retry_cnt = hr_reg_read(&context, QPC_RETRY_NUM_INIT);
	qp_attr->rnr_retry = hr_reg_read(&context, QPC_RNR_NUM_INIT);

done:
	qp_attr->cur_qp_state = qp_attr->qp_state;
	qp_attr->cap.max_recv_wr = hr_qp->rq.wqe_cnt;
	qp_attr->cap.max_recv_sge = hr_qp->rq.max_gs - hr_qp->rq.rsv_sge;
	qp_attr->cap.max_inline_data = hr_qp->max_inline_data;

	qp_attr->cap.max_send_wr = hr_qp->sq.wqe_cnt;
	qp_attr->cap.max_send_sge = hr_qp->sq.max_gs;

	qp_init_attr->qp_context = ibqp->qp_context;
	qp_init_attr->qp_type = ibqp->qp_type;
	qp_init_attr->recv_cq = ibqp->recv_cq;
	qp_init_attr->send_cq = ibqp->send_cq;
	qp_init_attr->srq = ibqp->srq;
	qp_init_attr->cap = qp_attr->cap;
	qp_init_attr->sq_sig_type = hr_qp->sq_signal_bits;

out:
	mutex_unlock(&hr_qp->mutex);
	return ret;
}

int hihtr_v3_set_dca_buf(struct hihtr_dev *hr_dev,
				   struct hihtr_qp *hr_qp)
{
	struct ib_device *ibdev = &hr_dev->ib_dev;
	struct hihtr_v3_qp_context *qpc, *msk;
	dma_addr_t dma_handle;
	int qpc_sz;
	int ret;

	qpc_sz = hr_dev->caps.qpc_sz;
	WARN_ON(QPC_MAILBOX_ENTRY_CNT * qpc_sz > HIHTR_MAILBOX_SIZE);
	qpc = dma_pool_alloc(hr_dev->cmd.pool, GFP_ATOMIC, &dma_handle);
	if (!qpc)
		return -ENOMEM;

	msk = (struct hihtr_v3_qp_context *)((void *)qpc + qpc_sz);
	memset(msk, 0xff, qpc_sz);

	ret = config_qp_rq_buf(hr_dev, hr_qp, qpc, msk);
	if (ret) {
		ibdev_err(ibdev, "failed to config rq qpc, ret = %d.\n", ret);
		goto done;
	}

	ret = config_qp_sq_buf(hr_dev, hr_qp, qpc, msk);
	if (ret) {
		ibdev_err(ibdev, "failed to config sq qpc, ret = %d.\n", ret);
		goto done;
	}

	ret = hihtr_cmq_mbox(hr_dev, dma_handle, 0, HIHTR_CMD_MODIFY_QPC, hr_qp->qpn);
	if (ret)
		ibdev_err(ibdev,
			  "failed to send modify cmd for QP(0x%lx), ret = %d.\n",
			  hr_qp->qpn, ret);

done:
	dma_pool_free(hr_dev->cmd.pool, qpc, dma_handle);

	return ret;
}

bool hihtr_v3_chk_dca_buf_inactive(struct hihtr_dev *hr_dev,
					     struct hihtr_qp *hr_qp)
{
	struct hihtr_dca_cfg *cfg = &hr_qp->dca_cfg;
	struct hihtr_v3_qp_context context = {};
	struct ib_device *ibdev = &hr_dev->ib_dev;
	u32 tmp, sq_idx;
	int state;
	int ret;

	ret = hihtr_v3_query_qpc(hr_dev, hr_qp->qpn, &context);
	if (ret) {
		ibdev_err(ibdev, "failed to query DCA QPC, ret = %d.\n", ret);
		return false;
	}

	state = hr_reg_read(&context, QPC_QP_ST);
	if (state == HIHTR_QP_ST_ERR || state == HIHTR_QP_ST_RST)
		return true;

	if (hr_qp->sq.wqe_cnt > 0) {
		tmp = (u32)hr_reg_read(&context, QPC_RETRY_MSG_MSN);
		sq_idx = tmp & (hr_qp->sq.wqe_cnt - 1);
		/* If SQ-PI equals to retry_msg_msn in QPC, the QP is
		 * inactive.
		 */
		if (sq_idx != cfg->sq_idx)
			return false;
	}

	return true;
}

void dca_detach_qp_buf(struct hihtr_dev *hr_dev,
			      struct hihtr_qp *hr_qp)
{
	struct hihtr_dca_detach_attr attr = {};
	unsigned long flags_sq, flags_rq;
	bool is_empty;

	spin_lock_irqsave(&hr_qp->sq.lock, flags_sq);
	spin_lock_irqsave(&hr_qp->rq.lock, flags_rq);
	is_empty = hr_qp->sq.head == hr_qp->sq.tail &&
		   hr_qp->rq.head == hr_qp->rq.tail;
	if (is_empty && hr_qp->sq.wqe_cnt > 0)
		attr.sq_idx = hr_qp->sq.head & (hr_qp->sq.wqe_cnt - 1);

	spin_unlock_irqrestore(&hr_qp->rq.lock, flags_rq);
	spin_unlock_irqrestore(&hr_qp->sq.lock, flags_sq);

	if (is_empty)
		hihtr_dca_detach(hr_dev, hr_qp, &attr);
}

int get_cur_qp(struct hihtr_cq *hr_cq, struct hihtr_v3_cqe *cqe,
		      struct hihtr_qp **cur_qp)
{
	struct hihtr_dev *hr_dev = to_hr_dev(hr_cq->ib_cq.device);
	struct hihtr_qp *hr_qp = *cur_qp;
	u32 qpn;

	qpn = hr_reg_read(cqe, CQE_LCL_QPN);
	if (!hr_qp || qpn != hr_qp->qpn) {
#if defined(RDMA_HAVE_XARRAY)
		hr_qp = __hihtr_qp_lookup(hr_dev, qpn);
#else
		hr_qp = radix_tree_lookup(&hr_dev->qp_table_root, qpn);
#endif
		if (unlikely(!hr_qp)) {
			ibdev_err(&hr_dev->ib_dev,
				  "CQ %06lx with entry for unknown QPN %06x\n",
				  hr_cq->cqn, qpn);
			return -EINVAL;
		}
		*cur_qp = hr_qp;
	}

	return 0;
}

int hihtr_v3_query_sccc(struct hihtr_dev *hr_dev, u32 qpn,
				 void *buffer)
{
	struct hihtr_cmd_mailbox *mailbox;
	int ret;
	int count = hr_dev->qp_table.sccc_table.obj_count;

	if (qpn >= count) {
		return -EINVAL;
	}
	mailbox = hihtr_alloc_cmd_mailbox(hr_dev);
	if (IS_ERR(mailbox))
		return PTR_ERR(mailbox);
	ret = hihtr_cmq_mbox(hr_dev, 0, mailbox->dma, HIHTR_CMD_QUERY_SCCC,
				qpn);
	if (ret)
		goto out;

	memcpy(buffer, mailbox->buf, hr_dev->caps.sccc_sz);

out:
	hihtr_free_cmd_mailbox(hr_dev, mailbox);
	return ret;
}