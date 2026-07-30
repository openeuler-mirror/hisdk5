/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
 * Filename      : mpu_outband_smbus_cmd.h
 * Version       : Initial Draft
 * Creation time : 2023/09/22
 * Last Modified :
 * Description   : SMBUS protocol out-of-band commands
 */

#ifndef MPU_OUTBAND_SMBUS_CMD_H
#define MPU_OUTBAND_SMBUS_CMD_H

/**
 * @brief SMBUS command code
 */

/* SMBUS 命令字, 操作码定义，与平台保持一致 */
typedef enum {
    SMB_OPC_ABILITY = 0x0,         /**< collect ability info,connect to bmc framework @see smb_ability_s */
    SMB_OPC_DEVICE_HEALTH = 0x1,   /**< get device health status @see struct smb_device_health_s */
    SMB_OPC_ERR_CODE = 0x2,        /**< get error code when status is not 0  @see smb_com_res_s */
    SMB_OPC_TEMP = 0x3,            /**< get temperature @see smb_temperature_s */
    SMB_OPC_FIRMWARE_VER = 0x05,   /**< get fw version @see smb_firmware_ver_s */
    SMB_OPC_TEMP_THRESHOLD = 0x7,  /**< set or get temp threshold @see smb_temp_threshold_data_s */
    SMB_OPC_LOG = 0x0C,            /**< get log @see smb_log_s */
    SMB_OPC_LAST_WORD = 0x0D,      /**< get last word @see smb_log_s */
    SMB_OPC_ROUTING_INSPEC = 0x0E, /**< get inspec info @see smb_log_s */
    SMB_OPC_BOARD_ID = 0x0F,       /**< get borad id @see smb_board_id_s */
    SMB_OPC_PCB_ID = 0x10,         /**< get pcb id @see smb_pcb_id_s */
    SMB_OPC_SET_EEPROM_WP = 0x31,  /**< set EEPROM write protect status @see smb_eeprom_wp_rsp_s */

    /* SLT opcode define add opc need modify get tx len */
    SMB_SLT_OPC_WR = 0x50,   /**< STL write reg data @see smb_read_write_reg_s */
    SMB_SLT_OPC_RD = 0x51,   /**< STL read reg data @see smb_read_write_reg_s */
    SMB_SLT_OPC_TEMP = 0x52, /**< reversed */

    SMB_OPC_MAC_ADDR = 0x404,      /**< get mac addr @see smb_mac_addr_s */
    SMB_OPC_WORK_MAC_ADDR = 0x408,      /**< get work mac addr @see smb_mac_addr_s */
    SMB_OPC_PHYPORT_LINK_INFO = 0x409, /**< get phyport link info @see smb_xsfp_present_status_rep_s */
    SMB_OPC_PCIE_UBC_CAP_INFO = 0x410, /**< get pcie/ubc info @see smb_pcie_ubc_info_s */
    SMB_OPC_PCIE_UBC_INFO = 0x411,     /**< get pcie/ubc info @see smb_pcie_ubc_info_s */
    SMB_OPC_GET_XSFP_STATIC_INFO = 0x412, /**< get xsfp static info @see smb_xsfp_dynamic_info_rep_s */
    SMB_OPC_GET_XSFP_DYNAMIC_INFO = 0x413, /**< get xsfp dynamic info @see smb_xsfp_static_info_rep_s */
    SMB_OPC_XSFP_PERSENT_STATUS = 0x414, /**< get xsfp present status @see smb_xsfp_present_status_rep_s */
    SMB_OPC_SET_SLOT_ID = 0x4F0, /**< set slot_id @see smb_pcie_ubc_info_s */
    SMB_OPC_SET_PORT_NUM = 0x4F1, /**< set port num @see smb_respon_header_s */
    SMB_OPC_GET_PORT_NUM = 0x4F2, /**< get port num @see smb_get_port_num_s */
    SMB_OPC_CONVERGE_PORT_INFO = 0x4F3, /**< get converge port info @see smb_converge_port_info_rep_s */
    SMB_OPC_CONVERGE_XSFP_INFO = 0x4F4, /**< get converge port info @see smb_converge_xsfp_info_rep_s */
    SMB_OPC_GET_FIRMWARE_VER_STR = 0x0030, /**< get fiemware version string @see smb_firmware_ver_str_s */
    SMB_OPC_PCIE_DFX = 0xffffffff, /* pcie dfx带外暂时不实现 */

    /* MAG */
    SMB_OPC_SFP_TEMP_THRESHOLD = 0x8,  /**< get or set sfp temp threshold addr @see smb_sfp_temp_threshold_s */
    /* 注意：SMB_PANGEA_V6_OPC_SFP_TEMP该命令字对应opcode与1872中使用的SMB_OPC_PCB_ID命令字的opcode存在冲突，经讨论，
       针对SMB_PANGEA_V6_OPC_SFP_TEMP该仅在1825盘古场景上使用的命令字在后续使用时再行确定具体opcode值 */
    SMB_PANGEA_V6_OPC_SFP_TEMP = 0x10, /**< PANGEA_V6 get sfp temp @see smb_sfp_temp_threshold_s */
    SMB_PANGEA_V6_OPC_PHY_TEMP = 0x15, /**< not supported */
    SMB_OPC_SFP_TEMP = 0x400,          /**< get sfp temperature @see smb_sfp_temp_s */
    SMB_OPC_LINK_STATUS = 0x403,       /**< get link status @see smb_link_stat_s */
    SMB_OPC_PHY_TEMP = 0x405,          /**< not supported */
    SMB_OPC_RESTORE_VER = 0x406,       /**< mpu restore version */
    SMB_OPC_SFP_ID = 0x407,            /**< get sfp id @see smb_sfp_id_s */

    /* IMU */
    IMU_MCU_OPC_GET_SPU_TEMP = 0x10f0, /**< get Totem temperature @see smb_spu_temp_s */
    IMU_MCU_OPC_SET_SPU_FREQ_UP,       /**< set Totem frequency up @see smb_spu_freq_resp_s */
    IMU_MCU_OPC_SET_SPU_FREQ_DOWN,     /**< set Totem frequency down @see smb_spu_freq_resp_s */
    IMU_MCU_OPC_ENABLE_PWRLIMIT,       /**< enable powerlimit @see smb_spu_pwrlimit_en_s */
    IMU_MCU_OPC_DISABLE_PWRLIMIT,      /**< disable powerlimit @see smb_spu_pwrlimit_en_s */
    IMU_MCU_OPC_GET_SPU_RAS_INFO_NUM,  /**< get Totem RAS info num @see smb_spu_ras_num_s */
    IMU_MCU_OPC_GET_SPU_RAS_INFO,      /**< get Totem RAS info @see smb_spu_ras_s */

    /* DFT */
    SMB_DFT_OPC_EQUIP = 0x04FE,        /**< DFT equip test and use */
} SMBUS_OP_CODE_E;

/* smbus equip sub_opcode使用:装备命令字 opcode = 0x04FE */
typedef enum {
    SMB_SUB_OPC_DIE_ID = 0x1,
    SMB_SUB_OPC_SRAM_MBIST = 0x2,
    SMB_SUB_OPC_DCIP_TEST = 0x3,
    SMB_SUB_OPC_VPD_SET = 0x4,
    SMB_SUB_OPC_VPD_GET = 0x5,
    SMB_SUB_OPC_RESET = 0x6,
    SMB_SUB_OPC_GUID_SET = 0x7,
    SMB_SUB_OPC_GUID_GET = 0x8,
    SMB_SUB_OPC_I2C_TEST = 0x9,

    SMB_SUB_OPC_SET_LOOPBACK = 0x10,
    SMB_SUB_OPC_GET_SNR = 0x11,

    SMB_SUB_OPC_PRBS = 0x20,
    SMB_SUB_OPC_MAC_SET = 0x21,
    SMB_SUB_OPC_MAC_GET = 0x22,
    SMB_SUB_OPC_EFUSE_BURN = 0x23,
    SMB_SUB_OPC_LED_SET = 0x24,
    SMB_SUB_OPC_POWER_GET = 0x25,
    SMB_SUB_OPC_GPIO_TEST = 0x26,

    SMB_SUB_OPC_H2H = 0x40,
} smbus_equip_sub_opcode_e;

#endif
