/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 * Description: RDMA CC context.
 * Create: 2021-12-30
 */

#ifndef ROCE_CC_FORMAT_H
#define ROCE_CC_FORMAT_H

#include "ccp_algo_format.h"

/* Align each field with 4bytes. */
#pragma pack(push) // 保存对齐状态
#pragma pack(4)

#ifndef BIG_ENDIAN
#define BIG_ENDIAN    0x4321
#endif

#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN    0x1234
#endif

enum ROCE_CC_ALGO_E {
    ROCE_CC_DISABLE = 0,
    ROCE_CC_DCQCN_ALGO,
    ROCE_CC_LDCP_ALGO,
    ROCE_CC_IPQCN_ALGO,
    ROCE_CC_USER_A_ALGO = 6,
    ROCE_CC_USER_B_ALGO
};

#define ROCE_CC_COMMON_PARAM ROCE_CC_DISABLE

/* *************************** EXT TABLE *************************** */
/* cc common param tbl */
typedef struct roce_cc_param {
    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 rsvd : 3;
            u32 algo_mode : 1;
            u32 min_cnp_period : 8;
            u32 rtt_rsp_prio : 3;
            u32 rtt_rsp_prio_enable : 1;
            u32 rtt_version : 2;
            u32 ecn_cnp_en : 1;
            u32 flow_ctrl_unit : 1;
            u32 cnp_prio_enable : 1;
            u32 rsvd1 : 11;
#else
            u32 rsvd1 : 11;
            u32 cnp_prio_enable : 1;
            u32 flow_ctrl_unit : 1;
            u32 ecn_cnp_en : 1;
            u32 rtt_version : 2;
            u32 rtt_rsp_prio_enable : 1;
            u32 rtt_rsp_prio : 3;
            u32 min_cnp_period : 8;
            u32 algo_mode : 1;
            u32 rsvd : 3;
#endif
        } bs;
        u32 value;
    } dw0;

    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 cnp_cos : 3;
            u32 cnp_prio : 3;
            u32 cc_appid : 8;
            u32 rsvd : 18;
#else
            u32 rsvd : 18;
            u32 cc_appid : 8;
            u32 cnp_prio : 3;
            u32 cnp_cos : 3;
#endif
        } bs;
        u32 value;
    } dw1;

    u32 rsvd[2];
} roce_cc_param_s;

typedef struct roce_ipqcn_param {
    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 rsvd : 18;
            u32 flow_min_rate : 6;
            u32 token_period : 8;
#else
            u32 token_period : 8;
            u32 flow_min_rate : 6;
            u32 rsvd : 18;
#endif
        } bs;
        u32 value;
    } dw0;

    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 rate_inc_period : 10;
            u32 alpha_dec_period : 10;
            u32 rsvd : 12;
#else
            u32 rsvd : 12;
            u32 alpha_dec_period : 10;
            u32 rate_inc_period : 10;
#endif
        } bs;

        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 rate_inc_period : 16;
            u32 alpha_dec_period : 16;
#else
            u32 alpha_dec_period : 16;
            u32 rate_inc_period : 16;
#endif
        } bs1;
        u32 value;
    } dw1;

    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 rate_inc_ai : 8;
            u32 rate_inc_hai : 8;
            u32 rate_dec_period : 8;
            u32 min_cnp_period : 8;
#else
            u32 min_cnp_period : 8;
            u32 rate_dec_period : 8;
            u32 rate_inc_hai : 8;
            u32 rate_inc_ai : 8;
#endif
        } bs;
        u32 value;
    } dw2;

    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 factor_gita : 4;
            u32 rt_clamp : 1;
            u32 rsvd : 1;
            u32 initial_alpha : 10;
            u32 rate_first_set : 16;
#else
            u32 rate_first_set : 16;
            u32 initial_alpha : 10;
            u32 rsvd : 1;
            u32 rt_clamp : 1;
            u32 factor_gita : 4;
#endif
        } bs;
        u32 value;
    } dw3;
} roce_ipqcn_param_s;

typedef struct roce_ldcp_param {
    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 alpha : 4;
            u32 beta : 4;
            u32 gamma : 4;
            u32 eta : 4;
            u32 wnd_min : 4;
            u32 set_flag : 1;
            u32 rsvd : 11;
#else
            u32 rsvd : 11;
            u32 set_flag : 1;
            u32 wnd_min : 4;
            u32 eta : 4;
            u32 gamma : 4;
            u32 beta : 4;
            u32 alpha : 4;
#endif
        } bs;
        u32 value;
    } dw0;

    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 rsvd : 16;
            u32 wnd_default : 16;
#else
            u32 wnd_default : 16;
            u32 rsvd : 16;
#endif
        } bs;
        u32 value;
    } dw1;

    u32 rsvd[2];
} roce_ldcp_param_s;

typedef union roce_ccp_comm_param {
    ccp_comm_param_s ccp_comm_param;
    struct {
        u32 used_param[2];
        union {
            struct {
    #if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
                u32 ccp_appid : 3;
                u32 rsvd : 29;
    #else
                u32 rsvd : 29;
                u32 ccp_appid : 3;
    #endif
            } bs;
            u32 value;
        } dw2;

        union {
            struct {
    #if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
                u32 high_waterline : 16;
                u32 low_waterline : 16;
    #else
                u32 low_waterline : 16;
                u32 high_waterline : 16;
    #endif
            } bs;
            u32 value;
        } dw3;
    } roce_comm_param;
} roce_ccp_comm_param_s;

#pragma pack(0)
#pragma pack(pop) // 恢复对齐状态

#endif // ROCE_CC_FORMAT_H
