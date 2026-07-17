/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2022. All rights reserved.
 * Description   : RoCE service common API b/w driver and MPU
 * Author        : /
 * Create        : /
 * Notes         : /
 * History       : /
 */

#ifndef ROCE_MPU_CMD_DEFS_H
#define ROCE_MPU_CMD_DEFS_H

#include "mpu_cmd_base_defs.h"

/* ********************************** Func cmd between driver and mpu ********************************** */
typedef struct roce_set_func_state_cmd {
    struct mgmt_msg_head head;

    u16 func_id;
    u8 func_en;
    u8 rsvd;
} roce_set_func_state_cmd_s;

typedef struct roce_get_func_table_cmd {
    struct mgmt_msg_head head;

    u16 func_id;
    u8 rsvd[2];
} roce_get_func_table_cmd_s;

typedef struct roce_get_func_table_rsp {
    struct mgmt_msg_head head;
    u32 func_tbl_value;
} roce_get_func_table_rsp_s;

typedef struct roce_get_cfg_info_cmd {
    struct mgmt_msg_head head;

    u16 func_id;
    u8 rsvd[2];
} roce_get_cfg_info_cmd_s;

typedef struct roce_get_cfg_info_resp {
    struct mgmt_msg_head head;

    u8 scence_id;
    u8 lb_en;
    u8 lb_mode;
    u8 container_mode;

    u8 fake_en;
    u8 pf_start_bit;
    u8 pf_end_bit;
    u8 page_bit;

    u8 port_num;
    u8 host_num;
    struct {
        u16 pcie_atomic_en : 1;
        u16 is_vroce : 1;
        u16 ip2func : 1;
        u16 rsvd : 13;
    };

    u32 max_wqe;
    u32 max_cqe;
    u32 max_srq_wqe;
    u32 max_sq_sge;
    u32 max_rq_sge;
    u32 max_srq_sge;
    u32 max_sq_wqe_len : 12;
    u32 max_pd : 20;
    u32 max_rq_wqe_len : 12;
    u32 max_xrcd : 20;
    u32 max_sq_inline;
    u32 max_gid : 12;
    u32 rsvd0 : 20;
    u32 rsvd1[5];
} roce_get_cfg_info_resp_s;

typedef struct roce_get_cfg_info_comp_resp {
    struct mgmt_msg_head head;

    u8 scence_id;
    u8 lb_en;
    u8 lb_mode;
    u8 container_mode;

    u8 fake_en;
    u8 pf_start_bit;
    u8 pf_end_bit;
    u8 page_bit;

    u8 port_num;
    u8 host_num;
    struct {
        u16 pcie_atomic_en : 1;
        u16 rsvd : 15;
    };
} roce_get_cfg_info_comp_resp_s;

typedef struct roce_set_cpu_endian_cmd {
    struct mgmt_msg_head head;

    u16 func_id;
    u8 cpu_endian;
    u8 rsvd;
} roce_set_cpu_endian_cmd_s;

typedef struct roce_cfg_mac_cmd {
    struct mgmt_msg_head head;

    u16 func_id;
    u16 rsvd;
    u16 er_fwd_id;
    u8 mac[6];
    u8 vni_en;
    u32 vlan_id;
} roce_cfg_mac_cmd_s;

typedef struct roce_cfg_ipsu_mac_cmd {
    struct mgmt_msg_head head;

    u16 func_id;
    u8 er_id;
    u8 vni_en; /* vni enable */
    u8 mac[6];
    u16 src_func_id;
    u32 vlanid_vni;
} roce_cfg_ipsu_mac_cmd_s;

/* Keep the same definition with sml_addr_info */
union roce_addr_info {
    struct {
        u64 high;
        u64 low;
    } ipv6;
    struct {
        u32 rsvd[3];
        u32 val;
    } ipv4;
    struct {
        u16 vlan_id;
        u8 addr[6];
        u8 rsvd2[8];
    } mac;
    u32 dw[4];
};

struct roce_cmd_mpu_addr_info {
    u8 ip_ver; // 0x0：IPv4; 0x1：IPv6。
    u8 rsvd[3];
    union roce_addr_info ip_addr;
};

typedef struct roce_cfg_ipsu_ip_cmd {
    struct mgmt_msg_head head;

    u16 func_id;
    u8 port_id;
    u8 ip_ver; // 0x0：IPv4; 0x1：IPv6
    union roce_addr_info ip;
} roce_cfg_ipsu_ip_cmd_s;

typedef struct roce_cfg_ipsu_tcam_cmd {
    struct mgmt_msg_head head;

    u16 func_id;
    u8 ip_ver; // 0x0：IPv4; 0x1：IPv6
    u8 gid_idx;
    union roce_addr_info ip;
} roce_cfg_ipsu_tcam_cmd_s;

struct roce_cmd_get_group_id {
    struct mgmt_msg_head head;
    u8 status;
    u8 version;
    u8 rsvd0[6];

    u16 func_id;
    u8 group_rc_cos;
    u8 group_ud_cos;
    u8 group_xrc_cos;
};

/* ********************************** Bond cmd between driver and mpu ********************************** */
typedef struct roce_bond_cfg_state_cmd {
    struct mgmt_msg_head head;

    u16 func_id;
    u8 bond_en;
    u8 rsvd;
} roce_bond_cfg_state_cmd_s;

typedef struct roce_bond_set_ipsu_mac_cmd {
    struct mgmt_msg_head head;

    u16 func_id;
    u16 vlan_id;
    u16 er_fwd_id;
    u8 mac[6];
} roce_bond_set_ipsu_mac_cmd_s;

typedef struct roce_bond_cfg_er_fwd_id_cmd {
    struct mgmt_msg_head head;

    u16 func_id;
    u16 er_fwd_id;
    u32 bond_tbl_val;
} roce_bond_cfg_er_fwd_id_cmd_s;

typedef struct roce_bond_combine_er_fwd_id_cmd {
    struct mgmt_msg_head head;
    u16 func_id_src;
    u16 func_id_dst;
    u16 er_fwd_id;
    u8 rsvd[2];
} roce_bond_combine_er_fwd_id_cmd_s;

typedef struct roce_bond_compact_er_fwd_id_cmd {
    struct mgmt_msg_head head;

    u16 func_id;
    u16 rsvd;
    u32 value; /* dw14 value */
} roce_bond_compact_er_fwd_id_cmd_s;

/* ********************************** CC cmd between driver and mpu ********************************** */
typedef struct roce_cc_cfg_param_cmd {
    struct mgmt_msg_head head;
    u16 func_id;
    u16 rsvd;
    u32 param[4];
} roce_cc_cfg_param_cmd_s;

typedef struct roce_ovs_qos_policing_cmd {
    u32 index;
    u32 apply_mode;
    u32 profile_id;
    u32 cir; /**< 1-100000000kbps */
    u32 cbs; /**< 1-2560000kbits */
    u32 pir; /**< 1-100000000kbps */
    u32 pbs; /**< 1-2560000kbits */
} roce_ovs_qos_policing_cmd_s;

typedef struct roce_ovs_set_policing_cmd {
    struct mgmt_msg_head comm_head;
    u16 sub_cmd;
    u16 rsvd;

    roce_ovs_qos_policing_cmd_s data;    /**< the real message begin */
} roce_ovs_set_policing_cmd_s;

typedef struct roce_ovs_vport_cmd {
    u16 func_id;
    u16 vport_id;

    u16 vport_type;
    u8 vhd_type;
    u8 rsvd;
} roce_ovs_vport_cmd_s;

typedef struct roce_ovs_vport_attr {
#if defined(BYTE_ORDER) && (BYTE_ORDER == BIG_ENDIAN)
    u32 vlan_tag        : 16;
    u32 vlan_mode       : 8;
    u32 vlan_ol         : 8;

    u32 vni; /**< vxlan id */
    u32 vm_id;
    u32 trunk_port_en;
    u32 local_lro_en;
    u32 rsvd0;

    u32 rsvd1           : 6;
    u32 max_queue_flag  : 1;
    u32 max_queue       : 8;
    u32 clr_vm_id_flag  : 1;
    u32 profile_id      : 7;
    u32 set_vm_id_flag  : 1;
    u32 rsvd2           : 8;

    u32 mirror_src_ip;
    u32 rsvd3[18];

#else
    u32 vlan_ol         : 8;
    u32 vlan_mode       : 8;
    u32 vlan_tag        : 16;

    u32 vni; /**< vxlan id */
    u32 vm_id;
    u32 trunk_port_en;
    u32 local_lro_en;
    u32 rsvd0;

    u32 rsvd2           : 8;
    u32 set_vm_id_flag  : 1;
    u32 profile_id      : 7;
    u32 clr_vm_id_flag  : 1;
    u32 max_queue       : 8;
    u32 max_queue_flag  : 1;
    u32 rsvd1           : 6;

    u32 mirror_src_ip;
    u32 rsvd3[18];
#endif
} roce_ovs_vport_attr_s;

typedef struct roce_hiovs_vport_attr {
    roce_ovs_vport_cmd_s vport;
    roce_ovs_vport_attr_s attr;
} roce_hiovs_vport_attr_s;

typedef struct roce_ovs_map_vport_cmd {
    struct mgmt_msg_head comm_head;
    u16 sub_cmd;
    u16 rsvd;

    roce_hiovs_vport_attr_s data;    /**< the real message begin */
} roce_ovs_map_vport_cmd_s;

typedef struct roce_cc_cfg_bw_ctrl_cmd {
    struct mgmt_msg_head head;

    u16 func_id;
    u8 cmd;
    u8 rsvd;

    u8 color_type;
    u16 ptype;
    u8 hw_wred_mode;

    u32 cir;
    u32 pir;
    u32 cbs;
    u32 xbs;
    u32 cnp;
    u32 enable;
} roce_cc_cfg_bw_ctrl_cmd_s;

typedef struct roce_cc_cfg_wl_cmd_param {
    /* ROCE_CCF_ALGO_ID对应的拥塞参数 dw0，dw1 */
    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 rsvd : 20;
            u32 cnp_prio_enable : 1;
            u32 rsvd1 : 11;
#else
            u32 rsvd1 : 11;
            u32 cnp_prio_enable : 1;
            u32 rsvd : 20;
#endif
        } bs;
        u32 value;
    } dw0;

    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 cnp_cos : 3;
            u32 cnp_prio : 3;
            u32 ccf_appid : 8;
            u32 rsvd : 18;
#else
            u32 rsvd : 18;
            u32 ccf_appid : 8;
            u32 cnp_prio : 3;
            u32 cnp_cos : 3;
#endif
        } bs;
        u32 value;
    } dw1;

    /* 整机限速水线参数 */
    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 high_waterline : 16;
            u32 low_waterline : 16;
#else
            u32 low_waterline : 16;
            u32 high_waterline : 16;
#endif
        } bs;
        u32 value;
    } dw2;

    u32 rsvd;
} roce_cc_cfg_wl_cmd_param_s;

typedef struct roce_cc_cfg_bw_wl_cmd {
    struct mgmt_msg_head head;

    u16 func_id;
    u8 cmd;
    u8 rsvd;

    roce_cc_cfg_wl_cmd_param_s param;
} roce_cc_cfg_bw_wl_cmd_s;

typedef struct roce_mpu_vnic_waterline_info {
    u16 red_waterline;
    u16 yellow_waterline;
} roce_mpu_vnic_waterline_info_s;

typedef struct roce_cfg_ipsu_vnic_waterline_cmd {
    roce_cfg_ipsu_ip_cmd_s ip_cmd;
    roce_mpu_vnic_waterline_info_s waterline_info;
} roce_cfg_ipsu_vnic_waterline_cmd_s;

typedef struct roce_cc_cfg_vnic_waterline_cmd_param {
    union {
        struct {
            u64 high;
            u64 low;
        } ipv6;
        struct {
            u32 rsvd[3];
            u32 val;
        } ipv4;
    } ip_info;
    u16 red_waterline;
    u16 yellow_waterline;
} roce_cc_cfg_vnic_waterline_cmd_param_s;

typedef struct roce_cc_cfg_vnic_waterline_cmd {
    struct mgmt_msg_head head;

    u16 func_id;
    u8 port_id;
    u8 ip_ver;

    roce_cc_cfg_vnic_waterline_cmd_param_s param;
} roce_cc_cfg_vnic_waterline_cmd_s;

/* ********************************** DFX cmd between driver and mpu and tools ********************************** */
#define ROCE_PORT_NUM_MAX 4
#define ROCE_COS_NUM_MAX 8
#define ROCE_PORT_TRAFFIC_CTR_ID(port, cos) ((((port) << 0x3) + ((cos) & 0x7)) << 1)
#define ROCE_MAX_LATENCY_PROF_NUM 5 // 与L2D的统计大小一致
#define ROCE_MAX_TILE_NUM 8 // 最大可统计的TILE数量

typedef struct {
    u64 pkt_bytes;
    u64 pkt_num;
} roce_ctr_bytes_t;

typedef struct roce_ctr_array_bytes {
    roce_ctr_bytes_t tx_cur[ROCE_PORT_NUM_MAX][ROCE_COS_NUM_MAX];
    roce_ctr_bytes_t rx_cur[ROCE_PORT_NUM_MAX][ROCE_COS_NUM_MAX];
} roce_ctr_array_bytes_t;

typedef struct {
    u32 cmd_type;
    u8 port_id;
    u8 cos;
    u8 exec_cos_set;
    u8 exec_port_set;
} roce_port_sta_inbuf_t;

typedef struct {
    struct mgmt_msg_head head;
    roce_ctr_array_bytes_t ctr_array;
    u64 cur_time;
} roce_port_sta_outbuf_t;

typedef struct roce_mpu_set_sub_cmd_param {
    union {
        struct {
            u32 max_tso_len_cnt : 5;
            u32 port_traffic_enable : 1;
            u32 ack_pkt_num : 5;
            u32 pfc_free_en : 1;
            u32 rsvd : 20;
        } bs;
        u32 value;
    } dw0;
    struct {
        u32 latency_prof_en : 1;
        u32 frequency_reduction_ratio : 15;
        u32 rsvd : 16;
        u8 latency_prof_num[ROCE_MAX_TILE_NUM];
        u32 wqe_latency[ROCE_MAX_TILE_NUM][ROCE_MAX_LATENCY_PROF_NUM];
    } latency_prof;
} roce_mpu_set_sub_cmd_param_s;

typedef struct roce_dfx_set_inbuf {
    u32 cmd_type;
    u32 sub_cmd_type;
    u32 func_id;
    roce_mpu_set_sub_cmd_param_s sub_param;
    u32 show;
} roce_dfx_set_inbuf_s;

/* ********************************** DFX cmd between driver and mpu ********************************** */
typedef struct roce_dfx_cache_out_cmd {
    struct mgmt_msg_head head;
    u16 func_idx;
    u8 cache_index;
    u8 rsvd;
} roce_dfx_cache_out_cmd_s;

typedef struct roce_dfx_cfg_cap_param_cmd {
    struct mgmt_msg_head head;
    u16 index;
    u16 rsvd;
    u32 param[4];
} roce_dfx_cfg_cap_param_cmd_s;

typedef struct roce_dfx_cap_ctr_cmd {
    struct mgmt_msg_head head;
    u16 index;
    u16 rsvd;
    u32 value;
} roce_dfx_cap_ctr_cmd_s;

typedef struct {
    struct mgmt_msg_head head;
    roce_port_sta_inbuf_t cmd;
} roce_mpu_port_statics_cmd_inbuf_t;

typedef struct roce_dfx_set_cmd {
    struct mgmt_msg_head head;
    roce_dfx_set_inbuf_s cmd;
} roce_dfx_set_cmd_s;

typedef struct roce_dfx_set_outbuf {
    struct mgmt_msg_head head;
    roce_mpu_set_sub_cmd_param_s sub_param;
} roce_dfx_set_outbuf_s;

#endif /* ROCE_MPU_CMD_DEFS_H */
