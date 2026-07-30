/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) Huawei Technologies Co., Ltd. 2021. All rights reserved. */

#ifndef HINIC5_IRQ_H
#define	HINIC5_IRQ_H

#include "hinic5_nic_dev.h"

int hinic5_qps_irq_init(struct hinic5_nic_dev *nic_dev);

void hinic5_qps_irq_deinit(struct hinic5_nic_dev *nic_dev);

#endif