/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 * Filename      : bond_pub_cmd.h
 * Creation time : 2025/09/08
 * Description   : bond工具和驱动的交互接口定义
 * Version       : 1.0
 */

#ifndef BOND_PUB_CMD_H
#define BOND_PUB_CMD_H

#include "bond_common_defs.h"
#include "hinic5_mt.h"

#define MAX_NETDEV_NUM 4

/**
 * @brief enum hinic5_bond_cmd_to_custom_e
 * @details 定义了与自定义设备相关的命令类型
 */
enum hinic5_bond_cmd_to_custom_e {
    CMD_CUSTOM_BOND_DEV_CREATE = 1,     /**< 创建自定义设备 */
    CMD_CUSTOM_BOND_DEV_DELETE,         /**< 删除自定义设备 */
    CMD_CUSTOM_BOND_GET_CHIP_NAME,      /**< 获取芯片名称 */
    CMD_CUSTOM_BOND_GET_CARD_INFO,      /**< 获取卡片信息 */
    CMD_CUSTOM_BOND_GET_ULD_DEV_NAME
};

#define BOND_NAME_LEN (16)
#define BOND_DFX_OP_ADD (0)
#define BOND_DFX_OP_DEL (1)

/**
 * @brief struct bond_dfx_ops_info
 * @details 用于bond绑定解绑的dfx操作
 */
struct bond_dfx_ops_info {
    struct mt_msg_head head;
    char bond_name[BOND_NAME_LEN];
    u32 ops;
    u32 user;
};

#endif