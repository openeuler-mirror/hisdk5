/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 * Description: mpu common cmd related structure and macro defined here
 * Create: 2025/11/26
 */

#ifndef NIC_MPU_EXTCMD_H
#define NIC_MPU_EXTCMD_H

#include "nic_mpu_cmd_structs.h"

#define FUNC_MAX_DPDK_NUM 32    // 单func最大的dpdk进程数

#define Q_GRP_FUNC_INDX_OFFSET 2048 // qgroup相对于func偏置
#define Q_GRP_FUNC_INDX_NUM 256     // 单卡最大的qgroup

#define NIC_MAX_FUNC_NUM  4096
#define NIC_MAX_PORT_NUM  8

#define MGMT_CMD_UNSUPPORT (255)

#define TCAM_RULE_QGRP_TYPE  2

#define HINIC5_NIC_QGRP_ID_FREE 0
#define HINIC5_NIC_QGRP_ID_APPLY 1

#define HINIC5_NIC_QGRP_TBL_FREE 0
#define HINIC5_NIC_QGRP_TBL_APPLY 1

enum hinic5_nic_extcmd {
    /* NIC Mbox保留命令字起始，计算产品线使用
       同一类功能用同一命令字，使用nic_mpu_sub_msg_extend作为buf_in，方便后续扩展 */

    HINIC5_NIC_CMD_QUEUE_GROUP = 258,                     /** < FDIR-RSS和队列池化相关命令 */
    HINIC5_NIC_CMD_GTP_OUTER_PARSE_STATUS,                /** < GTP控制使能状态 */
#ifndef HINIC5_NIC_CMD_CLEAR_ASSIGN_QP_RES // 防重定义 该命令归一到海思仓, 待1825移除
    HINIC5_NIC_CMD_CLEAR_ASSIGN_QP_RES = 261,             /** < clear resources of assigned qp > hinic5_cmd_clear_assign_qp_res */
#endif
    COMM_MGMT_CMD_GET_CARD_LOCATION_ID = 262,               /**< 获取slot chip id信息 */
};

#define EXTEND_CMD_LENGTH_MAX 124

typedef struct nic_mpu_cmd_extend_comm {
    u32 common[EXTEND_CMD_LENGTH_MAX];
} nic_mpu_cmd_extend_comm_s;

struct nic_mpu_sub_msg_extend {
    struct hinic5_mgmt_msg_head msg_head;

    u32 sub_cmd;
    u16 sub_msg_len;
    u16 rsvd;
    nic_mpu_cmd_extend_comm_s sub_msg;
};

enum hinic5_qpool_subcmd {
    /* 队列池化的子命令 */
    HINIC5_NIC_QPOOL_CMD_CFG_QGRP_ID = 0x0,         /** < 申请/释放q_grp_id */
    HINIC5_NIC_QPOOL_CMD_GET_RSS_ID,                /** < 获取rss模板的temp_id/inst_id/node_id */
    HINIC5_NIC_QPOOL_CMD_QGRP_MAP_TBL,              /** < 插入/删除sml中queue group映射表 */
};

enum nic_ub_card_type {
    /* UB网卡类型 */
    NON_UB_CARD = 0,
    POD_8CHIP,
    POD_16CHIP,
    SERVER_8CHIP,
    SERVER_16CHIP, /* reserved */
    INVALID_CARD = 255
};

#ifndef FUNC_MAX_CLEAR_QP_NUM // 防重定义 该命令归一到海思仓, 待1825移除
#define FUNC_MAX_CLEAR_QP_NUM 256
struct hinic5_cmd_clear_assign_qp_res {
    struct hinic5_mgmt_msg_head msg_head;

    u16 func_id;
    u16 qp_num;
    u32 rsvd[4];
    u16 qp[FUNC_MAX_CLEAR_QP_NUM];
};
#endif

struct hinic5_cmd_cfg_qgrp_id {
    u16 func_id;
    u16 q_grp_id;
    u8 opcode;
    u8 rsvd1[3];
};

struct nic_cmd_get_rss_id {
    u16 func_id;
    u16 rss_temp_id;
    u16 rss_node_id;
    u16 rss_instance_id;
};

struct hinic5_cmd_qgrp_map_tbl {
    u16 local_qid;
    u16 q_grp_id;
    u8 opcode;
    u8 rsvd[3];
};

/**
 * @brief hinic5_cmd_gtp_outer_parse
 * @details nic 设置gtp解析外层报文status
 */
struct hinic5_cmd_gtp_outer_parse {
    struct hinic5_mgmt_msg_head msg_head;     /**< 命令字消息头 */
    u16 func_id;     /**< Func ID */
    u8 opcode;     /**< 0: get state, 1: set state */
    u8 status;     /**< 0: disable, 1: enable */
    u32 rsvd2[125];
};

/**
 * @brief hinic5_func_flow_bifur_en_cmd
 * @details nic 设置function分流使能
 */
struct hinic5_func_flow_bifur_en_cmd {
    struct hinic5_mgmt_msg_head msg_head;     /**< 命令字消息头 */
    u16 func_id;         /**< function id */
    u8 flow_bifur_en;    /**< flow bifur enable */
    u8 opcode;          /**< 0:set, 1:get */
    u32 rsvd2;
};

/**
 * @brief hinic5_port_flow_bifur_en_cmd
 * @details nic 设置port分流使能
 */
struct hinic5_port_flow_bifur_en_cmd {
    struct hinic5_mgmt_msg_head msg_head;     /**< 命令字消息头 */
    u16 port_id;         /**< port id */
    u8 flow_bifur_en;    /**< flow bifur enable */
    u8 opcode;          /**< 0:set, 1:get */
    u32 rsvd2;
};

/**
 * @brief hinic5_get_board_card_location_cmd
 * @details 读取slot和chip号
 */
struct hinic5_get_board_card_location_cmd {
    struct hinic5_mgmt_msg_head msg_head;  /**< 命令字消息头 */
    u8 slot_id;    /**< slot槽位号 */
    u8 chip_id;   /**< chip号 */
    u8 ub_slot_id;  /**< 对外呈现的slot槽位号 */
    u8 ub_chip_id;/**< 对外呈现的chip槽位号 */
    u8 card_type; /**< 网卡类型 */
    u8 rsvd0[3];
    u32 rsvd1[60];
};

#endif