/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
 * Description: commonds between hinicadmdfx tools and roce driver.
 * Create: 2024-12-20
 */

#ifndef ROCE_PUB_CMD_H
#define ROCE_PUB_CMD_H

#include "base_type.h"

#define ROCE_SERVICE_TYPE 2
#define INVALID_QPN ((u32)~0u)
#define ROCE_ATTACK_LEN 4
#define ATTACK_VALUE_SIZE 256
#define ATTACK_VALUE_MAX 128

enum roce_attack_type {
    ROCE_ATTACK_TYPE_SQDB_HARD = 0,
    ROCE_ATTACK_TYPE_DWQE,
    ROCE_ATTACK_TYPE_SQWQE,
    ROCE_ATTACK_TYPE_RQWQE,
    ROCE_ATTACK_TYPE_CMTT,
    ROCE_ATTACK_TYPE_DMTT,
    ROCE_ATTACK_TYPE_RDMARC,
    ROCE_ATTACK_TYPE_CQE,
    ROCE_ATTACK_TYPE_INVALID
};

struct roce_attack_outbuf {
    u64 db;
};

struct roce_attack_inbuf {
    u32 convert_endian;
    u32 qpn;
    u32 mpt_index;
    u32 attack_type;
    u32 offset;
    u32 length;
    u32 srv_type;
    u32 attack_len;
    u32 mask;
    u32 attack_value[32];
};

#endif