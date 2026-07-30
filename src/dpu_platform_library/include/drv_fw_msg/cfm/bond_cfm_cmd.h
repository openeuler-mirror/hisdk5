/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026-2026. All rights reserved.
 * Description   : cfm bond datastruct
 * Creation time : 2026/04/16
 */

#ifndef BOND_CFM_CMD_H
#define BOND_CFM_CMD_H

#include "bond_mpu_cmd_defs.h"
#include "nic_mpu_cmd_structs.h"

#define ETH_ALEN 6     /**< Ethernet address length */

#define BOND_MAX_PORT_NUM 4     /**< bond 支持最大port数量 */
#define BOND_MAX_HOST_NUM 4     /**< bond 支持最大host数量 */

/**
 * @brief enum bond_mpu_type
 * @details bonding类型，全卸载bond或者半卸载bond
 */
enum bond_mpu_type {
    BOND_TYPE_HALF,
    BOND_TYPE_FULL,
    BOND_TYPE_BUTT
};

struct bond_half_data_s {
    u16 bond_mode;                /**< bond mode:1 for active-backup,2 for balance-xor,4 for 802.3ad */
    u16 bond_id;                  /**< bond id */
    u16 up_delay;                 /**< defualt:200ms */
    u16 down_delay;               /**< defualt:200ms */
    u32 active_slaves        : 8; /**< active port slaves(bitmaps) */
    u32 slaves               : 8; /**< bond port id bitmaps */
    u32 lacp_collect_slaves  : 8; /**< 可用于LACP报文收发的slave bitmap */
    u32 hash_policy          : 8; /**< hash:0 for layer 2 ,1 for layer 2+3 ,2 for layer 3+4 */
    u32 first_roce_func;          /**< RoCE used */
    u32 bond_pf_bitmap;           /**< slave pf bitmap */
    u32 user_bitmap;              /**< user bitmap */
    u8  bond_name[BOND_NAME_MAX_LEN];   /**< bond name, length must be less than 16 */
    u32 rsvd[8];
};

struct bond_full_data_s {
    u32 bond_id;     /**< bond设备号,output时有效,mpu操作成功返回时回填 */
    u32 master_slave_port_id;     /**< 主从端口id,最小的port id作为主口 */
    u32 slave_bitmap;     /**< bond port id bitmap */
    u32 poll_timeout;     /**< bond设备链路检查时间 */
    u32 up_delay;     /**< 暂时预留 */
    u32 down_delay;     /**< 暂时预留 */
    u32 bond_mode;     /**< 暂时预留 */
    u32 xmit_hash_policy;     /**< hash策略，用于微码选路逻辑 */
    u8  lacp_rate;            /**< lacp协商速率 0 为慢速， 1 为快速 */
    u8 rsvd1[3];      /**< 保留域段 */
    u32 rsvd[9];     /**< 保留域段 */
};

typedef union tag_cfm_bond_data {
    struct bond_half_data_s bond_half_data;
    struct bond_full_data_s bond_full_data;
} cfm_bond_data_u;

typedef struct tag_cfm_bond_cmd {
    struct mgmt_msg_head comm_head;
    u16 sub_cmd;
    u8  rsvd0;
    u8  bond_type;
    cfm_bond_data_u data;
    u8 rsvd1[32];
} cfm_bond_cmd_s;

/**
 * @brief 定义了一个用于存储bond状态信息的结构体
 * @details 该结构体包含了bond状态的各种信息，如bond_id、链路状态、slave port状态、port个数等，并且还包含了每个port的lacp信息，以及每个host成功和失败上报lacp协商结果的次数等。
 */
typedef struct tag_bond_full_get {
    u32 bond_id;     /**< bond id */
    u32 bon_mmi_status;     /**< 该bond子设备的链路状态 */
    u32 active_bitmap;     /**< 该bond子设备的slave port状态 */
    u32 port_count;     /**< 该bond子设备个数 */
    struct lacp_port_info port_info[BOND_MAX_PORT_NUM];     /**< 每个port的lacp信息 */
    u64 rsvd0[8];     /**< 保留域段 */
    u64 poll_timeout;     /**< 轮询超时时间 */
    u64 fast_periodic_timeout;     /**< 快速周期性超时时间 */
    u64 slow_periodic_timeout;     /**< 慢速周期性超时时间 */
    u64 short_timeout;     /**< 短超时时间 */
    u64 long_timeout;     /**< 长超时时间 */
    u64 aggregate_wait_timeout;     /**< 聚合等待超时时间 */
    u64 tx_period_timeout;     /**< 发送周期超时时间 */
    u64 rx_marker_timer;     /**< RX标记定时器 */
    u8 bond_mode;     /**< bond模式 */
    u8 arp_dual_en;     /**< arp双发使能标记 */
    u8 rsvd[6];     /**< 保留域段 */
} bond_full_get_s;

typedef union tag_cfm_bond_get_s {
    hinic5_bond_get_s bond_half_data;
    bond_full_get_s bond_full_data;
} cfm_bond_get_u;


typedef struct tag_cfm_bond_info_get {
    struct mgmt_msg_head comm_head;
    u16 sub_cmd;
    u8  rsvd0;
    u8  bond_type;
    cfm_bond_get_u data;
} cfm_bond_info_get_s;

/* bond配置信息结构体 */
typedef struct tag_cfm_bond_cfg_cmd {
    struct mgmt_msg_head head;        /**< 消息头 */
    u32 cfg_bitmap;                   /**< bond cfg bitmap */
    u8  bond_name[BOND_NAME_MAX_LEN]; /**< if bond_id_vld=0 input, else output */
    u8  op_code;                      /**< 操作类型：0: 查询GET，1:配置SET */
    u8  arp_en;                       /**< ARP双发使能 */
    u8  rsvd0[2];                     /**< 保留字段 */
    u32 rsvd1[58];                    /**< 保留字段 */
} cfm_bond_cfg_cmd_s;

/**
 * @brief 定义了创建bond设备所需的信息
 * @details 该结构体包含了创建bond设备所需的各种参数，如bond设备号、主从端口id、bond port id bitmap等。
 */
struct hinic5_create_bond_info {
    u32 bond_id;     /**< bond设备号,output时有效,mpu操作成功返回时回填 */
    u32 master_slave_port_id;     /**< 主从端口id,最小的port id作为主口 */
    u32 slave_bitmap;     /**< bond port id bitmap */
    u32 poll_timeout;     /**< bond设备链路检查时间 */
    u32 up_delay;     /**< 暂时预留 */
    u32 down_delay;     /**< 暂时预留 */
    u32 bond_mode;     /**< 暂时预留 */
    u32 xmit_hash_policy;     /**< hash策略，用于微码选路逻辑 */
    u8  lacp_rate;            /**< lacp协商速率 0 为慢速， 1 为快速 */
    u8 rsvd1[3];      /**< 保留域段 */
    u32 rsvd[1];     /**< 保留域段 */
};

/**
 * @brief 创建bond的消息接口结构体
 * @details 该结构体用于创建bond的消息接口
 */
struct hinic5_cmd_create_bond {
    struct hinic5_mgmt_msg_head head;     /**< 命令字消息头 */
    struct hinic5_create_bond_info create_bond_info;     /**< 创建bond的信息 */
};

#endif /* BOND_CFM_CMD_H */