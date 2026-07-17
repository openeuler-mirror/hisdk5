/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2025. All rights reserved.
 *
 * File Name     : hihtr_dfx_query.c
 * Version       : v0.1
 * Created       : 2024/10/30
 * Last Modified : 2024/10/30
 * Description   : The definition of callback functions and internal functions of 1872 RoCE driver dfx features on qp level.
 */

#ifdef __ROCE_DFX__

#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/atomic.h>
#include <rdma/ib_verbs.h>
#include "comm_defs.h"
#include "hinic5_hw.h"
#include "hihtr_dfx.h"
#include "shared/hihtr_shared.h"

static int hihtr_dfx_get_qp_count(const struct hihtr_dfx_query_inbuf *inbuf, struct hihtr_dev *hr_dev,
    union hihtr_dfx_query_outbuf *outbuf)
{
    struct hihtr_dfx_qp_count *qp_count = &outbuf->qp_count;

    qp_count->qp_alloced = hr_dev->qp_cnt.alloc_qp_cnt;
    qp_count->qp_deleted = hr_dev->qp_cnt.del_qp_cnt;
    qp_count->qp_alive = hr_dev->qp_cnt.alloc_qp_cnt - hr_dev->qp_cnt.del_qp_cnt;

    return 0;
}

int hihtr_dfx_query_mpt(const struct hihtr_dfx_query_inbuf *inbuf, struct hihtr_dev *hr_dev,
                                union hihtr_dfx_query_outbuf *outbuf)
{
	struct hihtr_v3_mpt_entry *mpt_ctx = (struct hihtr_v3_mpt_entry *)&(outbuf->mpt_ctx);
	int ret;

	if (!hr_dev->hw->query_mpt)
		return -EINVAL;

	ret = hr_dev->hw->query_mpt(hr_dev, inbuf->mpt_key, (void *)mpt_ctx);
	if (ret) {
		dev_err(hr_dev->dev, "Failed to query_mpt.(ret:%d)", ret);
	}
	return ret;
}

int hihtr_dfx_query_cqc(const struct hihtr_dfx_query_inbuf *inbuf, struct hihtr_dev *hr_dev,
                               union hihtr_dfx_query_outbuf *outbuf)
{
    int ret;
    struct hihtr_v3_cq_context *cq_ctx = (struct hihtr_v3_cq_context *)&(outbuf->cq_ctx);
    u32 cqn = inbuf->cqn;

	if (!hr_dev->hw->query_cqc)
		return -EINVAL;

    ret = hr_dev->hw->query_cqc(hr_dev, cqn, (void *)cq_ctx);
	if (ret) {
        dev_err(hr_dev->dev, "Failed to query_cqc.(ret:%d)", ret);
    }
	return ret;
}

int hihtr_dfx_query_qpc(const struct hihtr_dfx_query_inbuf *inbuf, struct hihtr_dev *hr_dev,
                               union hihtr_dfx_query_outbuf *outbuf)
{
    int ret;
    struct hihtr_v3_qp_context *qp_ctx = (struct hihtr_v3_qp_context *)&(outbuf->qp_ctx);
    u32 qpn = inbuf->qpn;

	if (!hr_dev->hw->query_qpc)
		return -EINVAL;

    ret = hr_dev->hw->query_qpc(hr_dev, qpn, (void *)qp_ctx);
	if (ret) {
        dev_err(hr_dev->dev, "Failed to query_qpc.(ret:%d)", ret);
    }
	return ret;
}

int hihtr_dfx_query_srqc(const struct hihtr_dfx_query_inbuf *inbuf, struct hihtr_dev *hr_dev,
                               union hihtr_dfx_query_outbuf *outbuf)
{
    int ret;
    struct hihtr_srq_context *srq_context = (struct hihtr_srq_context *)&(outbuf->srq_ctx);
    u32 srqn = inbuf->srqn;

	if (!hr_dev->hw->query_srqc)
		return -EINVAL;

	ret = hr_dev->hw->query_srqc(hr_dev, srqn, srq_context);
	if (ret) {
        dev_err(hr_dev->dev, "Failed to query_srqc.(ret:%d)", ret);
    }
	return ret;
}

int hihtr_dfx_query_scc_ctx(const struct hihtr_dfx_query_inbuf *inbuf, struct hihtr_dev *hr_dev,
                               union hihtr_dfx_query_outbuf *outbuf)
{
    int ret;
    struct hihtr_scc_context *scc_ctx = (struct hihtr_scc_context *)&(outbuf->scc_ctx);
    u32 qpn = inbuf->qpn;

    if (!hr_dev->hw->query_sccc)
        return -EINVAL;
    ret = hr_dev->hw->query_sccc(hr_dev, qpn, scc_ctx);
    if (ret) {
        pr_err("Failed to query qp %u scc ctx.(ret:%d)", qpn, ret);
    }

    return ret;
}

int hihtr_dfx_query_aeqc(const struct hihtr_dfx_query_inbuf *inbuf, struct hihtr_dev *hr_dev,
                               union hihtr_dfx_query_outbuf *outbuf)
{
    int ret;
    struct hihtr_aeq_context *aeq_context = (struct hihtr_aeq_context *)&(outbuf->aeq_ctx);

	if (!hr_dev->hw->query_aeqc)
		return -EINVAL;

	ret = hr_dev->hw->query_aeqc(hr_dev, aeq_context);
	if (ret) {
        dev_err(hr_dev->dev, "Failed to query_aeqc.(ret:%d)", ret);
    }
	return ret;
}

static void hihtr_dfx_query_pi_ci_set(struct hihtr_v3_qp_context qp_ctx, struct hihtr_dfx_pi_ci *pi_ci)
{
    pi_ci->qpc_sq_pi = LOWORD(qp_ctx.byte_160_sq_ci_pi);
    pi_ci->qpc_sq_ci = HIWORD(qp_ctx.byte_160_sq_ci_pi);
    pi_ci->qpc_rq_pi = LOWORD(qp_ctx.byte_84_rq_ci_pi);
    pi_ci->qpc_rq_ci = HIWORD(qp_ctx.byte_84_rq_ci_pi);
}

static int hihtr_dfx_query_pi_ci(const struct hihtr_dfx_query_inbuf *inbuf, struct hihtr_dev *hr_dev,
    union hihtr_dfx_query_outbuf *outbuf)
{
    int ret;
    struct hihtr_v3_qp_context qp_ctx;
    struct hihtr_v3_cq_context cq_ctx;
    u32 qpn = inbuf->query_pi_ci.qpn;
    u32 cqn = inbuf->query_pi_ci.cqn;
    struct hihtr_dfx_pi_ci *pi_ci = (struct hihtr_dfx_pi_ci *)&outbuf->pi_ci;
    memset((void *)pi_ci, 0, sizeof(struct hihtr_dfx_pi_ci));
    memset((void *)&qp_ctx, 0, sizeof(qp_ctx));


    if (!hr_dev->hw->query_qpc)
        return -EINVAL;
    ret = hr_dev->hw->query_qpc(hr_dev, qpn, (void *)&qp_ctx);
    if (ret != 0) {
        return ret;
    }
    // qpc中相关的sq rq pi_ci值记录
    hihtr_dfx_query_pi_ci_set(qp_ctx, pi_ci);

    memset((void *)&cq_ctx, 0, sizeof(cq_ctx));

    if (!hr_dev->hw->query_cqc)
        return -EINVAL;
    ret = hr_dev->hw->query_cqc(hr_dev, cqn, (void *)&cq_ctx);
    if (ret != 0) {
        return ret;
    }
    // cqc中相关的pi_ci值记录
    pi_ci->cq_ci = cq_ctx.byte_32_cq_ci;
    pi_ci->cq_pi = cq_ctx.byte_28_cq_pi;
    pi_ci->cq_ci_record_gpa_at_hop_num = cq_ctx.byte_16_hop_addr;
    pi_ci->cq_last_notified_pi = cq_ctx.db_record_addr;
    return 0;
}

static int hihtr_dfx_query_gid(const struct hihtr_dfx_query_inbuf *inbuf, struct hihtr_dev *hr_dev,
    union hihtr_dfx_query_outbuf *outbuf)
{
    int ret;
    u32 gid_index = inbuf->gid_index;

#if defined(RDMA_HAVE_IB_DEVICE_OPS)
    if (hr_dev->ib_dev.ops.query_gid == NULL) {
#else
    if (hr_dev->ib_dev.query_gid == NULL) {
#endif
        ret = -ENOMEM;
        goto err_to_print;
    }

#if defined(RDMA_HAVE_IB_DEVICE_OPS)
    ret = hr_dev->ib_dev.ops.query_gid(&hr_dev->ib_dev, 1, gid_index, (union ib_gid *)&outbuf->gid_entry);
#else
    ret = hr_dev->ib_dev.query_gid(&hr_dev->ib_dev, 1, gid_index, (union ib_gid *)&outbuf->gid_entry);
#endif
    if (ret != 0) {
        ret = -EINVAL;
        goto err_to_print;
    }
    return 0;

err_to_print:
    dev_err(hr_dev->dev, "***************port(1), gid_index(%d) *****************", gid_index);
    dev_err(hr_dev->dev, "Failed to query gid from cache!");
    dev_err(hr_dev->dev, "***************port(1), gid_index(%d) *****************", gid_index);
    return ret;
}
static int hihtr_dfx_query_specifications(const struct hihtr_dfx_query_inbuf *inbuf, struct hihtr_dev *hr_dev,
    union hihtr_dfx_query_outbuf *outbuf)
{
    outbuf->specifications.max_qp_number = hr_dev->caps.num_qps;
    outbuf->specifications.max_cq_number = hr_dev->caps.num_cqs;
    outbuf->specifications.max_pd_number = hr_dev->caps.num_pds;
    outbuf->specifications.max_mr_number = hr_dev->caps.num_mtpts;
    outbuf->specifications.max_gmv_entry_number = hr_dev->caps.gmv_entry_num;
    outbuf->specifications.max_sq_wqe_sge_number = hr_dev->caps.max_sq_sg;
    outbuf->specifications.max_rq_wqe_sge_number = hr_dev->caps.max_rq_sg;
    // 规避DTS2026031716563，详情看问题单，此规避用户不感知规格变化，故">> 1"
    outbuf->specifications.max_sq_wr_number = hr_dev->caps.max_sq_wqes >> 1;
    outbuf->specifications.max_rq_wr_number = hr_dev->caps.max_rq_wqes;
    outbuf->specifications.max_sq_inline_size = hr_dev->caps.max_sq_inline;
    outbuf->specifications.max_rq_inline_size = hr_dev->caps.max_sq_inline;
    outbuf->specifications.max_cqe_number = hr_dev->caps.max_cqes;
    outbuf->specifications.max_cqe_size = hr_dev->caps.cqe_sz;
    outbuf->specifications.max_extend_sge_number = hr_dev->caps.max_extend_sg;
    outbuf->specifications.max_xrcd_number = hr_dev->caps.num_xrcds;
    outbuf->specifications.max_frmr_pa_number = HIHTR_FRMR_MAX_PA;
    outbuf->specifications.dwqe_size = sizeof(struct hihtr_dwqe);
    outbuf->specifications.max_rd_depth = HIHTR_MAX_RD_DEPTH;

    if (hr_dev->caps.flags & HIHTR_CAP_FLAG_SRQ) {
        outbuf->specifications.max_srq_number = hr_dev->caps.num_srqs;
		outbuf->specifications.max_srq_wr_number = hr_dev->caps.max_srq_wrs;
		outbuf->specifications.max_srq_wqe_sge_number = hr_dev->caps.max_srq_sges;
	} else {
        outbuf->specifications.max_srq_number = 0;
		outbuf->specifications.max_srq_wr_number = 0;
		outbuf->specifications.max_srq_wqe_sge_number = 0;
    }
    return 0;
}

static int hihtr_dfx_query_hw_counter(const struct hihtr_dfx_query_inbuf *inbuf, struct hihtr_dev *hr_dev,
    union hihtr_dfx_query_outbuf *outbuf)
{
    int ret;
    u32 port = 1;
    outbuf->hw_counter_info.hw_num_counters = HIHTR_HW_CNT_TOTAL;

    if (hr_dev->hw->query_hw_counter == NULL) {
        return -ENOMEM;
    }

    ret = hr_dev->hw->query_hw_counter(hr_dev, outbuf->hw_counter_info.hw_counter_stats,
                                       port, &outbuf->hw_counter_info.hw_num_counters);
    if (ret != 0) {
        dev_err(hr_dev->dev, "Failed to query hw counter.(ret:%d)", ret);
        return -EINVAL;
    }

    return ret;
}

static int hihtr_dfx_query_driver_count(const struct hihtr_dfx_query_inbuf *inbuf, struct hihtr_dev *hr_dev,
    union hihtr_dfx_query_outbuf *outbuf)
{
    outbuf->driver_count.qp_create_err_cnt = atomic64_read(&hr_dev->dfx_cnt[HIHTR_DFX_QP_CREATE_ERR_CNT]);
    outbuf->driver_count.qp_modify_err_cnt = atomic64_read(&hr_dev->dfx_cnt[HIHTR_DFX_QP_MODIFY_ERR_CNT]);
    outbuf->driver_count.cq_create_err_cnt = atomic64_read(&hr_dev->dfx_cnt[HIHTR_DFX_CQ_CREATE_ERR_CNT]);
    outbuf->driver_count.cq_modify_err_cnt = atomic64_read(&hr_dev->dfx_cnt[HIHTR_DFX_CQ_MODIFY_ERR_CNT]);
    outbuf->driver_count.srq_create_err_cnt = atomic64_read(&hr_dev->dfx_cnt[HIHTR_DFX_SRQ_CREATE_ERR_CNT]);
    outbuf->driver_count.srq_modify_err_cnt = atomic64_read(&hr_dev->dfx_cnt[HIHTR_DFX_SRQ_MODIFY_ERR_CNT]);
    outbuf->driver_count.mr_reg_err_cnt = atomic64_read(&hr_dev->dfx_cnt[HIHTR_DFX_MR_REG_ERR_CNT]);
    outbuf->driver_count.mr_rereg_err_cnt = atomic64_read(&hr_dev->dfx_cnt[HIHTR_DFX_MR_REREG_ERR_CNT]);

    return 0;
}

static int hihtr_dfx_query_dp_pkt_count(const struct hihtr_dfx_query_inbuf *inbuf, struct hihtr_dev *hr_dev,
    union hihtr_dfx_query_outbuf *outbuf)
{
    int ret;
    int count;
    if (hr_dev->hw->query_hw_dp_pkt_counter == NULL) {
        return -ENOMEM;
    }
    ret = hr_dev->hw->query_hw_dp_pkt_counter(hr_dev, &outbuf->dp_pkt_info, &count);
    if (ret != 0) {
        dev_err(hr_dev->dev, "Failed to query dp pkt counter.(ret:%d)", ret);
        return -EINVAL;
    }
    return ret;
}

static hihtr_adm_query_cmd_proc_t g_hihtr_adm_dfx_query_cmd_proc[] = {
    // 1872 无论是cache还是host都执行同样的dfx查询
    { HIHTR_CMD_GET_GID, hihtr_dfx_query_gid },
    { HIHTR_CMD_GET_QPC, hihtr_dfx_query_qpc },
    { HIHTR_CMD_GET_CQC, hihtr_dfx_query_cqc },
    { HIHTR_CMD_GET_SRQC, hihtr_dfx_query_srqc },
    { HIHTR_CMD_GET_AEQC, hihtr_dfx_query_aeqc },
    { HIHTR_CMD_GET_MPT, hihtr_dfx_query_mpt },
    { HIHTR_CMD_GET_QPC_CQC_PI_CI, hihtr_dfx_query_pi_ci },
    { HIHTR_CMD_GET_QP_COUNT, hihtr_dfx_get_qp_count },
    { HIHTR_CMD_GET_HW_COUNT, hihtr_dfx_query_hw_counter },
    { HIHTR_CMD_GET_SPECIFICATIONS, hihtr_dfx_query_specifications },
    { HIHTR_CMD_GET_DRIVER_COUNT, hihtr_dfx_query_driver_count },
    { HIHTR_CMD_GET_SCC_CTX, hihtr_dfx_query_scc_ctx},
    { HIHTR_CMD_GET_DATA_PATH_MOD_PKT_CNT, hihtr_dfx_query_dp_pkt_count },
};

int hihtr_adm_dfx_query(struct hihtr_dev *hr_dev, const void *buf_in, u32 in_size, void *buf_out, u32 *out_size)
{
    const struct hihtr_dfx_query_inbuf *inbuf = (struct hihtr_dfx_query_inbuf *)buf_in;
    union hihtr_dfx_query_outbuf *outbuf = (union hihtr_dfx_query_outbuf *)buf_out;
    const hihtr_adm_generic_cmd_proc_t *generic_array =
        (const hihtr_adm_generic_cmd_proc_t *)g_hihtr_adm_dfx_query_cmd_proc;
    int elem_count = sizeof(g_hihtr_adm_dfx_query_cmd_proc) / sizeof(g_hihtr_adm_dfx_query_cmd_proc[0]);
    hihtr_adm_dfx_query_t hihtr_adm_dfx_query_func;

    if (in_size < sizeof(struct hihtr_dfx_query_inbuf)) {
        dev_err(hr_dev->dev, "Input parameter(in_size: %dB) is shorter than input buffer(allocated: %dB)",
                (int)in_size, (int)sizeof(struct hihtr_dfx_query_inbuf));
        return -ENOMEM;
    }
    if (*out_size < sizeof(union hihtr_dfx_query_outbuf)) {
        dev_err(hr_dev->dev, "Input parameter(out_size: %dB) is shorter than allocated buffer(allocated: %dB)",
                (int)*out_size, (int)sizeof(union hihtr_dfx_query_outbuf));
        return -ENOMEM;
    }
    (void)memset((void *)buf_out, 0, sizeof(union hihtr_dfx_query_outbuf));
    *out_size = (u32)sizeof(union hihtr_dfx_query_outbuf);

    if (inbuf->cmd_type >= HIHTR_CMD_INVALID) {
        hihtr_dfx_print("Not support this type(%d)", inbuf->cmd_type);
        return -EINVAL;
    }

    hihtr_adm_dfx_query_func =
        (hihtr_adm_dfx_query_t)hihtr_adm_generic_find_cmd_func(generic_array, elem_count, inbuf->cmd_type);
    if (hihtr_adm_dfx_query_func == NULL) {
        hihtr_dfx_print("Not support this type(%d)", inbuf->cmd_type);
        return -EINVAL;
    }

    return hihtr_adm_dfx_query_func(inbuf, hr_dev, outbuf);
}

void hihtr_dfx_record_err_event_count(struct hihtr_dev *hr_dev, enum ib_event_type event_type)
{
	hihtr_history_event_error_count *event_error_count = NULL;

	if (hr_dev->kernel_count_buf == NULL) {
		return;
	}
	event_error_count = &(hr_dev->kernel_count_buf->event_error_count);

    switch (event_type) {
		case IB_EVENT_CQ_ERR:
			atomic64_fetch_add_relaxed(1, (atomic64_t *)&event_error_count->hihtr_event_cq_err);
            break;
		case IB_EVENT_QP_FATAL:
			atomic64_fetch_add_relaxed(1, (atomic64_t *)&event_error_count->hihtr_event_qp_fatal);
            break;
		case IB_EVENT_QP_REQ_ERR:
			atomic64_fetch_add_relaxed(1, (atomic64_t *)&event_error_count->hihtr_event_qp_req_err);
            break;
		case IB_EVENT_QP_ACCESS_ERR:
			atomic64_fetch_add_relaxed(1, (atomic64_t *)&event_error_count->hihtr_event_qp_access_err);
            break;
		case IB_EVENT_PORT_ERR:
			atomic64_fetch_add_relaxed(1, (atomic64_t *)&event_error_count->hihtr_event_port_err);
            break;
		case IB_EVENT_SRQ_ERR:
			atomic64_fetch_add_relaxed(1, (atomic64_t *)&event_error_count->hihtr_event_srq_err);
            break;
		case IB_EVENT_SRQ_LIMIT_REACHED:
			atomic64_fetch_add_relaxed(1, (atomic64_t *)&event_error_count->hihtr_event_srq_limit_reached);
            break;
        case IB_EVENT_QP_LAST_WQE_REACHED:
			atomic64_fetch_add_relaxed(1, (atomic64_t *)&event_error_count->hihtr_event_qp_last_wqe_reached);
            break;
		default:
			return;
    }
}

#endif /* __ROCE_DFX__ */
