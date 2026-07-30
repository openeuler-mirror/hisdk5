/*********************************************************************************
* @copyright    Copyright (c) Huawei Technologies Co., Ltd. 2023. All rights reserved.
* @brief        counter字典的定义, 用于描述counter名字和counter id的对应关系
* @date         2023-07-10
**********************************************************************************/
#ifndef COUNTER_DICT_H
#define COUNTER_DICT_H

#include "base_type.h"

#define MAX_COUNTER_NAME_LEN 80
#define MAX_COUNTER_DESC_LEN 80
#define MAX_FEATURE_NAME_LEN 8
#define MAX_SUB_FEATURE_NAME_LEN 8

typedef struct {
    const char name[MAX_COUNTER_NAME_LEN];  // counter 的名字
    const char desc[MAX_COUNTER_DESC_LEN];  // counter 的描述
    const char feature[MAX_FEATURE_NAME_LEN]; // counter 所属的特性
    const char sub_feature[MAX_SUB_FEATURE_NAME_LEN]; // counter 所属的子特性
    u8 level;                               // counter 的级别，包括KEY, ERR, WARN, INFO, DBG
    u8 type;                                // counter 的类型, 包括CTR_32, CTR_64, CTR_PAIR等
    u8 unit;                                // counter 的单个大小，例如CTR_32就是32 bits，即4 bytes
    u8 step;                                // counter 的索引步进单位，即一个counter占多个个ID
    u8 node_id;                             // counter 所在的物理节点
    u8 inst_id;                             // counter 所在的实例号
    u32 base_id;                            // counter 在实例内的相对起始ID
    u32 num;                                // counter 的数量
} ctr_dict_s;

#endif /* COUNTER_DICT_H */
