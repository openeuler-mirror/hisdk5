// SPDX-License-Identifier: GPL-2.0 OR Linux-OpenIB
/*
 * Copyright (c) 2018 Hisilicon Limited.
 */

#include <linux/pci.h>
#include <rdma/ib_umem.h>
#include <rdma/uverbs_ioctl.h>
#include "hihtr_device.h"
#include "hihtr_cmd.h"
#include "hihtr_cmdq.h"
#include "hihtr_hem.h"
#include "dfx/hihtr_dfx.h"

void hihtr_srq_event(struct hihtr_dev *hr_dev, u32 srqn, int event_type)
{
	struct hihtr_srq_table *srq_table = &hr_dev->srq_table;
	struct hihtr_srq *srq;
#if !defined(RDMA_HAVE_XARRAY)
	unsigned long flags;
#endif

#if defined(RDMA_HAVE_XARRAY)
	xa_lock(&srq_table->xa);
	srq = xa_load(&srq_table->xa, srqn & (hr_dev->caps.num_srqs - 1));
	if (srq)
		refcount_inc(&srq->refcount);
	xa_unlock(&srq_table->xa);
#else
	spin_lock_irqsave(&srq_table->tree_lock, flags);
	srq = radix_tree_lookup(&srq_table->root, srqn & (hr_dev->caps.num_srqs - 1));
	if (srq)
		refcount_inc(&srq->refcount);
	spin_unlock_irqrestore(&srq_table->tree_lock, flags);
#endif

	if (!srq) {
		dev_warn(hr_dev->dev, "Async event for bogus SRQ %08x\n", srqn);
		return;
	}

	srq->event(srq, event_type);

	if (refcount_dec_and_test(&srq->refcount))
		complete(&srq->free);
}

static void hihtr_ib_srq_event(struct hihtr_srq *srq,
				  enum hihtr_event event_type)
{
	struct hihtr_dev *hr_dev = to_hr_dev(srq->ibsrq.device);
	struct ib_srq *ibsrq = &srq->ibsrq;
	struct ib_event event;

	switch (event_type) {
	case HIHTR_EVENT_TYPE_SRQ_LIMIT_REACH:
		event.event = IB_EVENT_SRQ_LIMIT_REACHED;
		break;
	case HIHTR_EVENT_TYPE_SRQ_CATAS_ERROR:
		event.event = IB_EVENT_SRQ_ERR;
		break;
	default:
		dev_err(hr_dev->dev,
			"hihtr_roce:Unexpected event type 0x%x on SRQ %06lx\n",
			event_type, srq->srqn);
		return;
	}

	hihtr_dfx_record_err_event_count(hr_dev, event.event);
	if (ibsrq->event_handler) {
		event.device      = ibsrq->device;
		event.element.srq = ibsrq;
		ibsrq->event_handler(&event, ibsrq->srq_context);
	}
}

static int alloc_srqn(struct hihtr_dev *hr_dev, struct hihtr_srq *srq)
{
	struct hihtr_ida *srq_ida = &hr_dev->srq_table.srq_ida;
	int id;

	id = ida_alloc_range(&srq_ida->ida, srq_ida->min, srq_ida->max,
			     GFP_KERNEL);
	if (id < 0) {
		ibdev_err(&hr_dev->ib_dev, "failed to alloc srq(%d).\n", id);
		return -ENOMEM;
	}

	srq->srqn = id;

	return 0;
}

static void free_srqn(struct hihtr_dev *hr_dev, struct hihtr_srq *srq)
{
	ida_free(&hr_dev->srq_table.srq_ida.ida, (int)srq->srqn);
}

static int hihtr_create_srqc(struct hihtr_dev *hr_dev,
				struct hihtr_srq *srq)
{
	struct ib_device *ibdev = &hr_dev->ib_dev;
	struct hihtr_cmd_mailbox *mailbox;
	int ret;

	mailbox = hihtr_alloc_cmd_mailbox(hr_dev);
	if (IS_ERR(mailbox)) {
		ibdev_err(ibdev, "failed to alloc mailbox for SRQC.\n");
		return PTR_ERR(mailbox);
	}

	ret = hr_dev->hw->write_srqc(srq, mailbox->buf);
	if (ret) {
		ibdev_err(ibdev, "failed to write SRQC.\n");
		goto err_mbox;
	}

	ret = hihtr_cmq_mbox(hr_dev, mailbox->dma, 0, HIHTR_CMD_CREATE_SRQ,
				     srq->srqn);
	if (ret)
		ibdev_err(ibdev, "failed to config SRQC, ret = %d.\n", ret);

err_mbox:
	hihtr_free_cmd_mailbox(hr_dev, mailbox);
	return ret;
}

static int alloc_srqc(struct hihtr_dev *hr_dev, struct hihtr_srq *srq)
{
	struct hihtr_srq_table *srq_table = &hr_dev->srq_table;
	struct ib_device *ibdev = &hr_dev->ib_dev;
	int ret;

	ret = hihtr_table_get(hr_dev, &srq_table->table, srq->srqn);
	if (ret) {
		ibdev_err(ibdev, "failed to get SRQC table, ret = %d.\n", ret);
		return ret;
	}

#if defined(RDMA_HAVE_XARRAY)
	ret = xa_err(xa_store(&srq_table->xa, srq->srqn, srq, GFP_KERNEL));
#else
	ret = radix_tree_preload(GFP_KERNEL);
	if (ret) {
		ibdev_err(ibdev, "failed to preload SRQC root, ret = %d.\n", ret);
		goto err_out;
	}
	ret = radix_tree_insert(&srq_table->root, srq->srqn, srq);
	radix_tree_preload_end();
#endif
	if (ret) {
		ibdev_err(ibdev, "failed to store SRQC, ret = %d.\n", ret);
		goto err_out;
	}

	ret = hihtr_create_srqc(hr_dev, srq);
	if (ret)
		goto err_xa;

	return 0;

err_xa:
#if defined(RDMA_HAVE_XARRAY)
	xa_erase(&srq_table->xa, srq->srqn);
#else
	radix_tree_delete(&srq_table->root, srq->srqn);
#endif
err_out:
	return ret;
}

static void free_srqc(struct hihtr_dev *hr_dev, struct hihtr_srq *srq)
{
	struct hihtr_srq_table *srq_table = &hr_dev->srq_table;
	int ret;

	ret = hihtr_cmq_mbox(hr_dev, 0, 0, HIHTR_CMD_DESTROY_SRQ,
				      srq->srqn);
	if (ret)
		dev_err(hr_dev->dev, "DESTROY_SRQ failed (%d) for SRQN %06lx\n",
			ret, srq->srqn);

#if defined(RDMA_HAVE_XARRAY)
	xa_erase(&srq_table->xa, srq->srqn);
#else
	radix_tree_delete(&srq_table->root, srq->srqn);
#endif

	if (refcount_dec_and_test(&srq->refcount))
		complete(&srq->free);
	wait_for_completion(&srq->free);
}

static int alloc_srq_idx(struct hihtr_dev *hr_dev, struct hihtr_srq *srq,
			 struct ib_udata *udata, unsigned long addr)
{
	struct hihtr_idx_que *idx_que = &srq->idx_que;
	struct ib_device *ibdev = &hr_dev->ib_dev;
	struct hihtr_buf_attr buf_attr = {};
	int ret;

	srq->idx_que.entry_shift = ilog2(HIHTR_IDX_QUE_ENTRY_SZ);

	buf_attr.page_shift = hr_dev->caps.idx_buf_pg_sz + PAGE_SHIFT;
	buf_attr.region[0].size = to_hr_hem_entries_size(srq->wqe_cnt,
					srq->idx_que.entry_shift);
	buf_attr.region[0].hopnum = hr_dev->caps.idx_hop_num;
	buf_attr.region_count = 1;

	ret = hihtr_mtr_create(hr_dev, &idx_que->mtr, &buf_attr,
				  hr_dev->caps.idx_ba_pg_sz + PAGE_SHIFT,
				  udata, addr, (struct ib_uobject *)srq->ibsrq.uobject);
	if (ret) {
		ibdev_err(ibdev, "Failed to alloc SRQ idx mtr, ret = %d.\n", ret);
		return ret;
	}

	if (!udata) {
		idx_que->bitmap = bitmap_zalloc(srq->wqe_cnt, GFP_KERNEL);
		if (!idx_que->bitmap) {
			ibdev_err(ibdev, "failed to alloc SRQ idx bitmap.\n");
			ret = -ENOMEM;
			goto err_idx_mtr;
		}
	}

	idx_que->head = 0;
	idx_que->tail = 0;

	return 0;
err_idx_mtr:
	hihtr_mtr_destroy(hr_dev, &idx_que->mtr);

	return ret;
}

static void free_srq_idx(struct hihtr_dev *hr_dev, struct hihtr_srq *srq)
{
	struct hihtr_idx_que *idx_que = &srq->idx_que;

	bitmap_free(idx_que->bitmap);
	idx_que->bitmap = NULL;
	hihtr_mtr_destroy(hr_dev, &idx_que->mtr);
}

static int alloc_srq_wqe_buf(struct hihtr_dev *hr_dev,
			     struct hihtr_srq *srq,
			     struct ib_udata *udata, unsigned long addr)
{
	struct ib_device *ibdev = &hr_dev->ib_dev;
	struct hihtr_buf_attr buf_attr = {};
	int ret;

	srq->wqe_shift = ilog2(roundup_pow_of_two(max(HIHTR_SGE_SIZE,
						      HIHTR_SGE_SIZE *
						      srq->max_gs)));

	buf_attr.page_shift = hr_dev->caps.srqwqe_buf_pg_sz + PAGE_SHIFT;
	buf_attr.region[0].size = to_hr_hem_entries_size(srq->wqe_cnt,
							 srq->wqe_shift);
	buf_attr.region[0].hopnum = hr_dev->caps.srqwqe_hop_num;
	buf_attr.region_count = 1;

	ret = hihtr_mtr_create(hr_dev, &srq->buf_mtr, &buf_attr,
				  hr_dev->caps.srqwqe_ba_pg_sz + PAGE_SHIFT,
				  udata, addr, (struct ib_uobject *)srq->ibsrq.uobject);
	if (ret)
		ibdev_err(ibdev,
			  "failed to alloc SRQ buf mtr, ret = %d.\n", ret);

	return ret;
}

static void free_srq_wqe_buf(struct hihtr_dev *hr_dev,
			     struct hihtr_srq *srq)
{
	hihtr_mtr_destroy(hr_dev, &srq->buf_mtr);
}

static int alloc_srq_wrid(struct hihtr_dev *hr_dev, struct hihtr_srq *srq)
{
	srq->wrid = kvmalloc_array(srq->wqe_cnt, sizeof(u64), GFP_KERNEL);
	if (!srq->wrid)
		return -ENOMEM;

	return 0;
}

static void free_srq_wrid(struct hihtr_srq *srq)
{
	kvfree(srq->wrid);
	srq->wrid = NULL;
}

static u32 proc_srq_sge(struct hihtr_dev *dev, struct hihtr_srq *hr_srq,
			bool user)
{
	u32 max_sge = dev->caps.max_srq_sges;

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
		hr_srq->rsv_sge = 1;

	return max_sge;
}

static int set_srq_basic_param(struct hihtr_srq *srq,
			       struct ib_srq_init_attr *init_attr,
			       struct ib_udata *udata)

{
	struct hihtr_dev *hr_dev = to_hr_dev(srq->ibsrq.device);
	struct ib_srq_attr *attr = &init_attr->attr;
	u32 max_sge;

	max_sge = proc_srq_sge(hr_dev, srq, !!udata);
	if (init_attr->attr.max_wr > hr_dev->caps.max_srq_wrs ||
	    init_attr->attr.max_sge > max_sge) {
		ibdev_err(&hr_dev->ib_dev,
			  "invalid SRQ attr, depth = %u, sge = %u.\n",
			   attr->max_wr, attr->max_sge);
		return -EINVAL;
	}

	attr->max_wr = max_t(u32, attr->max_wr, HIHTR_MIN_SRQ_WQE_NUM);
	srq->wqe_cnt = roundup_pow_of_two(attr->max_wr);
	srq->max_gs = roundup_pow_of_two(attr->max_sge + srq->rsv_sge);

	attr->max_wr = srq->wqe_cnt;
	attr->max_sge = srq->max_gs - srq->rsv_sge;
	attr->srq_limit = 0;

	return 0;
}

static void set_srq_ext_param(struct hihtr_srq *srq,
			      struct ib_srq_init_attr *init_attr)
{
	srq->cqn = ib_srq_has_cq(init_attr->srq_type) ?
		to_hr_cq(init_attr->ext.cq)->cqn : 0;
	srq->xrcdn = (init_attr->srq_type == IB_SRQT_XRC) ?
		to_hr_xrcd(init_attr->ext.xrc.xrcd)->xrcdn : 0;
}

static int set_srq_param(struct hihtr_srq *srq,
			 struct ib_srq_init_attr *init_attr,
			 struct ib_udata *udata)
{
	int ret;

	ret = set_srq_basic_param(srq, init_attr, udata);
	if (ret)
		return ret;

	set_srq_ext_param(srq, init_attr);

	return 0;
}

static int alloc_srq_buf(struct hihtr_dev *hr_dev, struct hihtr_srq *srq,
			 struct ib_udata *udata)
{
	struct hihtr_ib_create_srq ucmd = {};
	int ret;

	if (udata) {
		ret = ib_copy_from_udata(&ucmd, udata,
					 min(udata->inlen, sizeof(ucmd)));
		if (ret) {
			ibdev_err(&hr_dev->ib_dev,
				  "failed to copy SRQ udata, ret = %d.\n",
				  ret);
			return ret;
		}
	}

	ret = alloc_srq_idx(hr_dev, srq, udata, ucmd.que_addr);
	if (ret)
		return ret;

	ret = alloc_srq_wqe_buf(hr_dev, srq, udata, ucmd.buf_addr);
	if (ret)
		goto err_idx;

	if (!udata) {
		ret = alloc_srq_wrid(hr_dev, srq);
		if (ret)
			goto err_wqe_buf;
	}

	return 0;

err_wqe_buf:
	free_srq_wqe_buf(hr_dev, srq);
err_idx:
	free_srq_idx(hr_dev, srq);

	return ret;
}

static void free_srq_buf(struct hihtr_dev *hr_dev, struct hihtr_srq *srq)
{
	free_srq_wrid(srq);
	free_srq_wqe_buf(hr_dev, srq);
	free_srq_idx(hr_dev, srq);
}

static int get_srq_ucmd(struct hihtr_srq *srq, struct ib_udata *udata,
			struct hihtr_ib_create_srq *ucmd)
{
	struct ib_device *ibdev = srq->ibsrq.device;
	int ret;

	ret = ib_copy_from_udata(ucmd, udata, min(udata->inlen, sizeof(*ucmd)));
	if (ret) {
		ibdev_err(ibdev, "failed to copy SRQ udata, ret = %d.\n", ret);
		return ret;
	}

	return 0;
}

static void free_srq_db(struct hihtr_dev *hr_dev, struct hihtr_srq *srq,
			struct ib_udata *udata)
{
	struct hihtr_ucontext *uctx = NULL;
#if !defined(UVERBS_ATTR_BUNDLE_HAVE_DRIVER_UDATA)
	struct ib_uobject *uobject = NULL;
	struct ib_ucontext *ib_uctx = NULL;
#elif !defined(UVERBS_IOCTL_HAVE_RDMA_UDATA_TO_DRV_CONTEXT)
	struct uverbs_attr_bundle *attr = NULL;
#endif

	if (!(srq->cap_flags & HIHTR_SRQ_CAP_RECORD_DB))
		return;

	srq->cap_flags &= ~HIHTR_SRQ_CAP_RECORD_DB;
#if !defined(UVERBS_ATTR_BUNDLE_HAVE_DRIVER_UDATA)
	uobject = srq->ibsrq.uobject;
	if (uobject) {
		ib_uctx = uobject->context;
		uctx = to_hr_ucontext(ib_uctx);
	}
#elif !defined(UVERBS_IOCTL_HAVE_RDMA_UDATA_TO_DRV_CONTEXT)
	if (udata) {
		attr = container_of(udata, struct uverbs_attr_bundle, driver_udata);
		uctx = to_hr_ucontext(ib_uverbs_get_ucontext(attr));
	} else if (srq->ibsrq.uobject) {
		uctx = to_hr_ucontext(srq->ibsrq.uobject->context);
	}
#else
	if (udata) {
		uctx = rdma_udata_to_drv_context(udata,
					struct hihtr_ucontext, ibucontext);
	}
#endif

	if (uctx) {
		hihtr_db_unmap_user(uctx, &srq->db);
	} else {
		hihtr_free_db(hr_dev, &srq->db);
	}
}

static int alloc_srq_db(struct hihtr_dev *hr_dev, struct hihtr_srq *srq,
			struct ib_udata *udata,
			struct hihtr_ib_create_srq_resp *resp)
{
	struct hihtr_ib_create_srq ucmd = {};
	struct hihtr_ucontext *uctx = NULL;
	int ret;
#if !defined(UVERBS_ATTR_BUNDLE_HAVE_DRIVER_UDATA)
	struct ib_uobject *uobject = NULL;
	struct ib_ucontext *ib_uctx = NULL;
#elif !defined(UVERBS_IOCTL_HAVE_RDMA_UDATA_TO_DRV_CONTEXT)
	struct uverbs_attr_bundle *attr = NULL;
#endif

	if (udata) {
		ret = get_srq_ucmd(srq, udata, &ucmd);
		if (ret)
			return ret;

        if ((hr_dev->caps.flags & HIHTR_CAP_FLAG_SRQ_RECORD_DB) &&
		    (ucmd.req_cap_flags & HIHTR_SRQ_CAP_RECORD_DB)) {
#if !defined(UVERBS_ATTR_BUNDLE_HAVE_DRIVER_UDATA)
			uobject = srq->ibsrq.uobject;
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
			ret = hihtr_db_map_user(uctx, ucmd.db_addr,
						   &srq->db, udata);
			if (ret)
				return ret;

			srq->cap_flags |= HIHTR_SRQ_CAP_RECORD_DB;
		}
	} else {
		if (hr_dev->caps.flags & HIHTR_CAP_FLAG_SRQ_RECORD_DB) {
			ret = hihtr_alloc_db(hr_dev, &srq->db, 1);
			if (ret)
				return ret;

			*srq->db.db_record = 0;
			srq->cap_flags |= HIHTR_SRQ_CAP_RECORD_DB;
		}
		srq->db_reg = hr_dev->reg_base + SRQ_DB_REG;
	}

	return 0;
}

#if defined(ROCE_CREATE_SRQ_INT)
int hihtr_create_srq(struct ib_srq *ib_srq,
			struct ib_srq_init_attr *init_attr,
			struct ib_udata *udata)
{
	struct hihtr_dev *hr_dev = to_hr_dev(ib_srq->device);
	struct hihtr_ib_create_srq_resp resp = {};
	struct hihtr_srq *srq = to_hr_srq(ib_srq);
	int ret;

	mutex_init(&srq->mutex);
	spin_lock_init(&srq->lock);

	ret = set_srq_param(srq, init_attr, udata);
	if (ret)
		goto error_out;

	ret = alloc_srq_buf(hr_dev, srq, udata);
	if (ret)
		goto error_out;

	if (hr_dev->caps.flags & HIHTR_CAP_FLAG_SRQ_RECORD_DB) {
		ret = alloc_srq_db(hr_dev, srq, udata, &resp);
		if (ret)
			goto err_srq_buf;
	}

	ret = alloc_srqn(hr_dev, srq);
	if (ret)
		goto err_srq_db;

	ret = alloc_srqc(hr_dev, srq);
	if (ret)
		goto err_srqn;

	if (udata) {
        resp.cap_flags = srq->cap_flags;
		resp.srqn = srq->srqn;
		if (ib_copy_to_udata(udata, &resp,
				     min(udata->outlen, sizeof(resp)))) {
			ret = -EFAULT;
			goto err_srqc;
		}
	}

	srq->db_reg = hr_dev->reg_base + SRQ_DB_REG;
	srq->event = hihtr_ib_srq_event;
	refcount_set(&srq->refcount, 1);
	init_completion(&srq->free);

	return 0;

err_srqc:
	free_srqc(hr_dev, srq);
err_srqn:
	free_srqn(hr_dev, srq);
err_srq_db:
	if (hr_dev->caps.flags & HIHTR_CAP_FLAG_SRQ_RECORD_DB) {
		free_srq_db(hr_dev, srq, udata);
	}
err_srq_buf:
	free_srq_buf(hr_dev, srq);
error_out:
	atomic64_inc(&hr_dev->dfx_cnt[HIHTR_DFX_SRQ_CREATE_ERR_CNT]);

	return ret;
}
#else
struct ib_srq *hihtr_create_srq(struct ib_pd *ibpd, struct ib_srq_init_attr *init_attr, struct ib_udata *udata)
{
	struct hihtr_dev *hr_dev = to_hr_dev(ibpd->device);
	struct hihtr_ib_create_srq_resp resp = {};
	struct hihtr_srq *srq = (struct hihtr_srq *)kzalloc(sizeof(*srq), GFP_KERNEL);
	int ret;
	if (srq == NULL) {
		ret = -ENOMEM;
		dev_err(hr_dev->dev, "failed to alloc srq memory.\n");
		return (struct ib_srq *)ERR_PTR((long)ret);
	}

	mutex_init(&srq->mutex);
	spin_lock_init(&srq->lock);
	srq->ibsrq.pd = ibpd;
	srq->ibsrq.device = ibpd->device;
	srq->ibsrq.uobject = ibpd->uobject;

	ret = set_srq_param(srq, init_attr, udata);
	if (ret)
		goto error_out;

	ret = alloc_srq_buf(hr_dev, srq, udata);
	if (ret)
		goto error_out;

	if (hr_dev->caps.flags & HIHTR_CAP_FLAG_SRQ_RECORD_DB) {
		ret = alloc_srq_db(hr_dev, srq, udata, &resp);
		if (ret)
			goto err_srq_buf;
	}

	ret = alloc_srqn(hr_dev, srq);
	if (ret)
		goto err_srq_db;

	ret = alloc_srqc(hr_dev, srq);
	if (ret)
		goto err_srqn;

	if (udata) {
        resp.cap_flags = srq->cap_flags;
		resp.srqn = srq->srqn;
		if (ib_copy_to_udata(udata, &resp,
				     min(udata->outlen, sizeof(resp)))) {
			ret = -EFAULT;
			goto err_srqc;
		}
	}

	srq->db_reg = hr_dev->reg_base + SRQ_DB_REG;
	srq->event = hihtr_ib_srq_event;
	refcount_set(&srq->refcount, 1);
	init_completion(&srq->free);

	return &srq->ibsrq;

err_srqc:
	free_srqc(hr_dev, srq);
err_srqn:
	free_srqn(hr_dev, srq);
err_srq_db:
	if (hr_dev->caps.flags & HIHTR_CAP_FLAG_SRQ_RECORD_DB) {
		free_srq_db(hr_dev, srq, udata);
	}
err_srq_buf:
	free_srq_buf(hr_dev, srq);
error_out:
	atomic64_inc(&hr_dev->dfx_cnt[HIHTR_DFX_SRQ_CREATE_ERR_CNT]);
	kfree(srq);

	return (struct ib_srq *)ERR_PTR((long)ret);
}
#endif

#if defined(ROCE_DESTROY_SRQ_VOID)
void hihtr_destroy_srq(struct ib_srq *ibsrq, struct ib_udata *udata)
{
	struct hihtr_dev *hr_dev = to_hr_dev(ibsrq->device);
	struct hihtr_srq *srq = to_hr_srq(ibsrq);

	free_srqc(hr_dev, srq);
	free_srqn(hr_dev, srq);
	free_srq_db(hr_dev, srq, udata);
	free_srq_buf(hr_dev, srq);
	return;
}
#elif defined(ROCE_DESTROY_SRQ_HAVE_UDATA)
int hihtr_destroy_srq(struct ib_srq *ibsrq, struct ib_udata *udata)
{
	struct hihtr_dev *hr_dev = to_hr_dev(ibsrq->device);
	struct hihtr_srq *srq = to_hr_srq(ibsrq);

	free_srqc(hr_dev, srq);
	free_srqn(hr_dev, srq);
	free_srq_db(hr_dev, srq, udata);
	free_srq_buf(hr_dev, srq);
	return 0;
}
#else
int hihtr_destroy_srq(struct ib_srq *ibsrq)
{
	struct hihtr_dev *hr_dev = to_hr_dev(ibsrq->device);
	struct hihtr_srq *srq = to_hr_srq(ibsrq);

	free_srqc(hr_dev, srq);
	free_srqn(hr_dev, srq);
	free_srq_db(hr_dev, srq, NULL);
	free_srq_buf(hr_dev, srq);
	kfree(srq);
	return 0;
}
#endif

void hihtr_init_srq_table(struct hihtr_dev *hr_dev)
{
	struct hihtr_srq_table *srq_table = &hr_dev->srq_table;
	struct hihtr_ida *srq_ida = &srq_table->srq_ida;

#if defined(RDMA_HAVE_XARRAY)
	xa_init(&srq_table->xa);
#else
	INIT_RADIX_TREE(&srq_table->root, GFP_KERNEL);
#endif

	ida_init(&srq_ida->ida);
	srq_ida->max = hr_dev->caps.num_srqs - 1;
	srq_ida->min = hr_dev->caps.reserved_srqs;
}
