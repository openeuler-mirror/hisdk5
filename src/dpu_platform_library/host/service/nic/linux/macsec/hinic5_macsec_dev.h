/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
 * Description: 存放实体
 * Create: 2024/03/01
 */

#ifndef HINIC5_MACSEC_DEV_H
#define HINIC5_MACSEC_DEV_H

#include "macsec_mpu_cmd_defs.h"
#include "macsec_pub_cmd.h"
#include "ossl_knl.h"

typedef struct himacsec_spec {
    u8 macsec_support;     /* 硬件是否支持MACsec */
    u8 max_port;           /* Maximum number of ports on the chip */
    u8 max_port_sc;        /* Maximum number of SCs per port */
    u8 max_sa;             /* Maximum number of SA stored per SC */
} himacsec_spec_s;

typedef struct himacsec_stats {
    u64 sa_update_times;
    u64 sa_expire_times;
} himacsec_stats_s;

typedef struct himacsec_mib_stats {
    u32 max_port_size;
    u32 max_port_sc_size;
    macsec_port_mib_info_s *port_mibs;
    macsec_sc_mib_info_s *sc_mibs;
} himacsec_mib_stats_s;

typedef struct macsec_resource {
    struct himacsec_spec spec;
    u64 himacsec_feature[MACSEC_MAX_FEATURE_QWORD];
    u32 num_of_device;
    himacsec_stats_s *stats;  // 驱动内部计数
    struct notifier_block nb;
    u8 function_port;
    u32 offload_child_dev_idx[MACSEC_SC_NUM];  // macsec子设备 的ifindex,卸载的macsec设备的ifindex会存到此数组里
    u32 offload_dev_num;                       // offload_child_dev_idx数组中有效的元素个数
} macsec_resource_s;

struct macsec_port_res {
    struct himacsec_sc enc_sc;  // 数组头指针
    struct himacsec_sc dec_sc;
};

struct macsec_extra_param {
    u64 threshold;
    u8 algo_type;
    u8 offset;
    u8 bitmap;
    bool threshold_set;
    bool offset_set;
    u8 rsvd;
};

enum param_bit {
    PARAM_BIT_ENC_SC_ENCODING_SA,
    PARAM_BIT_ENC_SC_PROTECTION_MODE,
    PARAM_BIT_ENC_SC_PROTECT_FRAME_MODE,
    PARAM_BIT_DEC_SC_VALID_FRAM_MODE,
    PARAM_BIT_MAX,
};

#endif