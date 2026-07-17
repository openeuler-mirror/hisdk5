/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 * Description: RDMA cmdq command format.
 * Create: 2021-12-30
 */

#ifndef ROCE_NPU_CMD_DFX_DEFS_H
#define ROCE_NPU_CMD_DFX_DEFS_H

#include "roce_npu_cmd_defs.h"

/* 按照最大entry_size=32，最大深度2k配置 */
#define ROCE_RDMARC_MAX_ENTRY_SIZE 32
#define ROCE_LATCH_DATA_MAX_SIZE 2032
#define ROCE_LATCH_VALID_VERIFY_NUM 0x66
/**
 * @brief struct tag_roce_uni_cmd_latch_data/roce_uni_cmd_latch_data_s
 * @details roce latch data out buffer
 */
typedef struct roce_uni_cmd_latch_data {
    u32 latch_data_len;
    u32 rsvd[3];
    u8 latch_data[ROCE_LATCH_DATA_MAX_SIZE];
} roce_uni_cmd_latch_data_s;

/**
 * @brief struct tag_roce_uni_cmd_rdma_rc_outbuf/roce_uni_cmd_rdma_rc_outbuf_s
 * @details rdma rc out buffer
 */
typedef struct tag_roce_uni_cmd_rdma_rc_outbuf {
    u32 rc_table_size;
    u8 rdmarc[ROCE_RDMARC_MAX_ENTRY_SIZE];
} roce_uni_cmd_rdma_rc_outbuf_s;

typedef struct tag_roce_latch_info_rc {
    u8  get_latch_info;    // TRUE:获取锁存信息 FALSE：清理锁存信息，后续新增考虑枚举
    u8  rc_entry_idx;
    u16 rsvd;
} roce_latch_info_rc_s;

typedef struct tag_roce_dfx_cmd_latch_info {
    roce_verbs_cmd_header_s com;
    roce_latch_info_rc_s latch_info;
} roce_dfx_cmd_latch_info_s;

#endif /* ROCE_NPU_CMD_DFX_DEFS_H */
