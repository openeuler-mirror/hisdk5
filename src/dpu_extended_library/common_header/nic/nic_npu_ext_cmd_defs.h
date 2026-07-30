/*
* Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
* Description: npu common cmd related structure and macro defined here
* Create: 2025/11/26
*/

#ifndef NIC_NPU_CMD_DEFS_EXT_H
#define NIC_NPU_CMD_DEFS_EXT_H

#define Q_GRP_FUNC_INDX_OFFSET 2048 // qgroup相对于func偏置
#define Q_GRP_FUNC_INDX_NUM 256     // 单卡最大的qgroup

enum hinic5_nic_cmdq_id_type {
    NIC_CMDQ_ID_TYPE_FUNCTION_ID,
    NIC_CMDQ_ID_TYPE_QGRP_ID,
};

struct nic_rss_indirect_tbl_user_data {
    union {
        struct {
#if defined(BYTE_ORDER) && (BYTE_ORDER == BIG_ENDIAN)
            u32 op_code : 8; /* 0:old option 1:new fdir-rss */
            u32 qgrp_id : 8; /* need add 2048 */
            u32 rsvd : 16;
#else
            u32 rsvd : 16;
            u32 qgrp_id : 8; /* need add 2048 */
            u32 op_code : 8; /* 0:old option 1:new fdir-rss */
#endif
        } bs;
        u32 value;
    } dw0;
    union {
        struct {
#if defined(BYTE_ORDER) && (BYTE_ORDER == BIG_ENDIAN)
            u32 rss_temp_id : 12;
            u32 rss_instance_id : 6;
            u32 rss_node_id : 5;
            u32 rsvd : 9;
#else
            u32 rsvd : 9;
            u32 rss_node_id : 5;
            u32 rss_instance_id : 6;
            u32 rss_temp_id : 12;
#endif
        } bs;
        u32 value;
    } dw1;
};

#endif