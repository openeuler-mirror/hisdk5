/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 *
 * File Name     : hihtr_gid.h
 * Created       : 2026/2/12
 * Description   : The definition of roce gid cmd management
 */

#ifndef HIHTR_GID_H
#define HIHTR_GID_H

struct rdma_gid_entry {
    union {
        u8 raw[16];
        struct {
            __be64 subnet_prefix;
            __be64 interface_id;
        } global;
    };
    union {
        struct {
            u32 rsvd : 8;
            u32 cvlan : 12; /* 内层vlan customer vlan */
            u32 svlan : 12; /* 外层vlan */
        } bs;
        u32 value;
    } dw4;

    union {
        u32 hdr_len_value;
    };

    union {
        struct {
            u16 tag : 2;    /* 0:没有vlan； 1：一层vlan； 2: 2层vlan； 3：stag */
            u16 tunnel : 1; // rsvd for ppe, don't use. 'tunnel'
            u16 gid_type : 2;
            u16 ppe_rsvd1 : 1;
            u16 outer_tag : 2; // rsvd for ppe, don't use. 'outer_tag'
            u16 ppe_rsvd3 : 1; // rsvd for ppe, don't use. 'stag'
            u16 gid_update : 1;
            u16 rsvd : 6;
        } bs;
        u16 value;
    } dw6_h;

    u8 smac[6];
};

#endif // HIHTR_GID_H
