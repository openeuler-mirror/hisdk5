/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */

#ifndef BIFUR_COMMON_H__
#define BIFUR_COMMON_H__

#include "nic_mpu_cmd_structs.h"

#define BIFUR_VF_NUM 4 // FPGA不支持配置40个vf 暂时改为4个
#define BIFUR_FILE_PATH_SIZE 50

#define BIFUR_ENABLED   1
#define BIFUR_DISABLED  0

#define BIFUR_SET_ENABLE 0xc1
#define BIFUR_GET_ENABLE 0xc2
#define BIFUR_SET_USER_QUEUE_GROUP 0xc3
#define BIFUR_GET_USER_QUEUE_GROUP 0xc4

#define BIFUR_FUNC_MIN_QUEUE_GROUP 1
#define BIFUR_FUNC_MAX_QUEUE_GROUP 8

#define PCI_DBDF(dom, bus, dev, func) \
    ((((u32)(dom) << 16) | ((u32)(bus) << 8) | ((u32)(dev) << 3) | ((u32)(func) & 0x7)))
#define PCI_DBDF_DOM(dbdf) (((dbdf) >> 16) & 0xFFFF)
#define PCI_DBDF_BUS(dbdf) (((dbdf) >> 8) & 0xFF)
#define PCI_DBDF_DEVID(dbdf) (((dbdf) >> 3) & 0x1F)
#define PCI_DBDF_FUNCTION(dbdf) ((dbdf) & 0x7)
#define PCI_DBDF_DEVFN(dbdf) ((dbdf) & 0xFF)

struct bifur_cmd_queue_pooling {
    struct mgmt_msg_head head;

    u16 number;
    u16 group;
    u32 rsvd2[125];
};

#define BIFUR_DEV_INFO(lld_dev, fmt, arg...) dev_info((lld_dev->dev), "[BIFUR]" fmt, ##arg)

#define BIFUR_DEV_WARN(lld_dev, fmt, arg...) dev_warn((lld_dev->dev), "[BIFUR]" fmt, ##arg)

#define BIFUR_DEV_ERR(lld_dev, fmt, arg...) dev_err((lld_dev->dev), "[BIFUR]" fmt, ##arg)
#endif