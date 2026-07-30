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
#include <pthread.h>
#include <sys/mman.h>
#include <unistd.h>
#include <syslog.h>
#include <stdbool.h>

#include "hihtr_u.h"
#include "hihtr_shared.h"

static bool g_syslog_inited = false;

static void plog_close(void)
{
    if (g_syslog_inited) {
        syslog(LOG_INFO, "hihtr driver closed");
        closelog();
        g_syslog_inited = false;
    }
}

static int plog_init(void)
{
    if (g_syslog_inited)
        return 0;
    
    openlog("hihtr", LOG_PID, LOG_USER);
    setlogmask(LOG_UPTO(LOG_DEBUG));
    syslog(LOG_INFO, "hihtr driver initialized");
    
    atexit(plog_close);
    g_syslog_inited = true;
    
    return 0;
}

static void hihtr_free_context(struct ibv_context *ibctx);

#ifndef PCI_VENDOR_ID_HUAWEI
#define PCI_VENDOR_ID_HUAWEI			0x19E5
#endif

#define min_t(t, a, b) (((a)>(b))?(b):(a))

static const struct verbs_match_ent hca_table[] = {
#ifdef HAVE_VERBS_DRIVER_ID
    VERBS_DRIVER_ID(RDMA_DRIVER_HIHTR),
#endif
	VERBS_PCI_MATCH(PCI_VENDOR_ID_HUAWEI, 0x0229, &hihtr_u_hw_v2),
	VERBS_PCI_MATCH(PCI_VENDOR_ID_HUAWEI, 0x022a, &hihtr_u_hw_v2),
	{}
};

static const struct verbs_context_ops hihtr_common_ops = {
	.alloc_mw = hihtr_u_alloc_mw,
	.alloc_pd = hihtr_u_alloc_pd,
	.bind_mw = hihtr_u_bind_mw,
	.cq_event = hihtr_u_cq_event,
	.create_cq = hihtr_u_create_cq,
	.create_cq_ex = hihtr_u_create_cq_ex,
	.create_qp = hihtr_u_create_qp,
	.create_qp_ex = hihtr_u_create_qp_ex,
	.dealloc_mw = hihtr_u_dealloc_mw,
	.dealloc_pd = hihtr_u_free_pd,
	.dereg_mr = hihtr_u_dereg_mr,
	.destroy_cq = hihtr_u_destroy_cq,
	.modify_cq = hihtr_u_modify_cq,
	.query_device_ex = hihtr_u_query_device_ex,
#if defined(VERBS_OPS_HAVE_QUERY_DEVICE)
	.query_device = hihtr_u_query_device,
#endif
	.query_port = hihtr_u_query_port,
	.query_qp = hihtr_u_query_qp,
	.reg_mr = hihtr_u_reg_mr,
	.rereg_mr = hihtr_u_rereg_mr,
	.create_srq = hihtr_u_create_srq,
	.create_srq_ex = hihtr_u_create_srq_ex,
	.modify_srq = hihtr_u_modify_srq,
	.query_srq = hihtr_u_query_srq,
	.destroy_srq = hihtr_u_destroy_srq,
#if defined(CONTEXT_OPS_HAVE_FREE_CTX)
	.free_context = hihtr_free_context,
#endif
	.create_ah = hihtr_u_create_ah,
	.destroy_ah = hihtr_u_destroy_ah,
	.open_xrcd = hihtr_u_open_xrcd,
	.close_xrcd = hihtr_u_close_xrcd,
	.open_qp = hihtr_u_open_qp,
	.get_srq_num = hihtr_u_get_srq_num,
	.attach_mcast = hihtr_u_attach_mcast,
	.detach_mcast = hihtr_u_detach_mcast,
};

static const struct verbs_context_ops hihtr_resize_cq_ops = {
	.resize_cq = hihtr_u_resize_cq,
};

static uint32_t calc_table_shift(uint32_t entry_count, uint32_t size_shift)
{
	uint32_t count_shift = hr_ilog32(entry_count);

	return count_shift > size_shift ? count_shift - size_shift : 0;
}

static void ucontext_set_cmd(struct hihtr_alloc_ucontext *cmd,
			     struct hnsdv_context_attr *attr)
{
	cmd->config |= HIHTR_EXSGE_FLAGS | HIHTR_RQ_INLINE_FLAGS |
		       HIHTR_CQE_INLINE_FLAGS | HIHTR_UCTX_DYN_QP_PGSZ;

	if (!attr || !(attr->flags & HNSDV_CONTEXT_FLAGS_DCA))
		return;

	cmd->config |= HIHTR_UCTX_CONFIG_DCA;

	if (attr->comp_mask & HNSDV_CONTEXT_MASK_DCA_PRIME_QPS) {
		cmd->comp |= HIHTR_ALLOC_UCTX_COMP_DCA_MAX_QPS;
		cmd->dca_max_qps = attr->dca_prime_qps;
	}
}

static int mmap_dca(struct hihtr_context *ctx, int cmd_fd,
		    int page_size, size_t size, uint64_t mmap_key)
{
	struct hihtr_dca_ctx *dca_ctx = &ctx->dca_ctx;
	void *addr;

	addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, cmd_fd,
		    mmap_key);
	if (addr == MAP_FAILED) {
		verbs_err(&ctx->ibv_ctx, "failed to mmap() dca prime qp.\n");
		return -EINVAL;
	}

	dca_ctx->buf_status = addr;
	dca_ctx->sync_status = addr + size / DCA_BUF_SYNC_RATIO;

	return 0;
}

static void load_dca_config_from_env_var(struct hnsdv_context_attr *attr,
					 int page_size)
{
	int unit_size = 0;
	long max_size = 0;
	long min_size;
	char *env;

	env = getenv("HIHTR_DCA_UNIT_SIZE");
	if (env)
		unit_size = atoi(env);

	/* Disable DCA only for this process */
	if (unit_size == 0)
		return;

	attr->flags |= HNSDV_CONTEXT_FLAGS_DCA;

	if (unit_size > 0) {
		attr->comp_mask |= HNSDV_CONTEXT_MASK_DCA_UNIT_SIZE;
		attr->dca_unit_size = unit_size;
	}

	/*
	 * not set OR 0: Unlimited memory pool increase.
	 * others: Maximum memory pool size to be increased.
	 */
	env = getenv("HIHTR_DCA_MAX_SIZE");
	if (env)
		max_size = atol(env);
	if (max_size > 0) {
		attr->comp_mask |= HNSDV_CONTEXT_MASK_DCA_MAX_SIZE;
		attr->dca_max_size = max_size;
	}

	/*
	 * not set: The memory pool cannot be reduced.
	 * others: The size of free memory in the pool cannot exceed this value.
	 * 0: Always reduce the free memory in the pool.
	 */
	env = getenv("HIHTR_DCA_MIN_SIZE");
	if (env) {
		min_size = atol(env);
		attr->comp_mask |= HNSDV_CONTEXT_MASK_DCA_MIN_SIZE;
		if (min_size > 0)
			attr->dca_min_size = min_size;
		else
			attr->dca_min_size = 0;
	}

	env = getenv("HIHTR_DCA_PRIME_QP_NUM");
	if (env) {
		attr->comp_mask |= HNSDV_CONTEXT_MASK_DCA_PRIME_QPS;
		attr->dca_prime_qps = atoi(env);
	}
}

static void set_dca_pool_param(struct hihtr_context *ctx,
			       struct hnsdv_context_attr *attr, int page_size)
{
	struct hihtr_dca_ctx *dca_ctx = &ctx->dca_ctx;

	if (attr->comp_mask & HNSDV_CONTEXT_MASK_DCA_UNIT_SIZE)
		dca_ctx->unit_size = align(attr->dca_unit_size, page_size);
	else
		dca_ctx->unit_size = page_size * HIHTR_DCA_DEFAULT_UNIT_PAGES;

	/* The memory pool cannot be expanded, only init the DCA context. */
	if (dca_ctx->unit_size == 0)
		return;

	/* If not set, the memory pool can be expanded unlimitedly. */
	if (attr->comp_mask & HNSDV_CONTEXT_MASK_DCA_MAX_SIZE)
		dca_ctx->max_size = DIV_ROUND_UP(attr->dca_max_size,
						 dca_ctx->unit_size) *
					dca_ctx->unit_size;
	else
		dca_ctx->max_size = HIHTR_DCA_MAX_MEM_SIZE;

	/* If not set, the memory pool cannot be shrunk. */
	if (attr->comp_mask & HNSDV_CONTEXT_MASK_DCA_MIN_SIZE)
		dca_ctx->min_size = DIV_ROUND_UP(attr->dca_min_size,
						 dca_ctx->unit_size) *
					dca_ctx->unit_size;
	else
		dca_ctx->min_size = HIHTR_DCA_MAX_MEM_SIZE;

	verbs_debug(&ctx->ibv_ctx,
		    "Support DCA, unit %u, max %lu, min %lu Bytes.\n",
		    dca_ctx->unit_size, dca_ctx->max_size, dca_ctx->min_size);
}

static int init_dca_context(struct hihtr_context *ctx, int cmd_fd,
			    struct hihtr_alloc_ucontext_resp *resp,
			    struct hnsdv_context_attr *attr,
			    int page_size)
{
	struct hihtr_dca_ctx *dca_ctx = &ctx->dca_ctx;
	uint64_t mmap_key = resp->dca_mmap_key;
	int mmap_size = resp->dca_mmap_size;
	int max_qps = resp->dca_qps;
	int ret;

	if (!(ctx->config & HIHTR_UCTX_RSP_DCA_FLAGS))
		return 0;

	dca_ctx->unit_size = 0;
	dca_ctx->mem_cnt = 0;

	list_head_init(&dca_ctx->mem_list);
	ret = pthread_spin_init(&dca_ctx->lock, PTHREAD_PROCESS_PRIVATE);
	if (ret) {
		verbs_err(&ctx->ibv_ctx, "failed to init DCA spin lock ret %d",
				  ret);
		return ret;
	}

	if (!attr || !(attr->flags & HNSDV_CONTEXT_FLAGS_DCA))
		return 0;

	set_dca_pool_param(ctx, attr, page_size);

	if (mmap_key) {
		const unsigned int bits_per_qp = 2 * HIHTR_DCA_BITS_PER_STATUS;

		if (!mmap_dca(ctx, cmd_fd, page_size, mmap_size, mmap_key)) {
			dca_ctx->status_size = mmap_size;
			dca_ctx->max_qps = min_t(int, max_qps,
				mmap_size *  8 / bits_per_qp);
		}
	}

	return 0;
}

static void uninit_dca_context(struct hihtr_context *ctx)
{
	struct hihtr_dca_ctx *dca_ctx = &ctx->dca_ctx;

	if (!(ctx->config & HIHTR_UCTX_RSP_DCA_FLAGS))
		return;

	pthread_spin_lock(&dca_ctx->lock);
	hihtr_cleanup_dca_mem(ctx);
	pthread_spin_unlock(&dca_ctx->lock);
	if (dca_ctx->buf_status)
		munmap(dca_ctx->buf_status, dca_ctx->status_size);

	pthread_spin_destroy(&dca_ctx->lock);
}

struct verbs_context *hihtr_alloc_context(struct ibv_device *ibdev,
						    int cmd_fd,
						    void *private_data)
{
	struct hnsdv_context_attr *ctx_attr = private_data;
	struct hihtr_device *hr_dev = to_hr_dev(ibdev);
	struct hnsdv_context_attr hnsdv_env_attr = {};
	struct hihtr_alloc_ucontext_resp resp = {};
	struct ibv_device_attr dev_attrs;
	struct hihtr_context *context;
	struct hihtr_alloc_ucontext cmd = {};
	int i;

	plog_init();

	context = verbs_init_and_alloc_context(ibdev, cmd_fd, context, ibv_ctx,
					       RDMA_DRIVER_HIHTR);
	if (!context)
		return NULL;

	/* temphack for DCA to load parameters from env */
	if (!ctx_attr) {
		load_dca_config_from_env_var(&hnsdv_env_attr, hr_dev->page_size);
		ctx_attr = &hnsdv_env_attr;
	}

	ucontext_set_cmd(&cmd, ctx_attr);
	if (ibv_cmd_get_context(&context->ibv_ctx, &cmd.ibv_cmd, sizeof(cmd),
#ifdef IBV_CMD_GET_CONTEXT_HAVE_IBV_FD_ARR
				NULL, &resp.ibv_resp, sizeof(resp)))
#else
				&resp.ibv_resp, sizeof(resp)))
#endif
		goto err_free;

	if (!resp.cqe_size)
		context->cqe_size = HIHTR_CQE_SIZE;
	else if (resp.cqe_size <= HIHTR_V3_CQE_SIZE)
		context->cqe_size = resp.cqe_size;
	else
		context->cqe_size = HIHTR_V3_CQE_SIZE;

	context->qp_table_shift = calc_table_shift(resp.qp_tab_size,
						   HIHTR_QP_TABLE_BITS);
	context->qp_table_mask = (1 << context->qp_table_shift) - 1;
	pthread_mutex_init(&context->qp_table_mutex, NULL);
	for (i = 0; i < HIHTR_QP_TABLE_SIZE; ++i)
		context->qp_table[i].refcnt = 0;

	context->srq_table_shift = calc_table_shift(resp.srq_tab_size,
						    HIHTR_SRQ_TABLE_BITS);
	context->srq_table_mask = (1 << context->srq_table_shift) - 1;
	pthread_mutex_init(&context->srq_table_mutex, NULL);
	for (i = 0; i < HIHTR_SRQ_TABLE_SIZE; ++i)
		context->srq_table[i].refcnt = 0;

	if (hihtr_u_query_device_ex(&context->ibv_ctx.context, NULL,
				    container_of(&dev_attrs,
						 struct ibv_device_attr_ex,
						 orig_attr),
				    sizeof(dev_attrs)))
		goto err_free;

	hr_dev->hw_version = dev_attrs.hw_ver;
	// context->max_sq_wr仅用于拦截用户态传的tx深度，与rx深度一致
	context->max_sq_wr = dev_attrs.max_qp_wr;
	context->max_rq_wr = dev_attrs.max_qp_wr;
	context->max_sge = dev_attrs.max_sge;
	context->max_cqe = dev_attrs.max_cqe;
	context->max_srq_wr = dev_attrs.max_srq_wr;
	context->max_srq_sge = dev_attrs.max_srq_sge;
	context->config = resp.config;
	context->max_inline_data = resp.max_inline_data;
	context->qp_mtt_buf_pg_sz = resp.qp_mtt_buf_pg_sz;
	context->cqe_mtt_buf_pg_sz = resp.cqe_mtt_buf_pg_sz;
	context->resize_cq_hw_cq_depth = resp.resize_cq_hw_cq_depth;

	context->u_error_cnt = (hihtr_history_cqe_error_count *)mmap(NULL, hr_dev->page_size,
		PROT_READ | PROT_WRITE, MAP_SHARED, context->ibv_ctx.context.cmd_fd, resp.stats_mmap_key);
	if (context->u_error_cnt == MAP_FAILED) {
		goto err_free;
	}

	context->db_page = mmap(NULL, HIHTR_DWQE_PAGE_SIZE, PROT_WRITE, MAP_SHARED,
		context->ibv_ctx.context.cmd_fd, resp.db_mmap_key);
	if (context->db_page == MAP_FAILED)
		goto cnt_mmap_free;

	if (init_dca_context(context, cmd_fd,
			     &resp, ctx_attr, hr_dev->page_size))
		goto db_mmap_free;

	context->uar = mmap(NULL, hr_dev->page_size, PROT_READ | PROT_WRITE,
			    MAP_SHARED, cmd_fd, 0);
	if (context->uar == MAP_FAILED)
		goto mmap_free;

	pthread_spin_init(&context->uar_lock, PTHREAD_PROCESS_PRIVATE);
	(void)pthread_spin_init(&context->dwqe_lock, PTHREAD_PROCESS_PRIVATE);

	verbs_set_ops(&context->ibv_ctx, &hihtr_common_ops);
	verbs_set_ops(&context->ibv_ctx, &hr_dev->u_hw->hw_ops);

	if (context->config & HIHTR_RESIZE_CQ_FLAGS) {
		verbs_set_ops(&context->ibv_ctx, &hihtr_resize_cq_ops);
	}

	return &context->ibv_ctx;

mmap_free:
	uninit_dca_context(context);
db_mmap_free:
	munmap(context->db_page, HIHTR_DWQE_PAGE_SIZE);
    context->db_page = NULL;
cnt_mmap_free:
	munmap(context->u_error_cnt, hr_dev->page_size);
    context->u_error_cnt = NULL;
err_free:
	verbs_uninit_context(&context->ibv_ctx);
	free(context);
	return NULL;
}

static void hihtr_free_context(struct ibv_context *ibctx)
{
	struct hihtr_device *hr_dev = to_hr_dev(ibctx->device);
	struct hihtr_context *context = to_hr_ctx(ibctx);

	munmap(context->db_page, HIHTR_DWQE_PAGE_SIZE);
	munmap(context->uar, hr_dev->page_size);
	munmap(context->u_error_cnt, hr_dev->page_size);
	uninit_dca_context(context);
	verbs_uninit_context(&context->ibv_ctx);
	free(context);
}

static void hihtr_uninit_device(struct verbs_device *verbs_device)
{
	struct hihtr_device *dev = to_hr_dev(&verbs_device->device);

	free(dev);
}

static struct verbs_device *hihtr_device_alloc(struct verbs_sysfs_dev *sysfs_dev)
{
	struct hihtr_device *dev;

	dev = calloc(1, sizeof(*dev));
	if (!dev)
		return NULL;

	if (sysfs_dev->match) {
		dev->u_hw = sysfs_dev->match->driver_data;
	} else {
		// 适配1650 ubus场景sys文件系统缺失部分文件，导致匹配失败
		dev->u_hw = &hihtr_u_hw_v2;
	}
	dev->hw_version = dev->u_hw->hw_version;
	dev->page_size = sysconf(_SC_PAGESIZE);
	return &dev->ibv_dev;
}

#ifdef __UBUS_DRIVER__
// 临时方案：在1650 os 6.6系统上 match_table匹配方式失效，因此暂时添加match_device方式匹配1872设备
static bool hihtr_match_device(struct verbs_sysfs_dev *sysfs_dev)
{
	return true;
}
#endif

static const struct verbs_device_ops hihtr_dev_ops = {
	.name = "hihtr",
	.match_min_abi_version = 0,
	.match_max_abi_version = INT_MAX,
#ifdef __UBUS_DRIVER__
	.match_device = hihtr_match_device,
#endif
	.match_table = hca_table,
	.alloc_device = hihtr_device_alloc,
	.uninit_device = hihtr_uninit_device,
	.alloc_context = compat_hihtr_alloc_context,
#if defined(DEVICE_OPS_HAVE_FREE_CTX)
	.free_context = hihtr_free_context,
#endif
};

bool is_hihtr_dev(struct ibv_device *device)
{
	struct verbs_device *verbs_device = verbs_get_device(device);

	return verbs_device->ops == &hihtr_dev_ops;
}

#if defined(HAVE_PROVIDER_DRIVER_IN_DRIVER)
PROVIDER_DRIVER(hihtr, hihtr_dev_ops);
#else
PROVIDER_DRIVER(hihtr_dev_ops);
#endif