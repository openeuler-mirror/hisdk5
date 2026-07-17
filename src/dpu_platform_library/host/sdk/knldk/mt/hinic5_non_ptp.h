/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef HINIC5_NON_PTP_H
#define HINIC5_NON_PTP_H

#if !defined(__UEFI__) && !defined(__VMWARE__) && !defined(__WIN__)

#include <linux/cdev.h>
#include "hinic5_hwdev.h"

#define HINIC5_NON_PTP_CDEV_MAX_DEVICES 1

#define HINIC5_NON_PTP_SYNC_FW_TIME_PERIOD (500) /**< 系统和芯片时间同步周期单位毫秒 */

struct hinic5_non_ptp_cdev {
	dev_t devid;
	struct cdev dev;
	struct class *cdev_class;
	struct device *cdev_device;
};

int hinic5_non_ptp_cdev_init(struct hinic5_hwdev *hwdev);
void hinic5_non_ptp_cdev_deinit(struct hinic5_hwdev *hwdev);
int hinic5_sync_kernel_time(struct hinic5_hwdev *hwdev);
int hinic5_non_ptp_init(void *hwdev);
void hinic5_non_ptp_uninit(void *hwdev);

#endif
#endif
