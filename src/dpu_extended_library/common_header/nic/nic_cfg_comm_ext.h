/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026-2026. All rights reserved.
 * Description: fw common cmd related structure and macro defined here
 * Create: 2026/03/21
 */

#ifndef NIC_CFG_COMMON_EXT_H
#define NIC_CFG_COMMON_EXT_H

#define NIC_MAX_PF_NUM    32

#define CAL_NIC_F_1823_MASK 0x1FFEF   /**< 1823的所有属性 */
#define CAL_NIC_F_1825_MASK (NIC_F_CSUM | NIC_F_SCTP_CRC | NIC_F_TSO | NIC_F_LRO | NIC_F_RSS | NIC_F_RX_VLAN_FILTER | \
            NIC_F_RX_VLAN_STRIP | NIC_F_TX_VLAN_INSERT | NIC_F_VXLAN_OFFLOAD | NIC_F_IPSEC_OFFLOAD | NIC_F_FDIR | \
            NIC_F_PROMISC | NIC_F_ALLMULTI | NIC_F_XSFP_REPORT | NIC_F_VF_MAC | NIC_F_RATE_LIMIT | \
            NIC_F_TX_WQE_COMPACT_TASK | NIC_F_HALF_BOND_OFFLOAD | NIC_F_GET_COUNTER_BY_CMDQ)

#define CAL_NIC_F_182X_MASK (CAL_NIC_F_1823_MASK | CAL_NIC_F_1825_MASK) /**< 182x的所有属性 */

#define CAL_NIC_F_1872_PF_MASK (NIC_F_CSUM | NIC_F_SCTP_CRC | NIC_F_TSO | NIC_F_TX_WQE_COMPACT_TASK | NIC_F_RX_HW_COMPACT_CQE | \
            NIC_F_HTN_CMDQ | NIC_F_PROMISC | NIC_F_ALLMULTI | NIC_F_VXLAN_OFFLOAD | NIC_F_GENEVE_OFFLOAD |  \
            NIC_F_IPXIP_OFFLOAD | NIC_F_RX_VLAN_STRIP | NIC_F_TX_VLAN_INSERT | NIC_F_RSS | NIC_F_RX_VLAN_FILTER |   \
            NIC_F_LRO | NIC_F_FDIR | NIC_F_HTN_FDIR | NIC_F_SQ_RQ_CI_COALESCE | NIC_F_PTP_1588_V2 | NIC_F_TC_FLOWER_OFFLOAD | \
            NIC_F_MACSEC_OFFLOAD | NIC_F_VEB_OFFLOAD | NIC_F_RATE_LIMIT | NIC_F_HTN_CMDQ_CAR | NIC_F_ARP_DUAL | \
            NIC_F_XSFP_REPORT) /**< 187x PF的所有属性 */

#define CAL_NIC_F_1872_VF_MASK (NIC_F_CSUM | NIC_F_SCTP_CRC | NIC_F_TSO | NIC_F_TX_WQE_COMPACT_TASK | NIC_F_RX_HW_COMPACT_CQE | \
            NIC_F_HTN_CMDQ | NIC_F_ALLMULTI | NIC_F_VXLAN_OFFLOAD | NIC_F_GENEVE_OFFLOAD | NIC_F_IPXIP_OFFLOAD | \
            NIC_F_RX_VLAN_STRIP | NIC_F_TX_VLAN_INSERT | NIC_F_RSS | NIC_F_RX_VLAN_FILTER | NIC_F_LRO | NIC_F_FDIR | \
            NIC_F_HTN_FDIR | NIC_F_SQ_RQ_CI_COALESCE | NIC_F_TC_FLOWER_OFFLOAD | NIC_F_MACSEC_OFFLOAD | \
            NIC_F_VEB_OFFLOAD | NIC_F_RATE_LIMIT | NIC_F_HTN_CMDQ_CAR | NIC_F_ARP_DUAL) /**< 187x VF的所有属性 */

#define CAL_NIC_F_1872_MASK (CAL_NIC_F_1872_PF_MASK | CAL_NIC_F_1872_VF_MASK)
#define CAL_NIC_F_ALL_MASK (CAL_NIC_F_182X_MASK | CAL_NIC_F_1872_MASK)


struct tcam_ext_result {
    union {
        struct {
#if defined(BYTE_ORDER) && (BYTE_ORDER == BIG_ENDIAN)
            u32 rsvd : 6;
            u32 rss_level : 2;
            u32 rsvd0 : 1;
            u32 rss_node_id : 5;
            u32 rss_instance_id : 6;
            u32 rss_temp_id : 12;
#else
            u32 rss_temp_id : 12;
            u32 rss_instance_id : 6;
            u32 rss_node_id : 5;
            u32 rsvd0 : 1;
            u32 rss_level : 2;
            u32 rsvd : 6;
#endif
        } dw0;
        u32 qid;
    };

    union {
        struct {
#if defined(BYTE_ORDER) && (BYTE_ORDER == BIG_ENDIAN)
            u32 rsvd0 : 12;
            u32 action : 4; /* action: 0-func单queue, 1-func RSS, 2-port, 5-drop */
            u32 rsvd1 : 6;
            u32 id : 10;  /* func_id or port_id or qgrp_id */
#else
            u32 id : 10;  /* func_id or port_id or qgrp_id */
            u32 rsvd1 : 6;
            u32 action : 4; /* action: 0-func单queue, 1-func RSS, 2-port, 5-drop */
            u32 rsvd0 : 12;
#endif
        } dw1;
        u32 ext;
    };
};

enum tcam_result_action {
    TCAM_RESULT_ACTION_FUNC = 0, /* func single queue */
    TCAM_RESULT_ACTION_RSS = 1, /* func rss */
    TCAM_RESULT_ACTION_PORT = 2, /* hairpin, port */
    TCAM_RESULT_ACTION_DROP = 5, /* drop */
};

#define SML_QUEUE_GROUP_TBL_ENTRY_QNUM 24   /* sml表项中qgrp_ids的数量，entry_size = 48B / sizeof(u16) */
#define SML_QUEUE_GROUP_TBL_ENTRY_INDEX(global_qid) (u16)((global_qid) / (SML_QUEUE_GROUP_TBL_ENTRY_QNUM)) /* sml表项的索引 */
#define SML_QUEUE_GROUP_TBL_ENTRY_QOFFSET(global_qid) (u16)((global_qid) % (SML_QUEUE_GROUP_TBL_ENTRY_QNUM)) /* sml表项中qgrp_ids的偏移 */

/* global_qid -> group_id映射表表项 */
typedef struct sml_queue_group_tbl {
    u16 qgrp_ids[SML_QUEUE_GROUP_TBL_ENTRY_QNUM];
} sml_queue_group_tbl_s;

#endif