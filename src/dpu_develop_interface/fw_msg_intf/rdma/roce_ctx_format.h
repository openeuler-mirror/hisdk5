/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 * Description: RDMA common context format.
 * Create: 2021-12-30
 */

#ifndef ROCE_CTX_FORMAT_H
#define ROCE_CTX_FORMAT_H
#include "base_type.h"
/* ********************** sync info ************************ */
#define ROCE_CACHE_LINE_SIZE (0x100)
#define ROCE_PAGE_SIZE 4096
#define ROCE_RC_ENTRY_NUM_PER_CACELINE 8

/**
 * @brief struct roce_latch_header/roce_latch_header_u
 * @details roce latch data 4B header
 */
typedef union roce_latch_data_header {
    struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
        u32 magic_num : 8; // 使用magic number校验是否valid，对于invalid状态需清零
        u32 first_seq : 4;
        u32 last_seq : 4;
        u32 latch_total_len : 16;
#else
        u32 latch_total_len : 16;
        u32 last_seq : 4;
        u32 first_seq : 4;
        u32 magic_num : 8; // 使用magic number校验是否valid，对于invalid状态需清零
#endif
    } bs;
    u32 value;
} roce_latch_data_header_u;

enum {
    ROCE_QP_ST_RC = 0x0,  /* 000 */
    ROCE_QP_ST_UC = 0x1,  /* 001 */
    ROCE_QP_ST_RD = 0x2,  /* 010 */
    ROCE_QP_ST_UD = 0x3,  /* 011 */
    ROCE_QP_ST_XRC = 0x6, /* 110 */
    ROCE_QP_ST_PRIV = 0x7 /* 111 */
};

/* the same with stp */
enum roce5_gid_type {
    ROCE_IPv4_ROCEv2_GID = 0,
    ROCE_IPv6_ROCEv2_GID = 1,
    ROCE_ROCEv1_GID = 2,
    ROCE_INV_GID
};

#endif /* ROCE_CTX_FORMAT_H */

