/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2023. All rights reserved.
 * Description   : South Interface---ovs mpu bond interface between mpu and driver
 * Author        : /
 * Create        : 2019/1/1
 * Notes         : Attention--Use the doxygen comment style to generate interface document.
 * History       : None
 */
#ifndef BOND_COMMON_DEFS_H
#define BOND_COMMON_DEFS_H

#define BOND_PORT_MAX_NUM       4
#define BOND_NAME_MAX_LEN       16
#define BOND_ID_INVALID         0xFFFF
#define OVS_PORT_NUM_MAX        BOND_PORT_MAX_NUM
#define BOND_DEFAULT_ROCE_FUNC  0xFFFFFFFF

#define BOND_ID_IS_VALID(_id)   (((_id) >= BOND_FIRST_ID) && ((_id) <= BOND_MAX_ID))
#define BOND_ID_IS_INVALID(_id) (!(BOND_ID_IS_VALID(_id)))

/**
 * @brief enum bond_group_id
 * @details bond id
 */
enum bond_group_id {
    BOND_FIRST_ID = 1,
    BOND_MAX_ID = BOND_FIRST_ID,
    BOND_MAX_NUM,
};

/**
 * @brief enum hinic5_bond_user
 * @details bond用户枚举,为了兼容23,不适用通用定义
 */
enum hinic5_bond_user {
	HINIC5_BOND_USER_OVS,
	HINIC5_BOND_USER_TOE,
	HINIC5_BOND_USER_ROCE,
	HINIC5_BOND_USER_UB,
    HINIC5_BOND_USER_NIC,
	HINIC5_BOND_USER_NUM
};

/**
 * @brief enum tag_bond_mode
 * @details bond模式的枚举类型
 */
typedef enum tag_bond_mode {
    BOND_MODE_NONE      = 0, /**< 禁用网络绑定 */
    BOND_MODE_BACKUP    = 1, /**< 主备模式，1表示主备绑定 */
    BOND_MODE_BALANCE   = 2, /**< 负载均衡模式，2表示XOR负载均衡绑定 */
    BOND_MODE_LACP      = 4, /**< LACP模式，4表示802.3ad绑定 */
    BOND_MODE_MAX            /**< 网络绑定模式的最大值 */
} bond_mode_e;

/**
 * @brief enum tag_bond_hash
 * @details bond hash策略的枚举类型
 */
typedef enum tag_bond_hash {
    BOND_HASH_L2   = 0,   /**< 使用L2地址进行哈希 */
    BOND_HASH_L23  = 1,   /**< 使用L2和L3地址进行哈希 */
    BOND_HASH_L34  = 2,   /**< 使用L3和L4地址进行哈希 */
    BOND_HASH_MAX  = 3    /**< 最大的哈希策略值 */
} bond_hash_e;

/**
 * ovs bond hash policy
 */
typedef enum ovs_bond_hash_policy {
    OVS_BOND_HASH_POLICY_L2       = 0, /**< 0 for layer 2 */
    OVS_BOND_HASH_POLICY_L34      = 1, /**< 1 for layer 3+4 */
    OVS_BOND_HASH_POLICY_L23      = 2, /**< 2 for layer 2+3 */
    OVS_BOND_HASH_POLICY_VFID_SQN = 3, /**< 3 for vfid ^ sqn */
    OVS_BOND_HASH_POLICY_MAX
} ovs_bond_hash_policy_e;

/**
 * @brief enum bond_port_duplex_state
 * @details 端口双工状态枚举类型
 */
enum bond_port_duplex_state {
    BOND_PORT_HALF_DUPLEX = 0, /**< 半双工 */
    BOND_PORT_FULL_DUPLEX,     /**< 全双工 */
};

#endif /* BOND_COMMON_DEFS_H */
