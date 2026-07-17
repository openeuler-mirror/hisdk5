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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <sys/mman.h>
#include <linux/version.h>
#include "roce_ucompat_gen.h"
#if defined(ILOG_HAVE_ILOG32)
#include <ccan/ilog.h>
#endif
#include <ccan/minmax.h>
#include "hnsdv.h"
#include "hihtr_u.h"
#include "hihtr_u_db.h"
#include "hihtr_u_hw_v2.h"

void hihtr_init_qp_indices(struct hihtr_qp *qp)
{
	qp->sq.head = 0;
	qp->sq.tail = 0;
	qp->rq.head = 0;
	qp->rq.tail = 0;
	qp->next_sge = 0;
}

#if defined(VERBS_OPS_HAVE_QUERY_DEVICE)
int hihtr_u_query_device(struct ibv_context *context, struct ibv_device_attr *attr)
{
	struct ibv_query_device cmd = {};
	uint64_t raw_fw_ver;
	unsigned sub_minor;
	unsigned major;
	unsigned minor;
	int ret = 0;

	ret = ibv_cmd_query_device(context, attr, &raw_fw_ver, &cmd, sizeof cmd);
	if (ret)
		return ret;

	minor     = (raw_fw_ver >> RAW_FW_MINOR_VER_SHIFT) & 0xffff;
	sub_minor = raw_fw_ver & 0xffff;
	major     = (raw_fw_ver >> RAW_FW_MAJOR_VER_SHIFT) & 0xffff;

	ret = snprintf(attr->fw_ver, sizeof attr->fw_ver, "%d.%d.%03d", major, minor, sub_minor);
	if (ret < 0) {
		return ret;
	}

	return 0;
}
#endif

int hihtr_u_query_device_ex(struct ibv_context *context,
			    const struct ibv_query_device_ex_input *input,
			    struct ibv_device_attr_ex *attr, size_t attr_size)
{
#if defined(HAVE_IBV_CMD_QUERY_DEV_ANY)
	struct ib_uverbs_ex_query_device_resp resp;
	size_t resp_size = sizeof(resp);
	int ret;
	uint64_t raw_fw_ver;
	unsigned int major, minor, sub_minor;

	ret = ibv_cmd_query_device_any(context, input, attr, attr_size, &resp,
				       &resp_size);
	if (ret)
		return ret;

	raw_fw_ver = resp.base.fw_ver;
	major = (raw_fw_ver >> RAW_FW_MAJOR_VER_SHIFT) & RAW_FW_SUBMINOR_VER_MASK;
	minor = (raw_fw_ver >> RAW_FW_MINOR_VER_SHIFT) & RAW_FW_SUBMINOR_VER_MASK;
	sub_minor = raw_fw_ver & 0xffff;

	ret = snprintf(attr->orig_attr.fw_ver, sizeof(attr->orig_attr.fw_ver),
		 "%u.%u.%03u", major, minor, sub_minor);
	if (ret < 0) {
		return ret;
	}

	return 0;
#else
	struct ib_uverbs_ex_query_device_resp resp = {};
	struct ibv_query_device_ex cmd_ex = {};
	struct ibv_query_device cmd = {};
	uint64_t raw_fw_ver;
	unsigned sub_minor;
	unsigned major;
	unsigned minor;
	int err = 0;

	if (attr_size > sizeof(attr->orig_attr)) {
		err = compat_ibv_cmd_query_device_ex(context, input, attr, attr_size,
                                      &raw_fw_ver, &cmd_ex, sizeof(cmd),
                                      &resp, sizeof(resp));
	}
	if (err)
		return err;

	if (attr_size == sizeof(attr->orig_attr)) {
		err = ibv_cmd_query_device(context, &attr->orig_attr,
				    &raw_fw_ver, &cmd, sizeof(cmd));
	}
	if (err)
		return err;

	major     = (raw_fw_ver >> RAW_FW_MAJOR_VER_SHIFT) & RAW_FW_SUBMINOR_VER_MASK;
	minor     = (raw_fw_ver >> RAW_FW_MINOR_VER_SHIFT) & RAW_FW_SUBMINOR_VER_MASK;
	sub_minor = raw_fw_ver & 0xffff;

	err = snprintf(attr->orig_attr.fw_ver, sizeof attr->orig_attr.fw_ver,
		 "%d.%d.%03d", major, minor, sub_minor);
	if (err < 0) {
		return err;
	}

	return 0;
#endif
}

int hihtr_u_query_port(struct ibv_context *context, uint8_t port,
			  struct ibv_port_attr *attr)
{
	struct ibv_query_port cmd;

	return ibv_cmd_query_port(context, port, attr, &cmd, sizeof(cmd));
}

struct ibv_pd *hihtr_u_alloc_pd(struct ibv_context *context)
{
	struct ibv_alloc_pd cmd;
	struct hihtr_pd *pd;
	struct hihtr_alloc_pd_resp resp = {};

	pd = malloc(sizeof(*pd));
	if (!pd)
		return NULL;

	if (ibv_cmd_alloc_pd(context, &pd->ibv_pd, &cmd, sizeof(cmd),
			     &resp.ibv_resp, sizeof(resp))) {
		free(pd);
		return NULL;
	}

	pd->pdn = resp.pdn;

	return &pd->ibv_pd;
}

int hihtr_u_free_pd(struct ibv_pd *pd)
{
	int ret;

	ret = ibv_cmd_dealloc_pd(pd);
	if (ret)
		return ret;

	free(to_hr_pd(pd));

	return ret;
}

struct ibv_xrcd *hihtr_u_open_xrcd(struct ibv_context *context,
				      struct ibv_xrcd_init_attr *xrcd_init_attr)
{
	struct ib_uverbs_open_xrcd_resp resp = {};
	struct ibv_open_xrcd cmd = {};
	struct verbs_xrcd *xrcd;
	int ret;

	xrcd = calloc(1, sizeof(*xrcd));
	if (!xrcd)
		return NULL;

	ret = ibv_cmd_open_xrcd(context, xrcd, sizeof(*xrcd), xrcd_init_attr,
				&cmd, sizeof(cmd), &resp, sizeof(resp));
	if (ret) {
		free(xrcd);
		return NULL;
	}

	return &xrcd->xrcd;
}

int hihtr_u_close_xrcd(struct ibv_xrcd *ibv_xrcd)
{
	struct verbs_xrcd *xrcd =
			container_of(ibv_xrcd, struct verbs_xrcd, xrcd);
	int ret;

	ret = ibv_cmd_close_xrcd(xrcd);
	if (!ret)
		free(xrcd);

	return ret;
}

#if defined(REG_MR_HAVE_HCA_VA)
struct ibv_mr *hihtr_u_reg_mr(struct ibv_pd *pd, void *addr, size_t length,
				 uint64_t hca_va, int access)
#else
struct ibv_mr *hihtr_u_reg_mr(struct ibv_pd *pd, void *addr, size_t length, int access)
#endif
{
	int ret;
	struct verbs_mr *vmr;
	struct ibv_reg_mr cmd;
	struct ib_uverbs_reg_mr_resp resp;

	if (!addr) {
		verbs_err(verbs_get_ctx(pd->context),
			  "2nd parm addr is NULL!\n");
		return NULL;
	}

	if (!length) {
		verbs_err(verbs_get_ctx(pd->context),
			  "3st parm length is 0!\n");
		return NULL;
	}

	vmr = malloc(sizeof(*vmr));
	if (!vmr)
		return NULL;

#if defined(VERBS_DRIVER_HAVE_HCA_VA_IN_REG_MR)
	ret = ibv_cmd_reg_mr(pd, addr, length, hca_va, access, vmr, &cmd,
			     sizeof(cmd), &resp, sizeof(resp));
#else
	ret = ibv_cmd_reg_mr(pd, addr, length, (uint64_t)addr, access, vmr, &cmd,
			     sizeof(cmd), &resp, sizeof(resp));
#endif
	if (ret) {
		free(vmr);
		return NULL;
	}

	return &vmr->ibv_mr;
}

int hihtr_u_rereg_mr(struct verbs_mr *vmr, int flags, struct ibv_pd *pd,
			void *addr, size_t length, int access)
{
	struct ibv_rereg_mr cmd;
	struct ib_uverbs_rereg_mr_resp resp;

	return ibv_cmd_rereg_mr(vmr, flags, addr, length, (uintptr_t)addr,
				access, pd, &cmd, sizeof(cmd), &resp,
				sizeof(resp));
}

int hihtr_u_dereg_mr(struct verbs_mr *vmr)
{
	int ret;

	ret = ibv_cmd_dereg_mr(vmr);
	if (ret)
		return ret;

	free(vmr);

	return ret;
}

int hihtr_u_bind_mw(struct ibv_qp *qp, struct ibv_mw *mw,
		       struct ibv_mw_bind *mw_bind)
{
	struct ibv_mw_bind_info *bind_info = &mw_bind->bind_info;
	struct ibv_send_wr *bad_wr = NULL;
	struct ibv_send_wr wr = {};
	int ret;

	if (bind_info->mw_access_flags & ~(IBV_ACCESS_REMOTE_WRITE |
	    IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_ATOMIC))
		return EINVAL;

	wr.opcode = IBV_WR_BIND_MW;
	wr.next = NULL;

	wr.wr_id = mw_bind->wr_id;
	wr.send_flags = mw_bind->send_flags;

	wr.bind_mw.mw = mw;
	wr.bind_mw.rkey = ibv_inc_rkey(mw->rkey);
	wr.bind_mw.bind_info = mw_bind->bind_info;

	ret = hihtr_u_v2_post_send(qp, &wr, &bad_wr);
	if (ret)
		return ret;

	mw->rkey = wr.bind_mw.rkey;

	return 0;
}

struct ibv_mw *hihtr_u_alloc_mw(struct ibv_pd *pd, enum ibv_mw_type type)
{
	struct ibv_mw *mw;
	struct ibv_alloc_mw cmd = {};
	struct ib_uverbs_alloc_mw_resp resp = {};

	mw = malloc(sizeof(*mw));
	if (!mw)
		return NULL;

	if (ibv_cmd_alloc_mw(pd, type, mw, &cmd, sizeof(cmd),
			     &resp, sizeof(resp))) {
		free(mw);
		return NULL;
	}

	return mw;
}

int hihtr_u_dealloc_mw(struct ibv_mw *mw)
{
	int ret;

	ret = ibv_cmd_dealloc_mw(mw);
	if (ret)
		return ret;

	free(mw);

	return 0;
}

enum {
	CREATE_CQ_SUPPORTED_WC_FLAGS = IBV_WC_STANDARD_FLAGS |
				       IBV_WC_EX_WITH_CVLAN,
};

static int verify_cq_create_attr(struct ibv_cq_init_attr_ex *attr,
				 struct hihtr_context *context)
{
	if (!attr->cqe || attr->cqe > context->max_cqe)
		return -EINVAL;

	if (attr->comp_mask)
		return -EOPNOTSUPP;

	if (!check_comp_mask(attr->wc_flags, CREATE_CQ_SUPPORTED_WC_FLAGS))
		return -EOPNOTSUPP;

	attr->cqe = max_t(uint32_t, HIHTR_MIN_CQE_NUM,
			  roundup_pow_of_two(attr->cqe));

	return 0;
}

static int hihtr_alloc_cq_buf(struct hihtr_cq *cq, struct hihtr_context *ctx)
{
	uint32_t pagesize = 1 << ctx->cqe_mtt_buf_pg_sz;
	int buf_size = 0;
	int ret = 0;

	if (cq->resize_cq_en == HIHTR_CQ_RESIZE_CQ_ENABLE) {
		buf_size = align(cq->resize_cq_hw_cq_depth * cq->cqe_size, pagesize);
	} else {
		buf_size = align(cq->cq_depth * cq->cqe_size, pagesize);
	}

	ret = hihtr_alloc_buf(&cq->buf, buf_size, pagesize);
	if (ret)
		return -ENOMEM;

	return 0;
}

static int exec_cq_create_cmd(struct ibv_context *context,
			      struct hihtr_cq *cq,
			      struct ibv_cq_init_attr_ex *attr)
{
	struct hihtr_create_cq_ex_resp resp_ex = {};
	struct hihtr_ib_create_cq_resp *resp_drv;
	struct hihtr_create_cq_ex cmd_ex = {};
	struct hihtr_ib_create_cq *cmd_drv;
	int ret;

	cmd_drv = &cmd_ex.drv_payload;
	resp_drv = &resp_ex.drv_payload;

	cmd_drv->buf_addr = (uintptr_t)cq->buf.buf;
	cmd_drv->db_addr = (uintptr_t)cq->db;
	cmd_drv->cqe_size = (uintptr_t)cq->cqe_size;

#if !defined(CREATE_CQ_EX_HAVE_CMD_FLAGS) && defined(CREATE_CQ_EX_HAVE_VERBS_CQ)
	ret = ibv_cmd_create_cq_ex(context, attr, &cq->verbs_cq,
				   &cmd_ex.ibv_cmd, sizeof(cmd_ex),
				   &resp_ex.ibv_resp, sizeof(resp_ex));
#elif defined(CREATE_CQ_EX_HAVE_VERBS_CQ)
	ret = ibv_cmd_create_cq_ex(context, attr, &cq->verbs_cq,
				   &cmd_ex.ibv_cmd, sizeof(cmd_ex),
				   &resp_ex.ibv_resp, sizeof(resp_ex), 0);
#else
	ret = ibv_cmd_create_cq_ex(context, attr, &cq->verbs_cq.cq_ex,
				   &cmd_ex.ibv_cmd, sizeof(cmd_ex),
				   &resp_ex.ibv_resp, sizeof(resp_ex));
#endif
	if (ret)
		return ret;

	cq->cqn = resp_drv->cqn;
	cq->flags = resp_drv->cap_flags;

	return 0;
}

static struct ibv_cq_ex *create_cq(struct ibv_context *context,
			 struct ibv_cq_init_attr_ex *attr)
{
	struct hihtr_context *hr_ctx = to_hr_ctx(context);
	struct hihtr_cq *cq;
	int ret;

	ret = verify_cq_create_attr(attr, hr_ctx);
	if (ret)
		goto err;

	cq = calloc(1, sizeof(*cq));
	if (!cq) {
		errno = ENOMEM;
		goto err;
	}

	ret = pthread_spin_init(&cq->lock, PTHREAD_PROCESS_PRIVATE);
	if (ret)
		goto err_lock;

	cq->cq_depth = attr->cqe;
	cq->cqe_size = hr_ctx->cqe_size;
	cq->resize_cq_en = HIHTR_CQ_RESIZE_CQ_DISABLE;

	if ((hr_ctx->config & HIHTR_RESIZE_CQ_FLAGS) && (cq->cq_depth < hr_ctx->resize_cq_hw_cq_depth)) {
		cq->resize_cq_en = HIHTR_CQ_RESIZE_CQ_ENABLE;
		cq->resize_cq_hw_cq_depth = hr_ctx->resize_cq_hw_cq_depth;
	}

	ret = hihtr_alloc_cq_buf(cq, hr_ctx);
	if (ret)
		goto err_buf;

	cq->db = hihtr_alloc_db(hr_ctx, HIHTR_CQ_TYPE_DB);
	if (!cq->db) {
		ret = ENOMEM;
		goto err_db;
	}

	*cq->db = 0;

	ret = exec_cq_create_cmd(context, cq, attr);
	if (ret)
		goto err_cmd;

	cq->arm_sn = 1;

	return &cq->verbs_cq.cq_ex;

err_cmd:
	hihtr_free_db(hr_ctx, cq->db, HIHTR_CQ_TYPE_DB);
err_db:
	hihtr_free_buf(&cq->buf);
err_lock:
err_buf:
	free(cq);
err:
	if (ret < 0)
		ret = -ret;

	errno = ret;
	return NULL;
}

struct ibv_cq *hihtr_u_create_cq(struct ibv_context *context, int cqe,
				    struct ibv_comp_channel *channel,
				    int comp_vector)
{
	struct ibv_cq_ex *cq;
	struct ibv_cq_init_attr_ex attr = {
		.cqe = cqe,
		.channel = channel,
		.comp_vector = comp_vector,
	};

	cq = create_cq(context, &attr);
	return cq ? ibv_cq_ex_to_cq(cq) : NULL;
}

struct ibv_cq_ex *hihtr_u_create_cq_ex(struct ibv_context *context,
					  struct ibv_cq_init_attr_ex *attr)
{
	struct ibv_cq_ex *cq;

	cq = create_cq(context, attr);
	if (cq)
		hihtr_attach_cq_ex_ops(cq, attr->wc_flags);

	return cq;
}

void hihtr_u_cq_event(struct ibv_cq *cq)
{
	to_hr_cq(cq)->arm_sn++;
}

int hihtr_u_modify_cq(struct ibv_cq *cq, struct ibv_modify_cq_attr *attr)
{
	struct ibv_modify_cq cmd = {};

	return ibv_cmd_modify_cq(cq, attr, &cmd, sizeof(cmd));
}

int hihtr_u_destroy_cq(struct ibv_cq *cq)
{
	int ret;

	ret = ibv_cmd_destroy_cq(cq);
	if (ret)
		return ret;

	hihtr_free_db(to_hr_ctx(cq->context), to_hr_cq(cq)->db,
				   HIHTR_CQ_TYPE_DB);
	hihtr_free_buf(&to_hr_cq(cq)->buf);
	free(to_hr_cq(cq));

	return ret;
}

static int hihtr_u_resize_check(struct hihtr_cq *hr_cq, unsigned int cqe)
{
	if ((hr_cq->resize_cq_en == HIHTR_CQ_RESIZE_CQ_DISABLE) || (cqe <= hr_cq->cq_depth) || (cqe > hr_cq->resize_cq_hw_cq_depth)) {
        return -EINVAL;
	}
	return 0;
}

int hihtr_u_resize_cq(struct ibv_cq *cq, int cqe)
{
	struct hihtr_cq *hr_cq = to_hr_cq(cq);
	int ret;
	struct ibv_resize_cq cmd = {};
	struct ib_uverbs_resize_cq_resp resp;
	unsigned int resize_cq_depth = max_t(unsigned int, (unsigned int)HIHTR_MIN_CQE_NUM, (unsigned int)roundup_pow_of_two((unsigned int)cqe));

	ret = hihtr_u_resize_check(hr_cq, resize_cq_depth);
    if (ret != 0) {
        return ret;
    }

    (void)pthread_spin_lock(&hr_cq->lock);

    ret = ibv_cmd_resize_cq(cq, (int32_t)resize_cq_depth, &cmd, sizeof(cmd), &resp, sizeof(resp));
    if (ret != 0) {
        goto err_cq;
    }
	hr_cq->cq_depth = resize_cq_depth;
    (void)pthread_spin_unlock(&hr_cq->lock);

    return ret;
err_cq:
    (void)pthread_spin_unlock(&hr_cq->lock);

    return ret;
}

static int hihtr_store_srq(struct hihtr_context *ctx,
			      struct hihtr_srq *srq)
{
	uint32_t tind = to_hr_srq_table_index(srq->srqn, ctx);

	pthread_mutex_lock(&ctx->srq_table_mutex);

	if (!ctx->srq_table[tind].refcnt) {
		ctx->srq_table[tind].table =
					calloc(ctx->srq_table_mask + 1,
					       sizeof(struct hihtr_srq *));
		if (!ctx->srq_table[tind].table) {
			pthread_mutex_unlock(&ctx->srq_table_mutex);
			return -ENOMEM;
		}
	}

	++ctx->srq_table[tind].refcnt;
	ctx->srq_table[tind].table[srq->srqn & ctx->srq_table_mask] = srq;

	pthread_mutex_unlock(&ctx->srq_table_mutex);

	return 0;
}

struct hihtr_srq *hihtr_find_srq(struct hihtr_context *ctx,
				       uint32_t srqn)
{
	uint32_t tind = to_hr_srq_table_index(srqn, ctx);
	if (ctx->srq_table[tind].refcnt)
		return ctx->srq_table[tind].table[srqn & ctx->srq_table_mask];
	else
		return NULL;
}

static void hihtr_clear_srq(struct hihtr_context *ctx, uint32_t srqn)
{
	uint32_t tind = to_hr_srq_table_index(srqn, ctx);

	pthread_mutex_lock(&ctx->srq_table_mutex);

	if (!--ctx->srq_table[tind].refcnt)
		free(ctx->srq_table[tind].table);
	else
		ctx->srq_table[tind].table[srqn & ctx->srq_table_mask] = NULL;

	pthread_mutex_unlock(&ctx->srq_table_mutex);
}

static int verify_srq_create_attr(struct hihtr_context *context,
				  struct ibv_srq_init_attr_ex *attr)
{
	if (attr->srq_type != IBV_SRQT_BASIC &&
	    attr->srq_type != IBV_SRQT_XRC)
		return -EINVAL;

	if (!attr->attr.max_sge ||
	    attr->attr.max_wr > context->max_srq_wr ||
	    attr->attr.max_sge > context->max_srq_sge)
		return -EINVAL;

	attr->attr.max_wr = max_t(uint32_t, attr->attr.max_wr,
				  HIHTR_MIN_SRQ_WQE_NUM);

	return 0;
}

static void set_srq_param(struct ibv_context *context, struct hihtr_srq *srq,
			  struct ibv_srq_init_attr_ex *attr)
{
	if (to_hr_dev(context->device)->hw_version == HIHTR_HW_VER2)
		srq->rsv_sge = 1;

	srq->wqe_cnt = roundup_pow_of_two(attr->attr.max_wr);
	srq->max_gs = roundup_pow_of_two(attr->attr.max_sge + srq->rsv_sge);
	srq->wqe_shift = hr_ilog32(roundup_pow_of_two(HIHTR_SGE_SIZE *
						      srq->max_gs));
	attr->attr.max_sge = srq->max_gs;
	attr->attr.srq_limit = 0;
}

static int alloc_srq_idx_que(struct hihtr_srq *srq)
{
	struct hihtr_idx_que	*idx_que = &srq->idx_que;
	unsigned int buf_size;
	int i;

	idx_que->entry_shift = hr_ilog32(HIHTR_IDX_QUE_ENTRY_SZ);
	idx_que->bitmap_cnt = align(srq->wqe_cnt, BIT_CNT_PER_LONG) /
				    BIT_CNT_PER_LONG;
	idx_que->bitmap = calloc(idx_que->bitmap_cnt, sizeof(unsigned long));
	if (!idx_que->bitmap)
		return -ENOMEM;

	buf_size = to_hr_hem_entries_size(srq->wqe_cnt, idx_que->entry_shift);
	if (hihtr_alloc_buf(&idx_que->buf, buf_size, HIHTR_HW_PAGE_SIZE)) {
		free(idx_que->bitmap);
		idx_que->bitmap = NULL;
		return -ENOMEM;
	}

	/* init the idx_que bitmap */
	for (i = 0; i < idx_que->bitmap_cnt; ++i)
		idx_que->bitmap[i] = ~(0UL);

	idx_que->head = 0;
	idx_que->tail = 0;

	return 0;
}

static int alloc_srq_wqe_buf(struct hihtr_srq *srq)
{
	int buf_size = to_hr_hem_entries_size(srq->wqe_cnt, srq->wqe_shift);

	return hihtr_alloc_buf(&srq->wqe_buf, buf_size, HIHTR_HW_PAGE_SIZE);
}

static int alloc_srq_buf(struct hihtr_srq *srq)
{
	int ret;

	ret = alloc_srq_idx_que(srq);
	if (ret)
		return ret;

	ret = alloc_srq_wqe_buf(srq);
	if (ret)
		goto err_idx_que;

	srq->wrid = calloc(srq->wqe_cnt, sizeof(*srq->wrid));
	if (srq->wrid == NULL) {
		ret = -ENOMEM;
		goto err_wqe_buf;
	}
	srq->signal = calloc(srq->wqe_cnt, sizeof(*srq->signal));
	if (srq->signal == NULL) {
		free(srq->wrid);
		ret = -ENOMEM;
		goto err_wqe_buf;
	}

	return 0;

err_wqe_buf:
	hihtr_free_buf(&srq->wqe_buf);
err_idx_que:
	hihtr_free_buf(&srq->idx_que.buf);
	free(srq->idx_que.bitmap);

	return ret;
}

static void free_srq_buf(struct hihtr_srq *srq)
{
	free(srq->wrid);
	free(srq->signal);
	hihtr_free_buf(&srq->wqe_buf);
	hihtr_free_buf(&srq->idx_que.buf);
	free(srq->idx_que.bitmap);
}

static int exec_srq_create_cmd(struct ibv_context *context,
			       struct hihtr_srq *srq,
			       struct ibv_srq_init_attr_ex *init_attr)
{
	struct hihtr_create_srq_ex_resp resp_ex = {};
	struct hihtr_create_srq_ex cmd_ex = {};
	int ret;

	cmd_ex.buf_addr = (uintptr_t)srq->wqe_buf.buf;
	cmd_ex.que_addr = (uintptr_t)srq->idx_que.buf.buf;
	cmd_ex.db_addr = (uintptr_t)srq->db;

	cmd_ex.req_cap_flags |= HIHTR_SRQ_CAP_RECORD_DB;

#if defined(CREATE_SRQ_EX_HAVE_VQP_SZ)
	ret = ibv_cmd_create_srq_ex(context, &srq->verbs_srq, sizeof(&srq->verbs_srq), init_attr,
				    &cmd_ex.ibv_cmd, sizeof(cmd_ex),
				    &resp_ex.ibv_resp, sizeof(resp_ex));
#else
	ret = ibv_cmd_create_srq_ex(context, &srq->verbs_srq, init_attr,
				    &cmd_ex.ibv_cmd, sizeof(cmd_ex),
				    &resp_ex.ibv_resp, sizeof(resp_ex));
#endif
	if (ret)
		return ret;

	srq->srqn = resp_ex.srqn;
	srq->cap_flags = resp_ex.cap_flags;

	return 0;
}

static struct ibv_srq *create_srq(struct ibv_context *context,
				  struct ibv_srq_init_attr_ex *init_attr)
{
	struct hihtr_context *hr_ctx = to_hr_ctx(context);
	struct hihtr_srq *srq;
	int ret;

	ret = verify_srq_create_attr(hr_ctx, init_attr);
	if (ret)
		goto err;

	srq = calloc(1, sizeof(*srq));
	if (!srq) {
		ret = -ENOMEM;
		goto err;
	}

	if (pthread_spin_init(&srq->lock, PTHREAD_PROCESS_PRIVATE))
		goto err_free_srq;

	set_srq_param(context, srq, init_attr);
	if (alloc_srq_buf(srq))
		goto err_free_srq;

	srq->db = hihtr_alloc_db(hr_ctx, HIHTR_SRQ_TYPE_DB);
	if (!srq->db)
		goto err_srq_buf;

	*srq->db = 0;

	ret = exec_srq_create_cmd(context, srq, init_attr);
	if (ret)
		goto err_srq_db;

	ret = hihtr_store_srq(hr_ctx, srq);
	if (ret)
		goto err_destroy_srq;

	srq->max_gs = init_attr->attr.max_sge;
	init_attr->attr.max_sge =
		min(init_attr->attr.max_sge - srq->rsv_sge, hr_ctx->max_srq_sge);

	return &srq->verbs_srq.srq;

err_destroy_srq:
	ibv_cmd_destroy_srq(&srq->verbs_srq.srq);

err_srq_db:
	hihtr_free_db(hr_ctx, srq->db, HIHTR_SRQ_TYPE_DB);

err_srq_buf:
	free_srq_buf(srq);

err_free_srq:
	free(srq);

err:
	if (ret < 0)
		ret = -ret;

	errno = ret;
	return NULL;
}

struct ibv_srq *hihtr_u_create_srq(struct ibv_pd *pd,
				      struct ibv_srq_init_attr *attr)
{
	struct ibv_srq_init_attr_ex attrx = {};
	struct ibv_srq *srq;

	memcpy(&attrx, attr, sizeof(*attr));
	attrx.comp_mask = IBV_SRQ_INIT_ATTR_PD;
	attrx.pd = pd;

	srq = create_srq(pd->context, &attrx);
	if (srq)
		memcpy(attr, &attrx, sizeof(*attr));

	return srq;
}

struct ibv_srq *hihtr_u_create_srq_ex(struct ibv_context *context,
					 struct ibv_srq_init_attr_ex *attr)
{
	return create_srq(context, attr);
}

int hihtr_u_get_srq_num(struct ibv_srq *ibv_srq, uint32_t *srq_num)
{
	*srq_num = to_hr_srq(ibv_srq)->srqn;

	return 0;
}

int hihtr_u_modify_srq(struct ibv_srq *srq, struct ibv_srq_attr *srq_attr,
			  int srq_attr_mask)
{
	struct ibv_modify_srq cmd;

	return ibv_cmd_modify_srq(srq, srq_attr, srq_attr_mask, &cmd,
				  sizeof(cmd));
}

int hihtr_u_query_srq(struct ibv_srq *srq, struct ibv_srq_attr *srq_attr)
{
	struct ibv_query_srq cmd;
	int ret;

	ret = ibv_cmd_query_srq(srq, srq_attr, &cmd, sizeof(cmd));
	srq_attr->max_sge -= to_hr_srq(srq)->rsv_sge;

	return ret;
}

int hihtr_u_destroy_srq(struct ibv_srq *ibv_srq)
{
	struct hihtr_context *ctx = to_hr_ctx(ibv_srq->context);
	struct hihtr_srq *srq = to_hr_srq(ibv_srq);
	int ret;

	ret = ibv_cmd_destroy_srq(ibv_srq);
	if (ret)
		return ret;

	hihtr_clear_srq(ctx, srq->srqn);

	hihtr_free_db(ctx, srq->db, HIHTR_SRQ_TYPE_DB);
	free_srq_buf(srq);
	free(srq);

	return 0;
}

enum {
	CREATE_QP_SUP_COMP_MASK = IBV_QP_INIT_ATTR_PD | IBV_QP_INIT_ATTR_XRCD |
				  IBV_QP_INIT_ATTR_SEND_OPS_FLAGS,
};

static int check_qp_create_mask(struct hihtr_context *ctx,
				struct ibv_qp_init_attr_ex *attr)
{
	struct hihtr_device *hr_dev = to_hr_dev(ctx->ibv_ctx.context.device);

	if (!check_comp_mask(attr->comp_mask, CREATE_QP_SUP_COMP_MASK))
		return -EOPNOTSUPP;

	switch (attr->qp_type) {
	case IBV_QPT_UD:
		if (hr_dev->hw_version == HIHTR_HW_VER2)
			return -EINVAL;
		SWITCH_FALLTHROUGH;
	case IBV_QPT_RC:
	case IBV_QPT_XRC_SEND:
		if (!(attr->comp_mask & IBV_QP_INIT_ATTR_PD))
			return -EINVAL;
		break;
	case IBV_QPT_XRC_RECV:
		if (!(attr->comp_mask & IBV_QP_INIT_ATTR_XRCD))
			return -EINVAL;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int hihtr_qp_has_rq(struct ibv_qp_init_attr_ex *attr)
{
	if (attr->qp_type == IBV_QPT_XRC_SEND ||
	    attr->qp_type == IBV_QPT_XRC_RECV || attr->srq)
		return 0;

	return 1;
}

static int verify_qp_create_cap(struct hihtr_context *ctx,
				struct ibv_qp_init_attr_ex *attr)
{
	struct ibv_qp_cap *cap = &attr->cap;
	uint32_t min_wqe_num;
	int has_rq;

	has_rq = hihtr_qp_has_rq(attr);
	if (!cap->max_send_wr && attr->qp_type != IBV_QPT_XRC_RECV)
		return -EINVAL;

	if (!has_rq) {
		if (cap->max_send_wr > ctx->max_sq_wr ||
			cap->max_recv_wr > ctx->max_srq_wr ||
			cap->max_send_sge > ctx->max_sge ||
			cap->max_recv_sge > ctx->max_sge)
			return -EINVAL;
	} else {
		if (cap->max_send_wr > ctx->max_sq_wr ||
	    	cap->max_recv_wr > ctx->max_rq_wr ||
	    	cap->max_send_sge > ctx->max_sge ||
	    	cap->max_recv_sge > ctx->max_sge)
			return -EINVAL;
	}

	min_wqe_num = HIHTR_V2_MIN_WQE_NUM;
	if (cap->max_send_wr < min_wqe_num)
		cap->max_send_wr = min_wqe_num;

	if (!has_rq) {
		cap->max_recv_wr = 0;
		cap->max_recv_sge = 0;
		return 0;
	}

	if ((!cap->max_recv_wr) || (!cap->max_recv_sge)) {
		return -EINVAL;
	}

	if (cap->max_recv_wr < min_wqe_num)
		cap->max_recv_wr = min_wqe_num;

	return 0;
}

static int verify_qp_create_attr(struct hihtr_context *ctx,
				 struct ibv_qp_init_attr_ex *attr)
{
	int ret;

	ret = check_qp_create_mask(ctx, attr);
	if (ret)
		return ret;

	return verify_qp_create_cap(ctx, attr);
}

static int qp_alloc_recv_inl_buf(struct ibv_qp_cap *cap,
				 struct hihtr_qp *qp)
{
	unsigned int cnt;
	int i;

	cnt = qp->rq_rinl_buf.wqe_cnt;
	qp->rq_rinl_buf.wqe_list = calloc(cnt,
					  sizeof(struct hihtr_rinl_wqe));
	if (!qp->rq_rinl_buf.wqe_list)
		return ENOMEM;

	qp->rq_rinl_buf.wqe_list[0].sg_list = calloc(cnt * cap->max_recv_sge,
					sizeof(struct hihtr_rinl_sge));
	if (!qp->rq_rinl_buf.wqe_list[0].sg_list)
		return ENOMEM;

	for (i = 0; i < cnt; i++) {
		int wqe_size = i * cap->max_recv_sge;

		qp->rq_rinl_buf.wqe_list[i].sg_list =
			  &(qp->rq_rinl_buf.wqe_list[0].sg_list[wqe_size]);
	}

	return 0;
}

static void qp_free_recv_inl_buf(struct hihtr_qp *qp)
{
	if (qp->rq_rinl_buf.wqe_list) {
		if (qp->rq_rinl_buf.wqe_list[0].sg_list) {
			free(qp->rq_rinl_buf.wqe_list[0].sg_list);
			qp->rq_rinl_buf.wqe_list[0].sg_list = NULL;
		}

		free(qp->rq_rinl_buf.wqe_list);
		qp->rq_rinl_buf.wqe_list = NULL;
	}
}

static int calc_qp_buff_size(struct hihtr_device *hr_dev,
			     struct hihtr_qp *qp, struct hihtr_context *ctx)
{
	struct hihtr_wq *sq = &qp->sq;
	struct hihtr_wq *rq = &qp->rq;
	unsigned int size;
	uint32_t page_size = 1 << ctx->qp_mtt_buf_pg_sz;

	qp->buf_size = 0;

	/* SQ WQE */
	sq->offset = 0;
	size = align(to_hr_hem_entries_size(sq->wqe_cnt, sq->wqe_shift), page_size);
	qp->buf_size += size;

	/* extend SGE WQE in SQ */
	qp->ex_sge.offset = qp->buf_size;
	if (qp->ex_sge.sge_cnt > 0) {
		size = align(to_hr_hem_entries_size(qp->ex_sge.sge_cnt, qp->ex_sge.sge_shift), page_size);
		qp->buf_size += size;
	}

	/* RQ WQE */
	rq->offset = qp->buf_size;
	size = align(to_hr_hem_entries_size(rq->wqe_cnt, rq->wqe_shift), page_size);
	qp->buf_size += size;

	if (qp->buf_size < 1)
		return EINVAL;

	return 0;
}

static void qp_free_wqe(struct hihtr_qp *qp)
{
	qp_free_recv_inl_buf(qp);
	if (qp->sq.wqe_cnt) {
		free(qp->sq.wrid);
		free(qp->sq.signal);
	}

	if (qp->rq.wqe_cnt)
		free(qp->rq.wrid);
	hihtr_free_buf(&qp->buf);
}

static bool is_dca_enabled_by_env_var(void)
{
	int unit_size = 0;
	char *env;

	env = getenv("HIHTR_DCA_UNIT_SIZE");
	if (env)
		unit_size = atoi(env);

	return unit_size != 0;
}

static inline bool check_qp_support_dca(struct hihtr_dca_ctx *dca_ctx,
					struct ibv_qp_init_attr_ex *attr,
					struct hnsdv_qp_init_attr *hihtr_attr)
{
	/* DCA pool disable */
	if (!dca_ctx->unit_size)
		return false;

	/* Unsupport type */
	if (attr->qp_type != IBV_QPT_RC && attr->qp_type != IBV_QPT_XRC_SEND)
		return false;

	if (hihtr_attr &&
	    (hihtr_attr->comp_mask & HNSDV_QP_INIT_ATTR_MASK_QP_CREATE_FLAGS) &&
	    (hihtr_attr->create_flags & HNSDV_QP_CREATE_ENABLE_DCA_MODE))
		return dca_ctx->max_size > 0;

	if (is_dca_enabled_by_env_var())
		return dca_ctx->max_size > 0;

	return false;
}

static int qp_alloc_wqe(struct ibv_qp_init_attr_ex *attr,
			struct hnsdv_qp_init_attr *hihtr_attr,
			struct hihtr_qp *qp,
			struct hihtr_context *ctx)
{
	struct hihtr_device *hr_dev = to_hr_dev(ctx->ibv_ctx.context.device);
	bool dca_en = check_qp_support_dca(&ctx->dca_ctx, attr, hihtr_attr);

	if (calc_qp_buff_size(hr_dev, qp, ctx))
		return -EINVAL;

	qp->sq.wrid = malloc(qp->sq.wqe_cnt * sizeof(uint64_t));
	if (!qp->sq.wrid)
		return -ENOMEM;
	qp->sq.signal = calloc(qp->sq.wqe_cnt, sizeof(uint64_t));
	if (!qp->sq.signal) {
		free(qp->sq.wrid);
		return -ENOMEM;
	}

	if (qp->rq.wqe_cnt) {
		qp->rq.wrid = malloc(qp->rq.wqe_cnt * sizeof(uint64_t));
		if (!qp->rq.wrid)
			goto err_alloc;
	}

	if (is_inline_enabled(ctx) && qp->rq_rinl_buf.wqe_cnt) {
		if (qp_alloc_recv_inl_buf(&attr->cap, qp))
			goto err_alloc;
	}

	if (dca_en) {
		/* when DCA is enabled, use a buffer list to store page addr */
		qp->buf.buf = NULL;
		qp->dca_wqe.max_cnt = hr_hw_page_count(qp->buf_size);
		qp->dca_wqe.shift = HIHTR_HW_PAGE_SHIFT;
		qp->dca_wqe.bufs = calloc(qp->dca_wqe.max_cnt, sizeof(void *));
		if (!qp->dca_wqe.bufs)
			goto err_alloc;
		verbs_debug(&ctx->ibv_ctx, "alloc DCA buf.\n");
	} else {
		if (hihtr_alloc_buf(&qp->buf, qp->buf_size, 1 << ctx->qp_mtt_buf_pg_sz))
			goto err_alloc;
	}

	return 0;

err_alloc:
	qp_free_recv_inl_buf(qp);
	if (qp->rq.wrid)
		free(qp->rq.wrid);

	if (qp->sq.wrid)
		free(qp->sq.wrid);

	if (qp->sq.signal)
		free(qp->sq.signal);

	return -ENOMEM;
}

static inline int get_sge_num_from_max_send_sge(bool is_ud, uint32_t max_send_sge)
{
	uint32_t std_sge_num = is_ud ? 0 : HIHTR_SGE_IN_WQE;
	uint32_t min_sge = is_ud ? 1 : 0;
	return max_send_sge > std_sge_num ? (max_send_sge - std_sge_num) : min_sge;
}

static inline uint32_t get_sge_num_from_max_inl_data(bool is_ud, uint32_t max_inline_data)
{
	uint32_t inline_sge = 0;

	inline_sge = roundup_pow_of_two(max_inline_data) / HIHTR_SGE_SIZE;
	if (!is_ud && (inline_sge <= HIHTR_SGE_IN_WQE)) {
		inline_sge = 0;
	}
	return inline_sge;
}

static inline void update_inline_data(struct hihtr_context *context, struct hihtr_qp *qp, struct ibv_qp_init_attr_ex *attr)
{
	uint32_t sge_num = qp->sq.ext_sge_cnt;
	if (context->config & HIHTR_EXSGE_FLAGS) {
		if (!(attr->qp_type == IBV_QPT_UD)) {
			sge_num = max((uint32_t)HIHTR_SGE_IN_WQE, sge_num);
		}
		attr->cap.max_inline_data = max(attr->cap.max_inline_data, sge_num * HIHTR_SGE_SIZE);
	}
	qp->max_inline_data = attr->cap.max_inline_data;
}

static void set_ext_sge_param(struct hihtr_context *context, struct hihtr_device *hr_dev,
			      struct ibv_qp_init_attr_ex *attr,
			      struct hihtr_qp *qp, unsigned int wr_cnt)
{
	bool is_ud = (attr->qp_type == IBV_QPT_UD);
	uint32_t std_sge_num = 0;
	uint32_t inline_ext_sge = 0;
	uint32_t ext_wqe_sge_cnt = 0;
	unsigned int total_sge_cnt;

	if (attr->cap.max_inline_data) {
		attr->cap.max_inline_data = roundup_pow_of_two(attr->cap.max_inline_data);
		attr->cap.max_inline_data = min(attr->cap.max_inline_data, context->max_inline_data);
	}
	std_sge_num = is_ud ? 0 : HIHTR_SGE_IN_WQE;
	qp->ex_sge.sge_shift = HIHTR_SGE_SHIFT;
	ext_wqe_sge_cnt = get_sge_num_from_max_send_sge(is_ud, attr->cap.max_send_sge);

	if (context->config & HIHTR_EXSGE_FLAGS) {
		inline_ext_sge = max(ext_wqe_sge_cnt, get_sge_num_from_max_inl_data(is_ud, attr->cap.max_inline_data));
		ext_wqe_sge_cnt = qp->sq.ext_sge_cnt = !!(inline_ext_sge) ? roundup_pow_of_two(inline_ext_sge) : 0;
		qp->sq.max_gs = max((uint32_t)(1), (ext_wqe_sge_cnt + std_sge_num));
		qp->sq.max_gs = min(qp->sq.max_gs, context->max_sge);
	} else {
		qp->sq.max_gs = max((uint32_t)(1), attr->cap.max_send_sge);
		qp->sq.max_gs = min(qp->sq.max_gs, context->max_sge);
		qp->sq.ext_sge_cnt = qp->sq.max_gs;
	}

	/* If the number of extended sge is not zero, they MUST use the
	 * space of HIHTR_HW_PAGE_SIZE at least.
	 */
	if (ext_wqe_sge_cnt) {
		total_sge_cnt = roundup_pow_of_two(wr_cnt * ext_wqe_sge_cnt);
		qp->ex_sge.sge_cnt = max(total_sge_cnt, (unsigned int)HIHTR_HW_PAGE_SIZE / HIHTR_SGE_SIZE);
	}

	update_inline_data(context, qp, attr);
}

static void hihtr_set_qp_params(struct hihtr_context *context, struct ibv_qp_init_attr_ex *attr,
				   struct hihtr_qp *qp,
				   struct hihtr_context *ctx)
{
	struct hihtr_device *hr_dev = to_hr_dev(ctx->ibv_ctx.context.device);
	unsigned int cnt;

	qp->verbs_qp.qp.qp_type = attr->qp_type;

	if (attr->cap.max_recv_wr) {
		if (hr_dev->hw_version == HIHTR_HW_VER2)
			qp->rq.rsv_sge = 1;

		qp->rq.max_gs = roundup_pow_of_two(attr->cap.max_recv_sge +
						   qp->rq.rsv_sge);
		qp->rq.wqe_shift = hr_ilog32(HIHTR_SGE_SIZE * qp->rq.max_gs);
		cnt = roundup_pow_of_two(attr->cap.max_recv_wr);
		qp->rq.wqe_cnt = cnt;
		qp->rq.shift = hr_ilog32(cnt);

		if (!is_inline_enabled(context)) {
			qp->rq_rinl_buf.wqe_cnt = 0;
		} else {
			qp->rq_rinl_buf.wqe_cnt = cnt;
		}

		attr->cap.max_recv_wr = qp->rq.wqe_cnt;
		attr->cap.max_recv_sge = qp->rq.max_gs;
	}

	if (attr->cap.max_send_wr) {
		qp->sq.wqe_shift = HIHTR_SQWQE_SHIFT;
		// "<< 1" 规避DTS2026031716563，详情看问题单
		cnt = roundup_pow_of_two(attr->cap.max_send_wr) << 1;
		qp->sq.wqe_cnt = cnt;
		qp->sq.shift = hr_ilog32(cnt);

		// sge初始化不感知深度变化
		cnt = roundup_pow_of_two(attr->cap.max_send_wr);

		set_ext_sge_param(context, hr_dev, attr, qp, cnt);

		qp->sq.max_post = min(ctx->max_sq_wr, cnt);
		qp->sq.max_gs = min(ctx->max_sge, qp->sq.max_gs);

		qp->sq_signal_bits = attr->sq_sig_all ? 0 : 1;

		attr->cap.max_send_wr = qp->sq.max_post;
	}
}

static void qp_free_db(struct hihtr_qp *qp, struct hihtr_context *ctx)
{
	if (qp->sdb)
		hihtr_free_db(ctx, qp->sdb, HIHTR_QP_TYPE_DB);

	if (qp->rdb)
		hihtr_free_db(ctx, qp->rdb, HIHTR_QP_TYPE_DB);
}

static int qp_alloc_db(struct ibv_qp_init_attr_ex *attr, struct hihtr_qp *qp,
		       struct hihtr_context *ctx)
{
	if (attr->cap.max_send_wr) {
		qp->sdb = hihtr_alloc_db(ctx, HIHTR_QP_TYPE_DB);
		if (!qp->sdb)
			return -ENOMEM;

		*qp->sdb = 0;
	}

	if (attr->cap.max_recv_sge) {
		qp->rdb = hihtr_alloc_db(ctx, HIHTR_QP_TYPE_DB);
		if (!qp->rdb) {
			if (qp->sdb)
				hihtr_free_db(ctx, qp->sdb,
						 HIHTR_QP_TYPE_DB);

			return -ENOMEM;
		}

		*qp->rdb = 0;
	}

	return 0;
}

static int hihtr_store_qp(struct hihtr_context *ctx,
			     struct hihtr_qp *qp)
{
	uint32_t qpn = qp->verbs_qp.qp.qp_num;
	uint32_t tind = to_hr_qp_table_index(qpn, ctx);

	pthread_mutex_lock(&ctx->qp_table_mutex);
	if (!ctx->qp_table[tind].refcnt) {
		ctx->qp_table[tind].table = calloc(ctx->qp_table_mask + 1,
						  sizeof(struct hihtr_qp *));
		if (!ctx->qp_table[tind].table) {
			pthread_mutex_unlock(&ctx->qp_table_mutex);
			return -ENOMEM;
		}
	}

	++qp->refcnt;
	++ctx->qp_table[tind].refcnt;
	ctx->qp_table[tind].table[qpn & ctx->qp_table_mask] = qp;
	pthread_mutex_unlock(&ctx->qp_table_mutex);

	return 0;
}

static int qp_exec_create_cmd(struct ibv_qp_init_attr_ex *attr,
			      struct hihtr_qp *qp,
			      struct hihtr_context *ctx,
			      struct hihtr_cmd_flag *cmd_flag)
{
	struct hihtr_create_qp_ex_resp resp_ex = {};
	struct hihtr_create_qp_ex cmd_ex = {};
	int ret;

	cmd_ex.sdb_addr = (uintptr_t)qp->sdb;
	cmd_ex.db_addr = (uintptr_t)qp->rdb;
	cmd_ex.buf_addr = (uintptr_t)qp->buf.buf;
	cmd_ex.log_sq_stride = qp->sq.wqe_shift;
	cmd_ex.log_sq_bb_count = hr_ilog32(qp->sq.wqe_cnt);

	if (cmd_flag->create_flags) {
		cmd_ex.comp_mask |= HIHTR_CREATE_QP_MASK_CREATE_FLAGS;
		cmd_ex.create_flags = cmd_flag->create_flags;
	}

	if (cmd_flag->comp_mask & HIHTR_CREATE_QP_MASK_PROTOCOL_TYPE) {
		cmd_ex.comp_mask |= HIHTR_CREATE_QP_MASK_PROTOCOL_TYPE;
		cmd_ex.protocol_type = cmd_flag->protocol_type;
	}

	ret = compat_ibv_cmd_create_qp_ex2(&ctx->ibv_ctx.context, &qp->verbs_qp, sizeof(qp->verbs_qp), attr,
				    &cmd_ex.ibv_cmd, sizeof(cmd_ex), sizeof(cmd_ex.ibv_cmd),
				    &resp_ex.ibv_resp, sizeof(resp_ex), sizeof(resp_ex.ibv_resp));

	qp->flags = resp_ex.drv_payload.cap_flags;

	return ret;
}

static void qp_setup_config(struct ibv_qp_init_attr_ex *attr,
			    struct hihtr_qp *qp,
			    struct hihtr_context *ctx)
{
	hihtr_init_qp_indices(qp);

	if (qp->rq.wqe_cnt) {
		qp->rq.wqe_cnt = attr->cap.max_recv_wr;
		qp->rq.max_gs = attr->cap.max_recv_sge;

		/* adjust the RQ's cap based on the reported device's cap */
		attr->cap.max_recv_wr =
			min(ctx->max_rq_wr, attr->cap.max_recv_wr);
		attr->cap.max_recv_sge -= qp->rq.rsv_sge;
		qp->rq.max_post = attr->cap.max_recv_wr;
	}

	qp->max_inline_data = attr->cap.max_inline_data;
	qp->sq.db_reg =ctx->db_page;
}

void hihtr_free_qp_buf(struct hihtr_qp *qp, struct hihtr_context *ctx)
{
	qp_free_db(qp, ctx);
	qp_free_wqe(qp);
}

static int hihtr_alloc_qp_buf(struct ibv_qp_init_attr_ex *attr,
				 struct hnsdv_qp_init_attr *hihtr_attr,
				 struct hihtr_qp *qp,
				 struct hihtr_context *ctx)
{
	int ret;

	if (pthread_spin_init(&qp->sq.lock, PTHREAD_PROCESS_PRIVATE) ||
	    pthread_spin_init(&qp->rq.lock, PTHREAD_PROCESS_PRIVATE))
		return -ENOMEM;

	ret = qp_alloc_wqe(attr, hihtr_attr, qp, ctx);
	if (ret)
		return ret;

	ret = qp_alloc_db(attr, qp, ctx);
	if (ret)
		qp_free_wqe(qp);

	return ret;
}

enum {
	HNSDV_QP_SUP_COMP_MASK = HNSDV_QP_INIT_ATTR_MASK_QP_CREATE_FLAGS |
				 HNSDV_QP_INIT_ATTR_MASK_QP_CONGEST_TYPE |
				 HNSDV_QP_INIT_ATTR_MASK_QP_PROTOCOL_TYPE |
				 HNSDV_QP_INIT_ATTR_MASK_QP_ADAPTIVE_ROUTING |
				 HNSDV_QP_INIT_ATTR_MASK_QP_OUT_OF_ORDER_RECEIVE |
				 HNSDV_QP_INIT_ATTR_MASK_QP_DYNAMIC_RTT,
};

static int check_hnsdv_qp_mask(struct hihtr_context *ctx,
				struct hnsdv_qp_init_attr *hihtr_attr)
{
	if (!check_comp_mask(hihtr_attr->comp_mask, HNSDV_QP_SUP_COMP_MASK)) {
		verbs_err(&ctx->ibv_ctx, "invalid hnsdv comp_mask 0x%x.\n",
			  hihtr_attr->comp_mask);
		return -EINVAL;
	}

	if ((hihtr_attr->comp_mask & HNSDV_QP_INIT_ATTR_MASK_QP_CREATE_FLAGS) &&
	    !check_comp_mask(hihtr_attr->create_flags,
			     HNSDV_QP_CREATE_ENABLE_DCA_MODE)) {
		verbs_err(&ctx->ibv_ctx, "invalid create_flags 0x%x.\n",
			  hihtr_attr->create_flags);
		return -EINVAL;
	}

	return 0;
}

static int verify_qp_hnsdv_attr(struct hihtr_context *ctx,
				 struct hnsdv_qp_init_attr *hihtr_attr,
				 struct hihtr_cmd_flag *cmd_flag)
{
	if (!hihtr_attr)
		return 0;

	return check_hnsdv_qp_mask(ctx, hihtr_attr);
}

static struct ibv_qp *create_qp(struct ibv_context *ibv_ctx,
				struct ibv_qp_init_attr_ex *attr,
				struct hnsdv_qp_init_attr *hihtr_attr)
{
	struct hihtr_context *context = to_hr_ctx(ibv_ctx);
	struct hihtr_cmd_flag cmd_flag = {};
	struct hihtr_qp *qp;
	int ret;

	ret = verify_qp_create_attr(context, attr);
	if (ret)
		goto err;

	ret = verify_qp_hnsdv_attr(context, hihtr_attr, &cmd_flag);

	qp = calloc(1, sizeof(*qp));
	if (!qp) {
		ret = -ENOMEM;
		goto err;
	}

	hihtr_set_qp_params(context, attr, qp, context);

	ret = hihtr_alloc_qp_buf(attr, hihtr_attr, qp, context);
	if (ret)
		goto err_buf;

	ret = qp_exec_create_cmd(attr, qp, context, &cmd_flag);
	if (ret)
		goto err_cmd;

#if defined(VERBS_HAVE_IBV_QP_EX)
	ret = hihtr_attach_qp_ex_ops(attr, qp);
	if (ret)
		goto err_ops;
#endif

	ret = hihtr_store_qp(context, qp);
	if (ret)
		goto err_ops;

	qp_setup_config(attr, qp, context);

	return &qp->verbs_qp.qp;

err_ops:
	ibv_cmd_destroy_qp(&qp->verbs_qp.qp);
err_cmd:
	hihtr_free_qp_buf(qp, context);
err_buf:
	free(qp);
err:
	if (ret < 0)
		ret = -ret;

	errno = ret;
	return NULL;
}

struct ibv_qp *hihtr_u_create_qp(struct ibv_pd *pd,
				    struct ibv_qp_init_attr *attr)
{
	struct ibv_qp_init_attr_ex attrx = {};
	struct ibv_qp *qp;

	memcpy(&attrx, attr, sizeof(*attr));
	attrx.comp_mask = IBV_QP_INIT_ATTR_PD;
	attrx.pd = pd;

	qp = create_qp(pd->context, &attrx, NULL);
	if (qp)
		memcpy(attr, &attrx, sizeof(*attr));

	return qp;
}

struct ibv_qp *hihtr_u_create_qp_ex(struct ibv_context *context,
				       struct ibv_qp_init_attr_ex *attr)
{
	return create_qp(context, attr, NULL);
}

struct ibv_qp *hihtr_u_open_qp(struct ibv_context *context,
				  struct ibv_qp_open_attr *attr)
{
	struct ib_uverbs_create_qp_resp resp;
	struct ibv_open_qp cmd;
	struct hihtr_qp *qp;
	int ret;

	qp = calloc(1, sizeof(*qp));
	if (!qp)
		return NULL;

	ret = ibv_cmd_open_qp(context, &qp->verbs_qp, sizeof(qp->verbs_qp),
			      attr, &cmd, sizeof(cmd), &resp, sizeof(resp));
	if (ret)
		goto err_buf;

	ret = hihtr_store_qp(to_hr_ctx(context), qp);
	if (ret)
		goto err_cmd;

	return &qp->verbs_qp.qp;

err_cmd:
	ibv_cmd_destroy_qp(&qp->verbs_qp.qp);
err_buf:
	free(qp);
	return NULL;
}

int hihtr_u_query_qp(struct ibv_qp *ibqp, struct ibv_qp_attr *attr,
			int attr_mask, struct ibv_qp_init_attr *init_attr)
{
	struct hihtr_qp *qp = to_hr_qp(ibqp);
	struct ibv_query_qp cmd;
	int ret;

	ret = ibv_cmd_query_qp(ibqp, attr, attr_mask, init_attr, &cmd,
			       sizeof(cmd));
	if (ret)
		return ret;

	init_attr->cap.max_send_wr = qp->sq.max_post;
	init_attr->cap.max_send_sge = qp->sq.max_gs;

	if (init_attr->cap.max_recv_wr)
		init_attr->cap.max_recv_sge -= qp->rq.rsv_sge;

	attr->cap = init_attr->cap;

	return ret;
}

static uint16_t get_ah_udp_sport(const struct ibv_ah_attr *attr)
{
	uint32_t fl = attr->grh.flow_label & IB_GRH_FLOWLABEL_MASK;
	uint16_t sport;

	if (!fl)
		sport = get_random() % (IB_ROCE_UDP_ENCAP_VALID_PORT_MAX + 1 -
					IB_ROCE_UDP_ENCAP_VALID_PORT_MIN) +
			IB_ROCE_UDP_ENCAP_VALID_PORT_MIN;
	else
		sport = ibv_flow_label_to_udp_sport(fl);

	return sport;
}

static int get_dscp_from_tclass(struct ibv_context *context, struct ibv_ah_attr *attr,
		      uint8_t *tclass)
{
#define DSCP_SHIFT 2
#if defined(NEED_IBV_GID_TYPE_SYSFS)
	enum ibv_gid_type gid_type;
#else
	enum ibv_gid_type_sysfs gid_type;
#endif
	int ret;

	ret = ibv_query_gid_type(context, attr->port_num, attr->grh.sgid_index,
				 &gid_type);
	if (ret)
		return ret;

#if defined(NEED_IBV_GID_TYPE_SYSFS)
	*tclass = gid_type == IBV_GID_TYPE_ROCE_V2 ?
		  attr->grh.traffic_class >> DSCP_SHIFT :
		  attr->grh.traffic_class;
#else
	*tclass = gid_type == IBV_GID_TYPE_SYSFS_ROCE_V2 ?
		  attr->grh.traffic_class >> DSCP_SHIFT :
		  attr->grh.traffic_class;
#endif

	return ret;
}

struct ibv_ah *hihtr_u_create_ah(struct ibv_pd *pd, struct ibv_ah_attr *attr)
{
	struct hihtr_device *hr_dev = to_hr_dev(pd->context->device);
	struct hihtr_create_ah_resp resp = {};
	struct hihtr_ah *ah;
	uint16_t *vid;
	int err;

	/* HIP08 don't support create ah */
	if (hr_dev->hw_version == HIHTR_HW_VER2)
		return NULL;

	ah = malloc(sizeof(*ah));
	if (!ah)
		return NULL;

	// 规避rdma-core 22.4里ibv_resolve_eth_l2_from_gid传NULL导致的Segmentation fault
	vid = malloc(sizeof(*vid));
	if (!vid)
		goto err_ah;

	memset(ah, 0, sizeof(*ah));

	ah->av.port = attr->port_num;

	if (attr->is_global) {
		ah->av.gid_index = attr->grh.sgid_index;
		ah->av.hop_limit = attr->grh.hop_limit;

		if (get_dscp_from_tclass(pd->context, attr, &ah->av.tclass))
			goto err_vid;

		ah->av.flowlabel = attr->grh.flow_label;

		memcpy(ah->av.dgid, attr->grh.dgid.raw, ARRAY_SIZE(ah->av.dgid));
	}

	err = ibv_cmd_create_ah(pd, &ah->ibv_ah, attr, &resp.ibv_resp, sizeof(resp));
	if (err)
		goto err_vid;

	if (ibv_resolve_eth_l2_from_gid(pd->context, attr, ah->av.mac, vid)) {
		goto err_resolve_eth;
	}

	ah->av.udp_sport = get_ah_udp_sport(attr);
	ah->av.sl = resp.drv_payload.priority;

	free(vid);

	return &ah->ibv_ah;

err_resolve_eth:
	(void)ibv_cmd_destroy_ah(&ah->ibv_ah);
err_vid:
	free(vid);
err_ah:
	free(ah);
	return NULL;
}

int hihtr_u_destroy_ah(struct ibv_ah *ah)
{
	int ret;

	ret = ibv_cmd_destroy_ah(ah);
	if (ret)
		return ret;

	free(to_hr_ah(ah));

	return 0;
}

static void dump_hnsdv_qp_init(struct hihtr_context *ctx,
			       struct hnsdv_qp_init_attr *qp_attr)
{
	if (!qp_attr)
		return;
	verbs_debug(&ctx->ibv_ctx,
		    "create hnsdv qp, comp_mask: 0X%llx, create_flags: %llx!\n",
		    qp_attr->comp_mask, qp_attr->create_flags);
}

static void hnsdv_copy_qp_props(struct ibv_qp *qp, struct hnsdv_qp_props* props)
{
	struct hihtr_qp *hr_qp = to_hr_qp(qp);
	struct hihtr_context *ctx = to_hr_ctx(hr_qp->verbs_qp.qp.context);

	if (!props)
		return;

	props->buf = (void *)(&hr_qp->buf);
	props->sq = (void *)(&hr_qp->sq);
	props->rq = (void *)(&hr_qp->rq);
	props->sl = hr_qp->sl;
	props->rdb = hr_qp->rdb;
	props->sdb = hr_qp->sdb;
	props->max_inline_data = (int)hr_qp->max_inline_data;
	props->ex_sge = (void *)(&hr_qp->ex_sge);
	props->next_sge = hr_qp->next_sge;
	props->flags = hr_qp->flags;
	props->sge_info = (void *)(&hr_qp->sge_info);
	props->rq_rinl_buf = (void *)(&hr_qp->rq_rinl_buf);
	props->sq_signal_bits = hr_qp->sq_signal_bits;
	props->db_reg = ctx->uar;
}

struct ibv_qp *hnsdv_create_qp(struct ibv_context *context,
			       struct ibv_qp_init_attr_ex *qp_attr,
			       struct hnsdv_qp_init_attr *hihtr_attr)
{
	struct hihtr_context *ctx = context ? to_hr_ctx(context) : NULL;
	struct hnsdv_qp_init_attr *hnsdv_attr = hihtr_attr;
	bool need_ret = false;
	struct ibv_qp *qp;

	if (!ctx || !qp_attr) {
		errno = EINVAL;
		return NULL;
	}

	if (!is_hihtr_dev(context->device)) {
		errno = EOPNOTSUPP;
		return NULL;
	}

	if (hihtr_attr &&
	    (hihtr_attr->comp_mask & HNSDV_QP_INIT_ATTR_MASK_QP_RET_ATTR)) {
		need_ret = true;
		hnsdv_attr = NULL;
	}

	dump_hnsdv_qp_init(ctx, hihtr_attr);
	qp = create_qp(context, qp_attr, hnsdv_attr);
	if (qp != NULL && need_ret)
		hnsdv_copy_qp_props(qp, hihtr_attr->hihtr_qp_props);

	return qp;
}

int hihtr_u_attach_mcast(struct ibv_qp *qp, const union ibv_gid *gid, uint16_t lid)
{
	return ibv_cmd_attach_mcast(qp, gid, lid);
}

int hihtr_u_detach_mcast(struct ibv_qp *qp, const union ibv_gid *gid, uint16_t lid)
{
	return ibv_cmd_detach_mcast(qp, gid, lid);
}