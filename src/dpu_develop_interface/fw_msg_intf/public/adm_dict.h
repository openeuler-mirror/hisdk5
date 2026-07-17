/*********************************************************************************
* @copyright    Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
* @file         adm_dict.h
* @brief        管理命令地址解耦相关字典定义
* @author       l30012907
* @date         2023-07-17
**********************************************************************************/

#ifndef ADM_DICT_H
#define ADM_DICT_H

#include "typedef.h"

enum {
    DICT_ELEMENT_U8 = 1,
    DICT_ELEMENT_U16 = 2,
    DICT_ELEMENT_U32 = 4,
    DICT_ELEMENT_U64 = 8,
};

#define MAX_REG_DICT_NAME_LEN 48
#define MAX_DICT_FILE_NAME_LEN 40
#define MAX_REG_FEATURE_NAME_LEN 8
#define MAX_REG_SUB_FEATURE_NAME_LEN 8
#define MAX_SM_TBL_NAME_LEN 40

#define COUNTER_MPU_DICT_NAME "counter_mpu.bin"
#define COUNTER_IPSUTX_DICT_NAME "counter_ipsutx.bin"
#define COUNTER_IPSURX_DICT_NAME "counter_ipsurx.bin"
#define COUNTER_NPU_DICT_NAME "counter_dict.bin"
#define SML_TBL_DEFINE_DICT_NAME "sml_table_define_dict.bin"
#define SML_TABLE_STRUCT_DICT_NAME "sml_table_struct_dict.bin"

// 通用字典头
typedef struct {
    char file_name[MAX_DICT_FILE_NAME_LEN]; // 字典文件名
    u8 version;       // 字典自身的版本信息，与固件大包版本不同
    u8 rsvd;
    u16 item_size;    // 单个字典元素大小
    u32 item_num;     // 字典元素数量
    u32 rsvd1[3];
} dict_info_s;

// 寄存器字典描述
typedef struct {
    char name[MAX_REG_DICT_NAME_LEN];           // 寄存器或内存的显示名称
    char feature[MAX_REG_FEATURE_NAME_LEN];         // 所属的特性
    char sub_feature[MAX_REG_SUB_FEATURE_NAME_LEN]; // 所属的子特性
    u8 node_id;                                       // 业务隶属的模块ID
    u8 type;                                          // u16\u32\u64
    u8 bit_start;                                     // 单独bit描述的起始位置
    u8 bit_end;                                       // 单独bit描述的结束位置
    u32 addr;                                         // 模块内偏移地址
    u32 rsvd[4];
} reg_dict_s;

typedef struct {
    dict_info_s head;
    reg_dict_s dict[];
} reg_dict_file_s;

typedef struct {
    char name[MAX_SM_TBL_NAME_LEN];      // table表名字，40字节
    u8 rsvd1;
    u8 node_id;                        // table 所在的物理节点
    u8 inst_id;                        // table 所在的实例号
    u8 entry_size;                     // 单个表项大小
    u8 rsvd[20];
} sml_table_dict_s;

/* 导出符号给工具DT，检查基本格式 */
#ifdef TOOL_DT_MCRO
extern void *counter_mpu_dict_ptr;
extern void *counter_ipsutx_dict_ptr;
extern void *counter_ipsurx_dict_ptr;
#endif

#endif // ADM_DICT_H
