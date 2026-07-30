/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 * Description: sdk's inner chip related structure and macro defined here.
 * Note: TODO : put it in sdk's inner include folder after decoupling.
 * Create: 2025-08-13
*/

#ifndef HINIC5_CHIP_INFO_H
#define HINIC5_CHIP_INFO_H

#include "hinic5_crm.h"
#include "hinic5_mt.h"

/**
 * @brief struct card_node
 * @details 定义一个名为card_node的结构体，表示一个网卡节点
 */
struct card_node {
	struct list_head node;                       /**< 表示一个列表头 */
	struct list_head func_list;                  /**< 表示一个功能列表头 */
	char chip_name[IFNAMSIZ];                    /**< 存储芯片名称 */
	void *log_info;                              /**< 指向日志信息 */
	void *dbgtool_info;                          /**< 指向调试工具信息 */
	spinlock_t dbgtool_info_lock;           	 /**< 保护fm_show更新上下文 */
	void *func_handle_array[MAX_FUNCTION_NUM];   /**< 存储功能句柄 */
	u16 func_num;                                /**< function数量 */
	u32 rsvd1;
	void *priv_data;                             /**< 指向私有数据 */
	u64 rsvd2;
	void *fw_update_context;                     /**< 指向固件更新上下文 */
	spinlock_t fw_update_context_lock;           /**< 保护固件更新上下文 */
	struct hinic5_non_ptp_info *non_ptp_info;    /**< 非ptp时间差信息 */
	u64 id;                                      /**< 芯片唯一性id标识 */
	atomic_t ref_cnt;                            /**< 引用计数 */
};
#endif
