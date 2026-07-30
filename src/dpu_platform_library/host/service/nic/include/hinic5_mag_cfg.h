/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef HINIC5_MAG_CFG_H
#define HINIC5_MAG_CFG_H

#include <linux/types.h>

#define CAP_INFO_MAX_LEN	512
#define VENDOR_MAX_LEN		17

#define LOOP_MODE_MIN 1
#define LOOP_MODE_MAX 6

/* 这个结构在mgmt_tlv_info的基础上增加page_id，使匹配page_id更方便 */
struct tlv_block {
	u16 type;
	u16 length;     /* length是page_id和page_data的总长度，计算偏移时需注意 */
	u32 page_id;
	u8 page_data[];
};

/**
 * @brief 设置设备物理端口状态
 *
 * @param hwdev device pointer to hwdev
 * @param enable 设置端口状态值，true--使能，false--去使能
 * @param channel channel id, mailbox发送使用的channel id
 *
 * @details 设置该设备关联的物理端口状态，通过mailbox发给MPU设置MAG端口状态
 *
 * @attention: 仅PF支持，VF调用返回0；函数内部涉及发送mailbox消息会休眠，禁止中断上下文等不允许休眠的流程中调用
 *
 * @return: 设备物理端口状态设置返回成功或者失败.
 *     @retval 0 成功
 *     @retval 非0 失败
 */
int hinic5_set_port_enable(void *hwdev, bool enable, u16 channel);
int hinic5_get_fec(void *hwdev, u8 *advertised_fec, u8 *supported_fec);
int hinic5_set_fec(void *hwdev, u8 advertised_fec);

#endif