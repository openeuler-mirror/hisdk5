/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2025. All rights reserved.
 *
 * File Name     : hihtr_dfx.h
 * Version       : v0.1
 * Created       : 2024/10/30
 * Last Modified : 2024/10/30
 * Description   : The definition of macros, data structure, function prototype etc of 1872 RoCE driver dfx features.
 */
#ifndef ROCE_DFX_H
#define ROCE_DFX_H

#include <linux/types.h>
#include "hinic5_rdma.h"
#include "../hihtr_device.h"
#include "../hihtr_hw_v3.h"
#include "../hihtr_hw_v3_qp.h"
#include "../hihtr_hw_v3_cq.h"
#include "../hihtr_gid.h"
#include "roce_npu_cmd_srq_defs.h"
#include "roce_npu_cmd_qp_defs.h"
#include "hmm_context.h"
#include "hinic5_mt.h"
#include "hihtr_pub_cmd.h"
#include "roce_mpu_cmd.h"
#include "roce_mpu_cmd_defs.h"

#define MR_KEY_2_INDEX_SHIFT 8

#define ROCE_IO_DFX_CFG_VADDR_ID 0
#define ROCE_IO_DFX_CFG_PADDR_ID 1
#define ROCE_IO_DFX_CFG_ADDR_NUM 2

#define LOWORD(x) ((u16)(((u32)(x) & 0xFFFF)))       // /< 提取u32中的低u16
#define HIWORD(x) ((u16)(((u32)(x) >> 16)))          // /< 提取u32中的高u16

#define hihtr_dfx_print(fmt, args...)                    \
    do {                                                 \
        (void)printk("[hihtr_dfx] : " fmt "\n", ##args); \
    } while (0)

struct hihtr_dfx_query_inbuf {
    u32 cmd_type;
    union {
        u32 qpn;
        u32 cqn;
        u32 srqn;
        u32 mpt_key;
        u32 gid_index;
        struct {
            u32 qpn;
            u32 cqn;
        } query_pi_ci;
    };
};

struct hihtr_dfx_bypass_inbuf {
    u32 cmd_type;
    bool enable;
};

struct hihtr_dfx_bypass_outbuf {
    u32 cmd_type;
    u32 enable;
};

// 该数据结构与工具侧保持一致
struct hihtr_dfx_scc_buf {
    u32 cmd_type;
    u32 scc_enable;
    u32 algo_type;
    u32 template_id;
    u32 scc_mask; // 配置掩码
    u32 temp_param[SCC_TEMPLATE_PARAMS_NUM];
};

#define ROCE_DEV_NAME_LEN_MAX 64

struct hihtr_dfx_dbg_inbuf {
    u32 cmd_type;
    union {
        u32 qpn;
        u32 srqn;
    };
    char dev_name[ROCE_DEV_NAME_LEN_MAX];
};

#define ROCE_CTX_MAX_SIZE 128

union hihtr_dfx_dbg_outbuf {
    u32 value[ROCE_CTX_MAX_SIZE];
    u32 algo_type;
    u32 dev_type;
};

struct hihtr_dfx_common_inbuf {
    u32 cmd_type;
    char dev_name[ROCE_DEV_NAME_LEN_MAX];
};

union hihtr_dfx_common_outbuf {
    u32 dev_type;
};

struct hihtr_dfx_pi_ci {
    u32 qpc_sq_pi_on_chip;
    u32 qpc_sq_pi;
    u32 qpc_sq_load_pi;
    u32 qpc_rq_pi_on_chip;
    u32 qpc_rq_load_pi;
    u32 qpc_rq_pi;
    u32 qpc_rc_pi;
    u32 qpc_sq_ci;
    u32 qpc_sq_wqe_prefetch_ci;
    u32 qpc_sq_mtt_prefetch_wqe_ci;
    u32 qpc_sqa_ci;
    u32 qpc_sqa_wqe_prefetch_ci;
    u32 qpc_rq_ci;
    u32 qpc_rq_wqe_prefetch_ci;
    u32 qpc_rq_mtt_prefetch_wqe_ci;
    u32 qpc_rq_base_ci;
    u32 qpc_rc_ci;
    u32 qpc_rc_prefetch_ci;
    u32 cq_ci_on_chip;
    u32 cq_ci;
    u32 cq_load_ci;
    u64 cq_ci_record_gpa_at_hop_num;
    u32 cq_last_solicited_pi;
    u32 cq_pi;
    u32 cq_last_notified_pi;
};

struct hihtr_dfx_qp_count {
    u32 qp_alloced;
    u32 qp_deleted;
    u32 qp_alive;
};

struct hihtr_dfx_hw_counter_info {
    int hw_num_counters;
    u64 hw_counter_stats[HIHTR_HW_CNT_TOTAL];
};

#define HIHTR_MAX_RD_DEPTH 128

struct hihtr_dfx_specifications {
    u32 max_qp_number;
    u32 max_cq_number;
    u32 max_srq_number;
    int max_pd_number;
    u32 max_mr_number;

    u32 max_gmv_entry_number;

    u32 max_sq_wqe_sge_number;
    u32 max_rq_wqe_sge_number;
    u32 max_srq_wqe_sge_number;

    u32 max_sq_wr_number;  // 一个发送工作队列最大wr数量
    u32 max_rq_wr_number;  // 一个接收工作队列最大wr数量
    u32 max_srq_wr_number;

    u32 max_sq_inline_size;
    u32 max_rq_inline_size;

    u32 max_cqe_number;
    u32 max_cqe_size;

    u32 max_extend_sge_number;

    u32 max_xrcd_number;
    u32 max_frmr_pa_number;
    u32 dwqe_size;

    u32 max_rd_depth;
};

struct hihtr_dfx_driver_count {
    int64_t qp_create_err_cnt;
    int64_t qp_modify_err_cnt;
    int64_t cq_create_err_cnt;
    int64_t cq_modify_err_cnt;
    int64_t srq_create_err_cnt;
    int64_t srq_modify_err_cnt;
    int64_t mr_reg_err_cnt;
    int64_t mr_rereg_err_cnt;
};

union hihtr_dfx_query_outbuf {
    struct hihtr_v3_qp_context qp_ctx;
    struct hihtr_v3_cq_context cq_ctx;
    struct hihtr_srq_context srq_ctx;
    struct hihtr_aeq_context aeq_ctx;
    roce_verbs_srq_attr_s srq_attr;
    struct hihtr_v3_mpt_entry mpt_ctx;
    struct hihtr_scc_context scc_ctx;

    struct rdma_gid_entry gid_entry;
    struct hihtr_dfx_pi_ci pi_ci;
    struct hihtr_dfx_qp_count qp_count;
    struct hihtr_dfx_hw_counter_info hw_counter_info;
    struct hihtr_dfx_specifications specifications;
    struct hihtr_dfx_driver_count driver_count;
    struct hihtr_data_path_pkt_cnt dp_pkt_info;
    u32 algo_type;
};

enum hihtr_bw_ctrl_cmd_e {
    ROCE_BW_CTRL_DIS,
    ROCE_BW_CTRL_EN,
    ROCE_BW_CTRL_RESET
};

enum roce5_bw_wl_ctrl_cmd_e {
    ROCE_BW_WL_CTRL_CMD_SET,
    ROCE_BW_WL_CTRL_CMD_GET
};

struct rdma_gid_query_outbuf {
    struct rdma_gid_entry gid_entry;
};

struct hihtr_bw_ctrl_inbuf {
    u32 cmd_type;
    struct {
        u32 cir;
        u32 pir;
        u32 cnp;
    } ctrl_param;
};

struct hihtr_bw_ctrl_param {
    u8 color_type;
    u16 ptype;
    u8 hw_wred_mode;

    u32 cir;
    u32 pir;
    u32 cbs;
    u32 xbs;
    u32 cnp;
    u32 enable;
};

struct hihtr_ldcp_param {
    u32 cmd_type;
    struct {
        u8 alpha;
        u8 beta;
        u8 gamma;
        u8 eta;
        u8 set_flag;
    } ldcp_param;
};

struct hihtr_bw_ctrl_outbuf {
    struct hihtr_bw_ctrl_param bw_ctrl_param;
};

enum hihtr_dfx_io_cmd_type {
    ROCE_IO_CTRL_DIS,
    ROCE_IO_CTRL_EN
};

struct hihtr_dfx_io_alarm {
    enum hihtr_dfx_io_cmd_type en_flag;
    u16 pf_id;
    u16 rsvd;
    u16 io_latency_thd;
    u16 exec_time;
    u32 exp_qpn;
    void *rcd_uaddr;
    struct timespec64 start;
    struct mutex io_alarm_mutex;
};

struct hihtr_dfx_io_inbuf {
    u32 cmd_type;
    struct hihtr_dfx_io_alarm io_alarm;
};

struct hihtr_dfx_io_outbuf {
    struct hihtr_dfx_io_alarm io_alarm;
};

typedef struct hihtr_adm_generic_cmd_proc {
    u32 cmd_type;
    void *proc_func;
} hihtr_adm_generic_cmd_proc_t;

typedef int (*hihtr_adm_func_t)(struct hihtr_dev *hr_dev,
    const void *buf_in, u32 in_size, void *buf_out, u32 *out_size);

typedef struct hihtr_adm_cmd_proc {
    u32 cmd_type;
    hihtr_adm_func_t proc_func;
} hihtr_adm_cmd_proc_t;

typedef int (*hihtr_adm_dfx_query_t)(const struct hihtr_dfx_query_inbuf *inbuf, struct hihtr_dev *hr_dev,
    union hihtr_dfx_query_outbuf *outbuf);

typedef struct hihtr_adm_query_cmd_proc {
    u32 cmd_type;
    hihtr_adm_dfx_query_t proc_func;
} hihtr_adm_query_cmd_proc_t;

typedef enum {
	RO,		/* 不使能SO */
	FIRST_SO,	/* 第一个写保序 */
	ALWAYS_SO,	/* 所有写保序 */
    SO_TYPE_MAX,
} payload_so_type;

struct hihtr_order_cmd {
    u32 cmd;
	u32 so_type;
	u32 rsv[4];
};

int hihtr_get_drv_version(void *buf_out, u32 *out_size, struct hihtr_dev *hr_dev);
void *hihtr_adm_generic_find_cmd_func(const hihtr_adm_generic_cmd_proc_t *cmd_array,
                                         int elem_count, u32 target_cmd);
int hihtr_adm_dfx_query(struct hihtr_dev *hr_dev, const void *buf_in, u32 in_size, void *buf_out, u32 *out_size);
int hihtr_adm_dfx_bw_ctrl(struct hihtr_dev *hr_dev, const void *buf_in, u32 in_size, void *buf_out, u32 *out_size);
int hihtr_adm_dfx_set_ldcp_param(struct hihtr_dev *hr_dev, const void *buf_in, u32 in_size, void *buf_out, u32 *out_size);
int hihtr_adm_dfx_port_traffic(struct hihtr_dev *hr_dev, const void *buf_in, u32 in_size, void *buf_out, u32 *out_size);
int hihtr_adm_dfx_dbg(struct hihtr_dev *hr_dev, const void *buf_in, void *buf_out, u32 *out_size);
int hihtr_bypass_func_cnt_change(struct hihtr_dev *hr_dev, u8 func_add);
int hihtr_dfx_cmd_set_bypass(struct hihtr_dev *hr_dev, const void *buf_in, u32 in_size, void *buf_out, u32 *out_size);
int hihtr_dfx_cmd_query_bypass(struct hihtr_dev *hr_dev, const void *buf_in, u32 in_size,
                               void *buf_out, u32 *out_size);
int hihtr_dfx_cmd_set_scc(struct hihtr_dev *hr_dev, const void *buf_in, u32 in_size,
                             void *buf_out, u32 *out_size);
int hihtr_dfx_cmd_query_scc(struct hihtr_dev *hr_dev, const void *buf_in, u32 in_size,
                    void *buf_out, u32 *out_size);
int hihtr_dfx_cmd_set_scc_template(struct hihtr_dev *hr_dev, const void *buf_in, u32 in_size,
                    void *buf_out, u32 *out_size);
int hihtr_dfx_cmd_query_scc_template(struct hihtr_dev *hr_dev, const void *buf_in, u32 in_size,
                    void *buf_out, u32 *out_size);
void hihtr_dfx_clean_up(struct hihtr_dev *hr_dev);
void *global_hihtr_io_alarm_va_get(void);
void global_hihtr_io_alarm_va_set(u64 va);
void global_hihtr_io_alarm_pa_set(dma_addr_t pa);
dma_addr_t global_hihtr_io_alarm_pa_get(void);
int hihtr_dfx_cmd_query_cq(struct hihtr_dev *hr_dev, u32 cqn, struct hihtr_v3_cq_context *cq_ctx);
int hihtr_dfx_cmd_query_srq(struct hihtr_dev *hr_dev, u32 srqn, roce_verbs_srq_attr_s *srq_attr);
int hihtr_cmq_set_so_cfg(struct hihtr_dev *hr_dev, payload_so_type so_type);
int hihtr_dfx_set_so_type(struct hihtr_dev *hr_dev, const void *buf_in, u32 in_size,
                             void *buf_out, u32 *out_size);
int hihtr_dfx_query_so_type(struct hihtr_dev *hr_dev, const void *buf_in, u32 in_size,
                             void *buf_out, u32 *out_size);
void hihtr_dfx_record_err_event_count(struct hihtr_dev *hr_dev, enum ib_event_type event_type);
#endif // __ROCE_DFX_H__
