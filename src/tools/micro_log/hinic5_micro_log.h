/*
 * Huawei HiNIC PCI Express Linux driver
 * Copyright(c) 2017 Huawei Technologies Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 */

#ifndef __NIC_MICRO_LOG_H__
#define __NIC_MICRO_LOG_H__

#define MAX_PATH_NAME 256

#define MAX_LEN_MICRO_LOG_DATA (64*1024)

#define MAX_NUM_OF_ONE_TIME_ULOG (16*1024)

#define MAX_SIZE_OF_LOG_FILE (512*1024*1024)

#define MICRO_LOG_ITEM_LEN 64

#define MICRO_LOG_MAX_RECV_NUM_ONE_TIME 20

#define MICRO_LOG_MEM_TAG MEM_TAG("MICRO_LOG_INIT")

#define LINE_CHAR_NUM (20)

#define MICRO_LOG_POLLING_TIME 1

#define MICRO_LOG_Q_CI_MASK (0x000F)
#define MICRO_LOG_D2_CI_MASK (0x00FF)
#define MICRO_LOG_ALL_MASK (0xFFFF)

#define TBL_ID_DFX_LOG_POINTER 0 // 此处必须固定为0， 工具自动生成的TBL_ID_DFX_LOG_POINTER固定在0位置

/* Type */
#define ULOG_ERR 0
#define ULOG_WARN 1
#define ULOG_INFO 2
#define ULOG_DEBUG 3

int hinic5_comm_micro_log_init(struct hinic5_hwdev *hwdev);

int hinic5_micro_log_func_en(void *hwdev, u8 is_en);

int hinic5_micro_log_init(void * hwdev);

void hinic5_micro_log_uninit(void * hwdev);

void hinic5_micro_log_reset(void *hwdev);

#define BEIJING_TIMEZONE 8
#define SECONDS_OF_HOUR 3600
#define TIMEZONE_ADJUSTMENT(unix_time) \
	(unix_time + (BEIJING_TIMEZONE * SECONDS_OF_HOUR))

#endif
