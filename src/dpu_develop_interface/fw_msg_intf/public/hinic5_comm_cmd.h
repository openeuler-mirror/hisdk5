/* *****************************************************************************
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2022. All rights reserved.
 ******************************************************************************
  File Name     : hinic5_comm_cmd.h
  Version       : Initial Draft
  Created       : 2019/4/25
  Last Modified :
  Description   : COMM Commands between Driver and MPU
  Function List :
***************************************************************************** */

#ifndef HINIC5_COMMON_CMD_H
#define HINIC5_COMMON_CMD_H

#include "base_type.h"

#define DFX_LOG_PRINT_MAX_PARA 8

/**
 * @brief comm_cmdq_cmd - CmdQ Common subtype
 *
 * @details COMM Commands between Driver and MPU
 */
enum comm_cmdq_cmd {
    COMM_CMD_UCODE_ARM_BIT_SET = 2,    /**< 设置UCODE_ARM位的命令 */
    COMM_CMD_SEND_NPU_DFT_CMD,         /**< 发送NPU的调试命令 */
    COMM_CMD_MICROLOG_PRINT_CNT_CLEAR, /**< 清除微日志打印计数的命令 */
    COMM_CMD_UCODE_FAST_MSG_CMD,       /**< 发送快速消息的命令 */
    COMM_CMD_UCODE_FAST_MSG_CLEAR,     /**< 清除快速消息的命令 */
    COMM_CMD_MICROLOG_GPA_SET,         /**< 将缓存日志的主机地址保存到sml表 */
    COMM_CMD_MICROLOG_CTRL_INFO_SET,   /**< 更新日志控制信息到sml表 */
};

typedef struct tag_cmdq_microlog_gpa_set {
        u32 wr_init_pc_h32;
        u32 wr_init_pc_l32;
        u32 lt_index;
} cmdq_microlog_gpa_set_s;
 
typedef struct tag_cmdq_microlog_ctrl_info_set {
    struct {
#if (BYTE_ORDER == BIG_ENDIAN)
        u32 state : 8;
        u32 microlog_en : 1;
        u32 microlog_init_flag : 1;
        u32 rsvd : 11;
        u32 max_num : 11; /* Maximum data block size */
#else
        u32 max_num : 11; /* Maximum data block size */
        u32 rsvd : 11;
        u32 microlog_init_flag : 1;
        u32 microlog_en : 1;
        u32 state : 8;
#endif
    };
    u32 ci_index;
} cmdq_microlog_ctrl_info_set_s;

typedef struct tag_micro_log_item {
	union {
		struct {
#if (BYTE_ORDER == BIG_ENDIAN)
            u32 action : 1;	   /* 1:记录日志 0:打印 */
            u32 type : 2;		 /* 0:err 1:trace 2:info */
			u32 feature : 5;	  /* 0:l2nic 1:roce 2:toe 3:ioe 4:feoe */
			u32 core_id : 6;
			u32 thread_id : 2;
			u32 valid_param_num : 8; /* 有效的参数数量 */
			u32 tile_id : 3;
			u32 ctrl_flag : 1;
			u32 rsv : 4;
#else
			u32 rsv : 4;
			u32 ctrl_flag : 1;
			u32 tile_id : 3;
			u32 valid_param_num : 8; /* 有效的参数数量 */
			u32 thread_id : 2;
			u32 core_id : 6;
			u32 feature : 5;	  /* 0:l2nic 1:roce 2:toe 3:ioe 4:feoe */
			u32 type : 2;		 /* 0:err 1:trace 2:info */
			u32 action : 1;	   /* 1:记录日志 0:打印 */
#endif
		} bs;
		u32 value;
	} ctrl_info;

    u32 string_addr;
    u32 data[DFX_LOG_PRINT_MAX_PARA];
    u32 func_name_addr; /* 文件名不超过26个字符 */

	union {
		struct {
#if (BYTE_ORDER == BIG_ENDIAN)
			u32 line : 16;		  /* 对标mpu中log_items，其中的line只需要16bit */
			u32 log_seq : 16;	  /* 等价于log_pi，范围：0~65535。作为日志seq, 检测是否丢微码日志 */
#else
			u32 log_seq : 16;
			u32 line : 16;
#endif
		} bs;
		u32 value;
	} line_and_pi;
} micro_log_item_s;

#endif /* HINIC5_COMMON_CMD_H */
