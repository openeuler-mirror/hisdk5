/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
 * Filename      : macsec_mpu_cmd.h
 * Creation time : 2024/02/23
 * Description   : COMM Commands between host and MPU(macsec)
 * Version       : 1.0
 */

#ifndef MACSEC_MPU_CMD_H
#define MACSEC_MPU_CMD_H

#ifndef BIT
#define BIT(n) (1UL << (n))
#endif

/**
 * @brief macsec_mgmt_cmd_e - COMM Commands between hinicadm to MPU(macsec)
 * @details MACSEC与驱动之间交互的操作码，涉及SC、SA、MIB以及SERVICE的管理
 */
typedef enum {
    MACSEC_CMD_SC_OP = 0, /**< sc operation @see struct tag_macsec_cmd_sc_operation_s */
    MACSEC_CMD_SA_OP, /**< sa operation @see struct tag_macsec_cmd_sa_operation_s */
    MACSEC_CMD_GET_PORT_MIB, /**< get port mib @see struct tag_macsec_cmd_port_mib_operation_s */
    MACSEC_CMD_GET_SC_MIB, /**< get sc mib @see struct tag_macsec_cmd_sc_mib_operation_s */
    MACSEC_CMD_GET_ERR_CNT, /**< get error cnt @see struct tag_macsec_cmd_err_cnt_operation_s */
    MACSEC_CMD_SERVICE_OP, /**< service op @see struct macsec_cmd_service_operation_s */
    MACSEC_CMD_FEATURE_NEGO_OP, /**< macsec feature negotiate op @see struct macsec_feature_nego_cmd_s */
    MACSEC_CMD_FLUSH_OP, /**< macsec resource flush op @see struct tag_macsec_flush_cmd_s */
    MACSEC_CMD_WHITELIST_OP, /**< macsec whitelist op @see struct macsec_whitelist_cmd_out */
} macsec_mgmt_cmd_e;

/**
 * @brief macsec_mbox_sc_op_cmd_e - opcode of mailbox sc opcode
 * @details SC加解密侧的操作码，涉及创建、删除和更新等操作
 */
typedef enum {
    MACSEC_CMD_ENC_SC_CREATE = 0, /**< enc sc create */
    MACSEC_CMD_ENC_SC_DELETE, /**< enc sc delete */
    MACSEC_CMD_ENC_SC_UPDATE, /**< enc sc update */
    MACSEC_CMD_ENC_SC_GET_INFO, /**< get enc sc */
    MACSEC_CMD_DEC_SC_CREATE, /**< dec sc create */
    MACSEC_CMD_DEC_SC_DELETE, /**< dec sc delete */
    MACSEC_CMD_DEC_SC_UPDATE, /**< dec sc update */
    MACSEC_CMD_DEC_SC_GET_INFO, /**< get dec sc */
} macsec_mbox_sc_op_cmd_e;

/**
 * @brief macsec_mbox_sa_op_cmd_e - opcode of mailbox sa opcode
 * @details SA加解密侧的操作码，涉及创建、删除和更新等操作
 */
typedef enum {
    MACSEC_CMD_ENC_SA_CREATE = 0, /**< enc sa create */
    MACSEC_CMD_ENC_SA_DELETE, /**< enc sa delete */
    MACSEC_CMD_ENC_SA_UPDATE, /**< enc sa update */
    MACSEC_CMD_ENC_SA_GET_INFO, /**< get enc sa */
    MACSEC_CMD_DEC_SA_CREATE, /**< dec sa create */
    MACSEC_CMD_DEC_SA_DELETE, /**< dec sa delete */
    MACSEC_CMD_DEC_SA_UPDATE, /**< dec sa update */
    MACSEC_CMD_DEC_SA_GET_INFO, /**< get dec sa */
} macsec_mbox_sa_op_cmd_e;

/**
 * @brief macsec_mbox_service_op_cmd_e - opcode of mailbox service opcode
 * @details MACSEC服务相关操作，如驱动加载时的使能MACSEC功能，驱动卸载时的关闭MACSEC功能
 */
typedef enum {
    MACSEC_CMD_SERVICE_OP_MACSEC_DISABLE = 0, /**< macsec disable */
    MACSEC_CMD_SERVICE_OP_MACSEC_ENABLE, /**< macsec enable */
} macsec_mbox_service_op_cmd_e;

/**
 * @brief macsec_mbox_feature_nego_op_cmd_e - opcode of mailbox feature negotiate opcode
 * @details MACSEC特性协商相关操作，对特性进行查询或者设置
 */
typedef enum {
    MACSEC_FEATURE_NEGO_OPCODE_GET = 0, /**< feature negotiation get */
    MACSEC_FEATURE_NEGO_OPCODE_SET = 1, /**< feature negotiation set (reserve for using) */
} macsec_mbox_feature_nego_op_cmd_e;

/**
 * @brief macsec_mbox_feature_cap_e - list of features supported by macsec
 * @details MACSEC支持特性列表
 */
typedef enum {
    MACSEC_F_HARDEN_PATH = BIT(0), /**< macsec does not have NPU */
    MACSEC_F_SUPPORT_SM4 = BIT(1), /**< macsec support using sm4 */
} macsec_mbox_feature_cap_e;

/**
 * @brief macsec_mbox_flush_op_cmd_e - opcode of mailbox macsec flush opcode
 * @details MACSEC资源清理相关操作
 */
typedef enum {
    MACSEC_CMD_FLUSH_SC_OP = 0, /**< flush sc and sa resource */
    MACSEC_CMD_FLUSH_SA_OP, /**< flush sa resource */
} macsec_mbox_flush_op_cmd_e;

#endif /* MACSEC_MPU_CMD_H */
