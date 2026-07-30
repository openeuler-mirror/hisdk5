/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 * Description: driver common cmd related structure and macro defined here
 * Create: 2025/11/21
 */

#ifndef NIC_DRV_EXT_CMD_DEFS_H
#define NIC_DRV_EXT_CMD_DEFS_H

#include "nic_mpu_cmd_structs.h"
#include "hinic5_mt.h"

enum nic_driver_extcmd_type {
    /* NIC工具保留命令字，产品可以使用该范围内的命令字
       同一类功能用同一命令字，使用512字节进行数据传输，方便后续扩展 */

    MAC_VLAN_OPS = 0x201,              /**< bond卸载开关配置 */
    QUEUE_POOLING = 0x202,              /**< 队列池化获取或设置用户态队列 */
};

#define FUNC_MAX_DPDK_NUM 32
#define FUNC_MAX_QUEUE_NUM 64

struct nic_cmd_queue_pooling {
    struct mt_msg_head head;

    u16 current_queue_n;  // function用户态队列个数
    u16 max_queue_n;    // function可配置最大队列数
    u16 used_queue_n;   // function已使用队列个数
    u16 func_id;
    u16 dpdk_proc_n;
    u16 rsvd0;
    u32 pid[FUNC_MAX_DPDK_NUM];
    u16 q_grp_id[FUNC_MAX_DPDK_NUM];
    u16 queue_n[FUNC_MAX_DPDK_NUM];  // dpdk进程中队列个数
    u8 opcode;     // 0: get, 1: set
    u8 rsvd1[3];
    u32 rsvd2[16];
};

struct nic_cmd_mac_vlan {
    struct mgmt_msg_head head;

    u16 opcode;
    u16 rsvd;
    char chip_name[16];
    u32 rsvd2[121];
};

#endif