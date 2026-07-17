/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2025 Huawei Technologies Co., Ltd */

#ifndef HISDK5_LLD_H
#define HISDK5_LLD_H

#include "hinic5_crm.h"
#include "hinic5_lld.h"

/**
 * @brief 获取全部ULD名称数组
 *
 * @return 返回ULD名称数组指针
 */
const char **hinic5_get_uld_names(void);

/**
 * @brief hinic5_get_uld_info_by_type - get udl info by service type
 * @param type: service type
 *
 * @return uld_info
 **/
const struct hinic5_uld_info* hinic5_get_uld_info_by_type(enum hinic5_service_type type);

#endif