/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: serdes/mag cmd definition between driver and mpu
 * Author: ETH group
 * Create: 2021-07-30
 */

#ifndef MAG_MPU_CMD_H
#define MAG_MPU_CMD_H

/* serdes/mag消息命令字定义 */
enum mag_cmd {
    /* serdes命令字，统一封装所有serdes命令 */
    SERDES_CMD_PROCESS = 0,

    /* mag命令字，按功能划分 */
    /* 端口配置相关 0-29 */
    MAG_CMD_SET_PORT_CFG = 1,
    MAG_CMD_SET_PORT_ADAPT = 2,
    MAG_CMD_CFG_LOOPBACK_MODE = 3,

    MAG_CMD_GET_PORT_ENABLE = 5,
    MAG_CMD_SET_PORT_ENABLE = 6,
    MAG_CMD_GET_LINK_STATUS = 7,
    MAG_CMD_SET_LINK_FOLLOW = 8,
    MAG_CMD_SET_PMA_ENABLE = 9,
    MAG_CMD_CFG_FEC_MODE = 10,

    MAG_CMD_CFG_AN_TYPE = 12, /* reserved for future use */
    MAG_CMD_CFG_LINK_TIME = 13,

    MAG_CMD_SET_PANGEA_ADAPT = 15,
    MAG_CMD_HILINK_MODE = 16, /* set port hilink debug mode @see struct mag_cmd_hilink_mode */

    /* bios link配置相关 30-49 */
    MAG_CMD_CFG_BIOS_LINK_CFG = 31,
    MAG_CMD_RESTORE_LINK_CFG = 32,
    MAG_CMD_ACTIVATE_BIOS_LINK_CFG = 33,

    /* 光模块、LED、PHY等外设配置管理 50-99 */
    /* LED */
    MAG_CMD_SET_LED_CFG = 50,

    /* PHY */
    MAG_CMD_GET_PHY_INIT_STATUS = 55, /* reserved for future use */

    /* 光模块 */
    MAG_CMD_GET_XSFP_INFO = 60,
    MAG_CMD_SET_XSFP_ENABLE = 61,
    MAG_CMD_GET_XSFP_PRESENT = 62,
    MAG_CMD_SET_XSFP_RW = 63, /* sfp/qsfp single byte read/write, for equipment test */
    MAG_CMD_CFG_XSFP_TEMPERATURE = 64,
    MAG_CMD_SET_XSFP_TLV_INFO = 65,
    MAG_CMD_GET_XSFP_TLV_INFO = 66,

    /* 事件上报 100-149 */
    MAG_CMD_WIRE_EVENT = 100,
    MAG_CMD_LINK_ERR_EVENT = 101,

    /* DFX、Counter相关 */
    MAG_CMD_EVENT_PORT_INFO = 150,
    MAG_CMD_GET_PORT_STAT = 151,
    MAG_CMD_CLR_PORT_STAT = 152,
    MAG_CMD_GET_PORT_INFO = 153,
    MAG_CMD_GET_PCS_ERR_CNT = 154,
    MAG_CMD_GET_MAG_CNT = 155,
    MAG_CMD_DUMP_ANTRAIN_INFO = 156,
    MAG_CMD_GET_UBMAC_COUNTER = 157,
    MAG_CMD_GET_HIMAC_BER = 158,
    MAG_CMD_OP_HIMAC_THRD = 159,
    MAG_CMD_GET_HIMAC_BANDWIDTH = 160,
    MAG_CMD_SET_HIMAC_PRBS = 161,
    MAG_CMD_GET_HIMAC_PRBS = 162,

    /* patch预留cmd */
    MAG_CMD_PATCH_RSVD_0 = 200,
    MAG_CMD_PATCH_RSVD_1 = 201,
    MAG_CMD_PATCH_RSVD_2 = 202,
    MAG_CMD_PATCH_RSVD_3 = 203,
    MAG_CMD_PATCH_RSVD_4 = 204,

    MAG_CMD_MAX = 0xFF
};

#endif
