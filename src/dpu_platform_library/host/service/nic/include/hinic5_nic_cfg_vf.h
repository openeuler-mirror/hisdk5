/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef HINIC5_NIC_CFG_VF_H
#define HINIC5_NIC_CFG_VF_H

/* In order to adapt different linux version */
enum {
	HINIC5_IFLA_VF_LINK_STATE_AUTO, /* link state of the uplink */
	HINIC5_IFLA_VF_LINK_STATE_ENABLE, /* link always up */
	HINIC5_IFLA_VF_LINK_STATE_DISABLE, /* link always down */
};

#define NIC_CVLAN_INSERT_ENABLE 0x1
#define NIC_QINQ_INSERT_ENABLE  0x3
#define NIC_VF_TRUST_UNSUPPORT  0xFF

#endif