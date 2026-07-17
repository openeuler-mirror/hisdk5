/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2022. All rights reserved.
 * Description   : RoCE MPU cmd
 * Author        : /
 * Create        : /
 * Notes         : /
 * History       : /
 */

#ifndef ROCE_MPU_CMD_H
#define ROCE_MPU_CMD_H

/* Commands between RoCE driver to MPU */
enum {
    /* FUNC CFG */
    ROCE_MPU_CMD_SET_FUNC_STATE = 0, /**< Set roce_vld in function table @see > roce_set_func_state_cmd_s */
    ROCE_MPU_CMD_SET_CPU_ENDIAN, /**< Set cpu endian in function table @see > roce_set_cpu_endian_cmd_s */
    ROCE_MPU_CMD_GET_CFG_INFO, /**< Get roce-related configuration from cfg_data @see > roce_get_cfg_info_cmd_s */
    ROCE_MPU_CMD_DEL_FUNC_RES, /**< Clear relevant resources when FLR @see > roce_set_func_state_cmd_s */
    ROCE_MPU_CMD_GET_FUNC_TABLE, /**< Get function table @see > roce_get_func_table_cmd_s */
    ROCE_MPU_CMD_ADD_MAC = 10, /**< Add MAC to MAC table @see > roce_cfg_mac_cmd_s, Unused in HI1825V100 */
    ROCE_MPU_CMD_DEL_MAC, /**< Delete MAC from MAC table @see > roce_cfg_mac_cmd_s, Unused in HI1825V100 */
    ROCE_MPU_CMD_ADD_IPSU_MAC, /**< Add entry to IPSURX VF table MAC mode @see > roce_cfg_ipsu_mac_cmd_s */
    ROCE_MPU_CMD_DEL_IPSU_MAC, /**< Delete entry from IPSURX VF table MAC mode @see > roce_cfg_ipsu_mac_cmd_s */
    ROCE_MPU_CMD_GET_MAC_VNI, /**< Get vxlan roce vni and mac @see > vroce_mac_cfg_vni_info_s */
    ROCE_MPU_CMD_ADD_IPSU_IP, /**< Add entry to IPSURX VF table IP mode @see > roce_cfg_ipsu_ip_cmd_s */
    ROCE_MPU_CMD_DEL_IPSU_IP, /**< Delete entry from IPSURX VF table IP mode @see > roce_cfg_ipsu_ip_cmd_s */
    ROCE_MPU_CMD_ADD_IPSU_TCAM, /**< Add entry to IPSURX TCAM table @see > roce_cfg_ipsu_tcam_cmd_s */
    ROCE_MPU_CMD_DEL_IPSU_TCAM, /**< Delete entry from IPSURX TCAM table @see > roce_cfg_ipsu_tcam_cmd_s */

    /* BOND (V200 Discarded) */
    ROCE_MPU_CMD_BOND_SET_STATE = 20, /**< Unused */
    ROCE_MPU_CMD_BOND_GET_ER_FWD_ID, /**< Unused */
    ROCE_MPU_CMD_BOND_SET_ER_FWD_ID, /**< Unused */
    ROCE_MPU_CMD_BOND_ER_FWD_ID_COMBINE, /**< Unused */
    ROCE_MPU_CMD_BOND_ER_FWD_ID_COMPACT, /**< Unused */

    /* INNER ULP */
    ROCE_MPU_CMD_ULP_VROCE_GET_GROUP_ID = 25, /**< < Get vxlan roce group cos num @see > roce_cmd_get_group_id */

    /* CC */
    ROCE_MPU_CMD_CC_CFG_CC_PARAM = 40, /**< Config CC param table @see > roce_cc_cfg_param_cmd_s */
    ROCE_MPU_CMD_CC_CFG_DCQCN_PARAM, /**< Config DCQCN param table @see > roce_cc_cfg_param_cmd_s */
    ROCE_MPU_CMD_CC_CFG_IPQCN_PARAM, /**< Config IPQCN param table @see > roce_cc_cfg_param_cmd_s */
    ROCE_MPU_CMD_CC_CFG_LDCP_PARAM, /**< Config LDCP param table @see > roce_cc_cfg_param_cmd_s */
    ROCE_MPU_CMD_CC_SET_BW_CTRL, /**< Unused */
    ROCE_MPU_CMD_CC_QUERY_BW_CTRL, /**< Unused */
    ROCE_MPU_CMD_CC_CFG_UCC_PARAM, /**< Config UCC param table @see > roce_cc_cfg_param_cmd_s */
    ROCE_MPU_CMD_CC_RTT_TCAM_CTRL, /**< 计算ULP占用该位 */
    ROCE_MPU_CMD_CC_SET_VNIC_WATERLINE, /**< Config VNIC waterline @see > roce_cc_cfg_vnic_waterline_cmd_s */
    ROCE_MPU_CMD_CC_GET_VNIC_WATERLINE, /**< READ VNIC waterline @see > roce_cc_cfg_vnic_waterline_cmd_s */
    ROCE_MPU_CMD_CC_SET_BW_WATERLINE, /**< Config BW waterline @see > roce_cc_cfg_bw_wl_cmd_s */
    ROCE_MPU_CMD_CC_GET_BW_WATERLINE, /**< Read BW waterline @see > roce_cc_cfg_bw_wl_cmd_s */

    /* DFX */
    ROCE_MPU_CMD_DFX_CACHE_OUT = 55, /**< Unused */
    ROCE_MPU_CMD_DFX_SET_CAP_CFG, /**< Unused */
    ROCE_MPU_CMD_DFX_GET_CAP_CFG, /**< Unused */
    ROCE_MPU_CMD_DFX_READ_CAP_CTR, /**< Unused */
    ROCE_MPU_CMD_DFX_CLEAR_CAP_CTR, /**< Unused */
    ROCE_MPU_CMD_DFX_SET_ICDQ_SCOREBOARD, /**< 计算ULP占用该位 */
    ROCE_MPU_CMD_DFX_QUERY_ICDQ_SCOREBOARD, /**< 计算ULP占用该位 */
    ROCE_MPU_CMD_DFX_PORT_TRAFFIC, /**< Read port traffic counters @see > roce_mpu_port_statics_cmd_inbuf_t */
    ROCE_MPU_CMD_DFX_ROCE_SET, /**< Set roce configs @see > roce_dfx_set_cmd_s */

    /* EXTERN ULP, ULP MAILBOX命令字共预留128个, 范围128-255, 基础roce不可使用 */
    ROCE_MPU_CMD_ULP_AA_SET_DD_CFG = 128, /**< Unused */
    ROCE_MPU_CMD_ULP_AA_CTRL_READY, /**< Unused */
    ROCE_MPU_CMD_ULP_AA_SWITCH_IO, /**< Unused */
    ROCE_MPU_CMD_ULP_AA_FAKE_DATA, /**< Unused */
    ROCE_MPU_CMD_ULP_AA_CLAER_ACT_CTRL_BMP, /**< Unused */
    ROCE_MPU_CMD_ULP_EXTEND, /* 提供给ulp的mailbox命令字总入口 */

    /* 给计算分配32个命令字, 范围224-255 */
    ROCE_MPU_CMD_MAX = 256
};

#endif /* ROCE_MPU_CMD_H */
