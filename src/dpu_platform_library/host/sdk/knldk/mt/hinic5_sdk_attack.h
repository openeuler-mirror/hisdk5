/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef HINIC5_SDK_ATTACK_H
#define HINIC5_SDK_ATTACK_H

#include <linux/types.h>
#include "hinic5_hw.h"

int hinic5_sdk_attack_handler(struct hinic5_lld_dev *lld_dev, const void *buf_in, u32 in_size,
			     void *buf_out, u32 *out_size);
int hinic5_attack_fast_msg(void *hwdev, struct hinic5_cmd_buf *cmd_buf, u64 *out_param);
#endif
