/**< SPDX-License-Identifier: GPL-2.0 */
/**< Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef HINIC5_BOND_H
#define HINIC5_BOND_H

#include "nic_mpu_cmd_structs.h"
#include "drv_bond_api.h"

/**
 * @brief bond初始化接口
 * @param[in] void
 * @details nic初始化时调用，用于bond模块的初始化
 * @attention N/A
 * @return	返回初始化结果，0为成功，非0失败
 **/
int hinic5_bond_init(void);

/**
 * @brief bond去初始化接口
 * @param[in] void
 * @details nic驱动卸载时调用，用于bond模块去初始化
 * @attention N/A
 * @return	void
 **/
void hinic5_bond_deinit(void);

/**
 * @brief 通过bond名获取bond id
 * @param[in] bond_name bond名
 * @param[out] bond_id 找到的bondid
 * @details 返回的bondid为驱动侧维护的id
 * @attention N/A
 * @return	返回获取结果，0为找到，非0未找到
 **/
int hinic5_bond_get_id_by_name(u8 *bond_name, u16 *bond_id);

#endif /**< HINIC5_BOND_H */
