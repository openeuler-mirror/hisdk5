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

#define MAC_LEN 6

// hi1872 新增命令类型
typedef struct roce_mcast_node_cmd {
    struct mgmt_msg_head head;
    u16 func_id;          // global func id
    u16 vlan_id;
    u32 qpn;              // qpn
    u8 mac[MAC_LEN];
} roce_mcast_node_cmd_s;

typedef struct roce_set_irq_idx_cmd {
    struct mgmt_msg_head head;
    u16 func_id;          // global func id
    u16 eq_idx;           // 本func内的eq_idx
    u16 irq_idx;          // func内的irq idx
    u16 irq_cnt_per_func; // roce每个func的中断个数
} roce_set_irq_idx_cmd_s;

#define HIHTR_FUNC_CNT_REDUCE 0
#define HIHTR_FUNC_CNT_ADD 1
#define HIHTR_BYPASS_DISABLE 0
#define HIHTR_BYPASS_ENABLE 1

enum hihtr_bypass_op_type {
    HIHTR_CHANGE_VF_CNT,
    HIHTR_CHANGE_PF_CNT,
    HIHTR_CTRL_BYPASS_MANUAL,
    HIHTR_QUERY_BYPASS_STATUS
};

typedef struct roce_config_bypass_cmd {
    struct mgmt_msg_head head;
    u8 func_add; /* func(pf/vf)数量是否增加 0: 减少, 1: 增加 */
    u8 bypass_ctrl; /* 手动控制 bypass 状态 0: 不使能, 1: 使能 */
    u8 op_type; /* 操作类型: vf数量变化/手动控制bypass状态/查询bypass状态 */
} roce_config_bypass_cmd_s;

typedef struct roce_stop_flow_cmd {
    struct mgmt_msg_head head;
    u16 func_id;
} roce_stop_flow_cmd_s;

#endif /* ROCE_MPU_CMD_DEFS_H */