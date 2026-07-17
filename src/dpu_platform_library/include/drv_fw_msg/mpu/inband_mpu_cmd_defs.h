/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026-2026. All rights reserved.
 * Description   : mpu cmd
 * Creation time : 2026/04/10
 */

#ifndef INBAND_MPU_CMD_DEFS_H
#define INBAND_MPU_CMD_DEFS_H

#include "mpu_cmd_base_defs.h"
#include "outband_mpu_ncsi_cmd_defs.h"

typedef struct {
    struct mgmt_msg_head head;

    u8 low_power_enable;
    u8 opt_type;
    u8 rsv[6];
    u32 ncsi_enter_low_power_mode_cnt;
    u32 ncsi_exit_low_power_mode_cnt;
} comm_cmd_low_power_set_s;

#define MAX_LOG_BUF_SIZE 1024
struct nic_cmd_get_uart_log_info {
    struct mgmt_msg_head head;
    struct {
        u32 ret : 8;
        u32 version : 8;
        u32 log_elem_real_num : 16;
    } log_head;
    char uart_log[MAX_LOG_BUF_SIZE];
};

#define MAX_LOG_CMD_BUF_SIZE 128
struct nic_cmd_set_uart_log_cmd {
    struct mgmt_msg_head head;
    struct {
        u32 ret : 8;
        u32 version : 8;
        u32 cmd_elem_real_num : 16;
    } log_head;
    char uart_cmd[MAX_LOG_CMD_BUF_SIZE];
};

enum log_or_index_type {
    MPU_COMM_GET_LOG = 0,    /**< 获取mpu日志 */
    MPU_COMM_GET_INDEX,      /**< 获取index日志 */
    NPU_COMM_GET_SIM_DATA,   /**< 从flash中获取微码字典 */
};

enum log_module_type {
    LOG_MODULE_TYPE_MPU_LOG = 0,
    LOG_MODULE_TYPE_NPU_LOG,
    LOG_MODULE_TYPE_SMU_LOG,
    LOG_MODULE_TYPE_MPU_LASTWORD,
    LOG_MODULE_TYPE_NPU_LASTWORD,
    LOG_MODULE_TYPE_MPU_RELOAD_LOG = 5,
    LOG_MODULE_TYPE_MPU_CNT_DICT,
    LOG_MODULE_TYPE_NPU_CNT_DICT,
    LOG_MODULE_TYPE_UBC_IMP_LOG,
    LOG_MODULE_TYPE_UBC_IMP_LASTWORD,
    LOG_MODULE_TYPE_ROCE_IMP_LOG = 10,
    LOG_MODULE_TYPE_ROCE_SCC_LOG,
    LOG_MODULE_TYPE_BUTT
};

enum log_area_type {
    LOG_AREA_RAM = 0,
    LOG_AREA_FLASH,
};

struct nic_log_info {
    struct mgmt_msg_head msg_head;

    u32 offset;
    u8 log_or_index;           // 0:log;    1:index;
    u8 type;                   // 0:up; 1:ucode; 2:smu;(log_or_index: 0, 此位:日志类型, 1, 此位：字典类型)
                               // 3:mpu lastword 4.npu lastword
                               // 5:mpu cnt 字典文件， 6. npu cnt 字典文件
    u8 area;                   // 0:ram;    1:flash;(只有当log_or_index为0的时候，此位有效)
    u8 rsvd1;                  // reserved
    u8 data[MAX_LOG_BUF_SIZE]; // 单次获取1KB数据
};

/* 日志控制信息(用于日志防覆盖功能保留的pi和日志次序等信息) */
typedef struct {
    u32 log_valid;   /* 日志有效位 */
    u32 rsv_log_pi;
    u32 pi;          /* 日志偏移 */
    u32 log_seq;     /* 日志序列号，每写一次递增1，标识是哪一次的日志，以及下次要写入的分区（偶数写入主区，奇数写入备区） */
    u32 info_rsv[2]; /* 日志保留信息rsv字段 */
} log_ctrl_info_s;

typedef struct {
    u32 log_type : 2;  /* 日志数据的来源,0:代表up,1:代表ucode,2:表示sec */
    u32 rsvd1 : 6;
    u32 core_id : 2;
    u32 time_sync : 1;  /* 时间是否已同步，MPU_LOG_TIME_SYNC_TYPE类型 */
    u32 patch_log : 1; /* 补丁日志标识，0:非补丁日志，1:补丁日志 */
    u32 patch_log_level : 3; /* 补丁日志级别 */
    u32 rsvd3 : 17;
} log_head_mpu;

typedef struct {
    u32 index : 8;
    u32 valid : 1;
    u32 rsv : 23;
} log_head_imp;

typedef struct {
    u32 log_type : 2;  /* 日志数据的来源,0:代表up,1:代表ucode,2:表示sec */
    u32 core_id : 6;   /* 微码core id,当type为1时生效 */
    u32 thread_id : 2; /* 微码thread id,当type为1时生效 */
    u32 srv_data : 22; /* 特性私有数据 */
} log_head_def;

/* user日志条目 */
#define LOG_USER_ITEM 4

/* 日志的存储格式，总大小为32B */
typedef struct {
    /* DW0 */
    union {
        log_head_mpu mpu;
        log_head_imp imp;
        log_head_def def;
    } head;

    /* DW1 */
    u16 file_id;       /* 存放文件ID */
    u16 code_line_num; /* 打印代码所在行 */

    /* DW2~DW3 */
    u32 time_l32; /* 时间计数  */
    u32 time_h32; /* 时间计数  */

    /* Dw4~DW7 */
    u32 user_val[LOG_USER_ITEM]; /* 根据每种类型的日志条目确定  */
} log_item_s;

typedef struct tag_mpu_mctp_counter_info {
    u32 mctp_send_get_routing_tbl_port_err;
    u32 mctp_get_routing_tbl_trans_pkt_err;
    u32 mctp_dis_eid_proc_msg_len_err;
    u32 mctp_send_one_cmd_trans_pkt_err;
    u32 mctp_send_first_cmd_trans_pkt_err;
    u32 mctp_send_middle_cmd_trans_pkt_err;
    u32 mctp_send_last_cmd_trans_pkt_err;
    u32 mctp_get_routing_tbl_err;
    u32 mctp_lldp_capture_send_cmd_msg_err;
    u32 mctp_reset_unsupported_err;
    u32 mctp_set_eid_msg_len_err;
    u32 mctp_set_eid_req_eid_err;
    u32 mctp_get_eid_msg_len_err;
    u32 mctp_predis_eid_msg_len_err;
    u32 mctp_notify_dis_iid_err;
    u32 mctp_get_routing_tbl_fail_err;
    u32 mctp_ctrl_cmd_not_support_err;
    u32 mctp_handle_cmd_ic_err;
    u32 mctp_handle_cmd_proc_err;
    u32 mctp_handle_cmd_func_remap_err;
    u32 mctp_handle_cmd_send_msg_err;
    u32 mctp_assemble_not_first_pkt_err;
    u32 mctp_assemble_msg_check_tag_err;
    u32 mctp_assemble_msg_check_seq_err;
    u32 mctp_assemble_msg_rcv_offset_err;
    u32 mctp_check_pkg_len_shorter_err;
    u32 mctp_check_pkg_len_err;
    u32 mctp_check_pkg_trans_head_err;
    u32 mctp_ncsi_msg_proc_err;
    u32 mctp_pldm_msg_proc_err;
    u32 mctp_handle_msg_type_not_support_err;
    u32 mctp_handle_msg_fail_err;
    u32 mctp_handle_msg_send_cmd_msg_err;
    u32 mctp_alloc_rcv_buff_err;
    u32 mctp_alloc_send_buff_err;
    u32 mctp_assemble_msg_err;
    u32 mctp_pkt_proc_err;
    u32 mctp_recv_pkt_pldm_type;
    u32 mctp_pldm_msg_handle_err;
    u32 mctp_recv_pkt_ncsi_type;
    u32 mctp_ncsi_msg_handle_err;
    u32 mctp_recv_full_pkt_cnt;
    u32 mctp_recv_ctr_pkt_cnt;
    u32 mctp_recv_data_pkt_cnt;
    u32 mctp_handle_cmd_proc_cnt;
    u32 mctp_set_eid_msg_cnt;
    u32 mctp_get_eid_msg_cnt;
    u32 mctp_get_uuid_msg_cnt;
    u32 mctp_get_version_msg_cnt;
    u32 mctp_get_msgtype_msg_cnt;
    u32 mctp_resovle_eid_msg_cnt;
    u32 mctp_update_routing_tbl_msg_cnt;
    u32 mctp_get_routing_tbl_msg_cnt;
    u32 mctp_predis_eid_msg_cnt;
    u32 mctp_dis_eid_proc_msg_cnt;
    u32 mctp_notify_dis_msg_cnt;
    u32 mctp_get_network_id_msg_cnt;
    u32 mctp_query_hop_msg_cnt;
    u32 mctp_resolve_uuid_msg_cnt;
    u32 mctp_handle_cmd_send_cnt;
    u32 mctp_handle_msg_proc_cnt;
    u32 mctp_handle_msg_send_cnt;
    u32 mctp_assemble_msg_correct;
    u32 mctp_lldp_capture_send_cmd_msg_cnt;
} mpu_mctp_counter_info_s;

#define OOB_INFO_BUFFER_MAX 1024
typedef struct {
    struct mgmt_msg_head head; /* 8B */
    u8 oob_info_buf[OOB_INFO_BUFFER_MAX];
} comm_cmd_oob_info_resp_new;

enum sfp_type_enum {
    SFP_TYPE_SFP = 0,
    SFP_TYPE_QSFP = 1,
    SFP_TYPE_OSFP = 2,
    SFP_TYPE_DSFP = 3,
    SFP_TYPE_COUNT
};
 
enum power_channel_enum {
    POWER_CHANNEL_INDEX_0 = 0,
    POWER_CHANNEL_INDEX_1 = 1,
    POWER_CHANNEL_INDEX_2 = 2,
    POWER_CHANNEL_INDEX_3 = 3,
    POWER_CHANNEL_INDEX_COUNT
};

/* flash检测结果 0: ok   1: error  2:数据不一致 */
#define FLASH_CHECK_OK 0
#define FLASH_CHECK_ERR 1
#define FLASH_CHECK_DISMATCH 2

struct comm_info_check_flash {
    struct mgmt_msg_head head;
    u8 status;
    u8 rsv[3];
};

#define HINIC5_CAR_ENABLE_INVALID_NUM 0xFF

typedef struct tag_mpu_ncsi_counter_info_s {
    u32 ncsi_rx_octets_total_ok;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_rx_octets_bad;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_rx_uc_pkts;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_rx_mc_pkts;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_rx_bc_pkts;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_rx_pkts_64octets;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_rx_pkts_65to127octets;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_rx_pkts_128to255octets;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_rx_pkts_255to511octets;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_rx_pkts_512to1023octets;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_rx_pkts_1024to1518octets;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_rx_pkts_1519tomaxoctets;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_rx_fcs_errs;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_rx_tagged;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_rx_data_errs;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_rx_align_errs;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_rx_long_errs;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_rx_jabber_errs;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_rx_pause_maccontrol_framcounter;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_rx_unknow_maccontrol_framcounter;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_rx_very_long_err_cnt;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_rx_runt_err_cnt;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_rx_short_err_cnt;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_rx_filt_pkt_cnt;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_rx_octets_total_filt;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_tx_octets_transmitte_ok;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_tx_octets_transmitte_bad;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_tx_uc_pkts;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_tx_mc_pkts;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_tx_bc_pkts;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_tx_pkts_64octets;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_tx_pkts_65to127octets;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_tx_pkts_128to255octets;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_tx_pkts_255to511octets;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_tx_pkts_512to1023octets;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_tx_pkts_1024to1518octets;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_tx_pkts_1519tomaxoctets;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_tx_underrun;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_tx_tagged;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_tx_crc_err;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_tx_pause_frams;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_rx_overrun_cnt;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_rx_lengthfield_err_cnt;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_rx_fail_comma_cnt;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_tx_frm_cnt;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_tx_frm_err_cnt;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_tx_xon_cnt;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_tx_xoff_cnt;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_tx_xon_err_cnt;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_tx_empty_err_cnt;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_app_bd_cnt;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_add_bd_cnt;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_rx_txbd_cnt;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_rx_txbd_empty_cnt;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_rx_txbd_code_err_cnt;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_rx_txbd_min_frame_cnt;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_rx_txbd_max_frame_cnt;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_rls_bd_cnt;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_rx_pt_pkt_cnt_low;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_rx_pt_pkt_cnt_high;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_rx_pt_pkt_disc_cnt;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_tx_pt_ch_err_cnt0;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_tx_pt_ch_err_cnt1;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_tx_pt_ch_err_cnt2;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_tx_pt_ch_err_cnt3;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_tx_pt_pkt_cnt;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_rx_ctrl_ok_cnt;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_rx_ctrl_disc_cnt;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_rx_ctrl_chksum_err_cnt;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_tx_ctrl_pkt_cnt;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_tx_ctrl_len_mismatch_cnt;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_tx_ctrl_len_short;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_rx_pt_ch_ok_cnt0;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_rx_pt_ch_ok_cnt1;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_rx_pt_ch_ok_cnt2;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_rx_pt_ch_ok_cnt3;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_rx_ctrl_pavload_len_err_cnt;   /**< ncsi寄存器，定义详见nmanager */
    u32 ncsi_ipsurx_hit_count;   /**< ncsi寄存器，定义详见nmanager */
    u32 pie_to_mpu_bd_cnt;   /**< ncsi寄存器，定义详见nmanager */
    u32 pie_to_ipsu_bd_cnt;   /**< ncsi寄存器，定义详见nmanager */
    u32 pie_to_ncsi_bd_cnt;   /**< ncsi寄存器，定义详见nmanager */
    u32 rsv[10];
} mpu_ncsi_counter_info_s;   /**< ncsi寄存器，定义详见nmanager */

#define NCSI_COUNT_OPT_TYPE_READ 0   /**< ncsi counter读 */
#define NCSI_COUNT_OPT_TYPE_CLEAR 1   /**< nsci counter清除 */
#define NCSI_ALL_CNT_MAXLEN 120
#define NCSI_COM_CNT_MAXLEN 40
#define NCSI_OEM_CNT_MAXLEN (NCSI_ALL_CNT_MAXLEN - NCSI_COM_CNT_MAXLEN)
#define TIMEOUT_COUNTER_VALID 0x1
typedef struct {
    u16 opcode;
    u16 valid;
    u32 timeout_counter;
} oob_timeout_counter_s;
 
typedef struct {
    u32 len;
    oob_timeout_counter_s ncsi_cnt[NCSI_OEM_CNT_MAXLEN];
} ncsi_oem_timeout_counter_s;
 
typedef struct {
    u32 len;
    oob_timeout_counter_s ncsi_cnt[NCSI_COM_CNT_MAXLEN];
} ncsi_com_timeout_counter_s;

struct comm_cmd_ncsi_counter_req {   /**< 获取ncsi counter */
    struct mgmt_msg_head head;    /**< mbox消息头 */
    u8 opt_type;                  /**< 0:read counter 1:counter clear */
    u8 rsvd[3];
};

struct comm_cmd_ncsi_counter_resp {   /**< 获取ncsi counter */
    struct mgmt_msg_head head;       /**< mbox消息头 */

    mpu_ncsi_counter_info_s ncsi_cnt_info;   /**< counter信息 */
};

#pragma pack(1)
typedef struct tag_ncsi_chan_info {
    u8 aen_en;       /* AEN enable */
    u8 index;        /* index of channel */
    u8 port;         /* net port number */
    u8 state;        /* NCSI state(enum) */
    u8 ncsi_port_en; /* 标识该port的开关情况(1:开;0:关) */
    u8 rsv[3];
    ncsi_chan_capa_s capabilities;
    ncsi_parameters_s parameters;
} ncsi_chan_info_s;

typedef struct tag_ncsi_public_info {
    u8 ncsi_ver;
    u8 ncsi_pkg_id; // 需读取寄存器查询获得
    u8 arb_en;      /* arbitration en */
    u8 duplex_set;  /* 双工模式 */
    u8 chan_num; /* 虚拟channel数量，硬件最大只支持4个，使用多少个可由软件配置寄存器决定 */
    u8 iid;      /* identify new instances of a command */
    u8 lldp_over_ncsi_enable;
    u8 lldp_over_mctp_enable;
    u32 magicwd;
    u8 lldp_tx_enable;
    u8 rsd[2];
    u8 crc_flag;
    u32 crc;
} ncsi_pubcli_info_s;

/* 一个port的ncsi配置结构体 */
typedef struct {
    ncsi_pubcli_info_s ncsi_public_info;
    ncsi_chan_info_s ncsi_chan_info;
} comm_cmd_ncsi_settings;

#define IPSURX_MAC_NUM 0x4
#define IPSURX_MAC_BYTES 0x6
#define IPSURX_VLAN_NUM 0x8

typedef struct tag_ncsi_filter_cfg {
    /* 1.ipsurx的过滤配置 */
    u32 ipsurx_ncsi_ctrl;
    u32 ipsurx_ncsi_filter_mng;

    u8 ipsurx_mac_cfg[IPSURX_MAC_NUM][IPSURX_MAC_BYTES];
    u8 ipsurx_mac_vld[IPSURX_MAC_NUM];

    u32 ipsurx_ncsi_vlan_en;
    u16 ipsurx_vlan_tag[IPSURX_VLAN_NUM];
    u16 ipsurx_vlan_vld[IPSURX_VLAN_NUM];

    u32 ipsurx_ncsi_bc_ctrl;
    u32 ipsurx_ncsi_mc_ctrl;

    /* 2.ncsi的过滤配置 */
    u8 ncsi_smac_cfg[IPSURX_MAC_BYTES];
    u32 ncsi_chnl_ctrl;

    /* 3.微码的过滤配置 */
    u8 lldp_fwd_en;        /* lldp forward enable(4 port) */
    u8 lacp_fwd_en;        /* lacp forward enable(4 port) */
    u8 ncsi_arp_en;
    u8 lldp_over_ncsi_enable;
    u8 lldp_over_mctp_enable;
    u8 lldp_tx_enable;
    u8 rsv[22];
} comm_cmd_ncsi_filter_cfg;

typedef enum {
    OPERATOR_OBJECT_NCSI = 0,
    OPERATOR_OBJECT_PIE,
    OPERATOR_OBJECT_SMBUS,
    OPERATOR_OBJECT_MCTP,
    OPERATOR_OBJECT_I3C,
} oob_info_object_e;

typedef enum {
    OPERATOR_TYPE_READ_RAM_CFG = 0,
    OPERATOR_TYPE_READ_FLASH_CFG,
    OPERATOR_TYPE_READ_REG_CFG,
    OPERATOR_TYPE_CLEAR_FLASH_CFG,
    OPERATOR_TYPE_CLEAR_COUNTER,
    OPERATOR_TYPE_READ_COUNTER,
    OPERATOR_TYPE_READ_TIMEOUT_CMD,
} oob_info_type_e;

/* oob cfg req */
typedef struct {
    struct mgmt_msg_head head; /* 8B */
 
    u8 port;                    /* net port number */
    u8 oob_type;                  /**< 0:ncsi 1:mctp */
    u8 opt_type;                 /**< 0:read ram cfg  1:read flash cfg 2:clear flash cfg 3.clear counter 4.read counter*/
    u8 rsvd[5];
} comm_cmd_oob_info_req_s;

typedef struct {
    u8 ncsi_cable_state;        /* 0--线缆不在位，1--线缆在位 */
    u8 rsvd[3];
    comm_cmd_ncsi_settings setting_info;
    comm_cmd_ncsi_filter_cfg filter_cfg;
} mpu_ncsi_cfg_info_s;

/* PIE统计 */
typedef struct tag_pie_statistics {
    u32 recv_correct_pkt;
    u32 recv_err_pkt;
    u32 recv_correct_ncsi_pkt;
    u32 recv_err_ncsi_pkt;
    u32 send_ncsi_pkt;
    u32 recv_correct_vnet_pkt;
    u32 recv_err_vnet_pkt;
    u32 send_vnet_pkt;
    u32 recv_lacp_pkt_number;
    u32 task_sch;
    u32 bd_oq2up_cvg_int;
    u32 sgl_cvg_int;
    u32 bm_fifo_ful_int;
    u32 oq_eccm_int;
    u32 oq_eccs_int;
    u32 tx_parity_int;
    u32 rx_parity_int;
    u32 bd_oq2ipsu_ful_int;
    u32 bd_oq2ncsi_ful_int;
    u32 bd_oq2up_ful_int;
    u32 tx_ucerr;
    u32 rx_ucerr;
    u32 irq_nums;
} pie_statistics;

typedef struct {
    u16 opcode;
    u16 reserved;
    u32 rx_counter;
    u32 tx_counter;
} smbus_counter;
 
#define SMBUS_OPCODE_MAX_NUM ((OOB_INFO_BUFFER_MAX - sizeof(u32)) / sizeof(smbus_counter))
 
typedef struct {
    u16 len;
    u16 reserved;
    smbus_counter smb_counter[SMBUS_OPCODE_MAX_NUM];
} smbus_counter_info_s;

typedef struct {
    mpu_ncsi_counter_info_s ncsi_cnt_info;
    pie_statistics pie_cnt_info;
} oob_counter_info_s;

typedef union {
    mpu_ncsi_cfg_info_s ncsi_cfg_info;
    oob_counter_info_s cnt_info;
} comm_cmd_oob_info_buf;

typedef struct {
    struct mgmt_msg_head head; /* 8B */
    comm_cmd_oob_info_buf oob_info_buf;
} comm_cmd_oob_info_resp;

/* ncsi cfg */
typedef struct {
    struct mgmt_msg_head head; /* 8B */

    u8 ncsi_cable_state;        /* 0--线缆不在位，1--线缆在位 */
    u8 setting_type;            /* 0表示获取当前ram中的配置, 1表示获取flash中的配置 */
    u8 port;                    /* net port number */
    u8 erase_flag;
    comm_cmd_ncsi_settings setting_info;
} comm_cmd_ncsi_cfg_s;
#pragma pack()
#endif