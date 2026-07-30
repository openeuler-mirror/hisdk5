/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 * Description   : cfm cmd define
 * Author        : /
 * Create        : 2025/08/07
 * Notes         :
 * History       : None
 */
#ifndef CFM_CMD_H
#define CFM_CMD_H

#define HINIC5_BOND_MSG_TIMEOUT_MS   (60000)

/* TODO：bond ccp qos待整合在同一个枚举中 */
typedef enum tag_cfm_mpu_drv_cmd {
    CFM_MPU_CMD_BOND_CREATE = 0,
    CFM_MPU_CMD_BOND_DELETE = 1,
    CFM_MPU_CMD_BOND_SET = 2,
    CFM_MPU_CMD_BOND_GET = 3,
    CFM_MPU_CMD_BOND_CFG = 4, /* bond配置和查询 */
    CFM_MPU_CMD_BOND_LINK_INFO_GET = 5,
    CFM_MPU_CMD_PASS_ARP_PKT = 6, /* 通过mpu发送arp报文 */
    CFM_MPU_CMD_CCP_COMM_PARA_GET = 16,
    CFM_MPU_CMD_CCP_ALGO_PARA_GET = 17,
    CFM_MPU_CMD_QOS_VPORT_MAPPING_SET = 32,
    CFM_MPU_CMD_QOS_VPORT_SHAPER_SET = 33,
    CFM_MPU_CMD_QOS_VPORT_SHAPER_CLR = 34,
    CFM_MPU_CMD_QOS_VPORT_SHAPER_GET = 35, /* qos_base配置和查询 */
    CFM_MPU_CMD_QOS_CC_L2D_SET = 36, /* CFM QoS CC L2DMEM写操作 */
    CFM_MPU_CMD_QOS_CC_L2D_GET = 37, /* CFM QoS CC L2DMEM读操作 */
    CFM_MPU_CMD_EXTEND_END = 64
} cfm_mpu_drv_cmd_e;

typedef enum tag_cfm_npu_cmdq_cmd {
    CFM_NPU_CMD_CCP_CTX = 0,
    CFM_NPU_CMD_CCP_STATISTICS = 1,
    CFM_NPU_CMD_HMM_OPS = 2,
    CFM_NPU_CMD_MAX,
} cfm_npu_drv_cmd_e;

#endif