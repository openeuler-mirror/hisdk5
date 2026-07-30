/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef __HINIC5_SYSFS_H_
#define __HINIC5_SYSFS_H_

#include <linux/sysfs.h>

#include "hinic5_dev_mgmt.h"

extern const struct attribute_group * hisdk5_driver_attr_groups[];

int register_device_attr_groups(struct hinic5_adev *adev);
void unregister_device_attr_groups(struct hinic5_adev *adev);

#endif