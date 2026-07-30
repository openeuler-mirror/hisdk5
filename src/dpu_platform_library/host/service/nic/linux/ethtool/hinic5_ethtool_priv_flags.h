/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) Huawei Technologies Co., Ltd. 2021. All rights reserved. */

#ifndef HINIC5_ETHTOOL_PRIV_FLAGS_H
#define HINIC5_ETHTOOL_PRIV_FLAGS_H

#include <linux/types.h>
#include <linux/netdevice.h>

#define HINIC5_PRIV_FLAGS_SYMM_RSS     BIT(0)
#define HINIC5_PRIV_FLAGS_LINK_UP      BIT(1)
#define HINIC5_PRIV_FLAGS_RXQ_RECOVERY BIT(2)

/* Private flags functions - exported for external use */
int hinic5_set_rxq_recovery_flag(struct net_device *netdev, u32 priv_flags);

/* Private flags functions - internal use */
u32 hinic5_get_priv_flags(struct net_device *netdev);
int hinic5_set_priv_flags(struct net_device *netdev, u32 priv_flags);

#endif /* HINIC5_ETHTOOL_PRIV_FLAGS_H */
