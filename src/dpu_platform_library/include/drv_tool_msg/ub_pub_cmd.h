/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 * Description: commonds between hinicadm tools and ub driver.
 * Create: 2023-7-30
 */

#ifndef UB_PUB_CMD_H
#define UB_PUB_CMD_H

#include "ub_npu_jfrc_cmd_defs.h"
#include "ub_npu_jfc_cmd_defs.h"
#include "ub_npu_srq_cmd_defs.h"
#include "ub_npu_utp_cmd_defs.h"
#include "ub_npu_upi_cmd_defs.h"
#include "ub_npu_tp_cmd_defs.h"
#include "ub_npu_vtp_cmd_defs.h"
#include "ub_npu_eid_cmd_defs.h"
#include "ub_npu_mig_cmd_defs.h"
#include "ub_npu_jetty_cmd_defs.h"
#include "ub_npu_sip_cmd_defs.h"
#include "mpu_cmd_base_defs.h"
#include "ub_mpu_dfx_cmd_defs.h"
#include "ub_mpu_cmd_defs.h"
#include "ub_wqe_format.h"
#include "mig_mpu_cmd_defs.h"
#include "ub_npu_mapt_cmd_defs.h"

#define udma_dfx_print(fmt, args...)                    \
    do {                                                \
        (void)printk("[udma_dfx] : " fmt "\n", ##args); \
    } while (0)

#define MAX_JFC_CMTT_SIZE 256
#define MAX_JFS_CMTT_SIZE 256
#define MAX_JFR_CMTT_SIZE MAX_JFC_CMTT_SIZE
#define MAX_TP_CMTT_SIZE 256
#define UB_SQ_WRITE_WQEBB_LEN 64
#define UB_SRQ_WRITE_WQEBB_LEN 128
#define UB_MAX_JFS_DFX_BUF 1024
#define UB_MAX_JFR_DFX_BUF 4096
#define UB_MAX_JFC_DFX_BUF 1024
#define UB_MAX_IDX_WQE_DFX_BUF 32
#define UB_SEID_CTX_DFX_LEN 32
#define UB_QUERY_TYPE_BIT 8
#define UB_CAPTURE_TP_MAX_NUM 512
#define UB_CQM_MODIFY_DATA_LEN 4096
#define UB_CQM_MODIFY_DATA_LEN_MAPT 128
#define UB_CQM_MODIFY_DATA_LEN_JFC 256
#define UB_CQM_MODIFY_DATA_LEN_TP 1024
#define UB_ATTACK_DW_NUM 4
#define UB_ATTACK_LEN (UB_ATTACK_DW_NUM * sizeof(u32))
#define UB_ATTACK_TAMSN_MAX_LEN (32 * 256)  /**< tamsn size * tamasn num */
#define UDMA_DSCP_MAX_NUM 64
#define UB_MSN_ENTRY_SIZE 32  /**< msn entry size 32B */
#define UB_CMD_TP_MSN_ENTRY_SIZE        32 /**< TP msn entry size 32B */

#define UB_MAX_PORT_CNT 8
#define UB_QUERY_RES_CNT 64
#define UB_EID_SIZE 16
#define UB_EID_FMT "%2.2x%2.2x:%2.2x%2.2x:%2.2x%2.2x:%2.2x%2.2x:%2.2x%2.2x:%2.2x%2.2x:%2.2x%2.2x:%2.2x%2.2x"
#define UB_EID_RAW_ARGS(eid) eid[0], eid[1], eid[2], eid[3], eid[4], eid[5], eid[6], eid[7], eid[8], eid[9], eid[10], \
			eid[11], eid[12], eid[13], eid[14], eid[15]

struct ub_l2d_dfx_inbuf {
    u32 cmd_type;
    union {
        u32 rsvd;   /* 预留后续扩展使用 */
    };
    union {
        ub_l2d_mem_cfg_s l2d_cfg;
    };
};

union ub_l2d_dfx_outbuf {
    u32 ub_l2d_dfx_data[UB_L2D_DFX_CMD_RESP_DATA_LEN];
};

struct ub_cc_params {
    u8 index;
    u8 ub_cc_algo;
    union {
        struct {
            u8 init_wnd;
            u8 alpha;
            u8 beta;
            u8 gamma;
            u8 eta;
            u8 wnd_min;
            u8 cc_unit;
            u8 rsvd[9];
        };
        u32 dw[4];
    };
};

/* jetty/jfs/jfr等资源下发/接收wr的数量 */
typedef struct ub_driver_comm_ctr {
    u64 post_send_wr_num;
    u64 post_recv_wr_num;
    u64 poll_total_num;
    u64 poll_sq_num;
    u64 poll_rq_num;
} ub_driver_comm_ctr_t;

struct ub_rx_rate_limit_params {
    u16 down_period;
    u16 up_period;
    u16 alpha1;
    u16 n1;
    u16 alpha2;
    u8 n2;
    u16 alpha3;
    u8 beta;
    u16 min_rate;
    u16 init_rate;
};

struct ub_qos_params {
    u16 func_id;
    u16 vnic_id;
    u8 xir_type;
    u8 rsvd1;
    u16 rsvd2;
    u32 bps;
    u32 pps;
};

struct udma_tp_modify_dfx {
    union {
        struct {
            /* dw0 */
            /* flag */
            u32 oor_en : 1;
            u32 sr_en : 1;
            u32 cc_en : 1;
            u32 spray_en : 1;
            u32 cc_alg : 4;
            /* peer_tpn */
            u32 peer_tpn : 24;

            /* dw1 */
            /* rx_psn */
            u32 rx_psn : 24;
            /* cc_pattern_idx */
            u32 cc_pattern_idx : 4;
            /* state */
            u32 state : 3;
            u32 rsvd1 : 1;

            /* dw2 */
            /* tx_psn */
            u32 tx_psn : 24;
            /* mtu */
            u32 mtu : 3;
            u32 rsvd2 : 1;
            /* sq_err_state */
            u32 sq_err_state : 1;
            /* rq_err_state */
            u32 rq_err_state : 1;
            /* sqa_err_state */
            u32 sqa_err_state : 1;
            /* rqa_err_state */
            u32 rqa_err_state : 1;

            /* dw3 */
            u32 rsvd;
        };
        u32 value[4];
    } attr;
    union {
        struct {
            u32 flag : 1;
            u32 peer_tpn : 1;
            u32 cc_pattern_idx : 1;
            u32 rx_psn : 1;
            u32 tx_psn : 1;
            u32 state : 1;
            u32 mtu : 1;
            u32 sq_err_state : 1;
            u32 rq_err_state : 1;
            u32 sqa_err_state : 1;
            u32 rqa_err_state : 1;
            u32 rsvd : 21;
        };
        u32 value;
    } mask;
};

struct udma_jetty_modify_dfx {
    union {
        struct {
            /* dw0 */
            /* next_send_tassn */
            u32 next_send_tassn : 16;
            /* next_ack_tassn */
            u32 next_ack_tassn : 16;

            /* dw1 */
            u32 rsvd_dw1;

            /* dw2 */
            u32 rsvd_dw2;

            /* dw3 */
            u32 rsvd_dw3;
        };
        u32 value[4];
    } attr;
    union {
        struct {
            u32 next_send_tassn : 1;
            u32 next_ack_tassn : 1;
            u32 rsvd : 30;
        };
        u32 value;
    } mask;
};

struct ub_query_inbuf {
    u32 cmd_type;
    union {
        u32 xid;
        u32 tpn;
        u32 tokenid;
        u32 jfcid;
        u32 jfrid;
        u32 jfsid;
        u32 jettyid;
        u32 jetty_group_id;
        u32 sipid;
        u32 dipid;
        u32 rcq_id; /* rc queue ctx/pcqc xid */
        u32 tpg_id;
        u32 vtp_id;
        u32 seid_id;
        u32 pattern_index;
        u32 func_id;
    };
    u32 length;
    u32 offset;
    u32 layers;
    u32 wqe_id;
    u32 smf_id;
    u32 vfid;
    u32 peer_tpn;
    u32 tx_psn;
    u32 rx_psn;
    u32 state;
    u32 mtu;
    u32 msn;
    u32 cc_pattern_idx;
    u32 flag;
    u32 oor_en;
    u32 sr_en;
    u32 cc_en;
    u32 cc_alg;
    u32 spray_en;
    u32 drop_aeqe_en;
    u8 cfg_cos_en;
    u8 slice_cfg_en;
    u8 multi_tp_en;
    u8 default_tpn;
    u8 event_type;
    u8 queue_type;
    u8 event_sub_type;
    u32 peer_tpn_mask : 1;
    u32 tx_psn_mask : 1;
    u32 rx_psn_mask : 1;
    u32 state_mask : 1;
    u32 mtu_mask : 1;
    u32 mtu_1k_mask : 1;
    u32 max_jetty_mask : 1;
    u32 cc_pattern_idx_mask : 1;
    u32 multi_path_mode : 2;
    u32 slice_cfg_set : 1;
    u32 reserved : 21;
    u32 is_jetty;
    u32 rsvd_to_remove;
    ub_cmd_upi_key_attr_s upi_key;
    struct ub_cc_params cc_params;
    u32 tp_srp_index;
    u32 bitmap_index;
    u32 ctr_bitmap_index;
    u32 bitmap_mode;
    struct ub_qos_params qos_params;
    u32 entry_index;
    u32 q_depth;
    u8 mtu_1k_en;
    u32 max_jetty_num;
    u32 idx;
    u8 dscp;
    u8 cos;
    u8 msn_type;
    u8 rsvd[1];
    u64 gpa;
    struct {
        struct {
            u32 rsvd : 27;
            u32 mtt_pro_en : 1;
            u32 ub_link_en : 1;
            u32 traffic_en : 1;
            u32 debug_en : 1;
            u32 lwb_en : 1;
        } dw15;
        struct {
            u32 rsvd : 27;
            u32 mtt_pro_en : 1;
            u32 ub_link_en : 1;
            u32 traffic_en : 1;
            u32 debug_en : 1;
            u32 lwb_en : 1;
        } dw15_flag; /** 是否修改的标记*/
    } func_table_modify_attr;
    struct udma_jetty_modify_dfx jetty_attr;
    struct udma_tp_modify_dfx tp_attr;
};

struct ub_secure_addr_info {
    struct vmsec_ctx_gpa_info ctx_gpa;
    struct vmsec_pcie_hole_info pcie_hole;
};

typedef enum ub_port_state {
    UB_PORT_NOP = 0,
    UB_PORT_DOWN,
    UB_PORT_INIT,
    UB_PORT_ARMED,
    UB_PORT_ACTIVE,
    UB_PORT_ACTIVE_DEFER
} ub_port_state_t;

typedef enum ub_speed {
    UB_SP_10M = 0,
    UB_SP_100M,
    UB_SP_1G,
    UB_SP_2_5G,
    UB_SP_5G,
    UB_SP_10G,
    UB_SP_14G,
    UB_SP_25G,
    UB_SP_40G,
    UB_SP_50G,
    UB_SP_100G,
    UB_SP_200G,
    UB_SP_400G,
    UB_SP_800G
} ub_speed_t;

typedef enum ub_link_width {
    UB_LINK_X1 = 0x1,
    UB_LINK_X2 = 0x1 << 1,
    UB_LINK_X4 = 0x1 << 2,
    UB_LINK_X8 = 0x1 << 3,
    UB_LINK_X16 = 0x1 << 4,
    UB_LINK_X32 = 0x1 << 5
} ub_link_width_t;

typedef enum ub_mtu {
    UB_MTU_256 = 1,
	UB_MTU_512,
	UB_MTU_1024,
	UB_MTU_2048,
	UB_MTU_4096,
	UB_MTU_8192
} ub_mtu_t;

typedef struct ub_port_status {
    ub_port_state_t state;        /* PORT_DOWN, PORT_INIT, PORT_ACTIVE */
    ub_speed_t active_speed;      /* bandwidth */
    ub_link_width_t active_width; /* link width: X1, X2, X4 */
    ub_mtu_t active_mtu;
} ub_port_status_t;

typedef struct ub_seg_attr {
    u8 eid[UB_EID_SIZE];
    u32 uasid;
    u64 va;
    u64 len;
    u32 key_id;
} ub_seg_attr_t;

typedef struct ub_dev_res {
    u32 port_cnt;
    ub_port_status_t port_status[UB_MAX_PORT_CNT];
    u32 seg_cnt;
    struct ub_seg_attr seg_list[UB_QUERY_RES_CNT];  // SEG key_id list
    u32 jfs_cnt;
    u32 jfs_list[UB_QUERY_RES_CNT];  // JFS ID list
    u32 jfr_cnt;
    u32 jfr_list[UB_QUERY_RES_CNT];  // JFR ID list
    u32 jfc_cnt;
    u32 jfc_list[UB_QUERY_RES_CNT];  // JFC ID list
    u32 jetty_cnt;
    u32 jetty_list[UB_QUERY_RES_CNT];  // Jetty ID list
    u32 jetty_group_cnt;
    u32 jetty_group_list[UB_QUERY_RES_CNT];  // Jetty group ID list
    u32 tp_cnt;
    u32 tp_list[UB_QUERY_RES_CNT];  // RC
    u32 vtp_cnt;
    u32 vtp_list[UB_QUERY_RES_CNT];  // VTP
    u32 tpg_cnt;
    u32 tpg_list[UB_QUERY_RES_CNT];  // RM
    u32 rc_cnt;
    u32 rc_list[UB_QUERY_RES_CNT];
    u32 utp_cnt;
    u32 utp_list[UB_QUERY_RES_CNT];  // UM
} ub_dev_res_t;

typedef struct ub_query_dev_cap {
    u32 feature;
    u32 max_jfc_num;
    u32 max_jfs_num;
    u32 max_jfr_num;
    u32 max_jetty_num;
    u32 max_tp_num;
    u32 max_tpg_num;
    u32 max_jetty_group_num;
    u32 max_vtp_num;
    u32 max_jfc_depth;
    u32 max_jfs_depth;
    u32 max_jfr_depth;
    u32 max_jfs_inline_size;
    u32 max_jfc_inline_size;
    u32 max_jfs_sge;
    u32 max_jfs_rsge;
    u32 max_jfr_sge;
    u32 max_dma_msg_sz;
    u32 max_send_msg_sz;
    u32 max_rc_outstd_cnt;
    u32 trans_mode;
    u32 congestion_ctrl_alg;
    u32 comp_vector_cnt;
    u32 utp_cnt;
    u32 max_eid_cnt;
    u32 guid;
    u32 max_upi_cnt;
    u32 port_cnt;
    u32 virtualization;
    u32 vf_cnt;
    u32 is_tpf;
    u32 max_oor_cnt;
    u32 max_sip_cnt_per_vf;
    u32 max_dip_cnt_per_vf;
    u32 max_seid_cnt_per_vf;
    u32 max_jetty_cnt_per_jtg;
    u32 max_tp_cnt_per_tpg;
    u32 max_mtu;
    u32 seid_idx_start;
    u32 max_rc_num;
    u32 max_rc_depth;
    u32 rc_entry_size;
    u32 slice_min_size;
    u32 slice_max_size;
    u32 max_pi_on_chip_num;
    u32 max_mpts;
    u32 num_ceq_vectors;
    u32 cqc_entry_size;
    u32 qpc_entry_size;
    u32 srqc_entry_size;
    u32 direct_wqe_size;
    u32 log_mtt;
    u32 num_mtts;
    u32 log_mtt_seg;
    u32 mtt_entry_sz;
    u32 mpt_entry_sz;
    u32 dmtt_cl_start;
    u32 dmtt_cl_end;
    u32 dmtt_cl_sz;
    u32 max_jetty_dest_ub;
    u32 ubrc_entry_sz;
    u32 log_ubrc_seg;
    u32 cmtt_cl_start;
    u32 cmtt_cl_end;
    u32 cmtt_cl_sz;
    u32 wqe_cl_start;
    u32 wqe_cl_end;
    u32 wqe_cl_sz;
    u32 device_type; /* 设备类型 */
    u32 fe_idx;      /* func id */
} ub_query_dev_cap_t;

#define HINIC5_MAX_BUF_SIZE 262144
union ub_query_outbuf {
    struct ub_dfx_tp_ctx_query tp;
    struct ub_tp_ctx_ext_query tpc_ext;
    struct ub_mapt_ctx_query mpt;
    struct ub_jfc_ctx_query jfc_ctx;
    u64 cmtt[MAX_JFC_CMTT_SIZE];
    struct ub_ta_context_query jfs_ctx;
    struct ub_ta_context_query jetty_ctx;
    struct ub_jetty_grp_ctx_query jetty_grp_ctx;
    ub_cmd_srq_query_rsp_s srq_ctx;
    struct ub_ta_context_query rcq_ctx;
    ub_pcqc_query_rsp_s pcqc;
    struct ub_rc_queue_wqe rcqe;
    struct tag_ub_sq_wqe_s jfs_sqe;
    u8 sqe[UB_MAX_JFS_DFX_BUF];
    u8 rqe[UB_MAX_JFR_DFX_BUF];
    u8 cqe[UB_MAX_JFC_DFX_BUF];
    u8 idx_wqe[UB_MAX_IDX_WQE_DFX_BUF];
    struct ub_sip_attr sip_attr;
    struct ub_dip_ctx_query dip_ctx;
    struct ub_dev_res dev_res;
    u64 dmtt[HINIC5_MAX_BUF_SIZE];
    struct ub_tp_wqe tpqe;
    u32 tp_state_num[UB_TPC_STATE_MAX];
    struct ub_tpg_ctx_query tpg_ctx;
    struct ub_vtp_ctx_query vtp_ctx;
    struct ub_seid_ctx_query seid_ctx;
    ub_cmd_upi_value_attr_s upi_value;
    struct ub_tpg_flow tpg_flow;
    struct ub_cc_params cc_params;
    struct ub_oor_tp_info oor_tp_info;
    struct ub_ta_rdma_rc_outbuf ta_rdma_rc;
    struct ub_query_dev_cap dev_cap;
    struct ub_cmd_tp_query_srp_bitmap_outbuf srp_bitmap;
    struct ub_cmd_tp_query_srp_ctr_bitmap_outbuf srp_ctr_bitmap;
    ub_cc_dfx_info_s cc_info;
    struct ub_cmd_mig_drain_status_outbuf mig_ctr;
    struct ub_cmd_func_resp func_info;
    struct ub_secure_addr_info sec_addr_info;
    u32 q_depth;
    u8 dscp_dev[UDMA_DSCP_MAX_NUM];
    u8 dscp_port[UDMA_DSCP_MAX_NUM];
    struct ub_ta_rdma_rc_ext_outbuf ta_rdma_rc_ext;
    struct ub_cmd_get_ub_filter_info_resp func_filter;
    ub_driver_comm_ctr_t ub_driver_comm_ctr;
    struct ub_oor_ta_info oor_ta_info;
    struct ub_msn_entry_outbuf msn_entry_info;
    struct ub_cmd_safe_dmtt_bitmap_outbuf safe_dmtt_bitmap;
    struct ub_cmd_safe_dmtt_ctx_outbuf safe_dmtt_ctx;
    struct ub_ta_latch_data_outbuf ta_latch_data;
    struct ub_tp_latch_data_outbuf tp_latch_data;
};

struct ub_capture_info {
    u32 cap_status;
    u32 cap_mode;
    u32 tp_mode;
    u32 cap_block_num_shift;
    u32 cap_func;
    u32 cap_state;
    u32 cap_max_num;
    u32 cap_pi;
    u32 cap_ci;
};

struct ub_tp_capture_info {
    u32 tp_num;
    u32 tpn[UB_CAPTURE_TP_MAX_NUM];
};

union ub_packet_outbuf {
    struct ub_capture_info capture_info;
    struct ub_tp_capture_info tp_capture_info;
};

struct udma_l2d_tbl_dfx {
    l2d_ub_dfx_tbl_attr_s attr; /* ub l2d dfx_tbl_attr */
    l2d_ub_dfx_tbl_mask_s mask; /* ub l2d dfx_tbl_mask */
};
struct ub_packet_inbuf {
    u32 cmd_type;
    u32 mode;
    u32 tpn;
    u32 drop_level;
    u32 cc_inject_mode;
    u32 cc_percent;
    u32 cc_limit_low;
    u32 cc_limit_high;
    struct udma_l2d_tbl_dfx data;
};

struct ub_attack_outbuf {
    u64 db;
};

struct ub_attack_inbuf {
    u32 convert_endian;
    u32 pi_ci;
    u32 object;
    u32 cmd_type;
    u32 offset;
    u32 length;
    u32 srv_type;
    u32 attack_value[UB_ATTACK_DW_NUM];
    u32 attack_mask[UB_ATTACK_DW_NUM];
};

struct ub_cqm_modify_inbuf {
    u32 cmd_type;
    u32 xid;
    u32 offset;
    u32 length;
    u32 vf_id;
    u32 data_len;
    u8 data[UB_CQM_MODIFY_DATA_LEN + 1];
    bool is_data_set;
};

union ub_cqm_modify_outbuf {
    u32 rsvd;
};

struct ub_port_traffic {
    u64 dp_tx_pkt_num;
    u64 dp_tx_pkt_bytes;
    u64 dp_rx_pkt_num;
    u64 dp_rx_pkt_bytes;
    u64 local_tx_pkt_num;
    u64 local_tx_pkt_bytes;
    u64 local_rx_pkt_num;
    u64 local_rx_pkt_bytes;
};

union ub_port_sta_outbuf {
    struct ub_dfx_port_statistics_cmd port_traffic;
};

struct ub_port_sta_inbuf {
    u32 cmd_type;
    u8 port_id;
    u8 cos;
    u8 gap;
    u8 all;
    u8 exec_cos_set;
    u8 exec_port_set;
    u8 exec_time_set;
    u8 vf_id;
};

struct ub_perf_ctr_inbuf {
    u32 cmd_type;
    u8 gap_index;
    bool exec_time_set;
};

struct ub_perf_query_outbuf {
    u64 dispatch_times;    // 成功执行次数
    u64 cmdq_rtt_sum;      // 总时延（单位是纳秒）
    u64 cmdq_npu_rtt_sum;  // 微码部分总时延
};

struct ub_perf_query_inbuf {
    u32 cmd_type;
    u32 xid;
    u32 vfid;
    u8 clear_flag;
};

enum ub_dfx_type {
    UB_DFX_QUERY = 0x80,
    UB_DFX_MODIFY = 0x81,
    UB_DFX_PACKET = 0x82,
    UB_DFX_PORT_STA = 0x83,
    UB_DFX_CQM_MODIFY = 0x84,
    UB_DFX_ATTACK = 0x85,
    UB_DFX_PERF_CTR = 0x86,
    UB_DFX_L2D_MEM = 0x87,
    UB_DFX_PERF_QUERY = 0x88,
};

enum ub_query_type_source {
    UB_QUERY_FROM_CACHE = 0,
    UB_QUERY_FROM_HOST = 1,
};

enum ub_query_sq_rq {
    UB_QUERY_SQ = 0,
    UB_QUERY_RQ = 1,
};

enum ub_query_sub_type {
    UB_QUERY_SUB_TYPE_CTX = 0x0,
    UB_QUERY_SUB_TYPE_MTT = 0x1,
    UB_QUERY_SUB_TYPE_WQE = 0x2,
    UB_QUERY_SUB_TYPE_INFO = 0x3,
    UB_QUERY_SUB_TYPE_SIP = 0x4,
    UB_QUERY_SUB_TYPE_DIP = 0x5,
    UB_QUERY_SUB_TYPE_PCQC = 0x6,
    UB_QUERY_SUB_TYPE_MPT = 0x7,
    UB_QUERY_SUB_TYPE_SRQ = 0x8,
    UB_QUERY_SUB_TYPE_CNT = 0x9,
    UB_QUERY_SUB_TYPE_FLOW = 0xa,
    UB_QUERY_SUB_TYPE_RDMARC = 0xb,
    UB_QUERY_SUB_TYPE_OOR_INFO = 0xc,
    UB_QUERY_SUB_TYPE_TASK_LIST_INFO = 0xd,
    UB_QUERY_SUB_TYPE_SRP_BITMAP = 0xe,
    UB_QUERY_SUB_TYPE_SRP_CTR_BITMAP = 0xf,
    UB_QUERY_SUB_TYPE_CC_INFO = 0x10,
    UB_QUERY_SUB_TYPE_FUNC_INFO = 0x11,
    UB_QUERY_SUB_TYPE_MIG_COUNTER_INFO = 0x12,
    UB_QUERY_SUB_TYPE_INVALID = 0x13,
    UB_QUERY_SUB_TYPE_CAP = 0x14,
    UB_QUERY_SUB_TYPE_SECURE_MEM_INFO = 0x15,
    UB_QUERY_SUB_TYPE_Q_DEPTH = 0x16,
    UB_QUERY_SUB_TYPE_DSCP = 0x17,
    UB_QUERY_SUB_TYPE_RDMARC_EXT = 0x18,
    UB_QUERY_SUB_TYPE_CTX_EXT = 0x19,
    UB_QUERY_SUB_TYPE_SAFE_DMTT_BITMAP = 0x1a, /* 1823 添加，考虑1825 1823兼容性预留 */
    UB_QUERY_SUB_TYPE_SAFE_DMTT_CTX = 0x1b,    /* 1823 添加，考虑1825 1823兼容性预留 */
    UB_QUERY_SUB_TYPE_IDX_WQE = 0x1c,
    UB_QUERY_SUB_TYPE_IO_STAT = 0x1d,
    UB_QUERY_SUB_TYPE_TA_OOR_INFO = 0x1e,
    UB_QUERY_SUB_TYPE_MSN_ENTRY = 0x1f,
    UB_QUERY_SUB_TYPE_TA_LATCH = 0x20, /* 1823 1825 TA锁存信息查询命令归一 */
    UB_QUERY_SUB_TYPE_TP_LATCH = 0X21, /* 1825 添加TP锁存信息查询 */
    UB_QUERY_SUB_TYPE_XSN_TP  = 0X22,           /* UB_QUERY_TYPE_XSN 的 subtype 不会作为cmd传给芯片 */
    UB_QUERY_SUB_TYPE_XSN_TPG = 0X23
    /* 考虑兼容性，增加消息id，只能往后追加 */
};

enum udma_query_cmd_type {
    UB_QUERY_TYPE_JFS = 0x0,
    UB_QUERY_TYPE_JFR = 0x1,
    UB_QUERY_TYPE_JFC = 0x2,
    UB_QUERY_TYPE_JETTY = 0x3,
    UB_QUERY_TYPE_JETTY_GROUP = 0x4,
    UB_QUERY_TYPE_SEG = 0x5,
    UB_QUERY_TYPE_DEV = 0x6,
    UB_QUERY_TYPE_TP = 0x7,
    UB_QUERY_TYPE_TPG = 0x8,
    UB_QUERY_TYPE_VTP = 0x9,
    UB_QUERY_TYPE_RC_QUEUE = 0xa,
    UB_QUERY_TYPE_UPI = 0xb,
    UB_QUERY_TYPE_CC = 0xc,
    UB_QUERY_TYPE_SEID = 0xd,
    UB_QUERY_TYPE_MIG = 0xe,
    UB_QUERY_TYPE_LATCH = 0xf,
    UB_QUERY_TYPE_XSN = 0x10,           /* UB_QUERY_TYPE_XSN 不会作为cmd传给芯片 */
    UB_QUERY_TYPE_INVALID,
    /* 考虑兼容性，增加消息id，只能往后追加 */
};

enum udma_packet_cmd_type {
    /* 抓包 */
    UB_CMD_START_CAP_PACKET = 0x0,     /* 兼容性预留，暂无使用 */
    UB_CMD_STOP_CAP_PACKET = 0x1,      /* 兼容性预留，暂无使用 */
    UB_CMD_QUERY_CAP_INFO = 0x2,       /* 兼容性预留，暂无使用 */
    UB_CMD_ENABLE_TP_CAP_PACKET = 0x3, /* 兼容性预留，暂无使用 */
    UB_CMD_DISABLE_TP_CAP_PACKET = 0x4,
    UB_CMD_QUERY_TP_CAP_INFO = 0x5, /* 兼容性预留，暂无使用 */

    /* 丢包 */
    UB_CMD_START_DROP_FUNC_PACKET = 0x6,
    UB_CMD_STOP_DROP_FUNC_PACKET = 0x7,
    UB_CMD_START_DROP_TP_PACKET = 0x8,
    UB_CMD_STOP_DROP_TP_PACKET = 0x9,

    /* CC capture */
    UB_CMD_CC_INJECT = 0xa,

    /* 丢包 */
    UB_CMD_START_DROP_CARD_PACKET = 0xb,
    UB_CMD_STOP_DROP_CARD_PACKET = 0xc,
    /* 考虑兼容性，增加消息id，只能往后追加 */
};

enum udma_modity_cmd_type {
    UB_MODIFY_TYPE_TP = 0x0,
    UB_MODIFY_TYPE_TPG_MULTIPATH = 0x1,
    UB_MODIFY_TYPE_CC = 0x2,
    UB_MODIFY_TYPE_TP_SRP = 0x3,
    UB_MODIFY_TYPE_AEQE = 0x4,
    UB_MODIFY_TYPE_DROP_AEQE = 0x5,
    UB_MODIFY_TYPE_QOS = 0x6,
    UB_MODIFY_TYPE_MUL_MODE = 0x7,
    UB_MODIFY_TYPE_FUNC_TABLE = 0x8,
    UB_MODIFY_TYPE_CFG_COS_EN = 0x9,
    UB_MODIFY_TYPE_DEVICE = 0xa,
    UB_MODIFY_TYPE_JETTY = 0xb,
    UB_MODIFY_TYPE_LATCH_LOCK = 0xc,
    UB_MODIFY_TYPE_INVALID,
    /* 考虑兼容性，增加消息id，只能往后追加 */
};

enum udma_modity_cmd_sub_type {
    UB_MODIFY_SUB_TYPE_SRP_BITMAP = 0x0,
    UB_MODIFY_SUB_TYPE_SRP_CTR_BITMAP = 0x1,
    UB_MODIFY_SUB_TYPE_FUNC_MAPPING = 0x2,
    UB_MODIFY_SUB_TYPE_BPS = 0x3,
    UB_MODIFY_SUB_TYPE_PPS = 0x4,
    UB_MODIFY_SUB_TYPE_Q_DEPTH = 0x5,
    UB_MODIFY_SUB_TYPE_MTU_1K_EN = 0x6,
    UB_MODIFY_SUB_TYPE_CLEAR_IO_STAT = 0x7,
    UB_MODIFY_SUB_TYPE_CLEAR_TA_LATCH_LOCK = 0x8,
    UB_MODIFY_SUB_TYPE_CLEAR_TP_LATCH_LOCK = 0x9,
    UB_MODIFY_SUB_TYPE_MAX_JETTY = 0xa,
    UB_MODIFY_SUB_TYPE_INVALID,
    /* 考虑兼容性，增加消息id，只能往后追加 */
};

enum ub_attack_type {
    UB_ATTACK_TYPE_DB = 0,
    UB_ATTACK_TYPE_WQE,
    UB_ATTACK_TYPE_MTT,
    UB_ATTACK_TYPE_PICI,
    UB_ATTACK_TYPE_TAMSN,
    UB_ATTACK_TYPE_INVALID
};

enum ub_attack_sub_type {
    UB_ATTACK_SUB_TYPE_JETTY = 0,
    UB_ATTACK_SUB_TYPE_JFS,
    UB_ATTACK_SUB_TYPE_JFR,
    UB_ATTACK_SUB_TYPE_JFRC,
    UB_ATTACK_SUB_TYPE_JFC,
    UB_ATTACK_SUB_TYPE_SEG,
    UB_ATTACK_SUB_TYPE_IDX_Q,
    UB_ATTACK_SUB_TYPE_INVALID
};

enum udma_port_sta_cmd_type {
    UB_PORT_STA_RTCB = 0,
    UB_VF_STA_RTCB,
};

enum udma_performance_cmd_type {
    UB_PERF_CMD_TYPE = 0,
};

enum udma_cqm_modify_cmd_type {
    UB_CQM_MODIFY_TYPE_JETTY_CTX = 0x0,
    UB_CQM_MODIFY_TYPE_TP_CTX = 0x1,
    UB_CQM_MODIFY_TYPE_JFR_SRQ_CTX = 0x2,
    UB_CQM_MODIFY_TYPE_JFR_CTX = 0x3,
    UB_CQM_MODIFY_TYPE_JFC_CTX = 0x4,
    UB_CQM_MODIFY_TYPE_SEG_CTX = 0x5,
    UB_CQM_MODIFY_TYPE_JFS_CTX = 0x6,
    UB_CQM_MODIFY_TYPE_JFRC_CTX = 0x7,
    UB_CQM_MODIFY_TYPE_CLEAR_LATCH_LOCK = 0x8,
    UB_CQM_MODIFY_TYPE_VTP_CTX = 0x9,
    UB_CQM_MODIFY_TYPE_TPG_CTX = 0xa,
    UB_CQM_MODIFY_TYPE_JFRC_PCQC = 0xb,
    UB_CQM_MODIFY_TYPE_SIP_TBL = 0xc,
    UB_CQM_MODIFY_TYPE_INVALID,
    /* 考虑兼容性，增加消息id，只能往后追加 */
};

static inline u32 ub_dfx_get_cmd_type(u32 cmd_type, u32 sub_type, u32 source_type)
{
    return ((cmd_type << UB_QUERY_TYPE_BIT) + (sub_type << 1) + source_type);
}

#define UB_DFX_QUERY_SOURCE_TYPE_BIT (1U)
#define UB_DFX_QUERY_SOURCE_TYPE_SHIELD_MASK (~UB_DFX_QUERY_SOURCE_TYPE_BIT)

#endif /* UB_PUB_CMD_H */
