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

#include <linux/pci.h>
#include <rdma/ib_addr.h>
#include <rdma/ib_umem.h>
#include <rdma/uverbs_ioctl.h>
#include "hihtr_common.h"
#include "hihtr_device.h"
#include "hihtr_hem.h"
#include "hihtr_dca.h"
#include "hihtr_hw_v3.h"
#include "hihtr_hw_v3_cq.h"
#include "dfx/hihtr_dfx.h"

static void flush_work_handle(struct work_struct *work)
{
	struct hihtr_work *flush_work = container_of(work,
					struct hihtr_work, work);
	struct hihtr_qp *hr_qp = container_of(flush_work,
					struct hihtr_qp, flush_work);
	struct device *dev = flush_work->hr_dev->dev;
	struct ib_qp_attr attr;
	int attr_mask;
	int ret;

	attr_mask = IB_QP_STATE;
	attr.qp_state = IB_QPS_ERR;

	if (test_and_clear_bit(HIHTR_FLUSH_FLAG, &hr_qp->flush_flag)) {
		ret = hihtr_modify_qp(&hr_qp->ibqp, &attr, attr_mask, NULL);
		if (ret)
			dev_err(dev, "Modify QP to error state failed(%d) during CQE flush\n",
				ret);
	}

	/*
	 * make sure we signal QP destroy leg that flush QP was completed
	 * so that it can safely proceed ahead now and destroy QP
	 */
	if (refcount_dec_and_test(&hr_qp->refcount))
		complete(&hr_qp->free);
}

void init_flush_work(struct hihtr_dev *hr_dev, struct hihtr_qp *hr_qp)
{
	struct hihtr_work *flush_work = &hr_qp->flush_work;

	flush_work->hr_dev = hr_dev;
	INIT_WORK(&flush_work->work, flush_work_handle);
	refcount_inc(&hr_qp->refcount);
	if (hr_dev->irq_workq != NULL) {
		/* TO DO 等待适配 */
	    queue_work(hr_dev->irq_workq, &flush_work->work);
	}
}

void flush_cqe(struct hihtr_dev *dev, struct hihtr_qp *qp)
{
	/*
	 * Hip08 hardware cannot flush the WQEs in SQ/RQ if the QP state
	 * gets into errored mode. Hence, as a workaround to this
	 * hardware limitation, driver needs to assist in flushing. But
	 * the flushing operation uses mailbox to convey the QP state to
	 * the hardware and which can sleep due to the mutex protection
	 * around the mailbox calls. Hence, use the deferred flush for
	 * now.
	 */
	if (!test_and_set_bit(HIHTR_FLUSH_FLAG, &qp->flush_flag))
		init_flush_work(dev, qp);
}

void hihtr_qp_event(struct hihtr_dev *hr_dev, u32 qpn, int event_type)
{
	struct device *dev = hr_dev->dev;
	struct hihtr_qp *qp;
#if !defined(RDMA_HAVE_XARRAY)
	unsigned long flags;
#endif

#if defined(RDMA_HAVE_XARRAY)
	xa_lock(&hr_dev->qp_table_xa);
	qp = __hihtr_qp_lookup(hr_dev, qpn);
	if (qp)
		refcount_inc(&qp->refcount);
	xa_unlock(&hr_dev->qp_table_xa);
#else
	spin_lock_irqsave(&hr_dev->tree_lock, flags);
	qp = radix_tree_lookup(&hr_dev->qp_table_root, qpn);
	if (qp)
		refcount_inc(&qp->refcount);
	spin_unlock_irqrestore(&hr_dev->tree_lock, flags);
#endif

	if (!qp) {
		dev_warn(dev, "Async event for bogus QP %08x\n", qpn);
		return;
	}

	if (event_type == HIHTR_EVENT_TYPE_WQ_CATAS_ERROR ||
	    event_type == HIHTR_EVENT_TYPE_INV_REQ_LOCAL_WQ_ERROR ||
	    event_type == HIHTR_EVENT_TYPE_LOCAL_WQ_ACCESS_ERROR ||
	    event_type == HIHTR_EVENT_TYPE_XRCD_VIOLATION ||
	    event_type == HIHTR_EVENT_TYPE_INVALID_XRCETH) {
		qp->state = IB_QPS_ERR;

		flush_cqe(hr_dev, qp);
	}

	qp->event(qp, (enum hihtr_event)event_type);

	if (refcount_dec_and_test(&qp->refcount))
		complete(&qp->free);
}

static void hihtr_ib_qp_event(struct hihtr_qp *hr_qp,
				 enum hihtr_event type)
{
	struct ib_qp *ibqp = &hr_qp->ibqp;
	struct ib_event event;
	struct hihtr_dev *hr_dev = to_hr_dev(ibqp->device);

	switch (type) {
		case HIHTR_EVENT_TYPE_PATH_MIG:
			event.event = IB_EVENT_PATH_MIG;
			break;
		case HIHTR_EVENT_TYPE_COMM_EST:
			event.event = IB_EVENT_COMM_EST;
			break;
		case HIHTR_EVENT_TYPE_SQ_DRAINED:
			event.event = IB_EVENT_SQ_DRAINED;
			break;
		case HIHTR_EVENT_TYPE_SRQ_LAST_WQE_REACH:
			event.event = IB_EVENT_QP_LAST_WQE_REACHED;
			break;
		case HIHTR_EVENT_TYPE_WQ_CATAS_ERROR:
			event.event = IB_EVENT_QP_FATAL;
			break;
		case HIHTR_EVENT_TYPE_PATH_MIG_FAILED:
			event.event = IB_EVENT_PATH_MIG_ERR;
			break;
		case HIHTR_EVENT_TYPE_INV_REQ_LOCAL_WQ_ERROR:
			event.event = IB_EVENT_QP_REQ_ERR;
			break;
		case HIHTR_EVENT_TYPE_LOCAL_WQ_ACCESS_ERROR:
		case HIHTR_EVENT_TYPE_XRCD_VIOLATION:
		case HIHTR_EVENT_TYPE_INVALID_XRCETH:
			event.event = IB_EVENT_QP_ACCESS_ERR;
			break;
		default:
			dev_dbg(ibqp->device->dev.parent, "roce_ib: Unexpected event type %d on QP %06lx\n",
				type, hr_qp->qpn);
			return;
	}
	hihtr_dfx_record_err_event_count(hr_dev, event.event);
	if (ibqp->event_handler) {
		event.device = ibqp->device;
		event.element.qp = ibqp;
		ibqp->event_handler(&event, ibqp->qp_context);
	}
}

static int alloc_qpn(struct hihtr_dev *hr_dev, struct hihtr_qp *hr_qp)
{
	struct hihtr_qp_table *qp_table = &hr_dev->qp_table;
	struct hihtr_ida *qp_ida = &qp_table->qp_ida;
	int id = 0;

	if (hr_qp->ibqp.qp_type == IB_QPT_GSI) {
		id = 1;
		hr_qp->doorbell_qpn = 1;
	} else {
		mutex_lock(&qp_table->ida_mutex);
		id = ida_alloc_range(&qp_ida->ida, qp_ida->min, qp_ida->max, GFP_KERNEL);
		if (id < 0) {
			mutex_unlock(&qp_table->ida_mutex);
 			ibdev_err(&hr_dev->ib_dev,
				"failed to alloc QPN, id = %d\n", id);
			return id;
		}
		mutex_unlock(&qp_table->ida_mutex);
	}

	hr_qp->doorbell_qpn = id;
	hr_qp->qpn = id;
	return 0;
}

static void add_qp_to_list(struct hihtr_dev *hr_dev,
			   struct hihtr_qp *hr_qp,
			   struct ib_cq *send_cq, struct ib_cq *recv_cq)
{
	struct hihtr_cq *hr_send_cq, *hr_recv_cq;
	unsigned long flags;

	hr_send_cq = send_cq ? to_hr_cq(send_cq) : NULL;
	hr_recv_cq = recv_cq ? to_hr_cq(recv_cq) : NULL;

	spin_lock_irqsave(&hr_dev->qp_list_lock, flags);
	hihtr_lock_cqs(hr_send_cq, hr_recv_cq);

	list_add_tail(&hr_qp->node, &hr_dev->qp_list);
	if (hr_send_cq)
		list_add_tail(&hr_qp->sq_node, &hr_send_cq->sq_list);
	if (hr_recv_cq)
		list_add_tail(&hr_qp->rq_node, &hr_recv_cq->rq_list);

	hihtr_unlock_cqs(hr_send_cq, hr_recv_cq);
	spin_unlock_irqrestore(&hr_dev->qp_list_lock, flags);
}

static int hihtr_qp_store(struct hihtr_dev *hr_dev,
			     struct hihtr_qp *hr_qp,
			     struct ib_qp_init_attr *init_attr)
{
#if defined(RDMA_HAVE_XARRAY)
	struct xarray *xa = &hr_dev->qp_table_xa;
#else
	unsigned long flags;
#endif
	int ret;

	if (!hr_qp->qpn)
		return -EINVAL;

#if defined(RDMA_HAVE_XARRAY)
	ret = xa_err(xa_store_irq(xa, hr_qp->qpn, hr_qp, GFP_KERNEL));
#else
	ret = radix_tree_preload(GFP_KERNEL);
	if (ret) {
		dev_err(hr_dev->dev, "Failed to preload root for QPC\n");
		return ret;
	}

	spin_lock_irqsave(&hr_dev->tree_lock, flags);
	ret = radix_tree_insert(&hr_dev->qp_table_root, hr_qp->qpn, hr_qp);
	spin_unlock_irqrestore(&hr_dev->tree_lock, flags);

	radix_tree_preload_end();
#endif
	if (ret)
		dev_err(hr_dev->dev, "Failed to xa store for QPC\n");
	else
		/* add QP to device's QP list for softwc */
		add_qp_to_list(hr_dev, hr_qp, init_attr->send_cq,
			       init_attr->recv_cq);

	return ret;
}

static int alloc_qpc(struct hihtr_dev *hr_dev, struct hihtr_qp *hr_qp)
{
	struct hihtr_qp_table *qp_table = &hr_dev->qp_table;
	struct device *dev = hr_dev->dev;
	int ret;

	if (!hr_qp->qpn)
		return -EINVAL;

	/* Alloc memory for QPC */
	ret = hihtr_table_get(hr_dev, &qp_table->qp_table, hr_qp->qpn);
	if (ret) {
		dev_err(dev, "Failed to get QPC table\n");
		goto err_out;
	}

	/* Alloc memory for IRRL */
	ret = hihtr_table_get(hr_dev, &qp_table->irrl_table, hr_qp->qpn);
	if (ret) {
		dev_err(dev, "Failed to get IRRL table\n");
		goto err_out;
	}

	if (hr_dev->caps.trrl_entry_sz) {
		/* Alloc memory for TRRL */
		ret = hihtr_table_get(hr_dev, &qp_table->trrl_table,
					 hr_qp->qpn);
		if (ret) {
			dev_err(dev, "Failed to get TRRL table\n");
			goto err_put_irrl;
		}
	}

	/* get table for SCC CTX */
	ret = hihtr_table_get(hr_dev, &qp_table->sccc_table,
					hr_qp->qpn);
	if (ret) {
		dev_err(dev, "Failed to get SCC CTX table\n");
		goto err_put_trrl;
	}

	return 0;

err_put_trrl:
	if (hr_dev->caps.trrl_entry_sz)
		hihtr_table_put(hr_dev, &qp_table->trrl_table, hr_qp->qpn);

err_put_irrl:
	hihtr_table_put(hr_dev, &qp_table->irrl_table, hr_qp->qpn);

err_out:
	return ret;
}

void hihtr_qp_remove(struct hihtr_dev *hr_dev, struct hihtr_qp *hr_qp)
{
#if defined(RDMA_HAVE_XARRAY)
	struct xarray *xa = &hr_dev->qp_table_xa;
#endif
	unsigned long flags;

	/*
	 * Use list_del_init() instead of list_del() to reinitialize
	 * the list_head after removal. This prevents list corruption
	 * if the QP memory is later reused (e.g., kzalloc recycling)
	 * without proper INIT_LIST_HEAD - the stale prev/next pointers
	 * would cause "list_add corruption" panics.
	 *
	 * list_empty_careful() check prevents crash when QP was never
	 * added to the list (e.g., hihtr_qp_store failed before
	 * add_qp_to_list was called, leaving node fields as NULL/zero).
	 */
	if (!list_empty_careful(&hr_qp->node))
		list_del_init(&hr_qp->node);

	if (hr_qp->ibqp.qp_type != IB_QPT_XRC_TGT) {
		if (!list_empty_careful(&hr_qp->sq_node))
			list_del_init(&hr_qp->sq_node);
	}

	if (hr_qp->ibqp.qp_type != IB_QPT_XRC_INI &&
	    hr_qp->ibqp.qp_type != IB_QPT_XRC_TGT) {
		if (!list_empty_careful(&hr_qp->rq_node))
			list_del_init(&hr_qp->rq_node);
	}

#if defined(RDMA_HAVE_XARRAY)
	xa_lock_irqsave(xa, flags);
	__xa_erase(xa, hr_qp->qpn);
	xa_unlock_irqrestore(xa, flags);
#else
	spin_lock_irqsave(&hr_dev->tree_lock, flags);
	radix_tree_delete(&hr_dev->qp_table_root, hr_qp->qpn);
	spin_unlock_irqrestore(&hr_dev->tree_lock, flags);
#endif
}

static void free_qpc(struct hihtr_dev *hr_dev, struct hihtr_qp *hr_qp)
{
	struct hihtr_qp_table *qp_table = &hr_dev->qp_table;

	if (hr_dev->caps.trrl_entry_sz)
		hihtr_table_put(hr_dev, &qp_table->trrl_table, hr_qp->qpn);
	hihtr_table_put(hr_dev, &qp_table->irrl_table, hr_qp->qpn);
}

static void free_qpn(struct hihtr_dev *hr_dev, struct hihtr_qp *hr_qp)
{
	if (hr_qp->ibqp.qp_type == IB_QPT_GSI)
		return;

	if (hr_qp->qpn < hr_dev->caps.reserved_qps)
		return;
	ida_free(&hr_dev->qp_table.qp_ida.ida, hr_qp->qpn);
}

static u32 proc_rq_sge(struct hihtr_dev *dev, struct hihtr_qp *hr_qp,
		       bool user)
{
	u32 max_sge = dev->caps.max_rq_sg;

	if (HIHTR_GET_PCI_REVISION(dev) >= PCI_REVISION_ID_HIP09)
		return max_sge;

	/* Reserve SGEs only for HIP08 in kernel; The userspace driver will
	 * calculate number of max_sge with reserved SGEs when allocating wqe
	 * buf, so there is no need to do this again in kernel. But the number
	 * may exceed the capacity of SGEs recorded in the firmware, so the
	 * kernel driver should just adapt the value accordingly.
	 */
	if (user)
		max_sge = roundup_pow_of_two(max_sge + 1);
	else
		hr_qp->rq.rsv_sge = 1;

	return max_sge;
}

static int set_rq_size(struct hihtr_dev *hr_dev, struct ib_qp_cap *cap,
		       struct hihtr_qp *hr_qp, int has_rq, bool user)
{
	u32 max_sge = proc_rq_sge(hr_dev, hr_qp, user);
	u32 cnt;

	/* If srq exist, set zero for relative number of rq */
	if (!has_rq) {
		hr_qp->rq.wqe_cnt = 0;
		hr_qp->rq.max_gs = 0;
		cap->max_recv_wr = 0;
		cap->max_recv_sge = 0;

		return 0;
	}

	/* Check the validity of QP support capacity */
	if (!cap->max_recv_wr || cap->max_recv_wr > hr_dev->caps.max_rq_wqes ||
	    cap->max_recv_sge > max_sge) {
		ibdev_err(&hr_dev->ib_dev,
			  "RQ config error, depth = %u, sge = %u\n",
			  cap->max_recv_wr, cap->max_recv_sge);
		return -EINVAL;
	}

	cnt = roundup_pow_of_two(max(cap->max_recv_wr, hr_dev->caps.min_wqes));
	if (cnt > hr_dev->caps.max_rq_wqes) {
		ibdev_err(&hr_dev->ib_dev, "rq depth %u too large\n",
			  cap->max_recv_wr);
		return -EINVAL;
	}

	hr_qp->rq.max_gs = roundup_pow_of_two(max(1U, cap->max_recv_sge) +
					      hr_qp->rq.rsv_sge);

	if (hr_dev->caps.max_rq_sg <= HIHTR_SGE_IN_WQE)
		hr_qp->rq.wqe_shift = ilog2(hr_dev->caps.max_rq_desc_sz);
	else
		hr_qp->rq.wqe_shift = ilog2(hr_dev->caps.max_rq_desc_sz *
					    hr_qp->rq.max_gs);

	hr_qp->rq.wqe_cnt = cnt;

	cap->max_recv_wr = cnt;
	cap->max_recv_sge = hr_qp->rq.max_gs - hr_qp->rq.rsv_sge;

	return 0;
}

static u32 get_max_inline_data(struct hihtr_dev *hr_dev,
			       struct ib_qp_cap *cap)
{
	if (cap->max_inline_data) {
		cap->max_inline_data = roundup_pow_of_two(
					cap->max_inline_data);
		return min(cap->max_inline_data,
			   hr_dev->caps.max_sq_inline);
	}

	return 0;
}

static void update_inline_data(struct hihtr_qp *hr_qp,
			       struct ib_qp_cap *cap)
{
	u32 sge_num = hr_qp->sq.ext_sge_cnt;

	if (hr_qp->config & HIHTR_EXSGE_FLAGS) {
		if (!(hr_qp->ibqp.qp_type == IB_QPT_GSI ||
			hr_qp->ibqp.qp_type == IB_QPT_UD))
			sge_num = max((u32)HIHTR_SGE_IN_WQE, sge_num);

		cap->max_inline_data = max(cap->max_inline_data,
					sge_num * HIHTR_SGE_SIZE);
	}

	hr_qp->max_inline_data = cap->max_inline_data;
}

/**
 *  Calculated sge num according to attr's max_send_sge
 */
static u32 get_sge_num_from_max_send_sge(bool is_ud_or_gsi,
					 u32 max_send_sge)
{
	unsigned int std_sge_num;
	unsigned int min_sge;

	std_sge_num = is_ud_or_gsi ? 0 : HIHTR_SGE_IN_WQE;
	min_sge = is_ud_or_gsi ? 1 : 0;
	return max_send_sge > std_sge_num ? (max_send_sge - std_sge_num) :
				min_sge;
}

/**
 *  Calculated sge num according to attr's max_inline_data
 */
static unsigned int get_sge_num_from_max_inl_data(bool is_ud_or_gsi,
						  u32 max_inline_data)
{
	unsigned int inline_sge;

	inline_sge = roundup_pow_of_two(max_inline_data) / HIHTR_SGE_SIZE;
	/*
	 * if max_inline_data less than
	 * HIHTR_SGE_IN_WQE * HIHTR_SGE_SIZE,
	 * In addition to ud's mode, no need to extend sge.
	 */
	if ((!is_ud_or_gsi) && (inline_sge <= HIHTR_SGE_IN_WQE))
		inline_sge = 0;

	return inline_sge;
}

static void set_ext_sge_param(struct hihtr_dev *hr_dev, u32 sq_wqe_cnt,
			      struct hihtr_qp *hr_qp, struct ib_qp_cap *cap)
{
	bool is_ud_or_gsi = (hr_qp->ibqp.qp_type == IB_QPT_GSI ||
				hr_qp->ibqp.qp_type == IB_QPT_UD);
	unsigned int std_sge_num;
	u32 inline_ext_sge = 0;
	u32 ext_wqe_sge_cnt;
	u32 total_sge_cnt;

	cap->max_inline_data = get_max_inline_data(hr_dev, cap);

	hr_qp->sge.sge_shift = HIHTR_SGE_SHIFT;
	std_sge_num = is_ud_or_gsi ? 0 : HIHTR_SGE_IN_WQE;
	ext_wqe_sge_cnt = get_sge_num_from_max_send_sge(is_ud_or_gsi,
							cap->max_send_sge);

	if (hr_qp->config & HIHTR_EXSGE_FLAGS) {
		inline_ext_sge = max(ext_wqe_sge_cnt,
				 get_sge_num_from_max_inl_data(
				 is_ud_or_gsi, cap->max_inline_data));
		hr_qp->sq.ext_sge_cnt = !!(inline_ext_sge) ?
					roundup_pow_of_two(inline_ext_sge) : 0;

		hr_qp->sq.max_gs = max(1U, (hr_qp->sq.ext_sge_cnt + std_sge_num));
		hr_qp->sq.max_gs = min(hr_qp->sq.max_gs, hr_dev->caps.max_sq_sg);

		ext_wqe_sge_cnt = hr_qp->sq.ext_sge_cnt;
	} else {
		hr_qp->sq.max_gs = max(1U, cap->max_send_sge);
		hr_qp->sq.max_gs = min(hr_qp->sq.max_gs, hr_dev->caps.max_sq_sg);
		hr_qp->sq.ext_sge_cnt = hr_qp->sq.max_gs;
	}

	/* If the number of extended sge is not zero, they MUST use the
	 * space of HIHTR_HW_PAGE_SIZE at least.
	 */
	if (ext_wqe_sge_cnt) {
		total_sge_cnt = roundup_pow_of_two(sq_wqe_cnt * ext_wqe_sge_cnt);
		hr_qp->sge.sge_cnt = max(total_sge_cnt,
				(u32)HIHTR_HW_PAGE_SIZE / HIHTR_SGE_SIZE);
	}

	update_inline_data(hr_qp, cap);
}

static int check_sq_size_with_integrity(struct hihtr_dev *hr_dev,
					struct ib_qp_cap *cap,
					struct hihtr_ib_create_qp *ucmd)
{
	u32 roundup_sq_stride = roundup_pow_of_two(hr_dev->caps.max_sq_desc_sz);
	u8 max_sq_stride = ilog2(roundup_sq_stride);
	/* Sanity check SQ size before proceeding */
	if (ucmd->log_sq_stride > max_sq_stride ||
	    ucmd->log_sq_stride < HIHTR_IB_MIN_SQ_STRIDE) {
		ibdev_err(&hr_dev->ib_dev, "failed to check SQ stride size.\n");
		return -EINVAL;
	}

	if (cap->max_send_sge > hr_dev->caps.max_sq_sg) {
		ibdev_err(&hr_dev->ib_dev, "failed to check SQ SGE size %u.\n",
			  cap->max_send_sge);
		return -EINVAL;
	}

	return 0;
}

static int set_user_sq_size(struct hihtr_dev *hr_dev,
			    struct ib_qp_cap *cap, struct hihtr_qp *hr_qp,
			    struct hihtr_ib_create_qp *ucmd)
{
	struct ib_device *ibdev = &hr_dev->ib_dev;
	u32 tx_depth = 0;
	u32 cnt = 0;
	int ret;

	if (check_shl_overflow(1, ucmd->log_sq_bb_count, &tx_depth) ||
	    tx_depth > hr_dev->caps.max_sq_wqes)
		return -EINVAL;

	ret = check_sq_size_with_integrity(hr_dev, cap, ucmd);
	if (ret) {
		ibdev_err(ibdev, "failed to check user SQ size, ret = %d.\n",
			  ret);
		return ret;
	}

	// sge不感知深度变化
	cnt = tx_depth >> 1;
	set_ext_sge_param(hr_dev, cnt, hr_qp, cap);

	hr_qp->sq.wqe_shift = ucmd->log_sq_stride;
	hr_qp->sq.wqe_cnt = tx_depth;
	cap->max_send_sge = hr_qp->sq.max_gs;

	return 0;
}

static bool check_dca_is_enable(struct hihtr_dev *hr_dev,
				struct hihtr_qp *hr_qp,
				struct ib_qp_init_attr *init_attr, bool is_user,
				unsigned long addr)
{
	if (!(hr_dev->caps.flags & HIHTR_CAP_FLAG_DCA_MODE))
		return false;

	/* If the user QP's buffer addr is 0, the DCA mode should be enabled */
	if (is_user)
		return !addr;

	/* Only RC and XRC support DCA for kernel QP */
	if (hr_dev->dca_ctx.max_size > 0 &&
	    (init_attr->qp_type == IB_QPT_RC ||
	     init_attr->qp_type == IB_QPT_XRC_INI))
		return !!(init_attr->create_flags & HIHTR_QP_CREATE_DCA_EN);

	return false;
}

static int set_wqe_buf_attr(struct hihtr_dev *hr_dev,
			    struct hihtr_qp *hr_qp, bool dca_en,
			    struct hihtr_buf_attr *buf_attr)
{
	int buf_size;
	int idx = 0;
	uint32_t tmp_buf_size = 0;
	hr_qp->buff_size = 0;

	if (dca_en) {
		/*
		 * When enable DCA, there's no need to alloc buffer now, and
		 * the page shift should be fixed to 4K.
		 */
		buf_attr->mtt_only = true;
		buf_attr->page_shift = HIHTR_HW_PAGE_SHIFT;
	} else {
		buf_attr->mtt_only = false;
		buf_attr->page_shift = HIHTR_HW_PAGE_SHIFT +
				       hr_dev->caps.mtt_buf_pg_sz;
	}
	/* SQ WQE */
	hr_qp->sq.offset = 0;
	buf_size = to_hr_hem_entries_size(hr_qp->sq.wqe_cnt,
					  hr_qp->sq.wqe_shift);
	if (buf_size > 0 && idx < ARRAY_SIZE(buf_attr->region)) {
		tmp_buf_size = roundup(buf_size, (1 << buf_attr->page_shift));
		buf_attr->region[idx].size = tmp_buf_size;
		buf_attr->region[idx].hopnum = hr_dev->caps.wqe_sq_hop_num;
		idx++;
		hr_qp->buff_size += tmp_buf_size;
	}

	/* extend SGE WQE in SQ */
	hr_qp->sge.offset = hr_qp->buff_size;
	buf_size = to_hr_hem_entries_size(hr_qp->sge.sge_cnt,
					  hr_qp->sge.sge_shift);
	if (buf_size > 0 && idx < ARRAY_SIZE(buf_attr->region)) {
		tmp_buf_size = roundup(buf_size, (1 << buf_attr->page_shift));
		buf_attr->region[idx].size = tmp_buf_size;
		buf_attr->region[idx].hopnum = hr_dev->caps.wqe_sge_hop_num;
		idx++;
		hr_qp->buff_size += tmp_buf_size;
	}

	/* RQ WQE */
	hr_qp->rq.offset = hr_qp->buff_size;
	buf_size = to_hr_hem_entries_size(hr_qp->rq.wqe_cnt,
					  hr_qp->rq.wqe_shift);
	if (buf_size > 0 && idx < ARRAY_SIZE(buf_attr->region)) {
		tmp_buf_size = roundup(buf_size, (1 << buf_attr->page_shift));
		buf_attr->region[idx].size = tmp_buf_size;
		buf_attr->region[idx].hopnum = hr_dev->caps.wqe_rq_hop_num;
		idx++;
		hr_qp->buff_size += tmp_buf_size;
	}

	if (hr_qp->buff_size < 1)
		return -EINVAL;

	buf_attr->region_count = idx;

	return 0;
}

static int set_kernel_sq_size(struct hihtr_dev *hr_dev,
			      struct ib_qp_cap *cap, struct hihtr_qp *hr_qp)
{
	struct ib_device *ibdev = &hr_dev->ib_dev;
	u32 cnt;

	if (!cap->max_send_wr || cap->max_send_wr > hr_dev->caps.max_sq_wqes ||
	    cap->max_send_sge > hr_dev->caps.max_sq_sg) {
		ibdev_err(ibdev, "failed to check SQ WR or SGE num.\n");
		return -EINVAL;
	}

	cnt = roundup_pow_of_two(max(cap->max_send_wr, hr_dev->caps.min_wqes));
	if (cnt > hr_dev->caps.max_sq_wqes) {
		ibdev_err(ibdev, "failed to check WQE num, WQE num = %u.\n",
			  cnt);
		return -EINVAL;
	}

	hr_qp->sq.wqe_shift = ilog2(hr_dev->caps.max_sq_desc_sz);
	hr_qp->sq.wqe_cnt = cnt;

	set_ext_sge_param(hr_dev, cnt, hr_qp, cap);

	/* sync the parameters of kernel QP to user's configuration */
	cap->max_send_wr = cnt;
	cap->max_send_sge = hr_qp->sq.max_gs;

	return 0;
}

static int hihtr_qp_has_sq(struct ib_qp_init_attr *attr)
{
	if (attr->qp_type == IB_QPT_XRC_TGT || !attr->cap.max_send_wr)
		return 0;

	return 1;
}

static int hihtr_qp_has_rq(struct ib_qp_init_attr *attr)
{
	if (attr->qp_type == IB_QPT_XRC_INI ||
	    attr->qp_type == IB_QPT_XRC_TGT || attr->srq ||
	    !attr->cap.max_recv_wr)
		return 0;

	return 1;
}

static bool has_dwqe_cap(struct hihtr_dev *hr_dev, struct ib_udata *udata, struct hihtr_ucontext *hr_uctx)
{
	if (hr_dev->caps.flags & HIHTR_CAP_FLAG_DIRECT_WQE) {
		if (!udata && (hr_dev->en_flags & HIHTR_CAP_FLAG_DIRECT_WQE)) {
			return true;
		} else if (udata && hr_uctx && (hr_uctx->en_flags & HIHTR_CAP_FLAG_DIRECT_WQE)) {
			return true;
		}
	}
	return false;
}

static int alloc_wqe_buf(struct hihtr_dev *hr_dev, struct hihtr_qp *hr_qp,
			struct ib_pd *ib_pd, bool dca_en, struct hihtr_buf_attr *buf_attr,
			 struct ib_udata *udata, unsigned long addr)
{
	struct ib_device *ibdev = &hr_dev->ib_dev;
	struct ib_uobject *uobject = NULL;
	struct hihtr_ucontext *hr_uctx = NULL;
	int ret;

#if !defined(UVERBS_ATTR_BUNDLE_HAVE_DRIVER_UDATA)
	struct ib_ucontext *ibucontext = NULL;
	uobject = hr_qp->ibqp.uobject;
	if (uobject) {
		ibucontext = uobject->context;
		hr_uctx = to_hr_ucontext(ibucontext);
	}
#elif !defined(UVERBS_IOCTL_HAVE_RDMA_UDATA_TO_DRV_CONTEXT)
	struct uverbs_attr_bundle *uattr = NULL;
	uattr = udata ? container_of(udata, struct uverbs_attr_bundle, driver_udata) : NULL;
	hr_uctx = udata ? to_hr_ucontext(ib_uverbs_get_ucontext(uattr)) : NULL;
#else
	hr_uctx = rdma_udata_to_drv_context(udata, struct hihtr_ucontext, ibucontext);
#endif
	if (dca_en) {
		/* DCA must be enabled after the buffer attr is configured. */
		ret = hihtr_enable_dca(hr_dev, hr_qp, udata);
		if (ret) {
			ibdev_err(ibdev, "failed to enable DCA, ret = %d.\n",
				  ret);
			return ret;
		}

		hr_qp->en_flags |= HIHTR_QP_CAP_DYNAMIC_CTX_ATTACH;
	} else if (has_dwqe_cap(hr_dev, udata, hr_uctx)){
		/*
		 * Because DCA and DWQE share the same fileds in RCWQE buffer,
		 * so DWQE only supported when DCA is disable.
		 */
		hr_qp->en_flags |= HIHTR_QP_CAP_DIRECT_WQE;

	}
	// 对XRC类型QP创建，pd为NULL，旧版本的内核不支持XRC，XRC的完整引入在5.10以后
	if (ib_pd != NULL && uobject == NULL) {
		uobject = ib_pd->uobject;
	}
	ret = hihtr_mtr_create(hr_dev, &hr_qp->mtr, buf_attr,
				  PAGE_SHIFT + hr_dev->caps.mtt_ba_pg_sz,
				  udata, addr, uobject);
	if (ret) {
		ibdev_err(ibdev, "failed to create WQE mtr, ret = %d.\n", ret);
		if (dca_en)
			hihtr_disable_dca(hr_dev, hr_qp, udata);
	}

	return ret;
}

static void free_wqe_buf(struct hihtr_dev *hr_dev, struct hihtr_qp *hr_qp,
			 struct ib_udata *udata)
{
	hihtr_mtr_destroy(hr_dev, &hr_qp->mtr);

	if (hr_qp->en_flags & HIHTR_QP_CAP_DYNAMIC_CTX_ATTACH)
		hihtr_disable_dca(hr_dev, hr_qp, udata);
}

static int alloc_qp_wqe(struct hihtr_dev *hr_dev, struct hihtr_qp *hr_qp,
			struct ib_pd *ib_pd, struct ib_qp_init_attr *init_attr,
			struct ib_udata *udata, unsigned long addr)
{
	struct ib_device *ibdev = &hr_dev->ib_dev;
	struct hihtr_buf_attr buf_attr = {};
	bool dca_en;
	int ret;

	dca_en = check_dca_is_enable(hr_dev, hr_qp, init_attr, !!udata, addr);
	ret = set_wqe_buf_attr(hr_dev, hr_qp, dca_en, &buf_attr);
	if (ret) {
		ibdev_err(ibdev, "failed to split WQE buf, ret = %d.\n", ret);
		return ret;
	}

	ret = alloc_wqe_buf(hr_dev, hr_qp, ib_pd, dca_en, &buf_attr, udata, addr);
	if (ret)
		ibdev_err(ibdev, "failed to alloc WQE buf, ret = %d.\n", ret);

	return ret;
}

static void free_qp_wqe(struct hihtr_dev *hr_dev, struct hihtr_qp *hr_qp,
			struct ib_udata *udata)
{
	free_wqe_buf(hr_dev, hr_qp, udata);
}

static inline bool user_qp_has_sdb(struct hihtr_dev *hr_dev,
				   struct ib_qp_init_attr *init_attr,
				   struct ib_udata *udata,
				   struct hihtr_ib_create_qp_resp *resp,
				   struct hihtr_ib_create_qp *ucmd)
{
	return ((hr_dev->caps.flags & HIHTR_CAP_FLAG_QP_RECORD_DB) &&
		udata->outlen >= offsetofend(typeof(*resp), cap_flags) &&
		hihtr_qp_has_sq(init_attr) &&
		udata->inlen >= offsetofend(typeof(*ucmd), sdb_addr));
}

static inline bool user_qp_has_rdb(struct hihtr_dev *hr_dev,
				   struct ib_qp_init_attr *init_attr,
				   struct ib_udata *udata,
				   struct hihtr_ib_create_qp_resp *resp)
{
	return ((hr_dev->caps.flags & HIHTR_CAP_FLAG_QP_RECORD_DB) &&
		udata->outlen >= offsetofend(typeof(*resp), cap_flags) &&
		hihtr_qp_has_rq(init_attr));
}

static inline bool kernel_qp_has_rdb(struct hihtr_dev *hr_dev,
				     struct ib_qp_init_attr *init_attr)
{
	return ((hr_dev->caps.flags & HIHTR_CAP_FLAG_QP_RECORD_DB) &&
		hihtr_qp_has_rq(init_attr));
}

static int alloc_user_qp_db(struct hihtr_dev *hr_dev,
			    struct hihtr_qp *hr_qp, struct ib_pd *ib_pd,
			    struct ib_qp_init_attr *init_attr,
			    struct ib_udata *udata,
			    struct hihtr_ib_create_qp *ucmd,
			    struct hihtr_ib_create_qp_resp *resp)
{
	struct hihtr_ucontext *uctx = NULL;
	struct ib_device *ibdev = &hr_dev->ib_dev;
	int ret;
#if !defined(UVERBS_ATTR_BUNDLE_HAVE_DRIVER_UDATA)
	struct ib_uobject *uobject = ib_pd->uobject;
	if (uobject) {
		struct ib_ucontext *ib_uctx = uobject->context;
		uctx = to_hr_ucontext(ib_uctx);
	}
#elif !defined(UVERBS_IOCTL_HAVE_RDMA_UDATA_TO_DRV_CONTEXT)
	struct uverbs_attr_bundle *attr = NULL;

	attr = udata ? container_of(udata, struct uverbs_attr_bundle, driver_udata) : NULL;
	uctx = udata ? to_hr_ucontext(ib_uverbs_get_ucontext(attr)) : NULL;
#else
	uctx = rdma_udata_to_drv_context(udata, struct hihtr_ucontext, ibucontext);
#endif

	if (user_qp_has_sdb(hr_dev, init_attr, udata, resp, ucmd)) {
		ret = hihtr_db_map_user(uctx, ucmd->sdb_addr, &hr_qp->sdb, udata);
		if (ret) {
			ibdev_err(ibdev,
				  "failed to map user SQ doorbell, ret = %d.\n",
				  ret);
			goto err_out;
		}
		hr_qp->en_flags |= HIHTR_QP_CAP_SQ_RECORD_DB;
	}

	if (user_qp_has_rdb(hr_dev, init_attr, udata, resp)) {
		ret = hihtr_db_map_user(uctx, ucmd->db_addr, &hr_qp->rdb, udata);
		if (ret) {
			ibdev_err(ibdev,
				  "failed to map user RQ doorbell, ret = %d.\n",
				  ret);
			goto err_sdb;
		}
		hr_qp->en_flags |= HIHTR_QP_CAP_RQ_RECORD_DB;
	}

	return 0;

err_sdb:
	if (hr_qp->en_flags & HIHTR_QP_CAP_SQ_RECORD_DB)
		hihtr_db_unmap_user(uctx, &hr_qp->sdb);
err_out:
	return ret;
}

static int alloc_kernel_qp_db(struct hihtr_dev *hr_dev,
			      struct hihtr_qp *hr_qp,
			      struct ib_qp_init_attr *init_attr)
{
	struct ib_device *ibdev = &hr_dev->ib_dev;
	int ret;

	if (HIHTR_GET_PCI_REVISION(hr_dev) >= PCI_REVISION_ID_HIP09)
		hr_qp->sq.db_reg = hr_dev->reg_base +
				   HIHTR_DWQE_SIZE * hr_qp->qpn;
	else
		hr_qp->sq.db_reg = hr_dev->reg_base + hr_dev->sdb_offset +
				   DB_REG_OFFSET * hr_dev->priv_uar.index;

	hr_qp->rq.db_reg = hr_dev->reg_base + hr_dev->odb_offset +
			   DB_REG_OFFSET * hr_dev->priv_uar.index;

	if (kernel_qp_has_rdb(hr_dev, init_attr)) {
		ret = hihtr_alloc_db(hr_dev, &hr_qp->rdb, 0);
		if (ret) {
			ibdev_err(ibdev,
				  "failed to alloc kernel RQ doorbell, ret = %d.\n",
				  ret);
			return ret;
		}
		*hr_qp->rdb.db_record = 0;
		hr_qp->en_flags |= HIHTR_QP_CAP_RQ_RECORD_DB;
	}

	return 0;
}

static int alloc_qp_db(struct hihtr_dev *hr_dev, struct hihtr_qp *hr_qp,
		       struct ib_pd *ib_pd, struct ib_qp_init_attr *init_attr,
		       struct ib_udata *udata,
		       struct hihtr_ib_create_qp *ucmd,
		       struct hihtr_ib_create_qp_resp *resp)
{
	int ret;

	if (((hr_dev->caps.flags & HIHTR_CAP_FLAG_SDI_MODE) != 0) | (hr_qp->qpn >= hr_dev->caps.max_pi))
		hr_qp->en_flags |= HIHTR_QP_CAP_OWNER_DB;

	if (udata) {
		ret = alloc_user_qp_db(hr_dev, hr_qp, ib_pd, init_attr, udata, ucmd,
				       resp);
		if (ret)
			return ret;
	} else {
		ret = alloc_kernel_qp_db(hr_dev, hr_qp, init_attr);
		if (ret)
			return ret;
	}

	return 0;
}

static void free_qp_db(struct hihtr_dev *hr_dev, struct hihtr_qp *hr_qp,
		       struct ib_udata *udata)
{
	struct hihtr_ucontext *uctx = NULL;
#if !defined(UVERBS_ATTR_BUNDLE_HAVE_DRIVER_UDATA)
	struct ib_uobject *uobject = hr_qp->ibqp.uobject;
	if (uobject) {
		struct ib_ucontext *ib_uctx = uobject->context;
		uctx = to_hr_ucontext(ib_uctx);
	}
#elif !defined(UVERBS_IOCTL_HAVE_RDMA_UDATA_TO_DRV_CONTEXT)
	struct uverbs_attr_bundle *attr = NULL;

	if (udata) {
		attr = container_of(udata, struct uverbs_attr_bundle, driver_udata);
		uctx = to_hr_ucontext(ib_uverbs_get_ucontext(attr));
	} else if (hr_qp->ibqp.uobject) {
		uctx = to_hr_ucontext(hr_qp->ibqp.uobject->context);
	}
#else
	uctx = rdma_udata_to_drv_context(udata, struct hihtr_ucontext, ibucontext);
#endif

	if (uctx) {
		if (hr_qp->en_flags & HIHTR_QP_CAP_RQ_RECORD_DB)
			hihtr_db_unmap_user(uctx, &hr_qp->rdb);
		if (hr_qp->en_flags & HIHTR_QP_CAP_SQ_RECORD_DB)
			hihtr_db_unmap_user(uctx, &hr_qp->sdb);
	} else {
		if (hr_qp->en_flags & HIHTR_QP_CAP_RQ_RECORD_DB)
			hihtr_free_db(hr_dev, &hr_qp->rdb);
	}
}

static int alloc_kernel_wrid(struct hihtr_dev *hr_dev,
			     struct hihtr_qp *hr_qp)
{
	struct ib_device *ibdev = &hr_dev->ib_dev;
	u64 *sq_wrid = NULL;
	u64 *rq_wrid = NULL;
	u32 *sq_inv_rkey = NULL;
	int ret;

	sq_wrid = kcalloc(hr_qp->sq.wqe_cnt, sizeof(u64), GFP_KERNEL);
	if (ZERO_OR_NULL_PTR(sq_wrid)) {
		ibdev_err(ibdev, "failed to alloc SQ wrid.\n");
		return -ENOMEM;
	}

	sq_inv_rkey = kcalloc(hr_qp->sq.wqe_cnt, sizeof(u32), GFP_KERNEL);
	if (ZERO_OR_NULL_PTR(sq_inv_rkey)) {
		ibdev_err(ibdev, "failed to alloc SQ inv_rkey.\n");
		ret = -ENOMEM;
		goto err_sq;
	}

	if (hr_qp->rq.wqe_cnt) {
		rq_wrid = kcalloc(hr_qp->rq.wqe_cnt, sizeof(u64), GFP_KERNEL);
		if (ZERO_OR_NULL_PTR(rq_wrid)) {
			ibdev_err(ibdev, "failed to alloc RQ wrid.\n");
			ret = -ENOMEM;
			goto err_inv_rkey;
		}
	}

	hr_qp->sq.wrid = sq_wrid;
	hr_qp->sq.inv_rkey = sq_inv_rkey;
	hr_qp->rq.wrid = rq_wrid;

	/* Initialize deferred LOCAL_INV invalidation work */
	INIT_WORK(&hr_qp->sq.inv_work.work, hihtr_inv_work_handler);

	return 0;

err_inv_rkey:
	kfree(sq_inv_rkey);
err_sq:
	kfree(sq_wrid);

	return ret;
}

static void free_kernel_wrid(struct hihtr_qp *hr_qp)
{
	kfree(hr_qp->rq.wrid);
	kfree(hr_qp->sq.inv_rkey);
	kfree(hr_qp->sq.wrid);
}

static int set_qp_param(struct hihtr_dev *hr_dev, struct hihtr_qp *hr_qp,
			struct ib_qp_init_attr *init_attr,
			struct ib_udata *udata,
			struct hihtr_ib_create_qp *ucmd, struct ib_pd *ib_pd)
{
	struct ib_device *ibdev = &hr_dev->ib_dev;
	struct hihtr_ucontext *uctx = NULL;
	int ret;
#if !defined(UVERBS_ATTR_BUNDLE_HAVE_DRIVER_UDATA)
	struct ib_uobject *uobject = NULL;
	struct ib_ucontext *ib_uctx = NULL;
#elif !defined(UVERBS_IOCTL_HAVE_RDMA_UDATA_TO_DRV_CONTEXT)
	struct uverbs_attr_bundle *attr = NULL;
#endif

	hr_qp->ibqp.qp_type = init_attr->qp_type;
	hr_qp->mcast_flag = HIHTR_QP_MCAST_FREE;
	if (init_attr->sq_sig_type == IB_SIGNAL_ALL_WR)
		hr_qp->sq_signal_bits = IB_SIGNAL_ALL_WR;
	else
		hr_qp->sq_signal_bits = IB_SIGNAL_REQ_WR;

	ret = set_rq_size(hr_dev, &init_attr->cap, hr_qp,
			  hihtr_qp_has_rq(init_attr), !!udata);
	if (ret) {
		ibdev_err(ibdev, "failed to set user RQ size, ret = %d.\n",
			  ret);
		return ret;
	}

	if (udata) {
		ret = ib_copy_from_udata(ucmd, udata,
					 min(udata->inlen, sizeof(*ucmd)));
		if (ret) {
			ibdev_err(ibdev,
				  "failed to copy QP ucmd, ret = %d\n", ret);
			return ret;
		}
#if !defined(UVERBS_ATTR_BUNDLE_HAVE_DRIVER_UDATA)
		uobject = ib_pd->uobject;
		if (uobject) {
			ib_uctx = uobject->context;
			uctx = to_hr_ucontext(ib_uctx);
		}
#elif !defined(UVERBS_IOCTL_HAVE_RDMA_UDATA_TO_DRV_CONTEXT)
		attr = container_of(udata, struct uverbs_attr_bundle, driver_udata);
		uctx = to_hr_ucontext(ib_uverbs_get_ucontext(attr));

#else
		uctx = rdma_udata_to_drv_context(udata,
					struct hihtr_ucontext, ibucontext);
#endif
		hr_qp->config = uctx->config;
		ret = set_user_sq_size(hr_dev, &init_attr->cap, hr_qp, ucmd);
		if (ret)
			ibdev_err(ibdev, "Failed to set user SQ size, ret = %d\n",
				  ret);
	} else {
		if (init_attr->create_flags &
		    IB_QP_CREATE_BLOCK_MULTICAST_LOOPBACK) {
			ibdev_err(ibdev, "Failed to check multicast loopback\n");
			return -EINVAL;
		}

		if (init_attr->create_flags & IB_QP_CREATE_IPOIB_UD_LSO) {
			ibdev_err(ibdev, "Failed to check ipoib ud lso\n");
			return -EINVAL;
		}

		if (HIHTR_GET_PCI_REVISION(hr_dev) >= PCI_REVISION_ID_HIP09)
			hr_qp->config = HIHTR_EXSGE_FLAGS;
		ret = set_kernel_sq_size(hr_dev, &init_attr->cap, hr_qp);
		if (ret)
			ibdev_err(ibdev, "Failed to set kernel SQ size, ret = %d\n",
				  ret);
	}

	return ret;
}

static int hihtr_create_qp_common(struct hihtr_dev *hr_dev,
				     struct ib_pd *ib_pd,
				     struct ib_qp_init_attr *init_attr,
				     struct ib_udata *udata,
				     struct hihtr_qp *hr_qp)
{
	struct hihtr_ib_create_qp_resp resp = {};
	struct ib_device *ibdev = &hr_dev->ib_dev;
	struct hihtr_ib_create_qp ucmd;
	struct hihtr_cq *send_cq, *recv_cq;
	unsigned long flags;
	int ret;

	mutex_init(&hr_qp->mutex);
	spin_lock_init(&hr_qp->sq.lock);
	spin_lock_init(&hr_qp->rq.lock);
	INIT_LIST_HEAD(&hr_qp->node);
	INIT_LIST_HEAD(&hr_qp->sq_node);
	INIT_LIST_HEAD(&hr_qp->rq_node);

	hr_qp->state = IB_QPS_RESET;
	hr_qp->flush_flag = 0;
	hr_qp->dip_vld = 0; /* set default to disable */

	ret = set_qp_param(hr_dev, hr_qp, init_attr, udata, &ucmd, ib_pd);
	if (ret) {
		ibdev_err(ibdev, "failed to set QP param, ret = %d.\n", ret);
		return ret;
	}

	if (!udata) {
		ret = alloc_kernel_wrid(hr_dev, hr_qp);
		if (ret) {
			ibdev_err(ibdev, "failed to alloc wrid, ret = %d.\n",
				  ret);
			return ret;
		}
	}

	ret = alloc_qpn(hr_dev, hr_qp);
	if (ret) {
		ibdev_err(ibdev, "failed to alloc QPN, ret = %d.\n", ret);
		goto err_qpn;
	}

	ret = alloc_qp_wqe(hr_dev, hr_qp, ib_pd, init_attr, udata, ucmd.buf_addr);
	if (ret) {
		ibdev_err(ibdev, "failed to alloc QP buffer, ret = %d.\n", ret);
		goto err_buf;
	}

	ret = alloc_qp_db(hr_dev, hr_qp, ib_pd, init_attr, udata, &ucmd, &resp);
	if (ret) {
		ibdev_err(ibdev, "failed to alloc QP doorbell, ret = %d.\n",
			  ret);
		goto err_db;
	}

	ret = alloc_qpc(hr_dev, hr_qp);
	if (ret) {
		ibdev_err(ibdev, "failed to alloc QP context, ret = %d.\n",
			  ret);
		goto err_qpc;
	}

	ret = hihtr_qp_store(hr_dev, hr_qp, init_attr);
	if (ret) {
		ibdev_err(ibdev, "failed to store QP, ret = %d.\n", ret);
		goto err_store;
	}

	if (udata) {
		resp.cap_flags = hr_qp->en_flags;
		ret = ib_copy_to_udata(udata, &resp,
				       min(udata->outlen, sizeof(resp)));
		if (ret) {
			ibdev_err(ibdev, "copy qp resp failed!\n");
			goto err_store;
		}
	}

	ret = hr_dev->hw->qp_flow_control_init(hr_dev, hr_qp);
	if (ret)
		goto err_flow_ctrl;

	hr_qp->ibqp.qp_num = hr_qp->qpn;
	hr_qp->event = hihtr_ib_qp_event;
	refcount_set(&hr_qp->refcount, 1);
	init_completion(&hr_qp->free);

    mutex_lock(&hr_dev->qp_cnt.cur_qps_mutex);
    hr_dev->qp_cnt.alloc_qp_cnt++;
    mutex_unlock(&hr_dev->qp_cnt.cur_qps_mutex);

	return 0;

err_flow_ctrl:
err_store:
	/*
	 * QP was added to the list/xarray by hihtr_qp_store().
	 * Must remove it under qp_list_lock + CQ locks, matching
	 * the locking pattern in hihtr_v3_destroy_qp_common().
	 * Both err_flow_ctrl and ib_copy_to_udata failure reach here
	 * with QP still in the list - we must remove it before cleanup.
	 */
	send_cq = hr_qp->ibqp.send_cq ? to_hr_cq(hr_qp->ibqp.send_cq) : NULL;
	recv_cq = hr_qp->ibqp.recv_cq ? to_hr_cq(hr_qp->ibqp.recv_cq) : NULL;

	spin_lock_irqsave(&hr_dev->qp_list_lock, flags);
	hihtr_lock_cqs(send_cq, recv_cq);

	hihtr_qp_remove(hr_dev, hr_qp);

	hihtr_unlock_cqs(send_cq, recv_cq);
	spin_unlock_irqrestore(&hr_dev->qp_list_lock, flags);

	free_qpc(hr_dev, hr_qp);
err_qpc:
	free_qp_db(hr_dev, hr_qp, udata);
err_db:
	free_qp_wqe(hr_dev, hr_qp, udata);
err_buf:
	free_qpn(hr_dev, hr_qp);
err_qpn:
	free_kernel_wrid(hr_qp);
	return ret;
}

int free_dip_entry(struct hihtr_dev *hr_dev, u32 dip_idx)
{
	// dip_idx有规格限制，不超int范围
	struct hihtr_dip *hr_dip;
	unsigned long flags;

#if defined(RDMA_HAVE_XARRAY)
	hr_dip = xa_load(&hr_dev->qp_table.dip_table.dip_xa, dip_idx);
#else
	hr_dip = idr_find(&hr_dev->qp_table.dip_table.dip_idr, dip_idx);
#endif
	if (!hr_dip) {
		return -ENOENT;
	}

	spin_lock_irqsave(&hr_dev->dip_list_lock, flags);
	if (refcount_dec_and_test(&hr_dip->refcount)) {
		list_del(&hr_dip->node);
#if defined(RDMA_HAVE_XARRAY)
		xa_erase(&hr_dev->qp_table.dip_table.dip_xa, dip_idx);
#else
		idr_remove(&hr_dev->qp_table.dip_table.dip_idr, dip_idx);
#endif
		kfree(hr_dip);
	}

	spin_unlock_irqrestore(&hr_dev->dip_list_lock, flags);
	return 0;
}

void hihtr_qp_destroy(struct hihtr_dev *hr_dev, struct hihtr_qp *hr_qp,
			 struct ib_udata *udata)
{
	if (refcount_dec_and_test(&hr_qp->refcount))
		complete(&hr_qp->free);
	wait_for_completion(&hr_qp->free);

	if (hr_qp->sq.inv_work.work.func != NULL) {
		smp_store_release(&hr_qp->sq.inv_work.done, true);
		flush_work(&hr_qp->sq.inv_work.work);
	}

	// 释放DIP引用
	if (hr_qp->dip_vld) {
		free_dip_entry(hr_dev, hr_qp->dip_idx);
	}

	free_qpc(hr_dev, hr_qp);
	free_qpn(hr_dev, hr_qp);
	free_qp_wqe(hr_dev, hr_qp, udata);
	free_kernel_wrid(hr_qp);
	free_qp_db(hr_dev, hr_qp, udata);

    mutex_lock(&hr_dev->qp_cnt.cur_qps_mutex);
    hr_dev->qp_cnt.del_qp_cnt++;
    mutex_unlock(&hr_dev->qp_cnt.cur_qps_mutex);
// should not free hr_qp in kernel 6.6, because ofed will handle memory management
#if defined(ROCE_CREATE_QP_IB_QP)
	kfree(hr_qp);
#endif
}

static int check_qp_type(struct hihtr_dev *hr_dev, enum ib_qp_type type,
			 bool is_user)
{
	switch (type) {
	case IB_QPT_XRC_INI:
	case IB_QPT_XRC_TGT:
		if (!(hr_dev->caps.flags & HIHTR_CAP_FLAG_XRC))
			goto out;
		break;
	case IB_QPT_UD:
		if (HIHTR_GET_PCI_REVISION(hr_dev) == PCI_REVISION_ID_HIP08 &&
		    is_user)
			goto out;
		break;
	case IB_QPT_RC:
	case IB_QPT_GSI:
		break;
	default:
		goto out;
	}

	return 0;

out:
	ibdev_err(&hr_dev->ib_dev, "not support QP type %d\n", type);

	return -EOPNOTSUPP;
}

#if defined(ROCE_CREATE_QP_INT)
int hihtr_create_qp(struct ib_qp *ib_qp, struct ib_qp_init_attr *init_attr, struct ib_udata *udata)
{
	struct ib_pd *pd = ib_qp->pd;
	struct ib_device *ibdev = pd ? pd->device : init_attr->xrcd->device;
	struct hihtr_dev *hr_dev = to_hr_dev(ibdev);
	struct hihtr_qp *hr_qp;
	int ret;

	if (!ib_qp) {
    	ret = -EINVAL;
    	goto err_out;
	}

	ret = check_qp_type(hr_dev, init_attr->qp_type, !!udata);
	if (ret)
		goto err_out;

	hr_qp = container_of(ib_qp, struct hihtr_qp, ibqp);

	if (init_attr->qp_type == IB_QPT_XRC_TGT)
		hr_qp->xrcdn = to_hr_xrcd(init_attr->xrcd)->xrcdn;

	if (init_attr->qp_type == IB_QPT_GSI) {
		hr_qp->port = init_attr->port_num - 1;
		hr_qp->phy_port = hr_dev->iboe.phy_port[hr_qp->port];
	}

	ret = hihtr_create_qp_common(hr_dev, pd, init_attr, udata, hr_qp);
	if (ret) {
		ibdev_err(ibdev, "Create QP type 0x%x failed(%d)\n",
			  init_attr->qp_type, ret);
		/*
		 * In ROCE_CREATE_QP_INT path, hr_qp is embedded in ib_qp
		 * via container_of(). ib_qp memory is owned by RDMA core.
		 * Do NOT kfree(hr_qp) here - it would double-free the
		 * RDMA core's ib_qp allocation, causing use-after-free
		 * and kernel list corruption.
		 */
	}
err_out:
	if (ret)
		atomic64_inc(&hr_dev->dfx_cnt[HIHTR_DFX_QP_CREATE_ERR_CNT]);

    return ret;
}
#else
struct ib_qp *hihtr_create_qp(struct ib_pd *pd, struct ib_qp_init_attr *init_attr, struct ib_udata *udata)
{
	struct ib_device *ibdev = pd ? pd->device : init_attr->xrcd->device;
	struct hihtr_dev *hr_dev = to_hr_dev(ibdev);
	struct hihtr_qp *hr_qp;
	int ret;

	ret = check_qp_type(hr_dev, init_attr->qp_type, !!udata);
	if (ret)
        goto err_out;

	hr_qp = kzalloc(sizeof(*hr_qp), GFP_KERNEL);
    if (!hr_qp) {
        ret = -ENOMEM;
        goto err_out;
	}

	hr_qp->ibqp.pd = pd;

	if (init_attr->qp_type == IB_QPT_XRC_TGT)
		hr_qp->xrcdn = to_hr_xrcd(init_attr->xrcd)->xrcdn;

	if (init_attr->qp_type == IB_QPT_GSI) {
		hr_qp->port = init_attr->port_num - 1;
		hr_qp->phy_port = hr_dev->iboe.phy_port[hr_qp->port];
	}

	ret = hihtr_create_qp_common(hr_dev, pd, init_attr, udata, hr_qp);
	if (ret) {
		ibdev_err(ibdev, "Create QP type 0x%x failed(%d)\n",
			  init_attr->qp_type, ret);

		kfree(hr_qp);
		goto err_out;
	}
	return &hr_qp->ibqp;
err_out:
	atomic64_inc(&hr_dev->dfx_cnt[HIHTR_DFX_QP_CREATE_ERR_CNT]);

    return ERR_PTR(ret);
}
#endif

int to_hr_qp_type(int qp_type)
{
	switch (qp_type) {
	case IB_QPT_RC:
		return SERV_TYPE_RC;
	case IB_QPT_UD:
	case IB_QPT_GSI:
		return SERV_TYPE_UD;
	case IB_QPT_XRC_INI:
	case IB_QPT_XRC_TGT:
		return SERV_TYPE_XRC;
	default:
		return -1;
	}
}

static int check_mtu_validate(struct hihtr_dev *hr_dev,
			      struct hihtr_qp *hr_qp,
			      struct ib_qp_attr *attr, int attr_mask)
{
	enum ib_mtu active_mtu;
	int p;

	p = attr_mask & IB_QP_PORT ? (attr->port_num - 1) : hr_qp->port;
	active_mtu = iboe_get_mtu(hr_dev->iboe.netdevs[p]->mtu);
	if ((hr_dev->caps.max_mtu >= IB_MTU_2048 &&
	    attr->path_mtu > hr_dev->caps.max_mtu) ||
	    attr->path_mtu < IB_MTU_256 || attr->path_mtu > active_mtu) {
		ibdev_err(&hr_dev->ib_dev,
			"attr path_mtu(%d)invalid while modify qp",
			attr->path_mtu);
		return -EINVAL;
	}

	return 0;
}

static int hihtr_check_qp_attr(struct ib_qp *ibqp, struct ib_qp_attr *attr,
				  int attr_mask)
{
	struct hihtr_dev *hr_dev = to_hr_dev(ibqp->device);
	struct hihtr_qp *hr_qp = to_hr_qp(ibqp);
	int p;

	if ((attr_mask & IB_QP_PORT) &&
	    (attr->port_num == 0 || attr->port_num > hr_dev->caps.num_ports)) {
		ibdev_err(&hr_dev->ib_dev, "invalid attr, port_num = %u.\n",
			  attr->port_num);
		return -EINVAL;
	}

	if (attr_mask & IB_QP_PKEY_INDEX) {
		p = attr_mask & IB_QP_PORT ? (attr->port_num - 1) : hr_qp->port;
		if (attr->pkey_index >= hr_dev->caps.pkey_table_len[p]) {
			ibdev_err(&hr_dev->ib_dev,
				  "invalid attr, pkey_index = %u.\n",
				  attr->pkey_index);
			return -EINVAL;
		}
	}

	if ((attr_mask & IB_QP_MAX_QP_RD_ATOMIC) &&
	    (attr->max_rd_atomic > hr_dev->caps.max_qp_init_rdma)) {
		ibdev_err(&hr_dev->ib_dev,
			  "invalid attr, max_rd_atomic = %u.\n",
			  attr->max_rd_atomic);
		return -EINVAL;
	}

	if ((attr_mask & IB_QP_MAX_DEST_RD_ATOMIC) &&
	    (attr->max_dest_rd_atomic > hr_dev->caps.max_qp_dest_rdma)) {
		ibdev_err(&hr_dev->ib_dev,
			  "invalid attr, max_dest_rd_atomic = %u.\n",
			  attr->max_dest_rd_atomic);
		return -EINVAL;
	}

	if (attr_mask & IB_QP_PATH_MTU)
		return check_mtu_validate(hr_dev, hr_qp, attr, attr_mask);

	return 0;
}

static int check_modify_qp(struct ib_qp *ibqp, struct ib_qp_attr *attr,
			   int attr_mask, enum ib_qp_state cur_state,
			   enum ib_qp_state new_state)
{
	struct hihtr_qp *hr_qp = to_hr_qp(ibqp);
	int ret;

	if ((attr_mask & IB_QP_CUR_STATE) && (attr->cur_qp_state != hr_qp->state)) {
		ibdev_err(ibqp->device, "failed to check modify curr state\n");
		return -EINVAL;
	}

	if (ibqp->uobject &&
	    (attr_mask & IB_QP_STATE) && new_state == IB_QPS_ERR) {
		if (hr_qp->en_flags & HIHTR_QP_CAP_SQ_RECORD_DB) {
			hr_qp->sq.head = *(int *)(hr_qp->sdb.virt_addr);

			if (hr_qp->en_flags & HIHTR_QP_CAP_RQ_RECORD_DB)
				hr_qp->rq.head = *(int *)(hr_qp->rdb.virt_addr);
		} else {
			ibdev_warn(ibqp->device,
				  "flush cqe is not supported in userspace!\n");
			return -EINVAL;
		}
	}
#ifdef IB_MODIFY_QP_IS_OK_HAVE_RDMA_LINK_LAYER
	if (!ib_modify_qp_is_ok(cur_state, new_state, ibqp->qp_type,
				attr_mask, 0)) {
#else
	if (!ib_modify_qp_is_ok(cur_state, new_state, ibqp->qp_type,
				attr_mask)) {
#endif
		ibdev_err(ibqp->device, "failed to check modify qp state\n");
		return -EINVAL;
	}

	ret = hihtr_check_qp_attr(ibqp, attr, attr_mask);
	if (ret) {
		ibdev_err(ibqp->device, "failed to check modify qp attr\n");
		return ret;
	}

	return 0;
}

int hihtr_modify_qp(struct ib_qp *ibqp, struct ib_qp_attr *attr,
		       int attr_mask, struct ib_udata *udata)
{
	struct hihtr_dev *hr_dev = to_hr_dev(ibqp->device);
	struct hihtr_qp *hr_qp = to_hr_qp(ibqp);
	struct hihtr_ib_modify_qp_resp resp = {};
	enum ib_qp_state cur_state, new_state;
	int ret;

	mutex_lock(&hr_qp->mutex);

	cur_state = hr_qp->state;
	new_state = attr_mask & IB_QP_STATE ? attr->qp_state : cur_state;

	ret = check_modify_qp(ibqp, attr, attr_mask, cur_state, new_state);
	if (ret)
		goto out;

	if (cur_state == new_state && cur_state == IB_QPS_RESET)
		goto out;

	ret = hr_dev->hw->modify_qp(ibqp, attr, attr_mask, cur_state,
				    new_state, udata);
	if (ret)
		goto out;

	if (udata && udata->outlen) {
		resp.priority = hr_qp->priority;
		resp.dcan = hr_qp->dca_cfg.dcan;
		ret = ib_copy_to_udata(udata, &resp,
				       min(udata->outlen, sizeof(resp)));
		if (ret)
			ibdev_err(&hr_dev->ib_dev,
				  "failed to copy modify qp resp.\n");
	}

out:
	if (ret)
		atomic64_inc(&hr_dev->dfx_cnt[HIHTR_DFX_QP_MODIFY_ERR_CNT]);

	mutex_unlock(&hr_qp->mutex);
	return ret;
}

void hihtr_lock_cqs(struct hihtr_cq *send_cq, struct hihtr_cq *recv_cq)
		       __acquires(&send_cq->lock) __acquires(&recv_cq->lock)
{
	if (unlikely(send_cq == NULL && recv_cq == NULL)) {
		__acquire(&send_cq->lock);
		__acquire(&recv_cq->lock);
	} else if (unlikely(send_cq != NULL && recv_cq == NULL)) {
		spin_lock_irq(&send_cq->lock);
		__acquire(&recv_cq->lock);
	} else if (unlikely(send_cq == NULL && recv_cq != NULL)) {
		spin_lock_irq(&recv_cq->lock);
		__acquire(&send_cq->lock);
	} else if (send_cq == recv_cq) {
		spin_lock_irq(&send_cq->lock);
		__acquire(&recv_cq->lock);
	} else if (send_cq->cqn < recv_cq->cqn) {
		spin_lock_irq(&send_cq->lock);
		spin_lock_nested(&recv_cq->lock, SINGLE_DEPTH_NESTING);
	} else {
		spin_lock_irq(&recv_cq->lock);
		spin_lock_nested(&send_cq->lock, SINGLE_DEPTH_NESTING);
	}
}

void hihtr_unlock_cqs(struct hihtr_cq *send_cq,
			 struct hihtr_cq *recv_cq) __releases(&send_cq->lock)
			 __releases(&recv_cq->lock)
{
	if (unlikely(send_cq == NULL && recv_cq == NULL)) {
		__release(&recv_cq->lock);
		__release(&send_cq->lock);
	} else if (unlikely(send_cq != NULL && recv_cq == NULL)) {
		__release(&recv_cq->lock);
		spin_unlock(&send_cq->lock);
	} else if (unlikely(send_cq == NULL && recv_cq != NULL)) {
		__release(&send_cq->lock);
		spin_unlock(&recv_cq->lock);
	} else if (send_cq == recv_cq) {
		__release(&recv_cq->lock);
		spin_unlock_irq(&send_cq->lock);
	} else if (send_cq->cqn < recv_cq->cqn) {
		spin_unlock(&recv_cq->lock);
		spin_unlock_irq(&send_cq->lock);
	} else {
		spin_unlock(&send_cq->lock);
		spin_unlock_irq(&recv_cq->lock);
	}
}

static inline void *dca_buf_offset(struct hihtr_dca_cfg *dca_cfg, u32 offset)
{
	return (char *)(dca_cfg->buf_list[offset >> HIHTR_HW_PAGE_SHIFT]) +
			(offset & ((1 << HIHTR_HW_PAGE_SHIFT) - 1));
}

static inline void *get_wqe(struct hihtr_qp *hr_qp, u32 offset)
{
	if (unlikely(hr_qp->dca_cfg.buf_list))
		return dca_buf_offset(&hr_qp->dca_cfg, offset);
	else
		return hihtr_buf_offset(hr_qp->mtr.kmem, offset);
}

void *hihtr_get_recv_wqe(struct hihtr_qp *hr_qp, unsigned int n)
{
	return get_wqe(hr_qp, hr_qp->rq.offset + (n << hr_qp->rq.wqe_shift));
}

void *hihtr_get_send_wqe(struct hihtr_qp *hr_qp, unsigned int n)
{
	return get_wqe(hr_qp, hr_qp->sq.offset + (n << hr_qp->sq.wqe_shift));
}

void *hihtr_get_extend_sge(struct hihtr_qp *hr_qp, unsigned int n)
{
	return get_wqe(hr_qp, hr_qp->sge.offset + (n << hr_qp->sge.sge_shift));
}

bool hihtr_wq_overflow(struct hihtr_wq *hr_wq, u32 nreq,
			  struct ib_cq *ib_cq)
{
	struct hihtr_cq *hr_cq;
	u32 cur;

	cur = hr_wq->head - hr_wq->tail;
	if (likely(cur + nreq < hr_wq->wqe_cnt))
		return false;

	hr_cq = to_hr_cq(ib_cq);
	spin_lock(&hr_cq->lock);
	cur = hr_wq->head - hr_wq->tail;
	spin_unlock(&hr_cq->lock);

	return cur + nreq >= hr_wq->wqe_cnt;
}

int hihtr_init_qp_table(struct hihtr_dev *hr_dev)
{
	struct hihtr_qp_table *qp_table = &hr_dev->qp_table;
	struct hihtr_ida *qp_ida = &qp_table->qp_ida;

	mutex_init(&qp_table->scc_mutex);
	mutex_init(&qp_table->ida_mutex);
#if defined(RDMA_HAVE_XARRAY)
	xa_init(&hr_dev->qp_table_xa);
#else
	INIT_RADIX_TREE(&hr_dev->qp_table_root, GFP_KERNEL);
	spin_lock_init(&hr_dev->tree_lock);
#endif

#if defined(RDMA_HAVE_XARRAY)
	xa_init_flags(&qp_table->dip_table.dip_xa, XA_FLAGS_ALLOC);
#else
	idr_init(&qp_table->dip_table.dip_idr);
#endif
	qp_table->dip_table.next_dip_idx = hr_dev->caps.reserved_qps;
	ida_init(&hr_dev->qp_table.qp_ida.ida);
	qp_ida->min = hr_dev->caps.reserved_qps;
	qp_ida->max = hr_dev->caps.num_qps - HIHTR_FAKE_DB_NUM - 1; /* 从0开始计算，所以额外减去1 */

	return 0;
}

void hihtr_cleanup_qp_table(struct hihtr_dev *hr_dev)
{
	unsigned long flags;
	struct hihtr_dip *dip_entry;
#if defined(RDMA_HAVE_XARRAY)
	unsigned long i;
#else
	int i;
#endif

	ida_destroy(&hr_dev->qp_table.qp_ida.ida);
	spin_lock_irqsave(&hr_dev->dip_list_lock, flags);
	// dip entry 资源通过DIP list进行释放，此处仅释放xarray/idr数据
#if defined(RDMA_HAVE_XARRAY)
	xa_for_each(&hr_dev->qp_table.dip_table.dip_xa, i, dip_entry) {
		xa_erase(&hr_dev->qp_table.dip_table.dip_xa, i);
	}
	xa_destroy(&hr_dev->qp_table.dip_table.dip_xa);
#else
	idr_for_each_entry(&hr_dev->qp_table.dip_table.dip_idr, dip_entry, i) {
		idr_remove(&hr_dev->qp_table.dip_table.dip_idr, i);
	}
	idr_destroy(&hr_dev->qp_table.dip_table.dip_idr);
#endif
	spin_unlock_irqrestore(&hr_dev->dip_list_lock, flags);
}