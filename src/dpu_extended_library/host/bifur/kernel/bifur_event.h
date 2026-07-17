/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */

#ifndef BIFUR_EVENT_H__
#define BIFUR_EVENT_H__
#include <linux/workqueue.h>
#include <linux/notifier.h>
#include <linux/if.h>

#include "bifur_main.h"

#define BIFUR_BOND_2_FUNC_NUM 2

int bifur_register_net_event(void);
void bifur_unregister_net_event(void);
void bifur_netdev_event(struct work_struct *work);
int bifur_net_link_event(struct bifur_adapter *adp, unsigned long event, struct net_device *dev);

#endif
