/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 * Description: RDMA npu commonds.
 * Create: 2023-10-11
 */

#ifndef ROCE_NPU_CMD_H
#define ROCE_NPU_CMD_H

/*
 * Commands between RoCE driver to NPU
 */
enum {
    /* ULP CMD, ULP共预留16*4个, 范围0x20-0x5f */
    /* 给计算分配16个, 范围0x20-0x2f */
    ROCE_ULP_CMD_START = 0x20,
    ROCE_ULP_CMD_END = 0x5f,

    /* GID CMD */
    ROCE_CMD_UPDATE_GID = 0x60, /**< Update GID table @see > roce_uni_cmd_update_gid_s */
    ROCE_CMD_QUERY_GID = 0x61, /**< Query GID table @see > roce_uni_cmd_query_gid_s */
    ROCE_CMD_CLEAR_GID = 0x62, /**< Clear GID table @see > roce_uni_cmd_clear_gid_s */

    /* TPT CMD */
    ROCE_CMD_SW2HW_MPT = 0x70, /**< Init MPT context to HW @see > roce_uni_cmd_mpt_sw2hw_s */
    ROCE_CMD_HW2SW_MPT = 0x71, /**< Deinit MPT context from HW @see > roce_uni_cmd_mpt_hw2sw_s */
    ROCE_CMD_MODIFY_MPT = 0x72, /**< Unused */
    ROCE_CMD_QUERY_MPT = 0x73, /**< Query MPT context from HW @see > roce_uni_cmd_mpt_query_s */
    ROCE_CMD_FLUSH_TPT = 0x74, /**< Unused */
    ROCE_CMD_SYNC_TPT = 0x75, /**< Unused */

    /* CQ CMD */
    ROCE_CMD_SW2HW_CQ = 0x80, /**< Init CQ context to HW @see > roce_uni_cmd_create_cq_s */
    ROCE_CMD_RESIZE_CQ = 0x81, /**< Resize CQ and notify HW @see > roce_uni_cmd_resize_cq_s */
    ROCE_CMD_MODIFY_CQ = 0x82, /**< Modify CQ context in HW @see > roce_uni_cmd_modify_cq_s */
    ROCE_CMD_HW2SW_CQ = 0x83, /**< Deinit CQ context from HW @see > roce_uni_cmd_cq_hw2sw_s */
    ROCE_CMD_QUERY_CQ = 0x84, /**< Query CQ context in HW @see > roce_uni_cmd_cq_query_s */

    /* SRQ CMD */
    ROCE_CMD_SW2HW_SRQ = 0x90, /**< Init SRQ context to HW @see > roce_uni_cmd_create_srq_s */
    ROCE_CMD_ARM_SRQ = 0x91, /**< Arm SRQ to HW @see > roce_uni_cmd_srq_arm_s */
    ROCE_CMD_HW2SW_SRQ = 0x92, /**< Deinit SRQ context from HW @see > roce_uni_cmd_srq_hw2sw_s */
    ROCE_CMD_QUERY_SRQ = 0x93, /**< Query SRQ context in HW @see > roce_uni_cmd_srq_query_s */

    /* QP CMD */
    ROCE_CMD_RST2INIT_QP = 0xa0, /**< Modify QP context from reset to init state @see > roce_uni_cmd_modify_qpc_s */
    ROCE_CMD_INIT2INIT_QP = 0xa1, /**< Modify QP context from init to init state @see > roce_uni_cmd_modify_qpc_s */
    ROCE_CMD_INIT2RTR_QP = 0xa2, /**< Modify QP context from init to rtr state @see > roce_uni_cmd_modify_qpc_s */
    ROCE_CMD_RTR2RTS_QP = 0xa3, /**< Modify QP context from rtr to rts state @see > roce_uni_cmd_modify_qpc_s */
    ROCE_CMD_RTS2RTS_QP = 0xa4, /**< Modify QP context from rts to rts state @see > roce_uni_cmd_modify_qpc_s */
    ROCE_CMD_SQERR2RTS_QP = 0xa5, /**< Modify QP context from sqerr to rts state @see > roce_uni_cmd_modify_qpc_s */
    ROCE_CMD_2ERR_QP = 0xa6, /**< Modify QP context from any state to err state @see > roce_uni_cmd_modify_qpc_s */
    ROCE_CMD_RTS2SQD_QP = 0xa7, /**< Modify QP context from rts to sqd state @see > roce_uni_cmd_qp_modify_rts2sqd_s */
    ROCE_CMD_SQD2SQD_QP = 0xa8, /**< Modify QP context from sqd to sqd state @see > roce_uni_cmd_modify_qpc_s */
    ROCE_CMD_SQD2RTS_QP = 0xa9, /**< Modify QP context from sqd to rts state @see > roce_uni_cmd_modify_qpc_s */
    ROCE_CMD_2RST_QP = 0xaa, /**< Modify QP context from any state to reset state @see > roce_uni_cmd_qp_modify2rst_s */
    ROCE_CMD_QUERY_QP = 0xab, /**< Query QP context from HW @see > roce_uni_cmd_qp_query_s */
    ROCE_CMD_MIRROR_QP = 0xac, /**< Unused */
    ROCE_CMD_MODIFY_HASH_VALUE_QP = 0xad, /**< Modify bond hash in QP context @see > roce_uni_cmd_modify_hash_s */
    ROCE_CMD_GET_RX_PORT_QP = 0xae, /**< Query rx port from QP context @see > roce_uni_cmd_get_port_info_s */
    ROCE_CMD_MODIFY_UDP_SRC_PORT_QP = 0xaf, /**< Modify src port in QP context @see > roce_uni_cmd_set_udp_src_port_s */
    ROCE_CMD_GET_UDP_SRC_PORT_QP = 0xb0, /**< Query udp src port from QP context @see > roce_uni_cmd_get_port_info_s */
    ROCE_CMD_GET_HASH_VALUE_QP = 0xb1, /**< Query hash from QP context @see > roce_uni_cmd_get_hash_s */

    /* HyperRoCE CMD */
    RoCE_CMD_HYPER_ROCE = 0xbf, /**< HyperRoCE CMD entrance @see > e.g.roce_uni_cmd_enable_hyper_roce_s */

    /* MTT commands */
    ROCE_CMD_QUERY_MTT = 0xc0, /**< Unused */

    /* DFX commands */
    ROCE_CMD_MODIFY_CONTEXT = 0xd0, /**< Unused */
    ROCE_CMD_EN_QP_CAP_PKT = 0xd1, /**< Unused */
    ROCE_CMD_DIS_QP_CAP_PKT = 0xd2, /**< Unused */

    /* COMM commands */
    ROCE_CMD_GET_CHIP_ABILITY = 0xd3, /**< Provide cmd for feature negotiate @see > roce_get_chip_ability_s */

    /* LATCH commands */
    ROCE_CMD_LATCH_DFX = 0xd4,        /** 锁存的dfx接口处理 */
    ROCE_CMD_SET_LATCH_QP = 0xd5, /**< Set RoCE latch RDMARC table attribute to qpc @see > roce_uni_cmd_modify_qpc_s */

    /* CACHE commands */
    ROCE_CMD_MISC_CACHE_INVLD = 0xe0, /**< Invalid QP context cache from HW @see > roce_uni_cmd_qp_cache_invalid_s */
    ROCE_CMD_MISC_CQ_CACHE_INVLD = 0xf0, /**< Invalid CQ context cache from HW @see > roce_uni_cmd_cq_cache_invalid_s */

    /* ULP CMD */
    ROCE_CMD_CREAT_SLAVE_QP = 0xf1, /**< Unused */
    ROCE_CMD_QUERY_MASTER_QP_BITMAP = 0xf2, /**< Unused */
    ROCE_CMD_GET_MASTER_QPN = 0xf3, /**< Unused */
    ROCE_CMD_SET_CONN_STAT = 0xf4, /**< Unused */
    ROCE_CMD_DISCONNECT_QP = 0xf5, /**< Unused */
    ROCE_CMD_SET_SHARD_CFG = 0xf6, /**< Unused */

    ROCE_CMD_ULP_EXTEND = 0xff, /* Command word entry provided to ULP */
};

#define ROCE_VERBS_CMD_TYPE_GET(sub_type) ((sub_type) >> 4)
enum {
    ROCE_GID_CMD = 0x6,
    ROCE_MR_CMD = 0x7,
    ROCE_CQ_CMD = 0x8,
    ROCE_SRQ_CMD = 0x9,
    ROCE_QP_CMD = 0xa,
    ROCE_MTT_CMD = 0xc,
    ROCE_DFX_CMD = 0xd,
    ROCE_CACHE_INLVD_CMD = 0xe
};
#endif /* ROCE_NPU_CMD_H */
