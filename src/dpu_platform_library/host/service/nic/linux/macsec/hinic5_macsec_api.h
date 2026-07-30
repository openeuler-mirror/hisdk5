/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
 * Description: MACsec api header file
 * Create: 2025/11/04
 */

#ifndef HINIC5_MACSEC_API_H
#define HINIC5_MACSEC_API_H

#include <linux/types.h>
#include "hinic5_nic_dev.h"

/* MACsec初始化 */
int macsec_init_offload(struct hinic5_nic_dev *nic_dev);
void macsec_cleanup_offload(struct hinic5_nic_dev *nic_dev);

/* MACsec ioctl接口 */
int macsec_cmd_list(struct hinic5_nic_dev *nic_dev, const void *buf_in, u32 in_size, void *buf_out, u32 *out_size);
int macsec_cmd_mib(struct hinic5_nic_dev *nic_dev, const void *buf_in, u32 in_size, void *buf_out, u32 *out_size);
int macsec_cmd_add(struct hinic5_nic_dev *nic_dev, const void *buf_in, u32 in_size, void *buf_out, u32 *out_size);
int macsec_cmd_del(struct hinic5_nic_dev *nic_dev, const void *buf_in, u32 in_size, void *buf_out, u32 *out_size);
int macsec_cmd_set(struct hinic5_nic_dev *nic_dev, const void *buf_in, u32 in_size, void *buf_out, u32 *out_size);
int macsec_cmd_flush(struct hinic5_nic_dev *nic_dev, const void *buf_in, u32 in_size, void *buf_out, u32 *out_size);

#endif  // HINIC5_MACSEC_API_H