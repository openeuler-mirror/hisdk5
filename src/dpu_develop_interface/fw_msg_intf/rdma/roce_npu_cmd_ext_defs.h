/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 * Description: RDMA cmdq extended attributes.
 * Create: 2021-12-30
 */

#ifndef ROCE_VERBS_EXT_ATTR_H
#define ROCE_VERBS_EXT_ATTR_H

#include "roce_npu_cmd_defs.h"

#pragma pack(4)

typedef struct roce_vtep_context {
    u32 dmac_h32;

    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 dmac_l16 : 16;
            u32 smac_h16 : 16;
#else
            u32 smac_h16 : 16;
            u32 dmac_l16 : 16;
#endif
        } bs;
        u32 value;
    } dw1;

    u32 smac_l32;

    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 portid : 8;
            u32 vni : 24;
#else
            u32 vni : 24;
            u32 portid : 8;
#endif
        } bs;
        u32 value;
    } dw3;

    u32 sip[4]; // dw4 ~ 7
    u32 dip[4]; // dw8 ~ 11

    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 src_port : 16;
            u32 dst_port : 16;
#else
            u32 dst_port : 16;
            u32 src_port : 16;
#endif
        } bs;
        u32 value;
    } dw12;

    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 base_id : 8;
            u32 cvlan : 12;
            u32 svlan : 12;
#else
            u32 svlan : 12;
            u32 cvlan : 12;
            u32 base_id : 8;
#endif
        } bs;
        u32 value;
    } dw13;

    u32 remote_acl_id; // acl(access list) 减少重复查src vtep表次数以缩短时延，仅在第一次查找时将src vtep值赋给vtep

    /* DW15 */
    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 rsvd : 5;
            u32 vm_dp_capture_en : 1;
            u32 vm_dp_chk_en : 1;
            u32 vm_dp_chk_invld : 1;
            u32 dscp_rc : 8;
            u32 dscp_ud : 8;
            u32 dscp_xrc : 8;
#else
            u32 dscp_xrc : 8;
            u32 dscp_ud : 8;
            u32 dscp_rc : 8;
            u32 vm_dp_chk_invld : 1;
            u32 vm_dp_chk_en : 1;
            u32 vm_dp_capture_en : 1;
            u32 rsvd : 5;
#endif
        } bs;
        u32 value;
    } dw15;
} roce_vtep_context_s;

/* 暴露给上层业务的属性协商数据结构 */
typedef struct {
    u64 roce_uld_kernel_ext_ability;
    u64 roce_uld_user_ext_ability;
} roce_uld_feature_s;

/* 该数据结构由微码使用，在驱动侧进行大小端转换 */
typedef struct tag_roce_get_chip_ability {
    u64 roce_kernel_ability;
    u64 roce_kernel_ext_ability;
    u64 roce_user_ext_ability;
    roce_uld_feature_s uld_feature;
    u64 rsvd;
} roce_get_chip_ability_s;

#pragma pack()

#endif /* ROCE_VERBS_EXT_ATTR_H */
