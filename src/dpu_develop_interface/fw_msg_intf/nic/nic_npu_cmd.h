/*
 * Copyright (C), 2001-2011, Huawei Tech. Co., Ltd.
 * File Name     : nic_npu_cmd.h
 * Version       : Initial Draft
 * Created       : 2019/4/25
 * Last Modified :
 * Description   : NIC Commands between Driver and NPU
 * Function List :
 */

#ifndef NIC_NPU_CMD_H
#define NIC_NPU_CMD_H

/**
 * @enum hinic5_ucode_cmd
 * @brief 定义了各种与NIC ucode相关的命令
 *
 * @details 这个结构体定义了一系列与NIC ucode相关的命令，这些命令用于控制NIC ucode的行为。
 */
enum hinic5_ucode_cmd {
    HINIC5_UCODE_CMD_MODIFY_QUEUE_CTX = 0, /**< 修改队列上下文。@see > hinic5_sq_ctxt_block */
    HINIC5_UCODE_CMD_CLEAN_QUEUE_CONTEXT = 1, /**< 清理队列上下文。@see > hinic5_clean_queue_ctxt */
    HINIC5_UCODE_CMD_ARM_SQ = 2, /**< 未使用 */
    HINIC5_UCODE_CMD_ARM_RQ = 3, /**< 未使用 */
    HINIC5_UCODE_CMD_SET_RSS_INDIR_TABLE = 4, /**< 设置RSS间接表。@see > nic_rss_indirect_tbl */
    HINIC5_UCODE_CMD_SET_RSS_CONTEXT_TABLE = 5, /**< 设置RSS上下文表。@see > nic_rss_context_tbl */
    HINIC5_UCODE_CMD_GET_RSS_INDIR_TABLE = 6,   /**< 获取RSS间接表。@see > l2nic_cmdq_rss_indir_get */
    HINIC5_UCODE_CMD_GET_RSS_CONTEXT_TABLE = 7,  /**< 未使用 */
    HINIC5_UCODE_CMD_SET_IQ_ENABLE = 8, /**< 未使用 */
    HINIC5_UCODE_CMD_SET_RQ_FLUSH = 10, /**< 设置RQ刷新。@see > hinic5_cmd_set_rq_flush */
    HINIC5_UCODE_CMD_MODIFY_VLAN_CTX = 11, /**< 获取rxq信息。@see > nic_vlan_ctx */
    HINIC5_UCODE_CMD_PPA_HASH_TABLE = 12,    /**< 未使用*/
    HINIC5_UCODE_CMD_RXQ_INFO_GET = 13, /**< 获取rxq信息。@see > hinic5_rxq_hw, < rxq_check_info */
    HINIC5_UCODE_MIG_CFG_Q_CTX = 16,    /**< 热迁移操作上下文表。@see > l2nic_migrate_op_ctx */
    HINIC5_UCODE_MIG_CHK_SQ_STOP = 17,   /**< 未使用 */
    HINIC5_UCODE_MIG_CHK_RQ_STOP = 18,   /**< 热迁移检查rq启停状态。@see > l2nic_cmdq_migrate_check_rq_stop */
    HINIC5_UCODE_MIG_CHK_CMDQ_STOP = 19, /**< 热迁移检查cmdq启停状态。@see > l2nic_cmdq_migrate_check_cmdq_stop */
    HINIC5_UCODE_MIG_CFG_BAT_INFO = 20,  /**< 热迁移读写bat表。@see > l2nic_migrate_cfg_bat */
    HINIC5_UCODE_MIG_COMPENSATE_INTR = 21,   /**< 热迁移中断补偿。@see > l2nic_cmdq_migrate_compensate_intr */
    HINIC5_UCODE_MIG_CFG_FAST_MSG = 22,
    HINIC5_UCODE_CMD_CLEAR_VPORT_STATS = 23,   /**< 获取counter统计。@see > l2nic_cmdq_get_vport_stats */
    HINIC5_UCODE_CMD_GET_VPORT_STATS = 24,   /**< 清除counter统计。@see > l2nic_cmdq_clear_vport_stats */

    HINIC5_UCODE_CMD_EXTEND_SECTION1_START = 192, /** < NIC CMDQ保留命令字起始，计算产品线使用 */
    HINIC5_UCODE_CMD_EXTEND_SECTION1_END = 223, /** < NIC CMDQ保留命令字结束，计算产品线使用 */
    HINIC5_UCODE_CMD_EXTEND_SECTION2_START = 224, /** < NIC CMDQ保留命令字起始，数存产品线使用 */
    HINIC5_UCODE_CMD_EXTEND_SECTION2_END = 255,  /** < NIC CMDQ保留命令字结束，数存产品线使用 */

    HINIC5_UCODE_CMD_MAX = 255,
};

#endif /* NIC_NPU_CMD_H */
