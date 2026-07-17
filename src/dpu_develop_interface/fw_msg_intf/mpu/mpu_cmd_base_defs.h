/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
 * Filename      : mpu_cmd_base_defs.h
 * Version       : Initial Draft
 * Creation time : 2023/09/22
 * Last Modified :
 * Description   : MPU common definitions
 */
#ifndef MPU_CMD_BASE_DEFS_H
#define MPU_CMD_BASE_DEFS_H

#include "base_type.h"

typedef enum {
    RES_TYPE_FLUSH_BIT = 0, /**< flush的function reset标志位 */
    RES_TYPE_MQM, /**< mqm的function reset标志位 */
    RES_TYPE_SMF, /**< smf的function reset标志位 */
    RES_TYPE_PF_BW_CFG, /**< bw 配置的function reset标志位 */

    RES_TYPE_COMM = 10, /**< 公共的function reset标志位 */
    RES_TYPE_COMM_MGMT_CH,  /**< bw 配置的function reset标志位， clear mbox and aeq, The RES_TYPE_COMM bit must be set */
    RES_TYPE_COMM_CMD_CH,   /**< bw 配置的function reset标志位， clear cmdq and ceq, The RES_TYPE_COMM bit must be set */
    RES_TYPE_NIC, /**< nic的function reset标志位 */
    RES_TYPE_OVS, /**< ovs的function reset标志位 */
    RES_TYPE_VBS, /**< vbsfunction reset标志位 */
    RES_TYPE_ROCE, /**< roce的function reset标志位 */
    RES_TYPE_FC, /**< fc的function reset标志位 */
    RES_TYPE_TOE, /**< toe的function reset标志位 */
    RES_TYPE_IPSEC, /**< ipsec的function reset标志位 */
    RES_TYPE_SMF_CACHE_INVALID, /**< smf cache 无效的function reset标志位 */
    RES_TYPE_MAX, /**<function reset标志位的最大值 */
} func_reset_flag_e; /**< func reset的flag ,用于指示清理哪种资源 */

#define DEVICE_TYPE_L2NIC                   0 /**< L2NIC 设备 */
#define DEVICE_TYPE_NVME                    1 /**< nvme 设备 */
#define DEVICE_TYPE_VIRTIO_NET              2 /**< virtio net 设备 */
#define DEVICE_TYPE_VIRTIO_BLK              3 /**< virtio blk 设备 */
#define DEVICE_TYPE_VIRTIO_VSOCK            4 /**< virtio vsock 设备 */
#define DEVICE_TYPE_VIRTIO_NET_TRANSITION   5 /**< virtio net transition 设备 */
#define DEVICE_TYPE_VIRTIO_BLK_TRANSITION   6 /**< virtio blk  transition 设备 */
#define DEVICE_TYPE_VIRTIO_SCSI_TRANSITION  7 /**< virtio scsi transition 设备 */
#define DEVICE_TYPE_VIRTIO_HPC              8 /**< virtio nhpc 设备 */
#define DEVICE_TYPE_VIRTIO_FS               9 /**< virtio fs 设备 */

/**
 * @brief 判断是否为virtio net 设备
 * @param device：device type
 * @return true or false
 */
#define MPU_DEVICE_IS_VIRTIO_NET(device) \
    (((device) == DEVICE_TYPE_VIRTIO_NET) || ((device) == DEVICE_TYPE_VIRTIO_NET_TRANSITION))

/**
 * @brief 判断是否为virtio blk 设备
 * @param device：device type
 * @return true or false
 */
#define MPU_DEVICE_IS_VIRTIO_BLK(device) \
    (((device) == DEVICE_TYPE_VIRTIO_BLK) || ((device) == DEVICE_TYPE_VIRTIO_BLK_TRANSITION))

/**
 * @brief 判断是否为virtio scsi 设备
 * @param device：device type
 * @return true or false
 */
#define MPU_DEVICE_IS_VIRTIO_SCSI(device) \
    ((device) == DEVICE_TYPE_VIRTIO_SCSI_TRANSITION)

/**
 * @brief 判断是否为virtio 存储 设备
 * @param device：device type
 * @return true or false
 */
#define MPU_DEVICE_IS_VIRTIO_STORAGE(device) \
    (MPU_DEVICE_IS_VIRTIO_BLK(device) || MPU_DEVICE_IS_VIRTIO_SCSI(device))

/**
 * @brief 判断是否为virtio 设备
 * @param device：device type
 * @return true or false
 */
#define MPU_DEVICE_IS_VIRTIO(device) \
    (MPU_DEVICE_IS_VIRTIO_NET(device) || MPU_DEVICE_IS_VIRTIO_BLK(device) || MPU_DEVICE_IS_VIRTIO_SCSI(device))

enum hinic5_svc_type {
    SVC_T_COMM = 0,
    SVC_T_NIC,
    SVC_T_OVS,
    SVC_T_ROCE,
    SVC_T_TOE,
    SVC_T_IOE,
    SVC_T_FC,
    SVC_T_VBS,
    SVC_T_IPSEC,
    SVC_T_VIRTIO,
    SVC_T_MIGRATE,
    SVC_T_PPA,
    SVC_T_MAX,
};

#define MGMT_MSG_CMD_OP_SET	1 /**< 设置命令 */
#define MGMT_MSG_CMD_OP_GET	0 /**< 获取命令 */
#define MGMT_MSG_CMD_OP_START	1 /**< 开始命令 */
#define MGMT_MSG_CMD_OP_STOP	0 /**< 停止命令 */

#define MGMT_GET_DIE_ID(msg_head) (((msg_head)->die_id_valid != 0) ? (msg_head)->die_id : 0) /**< 获取die id，入参为mgmt_msg_head结构体 */

struct mgmt_msg_head {
    u8 status; /**< 响应消息的返回值 */
    u8 version; /**< 消息的版本号 */
    u8 rep_aeq_num; /**< response aeq number, unused for now */
    u8 rsvd0; /**< 保留字段 */
    u8 die_id_valid : 1; /**< 双die选择有效位（工具命令选择主/从die） */
    u8 die_id : 1; /**< die_id_valid有效的前提下，die_id:0, 主die  die_id:1, 从die */
    u8 rsvd1 : 6; /**< 保留字段 */
    u8 rsvd2[3]; /**< 保留字段 */
};

enum hinic5_fw_ver_type {
    HINIC5_FW_VER_TYPE_BOOT, /**< BOOT固件 */
    HINIC5_FW_VER_TYPE_MPU, /**< MPU固件 */
    HINIC5_FW_VER_TYPE_NPU, /**< NPU固件 */
    HINIC5_FW_VER_TYPE_SMU_L0, /**< SMU L0固件 */
    HINIC5_FW_VER_TYPE_SMU_L1, /**< SMU L1固件 */
    HINIC5_FW_VER_TYPE_CFG, /**< SMU 配置固件 */
    HINIC5_FW_VER_TYPE_PLATFORM,  /**< 基础平台 */
    HINIC5_FW_VER_TYPE_ROCE_SCC, /**< roce scc固件 */
    HINIC5_FW_VER_TYPE_ROCE_SCC_CS, /**< roce scc客户固件 */
    HINIC5_FW_VER_TYPE_ROCE_IMP, /**< roce imp固件 */
    HINIC5_FW_VER_TYPE_UBC_IMP, /**< ubc imp固件 */
    HINIC5_FW_VER_TYPE_IMP, /**< imp固件 */
    HINIC5_FW_VER_TYPE_PSM, /**< PSM固件 */
    HINIC5_FW_VER_TYPE_UBG_IMP, /**< ubg imp固件 */
    HINIC5_FW_VER_TYPE_UB_SCC, /**< ub scc固件 */
    HINIC5_FW_VER_TYPE_GRAY_NPU = 100, /**< 灰卡 NPU固件 */
};

#define PCIE_MODE_PORT_NUM 32
#ifdef HI1825V100
#define PCIE_MODE_HOST_NUM 6
#else
#define PCIE_MODE_HOST_NUM 4
#endif
#define PCIE_MODE_PF_NUM 32
#define PCIE_MODE1_VF_NUM 128
#define PCIE_MODE2_VF_NUM 256
#define PCIE_MODE_HOST2PORT_MAP 4
#define FLR_CUR_REG_NUM 128
#define MPU_FLR_INTR_BIT_NUM 32
#define FLR_STAT_CUR_REG_OFFSET 4
#define PCIE_MODE_FLR_STAT_REG_NUM 8
#define PCIE_FLR_MODE2_REG_OFFSET 64

#define PCIE_MODE_ALL_PORT_MAP 0x1111

/**
 * @brief enum bus_type_e - host侧总线类型
 * @details 用于区分host侧不同总线类型
 */
typedef enum {
    BUS_TYPE_PCIE = 0,      /**< pcie总线 */
    BUS_TYPE_UBC = 1,       /**< ubc总线 */
} bus_type_e;

#endif