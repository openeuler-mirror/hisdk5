/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 * Description: HyperRoCE verbs QP processing.
 */

#ifndef ROCE_HYPER_NPU_CMD_H
#define ROCE_HYPER_NPU_CMD_H

#include "roce_npu_cmd_defs.h"

/**< hyperRoCE sub_cmds */
enum hyper_sub_cmd {
    RoCE_CMD_ENABLE_HYPER_ROCE_QP,
    RoCE_CMD_HYPER_ROCE_QUERY_QP_FIRST_512B,
    RoCE_CMD_HYPER_ROCE_QUERY_QP_LAST_512B,
    RoCE_CMD_INIT_MSNC_QP
};

typedef struct roce_verbs_enable_hyper_qp {
    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 srp_en : 1;
            u32 hyper_mode : 2;
            u32 rsvd : 29;
#else
            u32 rsvd : 29;
            u32 hyper_mode : 2;
            u32 srp_en : 1;
#endif
        } bs;
        u32 value;
    } dw0;
} roce_verbs_enable_hyper_qp_info_s;

typedef struct roce_uni_cmd_enable_hyper_roce_cmd {
    roce_verbs_cmd_header_s com;
    roce_verbs_enable_hyper_qp_info_s cmd_info;
} roce_uni_cmd_enable_hyper_roce_s;

/**< init tx/rxmsnc data struct */
typedef struct roce_verbs_init_msnc_info {
    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 txmsn_entry_size : 2;
            u32 txmsn_table_size : 3;
            u32 sack_bitmap_size : 3;
            u32 txmsn_table_gpa_h : 24;
#else
            u32 txmsn_table_gpa_h : 24;
            u32 sack_bitmap_size : 3;
            u32 txmsn_table_size : 3;
            u32 txmsn_entry_size : 2;
#endif
        } bs;
        u32 value;
    } dw0;

    u32 txmsn_table_gpa_l;

    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 rxmsn_entry_size0 : 2;
            u32 rxmsn_table_size0 : 2;
            u32 rsvd : 4;
            u32 rxmsn_table_gpa0_h : 24;
#else
            u32 rxmsn_table_gpa0_h : 24;
            u32 rsvd : 4;
            u32 rxmsn_table_size0 : 2;
            u32 rxmsn_entry_size0 : 2;
#endif
        } bs;
        u32 value;
    } dw2;

    u32 rxmsn_table_gpa0_l;

    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 sack_bitmap_size : 3;
            u32 rxmsn_entry_size1 : 2;
            u32 rxmsn_table_size1 : 2;
            u32 rsvd : 1;
            u32 rxmsn_table_gpa1_h : 24;
#else
            u32 rxmsn_table_gpa1_h : 24;
            u32 rsvd : 1;
            u32 rxmsn_table_size1 : 2;
            u32 rxmsn_entry_size1 : 2;
            u32 sack_bitmap_size : 3;
#endif
        } bs;
        u32 value;
    } dw4;

    u32 rxmsn_table_gpa1_l;
} roce_verbs_init_msnc_info_s;

typedef struct roce_muiti_cmd_init_msnc_cmd {
    roce_verbs_cmd_header_s com;
    roce_verbs_init_msnc_info_s cmd_info;
} roce_uni_cmd_init_msnc_s;

/**< hyperRoCE query qp data construct */
typedef struct roce_uni_cmd_hyper_roce_query_qp_cmd {
    roce_verbs_cmd_header_s com;
} roce_uni_cmd_hyper_roce_query_qp_s;

typedef struct hyper_qp_first_512B {
    u32 qpc[128];    /* npu can translate it to hyper_roce_qp_context_s */
} hyper_qp_first_512B_s;

typedef struct hyper_qp_last_512B {
    u32 ext_sw_seg[16];
    u32 ext_hw_seg[112];
} hyper_qp_last_512B_s;

typedef struct roce_hyper_roce_query_512B {
    union {
        hyper_qp_first_512B_s first_512B;
        hyper_qp_last_512B_s last_512B;
    } hyper_qpc;
} roce_hyper_roce_query_512B_s;

#endif