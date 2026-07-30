/*
 * Copyright (c) 2016 Hisilicon Limited.
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

#include <rdma/ib_umem.h>
#include <rdma/uverbs_ioctl.h>
#include "hihtr_device.h"
#include "hihtr_cmd.h"
#include "hihtr_hem.h"
#include "hihtr_common.h"
#include "hihtr_cmdq.h"
#include "dfx/hihtr_dfx.h"

static int alloc_cqn(struct hihtr_dev *hr_dev, struct hihtr_cq *hr_cq)
{
	struct hihtr_cq_table *cq_table = &hr_dev->cq_table;
	struct hihtr_ida *cq_ida = &cq_table->cq_ida;
 	int id;

	mutex_lock(&cq_table->ida_mutex);
	id = ida_alloc_range(&cq_ida->ida, cq_ida->min, cq_ida->max, GFP_KERNEL);
	if (id < 0) {
		mutex_unlock(&cq_table->ida_mutex);
		ibdev_err(&hr_dev->ib_dev, "failed to alloc cqn id = %d.\n", id);
		return id;
	}
	hr_cq->cqn = id;
	mutex_unlock(&cq_table->ida_mutex);

	return 0;
}

static void free_cqn(struct hihtr_dev *hr_dev, unsigned long cqn)
{
	struct hihtr_ida *cq_ida = &hr_dev->cq_table.cq_ida;
	ida_free(&cq_ida->ida, cqn);
}

static int hihtr_create_cqc(struct hihtr_dev *hr_dev,
			       struct hihtr_cq *hr_cq,
			       u64 *mtts, dma_addr_t dma_handle)
{
	struct ib_device *ibdev = &hr_dev->ib_dev;
	struct hihtr_cmd_mailbox *mailbox;
	int ret;

	mailbox = hihtr_alloc_cmd_mailbox(hr_dev);
	if (IS_ERR(mailbox)) {
		ibdev_err(ibdev, "failed to alloc mailbox for CQC.\n");
		return PTR_ERR(mailbox);
	}

	hr_dev->hw->write_cqc(hr_dev, hr_cq, mailbox->buf, mtts, dma_handle);
	/* 调用cmq_mbox进行create cqc */
	ret = hihtr_cmq_mbox(hr_dev, mailbox->dma, 0, HIHTR_CMD_CREATE_CQC,
				     hr_cq->cqn);
	if (ret)
		ibdev_err(ibdev,
			  "failed to send create cmd for CQ(0x%lx), ret = %d.\n",
			  hr_cq->cqn, ret);

	hihtr_free_cmd_mailbox(hr_dev, mailbox);
	return ret;
}

static int alloc_cqc(struct hihtr_dev *hr_dev, struct hihtr_cq *hr_cq)
{
	struct hihtr_cq_table *cq_table = &hr_dev->cq_table;
	struct ib_device *ibdev = &hr_dev->ib_dev;
	u64 mtts[MTT_MIN_COUNT] = {};
	dma_addr_t dma_handle;
	int ret;

	ret = hihtr_mtr_find(hr_dev, &hr_cq->mtr, 0, mtts, ARRAY_SIZE(mtts),
				&dma_handle);
	if (!ret) {
		ibdev_err(ibdev, "failed to find CQ mtr, ret = %d.\n", ret);
		return -EINVAL;
	}

	/* Get CQC memory HEM(Hardware Entry Memory) table */
	ret = hihtr_table_get(hr_dev, &cq_table->table, hr_cq->cqn);
	if (ret) {
		ibdev_err(ibdev, "failed to get CQ(0x%lx) context, ret = %d.\n",
			  hr_cq->cqn, ret);
		return ret;
	}

#if defined(RDMA_HAVE_XARRAY)
	ret = xa_err(xa_store(&cq_table->array, hr_cq->cqn, hr_cq, GFP_KERNEL));
	if (ret) {
		ibdev_err(ibdev, "failed to xa_store CQ, ret = %d.\n", ret);
		goto err_out;
	}
#else
	ret = radix_tree_preload(GFP_KERNEL);
	if (ret) {
		ibdev_err(ibdev, "failed to preload CQ root, ret = %d.\n", ret);
		goto err_out;
	}
	ret = radix_tree_insert(&cq_table->root, hr_cq->cqn, hr_cq);
	radix_tree_preload_end();
	if (ret) {
		ibdev_err(ibdev, "failed to insert CQ in radix tree, ret = %d.\n", ret);
		goto err_out;
	}
#endif

	ret = hihtr_create_cqc(hr_dev, hr_cq, mtts, dma_handle);
	if (ret)
		goto err_xa;

	return 0;

err_xa:
#if defined(RDMA_HAVE_XARRAY)
	xa_erase(&cq_table->array, hr_cq->cqn);
#else
	radix_tree_delete(&cq_table->root, hr_cq->cqn);
#endif
err_out:
	return ret;
}

static void free_cqc(struct hihtr_dev *hr_dev, struct hihtr_cq *hr_cq)
{
	struct hihtr_cq_table *cq_table = &hr_dev->cq_table;
	struct device *dev = hr_dev->dev;
	int ret;

    /* 调用cmq_mbox进行destory cqc */
	ret = hihtr_cmq_mbox(hr_dev, 0, 0, HIHTR_CMD_DESTROY_CQC,
				      hr_cq->cqn);
	if (ret)
		dev_err(dev, "DESTROY_CQ failed (%d) for CQN %06lx\n", ret,
			hr_cq->cqn);

#if defined(RDMA_HAVE_XARRAY)
	xa_erase(&cq_table->array, hr_cq->cqn);
#else
	radix_tree_delete(&cq_table->root, hr_cq->cqn);
#endif
	if (hr_dev->eq_table.eq != NULL) {
		/* Waiting interrupt process procedure carried out */
		synchronize_irq(hr_dev->eq_table.eq[hr_cq->vector].irq);
	}
	/* wait for all interrupt processed */
	if (refcount_dec_and_test(&hr_cq->refcount))
		complete(&hr_cq->free);
	wait_for_completion(&hr_cq->free);
}

static int alloc_cq_buf(struct hihtr_dev *hr_dev, struct hihtr_cq *hr_cq,
			struct ib_udata *udata, unsigned long addr)
{
	struct ib_device *ibdev = &hr_dev->ib_dev;
	struct hihtr_buf_attr buf_attr = {};
	int ret;
	u32 create_cq_depth = hr_cq->cq_depth;

	if (hr_cq->resize_cq_en == HIHTR_CQ_RESIZE_CQ_ENABLE) {
		create_cq_depth = hr_cq->resize_cq_hw_cq_depth;
	}

	buf_attr.page_shift = hr_dev->caps.cqe_buf_pg_sz + PAGE_SHIFT;
	buf_attr.region[0].size = ALIGN(create_cq_depth * hr_cq->cqe_size, 1 << buf_attr.page_shift);
	buf_attr.region[0].hopnum = hr_dev->caps.cqe_hop_num;
	buf_attr.region_count = 1;

	ret = hihtr_mtr_create(hr_dev, &hr_cq->mtr, &buf_attr,
				  hr_dev->caps.cqe_ba_pg_sz + PAGE_SHIFT,
				  udata, addr, (struct ib_uobject *)hr_cq->ib_cq.uobject);
	if (ret)
		ibdev_err(ibdev, "Failed to alloc CQ mtr, ret = %d\n", ret);

	return ret;
}

static void free_cq_buf(struct hihtr_dev *hr_dev, struct hihtr_cq *hr_cq)
{
	hihtr_mtr_destroy(hr_dev, &hr_cq->mtr);
}

static int alloc_cq_db(struct hihtr_dev *hr_dev, struct hihtr_cq *hr_cq,
		       struct ib_udata *udata, unsigned long addr,
		       struct hihtr_ib_create_cq_resp *resp)
{
	bool has_db = hr_dev->caps.flags & HIHTR_CAP_FLAG_CQ_RECORD_DB;
	struct hihtr_ucontext *uctx = NULL;
#if !defined(UVERBS_ATTR_BUNDLE_HAVE_DRIVER_UDATA)
	struct ib_uobject *uobject = NULL;
	struct ib_ucontext *ib_uctx = NULL;
#elif !defined(UVERBS_IOCTL_HAVE_RDMA_UDATA_TO_DRV_CONTEXT)
	struct uverbs_attr_bundle *attr = NULL;
#endif
	int err;

	if (udata) {
		if (has_db &&
		    udata->outlen >= offsetofend(typeof(*resp), cap_flags)) {
#if !defined(UVERBS_ATTR_BUNDLE_HAVE_DRIVER_UDATA)
			uobject = hr_cq->ib_cq.uobject;
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
			err = hihtr_db_map_user(uctx, addr, &hr_cq->db, udata);
			if (err)
				return err;
			hr_cq->flags |= HIHTR_CQ_FLAG_RECORD_DB;
			resp->cap_flags |= HIHTR_CQ_FLAG_RECORD_DB;
		}
	} else {
		if (has_db) {
			err = hihtr_alloc_db(hr_dev, &hr_cq->db, 1);
			if (err)
				return err;
			hr_cq->set_ci_db = hr_cq->db.db_record;
			*hr_cq->set_ci_db = 0;
			hr_cq->flags |= HIHTR_CQ_FLAG_RECORD_DB;
		}
		hr_cq->db_reg = hr_dev->reg_base; // cq base is same as roce doorbell reg base
	}

	return 0;
}

static void free_cq_db(struct hihtr_dev *hr_dev, struct hihtr_cq *hr_cq,
		       struct ib_udata *udata)
{
	struct hihtr_ucontext *uctx = NULL;
#if !defined(UVERBS_ATTR_BUNDLE_HAVE_DRIVER_UDATA)
	struct ib_uobject *uobject = NULL;
	struct ib_ucontext *ib_uctx = NULL;
#elif !defined(UVERBS_IOCTL_HAVE_RDMA_UDATA_TO_DRV_CONTEXT)
	struct uverbs_attr_bundle *attr = NULL;
#endif

	if (!(hr_cq->flags & HIHTR_CQ_FLAG_RECORD_DB))
		return;

	hr_cq->flags &= ~HIHTR_CQ_FLAG_RECORD_DB;
#if !defined(UVERBS_ATTR_BUNDLE_HAVE_DRIVER_UDATA)
	uobject = hr_cq->ib_cq.uobject;
	if (uobject) {
		ib_uctx = uobject->context;
		uctx = to_hr_ucontext(ib_uctx);
	}
#elif !defined(UVERBS_IOCTL_HAVE_RDMA_UDATA_TO_DRV_CONTEXT)
	if (udata) {
		attr = container_of(udata, struct uverbs_attr_bundle, driver_udata);
		uctx = to_hr_ucontext(ib_uverbs_get_ucontext(attr));
	} else if (hr_cq->ib_cq.uobject) {
		uctx = to_hr_ucontext(hr_cq->ib_cq.uobject->context);
	}
#else
	if (udata) {
		uctx = rdma_udata_to_drv_context(udata,
						 struct hihtr_ucontext,
						 ibucontext);
	}
#endif

	if (uctx) {
		hihtr_db_unmap_user(uctx, &hr_cq->db);
	} else {
		hihtr_free_db(hr_dev, &hr_cq->db);
	}
}

static int verify_cq_create_attr(struct hihtr_dev *hr_dev,
				 const struct ib_cq_init_attr *attr)
{
	struct ib_device *ibdev = &hr_dev->ib_dev;

	if (!attr->cqe || attr->cqe > hr_dev->caps.max_cqes) {
		ibdev_err(ibdev, "failed to check CQ count %u, max = %u.\n",
			  attr->cqe, hr_dev->caps.max_cqes);
		return -EINVAL;
	}

	if (attr->comp_vector >= hr_dev->caps.num_comp_vectors) {
		ibdev_err(ibdev, "failed to check CQ vector = %u, max = %d.\n",
			  attr->comp_vector, hr_dev->caps.num_comp_vectors);
		return -EINVAL;
	}

	return 0;
}

static int get_cq_ucmd(struct hihtr_cq *hr_cq, struct ib_udata *udata,
		       struct hihtr_ib_create_cq *ucmd)
{
	struct ib_device *ibdev = hr_cq->ib_cq.device;
	int ret;

	ret = ib_copy_from_udata(ucmd, udata, min(udata->inlen, sizeof(*ucmd)));
	if (ret) {
		ibdev_err(ibdev, "failed to copy CQ udata, ret = %d.\n", ret);
		return ret;
	}

	return 0;
}

static void set_cq_param(struct hihtr_cq *hr_cq, u32 cq_entries, int vector,
			 struct hihtr_ib_create_cq *ucmd)
{
	struct hihtr_dev *hr_dev = to_hr_dev(hr_cq->ib_cq.device);
	cq_entries = max(cq_entries, hr_dev->caps.min_cqes);
	cq_entries = roundup_pow_of_two(cq_entries);
	hr_cq->ib_cq.cqe = cq_entries - 1; /* used as cqe index */
	hr_cq->cq_depth = cq_entries;
	hr_cq->vector = vector;

	/* 创建CQ的初始深度大于等于resize_cq_hw_cq_depth，不再支持resize_cq */
	if ((hr_dev->caps.flags & HIHTR_CAP_FLAG_RESIZE_CQ) && (hr_cq->cq_depth < hr_dev->caps.resize_cq_hw_cq_depth)) {
		hr_cq->resize_cq_en = HIHTR_CQ_RESIZE_CQ_ENABLE;
		hr_cq->resize_cq_hw_cq_depth = hr_dev->caps.resize_cq_hw_cq_depth;
	}
	spin_lock_init(&hr_cq->lock);
	INIT_LIST_HEAD(&hr_cq->sq_list);
	INIT_LIST_HEAD(&hr_cq->rq_list);
}

static int set_cqe_size(struct hihtr_cq *hr_cq, struct ib_udata *udata,
			struct hihtr_ib_create_cq *ucmd)
{
	struct hihtr_dev *hr_dev = to_hr_dev(hr_cq->ib_cq.device);

	if (!udata) {
		hr_cq->cqe_size = hr_dev->caps.cqe_sz;
		return 0;
	}

	if (udata->inlen >= offsetofend(typeof(*ucmd), cqe_size)) {
		if (ucmd->cqe_size != HIHTR_V2_CQE_SIZE &&
		    ucmd->cqe_size != HIHTR_V3_CQE_SIZE) {
			ibdev_err(&hr_dev->ib_dev,
				  "invalid cqe size %u.\n", ucmd->cqe_size);
			return -EINVAL;
		}

		hr_cq->cqe_size = ucmd->cqe_size;
	} else {
		hr_cq->cqe_size = HIHTR_V2_CQE_SIZE;
	}

	return 0;
}

#if defined(ROCE_CREATE_CQ_INT_HAVE_BUNDLE)
int hihtr_create_cq(struct ib_cq *ib_cq, const struct ib_cq_init_attr *attr,
            	struct uverbs_attr_bundle *attrs)
{
	struct hihtr_dev *hr_dev = to_hr_dev(ib_cq->device);
	struct hihtr_cq *hr_cq = to_hr_cq(ib_cq);
	struct ib_device *ibdev = &hr_dev->ib_dev;
	struct ib_udata *udata = &attrs->driver_udata;
	struct hihtr_ib_create_cq_resp resp = {};
	struct hihtr_ib_create_cq ucmd = {};
	int ret;
#elif defined(ROCE_CREATE_CQ_INT)
int hihtr_create_cq(struct ib_cq *ib_cq, const struct ib_cq_init_attr *attr,
		       struct ib_udata *udata)
{
	struct hihtr_dev *hr_dev = to_hr_dev(ib_cq->device);
	struct hihtr_cq *hr_cq = to_hr_cq(ib_cq);
	struct ib_device *ibdev = &hr_dev->ib_dev;
	struct hihtr_ib_create_cq_resp resp = {};
	struct hihtr_ib_create_cq ucmd = {};
	int ret;
#else
struct ib_cq *hihtr_create_cq(struct ib_device *ibdev, const struct ib_cq_init_attr *attr,
								 struct ib_ucontext *ibcontext, struct ib_udata *udata)
{
	struct hihtr_dev *hr_dev = to_hr_dev(ibdev);
	struct hihtr_cq *hr_cq = (struct hihtr_cq *)kzalloc(sizeof(*hr_cq), GFP_KERNEL);
	struct hihtr_ib_create_cq_resp resp = {};
	struct hihtr_ib_create_cq ucmd = {};
	struct ib_uobject* uobject;
	int ret;

	if (hr_cq == NULL) {
		dev_err(hr_dev->dev, "failed to alloc hr_cq memory.\n");
		return (struct ib_cq *)ERR_PTR((long)-ENOMEM);
	}
	hr_cq->ib_cq.device = ibdev;
	uobject = container_of(&ibcontext, struct ib_uobject, context);
	hr_cq->ib_cq.uobject = uobject;
#endif

	if (attr->flags) {
		ret = -EOPNOTSUPP;
		goto err_out;
	}

	ret = verify_cq_create_attr(hr_dev, attr);
	if (ret)
		goto err_out;

	if (udata) {
		ret = get_cq_ucmd(hr_cq, udata, &ucmd);
		if (ret)
			goto err_out;
	}

	set_cq_param(hr_cq, attr->cqe, attr->comp_vector, &ucmd);

	ret = set_cqe_size(hr_cq, udata, &ucmd);
	if (ret)
		goto err_out;

	ret = alloc_cq_buf(hr_dev, hr_cq, udata, ucmd.buf_addr);
	if (ret) {
		ibdev_err(ibdev, "failed to alloc CQ buf, ret = %d.\n", ret);
		goto err_out;
	}

	ret = alloc_cq_db(hr_dev, hr_cq, udata, ucmd.db_addr, &resp);
	if (ret) {
		ibdev_err(ibdev, "failed to alloc CQ db, ret = %d.\n", ret);
		goto err_cq_buf;
	}

	ret = alloc_cqn(hr_dev, hr_cq);
	if (ret) {
		ibdev_err(ibdev, "failed to alloc CQN, ret = %d.\n", ret);
		goto err_cq_db;
	}

	ret = alloc_cqc(hr_dev, hr_cq);
	if (ret) {
		ibdev_err(ibdev,
			  "failed to alloc CQ context, ret = %d.\n", ret);
		goto err_cqn;
	}
	if (udata) {
		resp.cqn = hr_cq->cqn;
		ret = ib_copy_to_udata(udata, &resp,
				    min(udata->outlen, sizeof(resp)));
		if (ret)
			goto err_cqc;
	}

	hr_cq->cons_index = 0;
	hr_cq->arm_sn = 1;
	refcount_set(&hr_cq->refcount, 1);
	init_completion(&hr_cq->free);
#if defined(ROCE_CREATE_CQ_INT)
	return 0;
#else
	return &hr_cq->ib_cq;
#endif

err_cqc:
	free_cqc(hr_dev, hr_cq);
err_cqn:
	free_cqn(hr_dev, hr_cq->cqn);
err_cq_db:
	free_cq_db(hr_dev, hr_cq, udata);
err_cq_buf:
	free_cq_buf(hr_dev, hr_cq);
err_out:
	atomic64_inc(&hr_dev->dfx_cnt[HIHTR_DFX_CQ_CREATE_ERR_CNT]);

#if defined(ROCE_CREATE_CQ_INT)
	return ret;
#else
	kfree(hr_cq);
	return (struct ib_cq *)ERR_PTR((long)ret);
#endif
}

#if defined(ROCE_DESTROY_CQ_VOID)
void hihtr_destroy_cq(struct ib_cq *ib_cq, struct ib_udata *udata)
{
	struct hihtr_dev *hr_dev = to_hr_dev(ib_cq->device);
	struct hihtr_cq *hr_cq = to_hr_cq(ib_cq);

	free_cqc(hr_dev, hr_cq);
	free_cqn(hr_dev, hr_cq->cqn);
	free_cq_db(hr_dev, hr_cq, udata);
	free_cq_buf(hr_dev, hr_cq);

	return;
}
#elif defined(ROCE_DESTROY_CQ_HAVE_UDATA)
int hihtr_destroy_cq(struct ib_cq *ib_cq, struct ib_udata *udata)
{
	struct hihtr_dev *hr_dev = to_hr_dev(ib_cq->device);
	struct hihtr_cq *hr_cq = to_hr_cq(ib_cq);

	free_cqc(hr_dev, hr_cq);
	free_cqn(hr_dev, hr_cq->cqn);
	free_cq_db(hr_dev, hr_cq, udata);
	free_cq_buf(hr_dev, hr_cq);

	return 0;
}
#else
int hihtr_destroy_cq(struct ib_cq *ib_cq)
{
	struct hihtr_dev *hr_dev = to_hr_dev(ib_cq->device);
	struct hihtr_cq *hr_cq = to_hr_cq(ib_cq);

	free_cqc(hr_dev, hr_cq);
	free_cqn(hr_dev, hr_cq->cqn);
	free_cq_db(hr_dev, hr_cq, NULL);
	free_cq_buf(hr_dev, hr_cq);

	kfree(hr_cq);
	return 0;
}
#endif

static int hihtr_resize_cq_check(struct ib_cq *ibcq, u32 entries, const struct ib_udata *udata)
{
	struct ib_device *ibdev = ibcq->device;
	struct hihtr_cq *hr_cq = to_hr_cq(ibcq);

	if ((hr_cq->resize_cq_en == HIHTR_CQ_RESIZE_CQ_DISABLE) || (entries <= hr_cq->cq_depth) || (entries > hr_cq->resize_cq_hw_cq_depth)) {
        ibdev_err(ibdev, "resize cq failed, resize_en:0x%x, new_depth:0x%x, old_depth:0x%x.\n", hr_cq->resize_cq_en, entries, hr_cq->cq_depth);
		return -EINVAL;
	}
    return 0;
}

int hihtr_resize_cq(struct ib_cq *ibcq, int entries, struct ib_udata *udata)
{
	struct hihtr_cq *hr_cq = to_hr_cq(ibcq);
    int ret = 0;
	unsigned long flags;
	entries = roundup_pow_of_two(entries);
    ret = hihtr_resize_cq_check(ibcq, entries, udata);
    if (ret != 0) {
        return ret;
    }
	spin_lock_irqsave(&hr_cq->lock, flags);
	hr_cq->cq_depth = entries;
	ibcq->cqe = entries - 1;
	spin_unlock_irqrestore(&hr_cq->lock, flags);
    return 0;
}

void hihtr_cq_completion(struct hihtr_dev *hr_dev, u32 cqn)
{
	struct hihtr_cq *hr_cq;
	struct ib_cq *ibcq;

#if defined(RDMA_HAVE_XARRAY)
	hr_cq = xa_load(&hr_dev->cq_table.array, cqn & (hr_dev->caps.num_cqs - 1));
#else
	hr_cq = radix_tree_lookup(&hr_dev->cq_table.root, cqn & (hr_dev->caps.num_cqs - 1));
#endif
	if (!hr_cq) {
		dev_warn(hr_dev->dev, "Completion event for bogus CQ 0x%06x\n",
			 cqn);
		return;
	}

	++hr_cq->arm_sn;
	ibcq = &hr_cq->ib_cq;
	if (ibcq->comp_handler)
		ibcq->comp_handler(ibcq, ibcq->cq_context);
}

void hihtr_cq_event(struct hihtr_dev *hr_dev, u32 cqn, int event_type)
{
	struct device *dev = hr_dev->dev;
	struct hihtr_cq *hr_cq;
	struct ib_event event;
	struct ib_cq *ibcq;

#if defined(RDMA_HAVE_XARRAY)
	hr_cq = xa_load(&hr_dev->cq_table.array, cqn & (hr_dev->caps.num_cqs - 1));
#else
	hr_cq = radix_tree_lookup(&hr_dev->cq_table.root, cqn & (hr_dev->caps.num_cqs - 1));
#endif
	if (!hr_cq) {
		dev_warn(dev, "Async event for bogus CQ 0x%06x\n", cqn);
		return;
	}

	if (event_type != HIHTR_EVENT_TYPE_CQ_ID_INVALID &&
	    event_type != HIHTR_EVENT_TYPE_CQ_ACCESS_ERROR &&
	    event_type != HIHTR_EVENT_TYPE_CQ_OVERFLOW) {
		dev_err(dev, "Unexpected event type 0x%x on CQ 0x%06x\n",
			event_type, cqn);
		return;
	}

	refcount_inc(&hr_cq->refcount);

	ibcq = &hr_cq->ib_cq;
	if (ibcq->event_handler) {
		event.device = ibcq->device;
		event.element.cq = ibcq;
		event.event = IB_EVENT_CQ_ERR;
		ibcq->event_handler(&event, ibcq->cq_context);
	}
	hihtr_dfx_record_err_event_count(hr_dev, IB_EVENT_CQ_ERR);
	if (refcount_dec_and_test(&hr_cq->refcount))
		complete(&hr_cq->free);
}

void hihtr_init_cq_table(struct hihtr_dev *hr_dev)
{
	struct hihtr_cq_table *cq_table = &hr_dev->cq_table;
	struct hihtr_ida *cq_ida = &cq_table->cq_ida;

	mutex_init(&cq_table->ida_mutex);
#if defined(RDMA_HAVE_XARRAY)
	xa_init(&cq_table->array);
#else
	INIT_RADIX_TREE(&cq_table->root, GFP_KERNEL);
#endif
	ida_init(&cq_table->cq_ida.ida);
	cq_ida->max = hr_dev->caps.num_cqs - 1;
	cq_ida->min = hr_dev->caps.reserved_cqs;
}

void hihtr_cleanup_cq_table(struct hihtr_dev *hr_dev)
{
	ida_destroy(&hr_dev->cq_table.cq_ida.ida);
}
