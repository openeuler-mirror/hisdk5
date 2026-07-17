/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2023. All rights reserved.
 * Description: hiudk_lld.c
 * Author: -
 * Create:
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": [UDK]" fmt

#include <linux/module.h>
#include <linux/moduleparam.h>

#include "hinic5_crm.h"

#define HIUDK_DRV_DESC "Intelligent Network Unified Driver"

static __init int hiudk5_init(void)
{
    pr_info("%s - version %s\n", HIUDK_DRV_DESC, HINIC5_DRV_VERSION);

    return 0;
}

static __exit void hiudk5_exit(void)
{
    return;
}

MODULE_LICENSE("GPL");
module_init(hiudk5_init);
module_exit(hiudk5_exit);
