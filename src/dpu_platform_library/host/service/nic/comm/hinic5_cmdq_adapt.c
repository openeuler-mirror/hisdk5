// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#include "hinic5_nic_cmdq.h"

void hinic5_nic_cmdq_adapt_init(struct hinic5_nic_io *nic_io)
{
	if (!HINIC5_SUPPORT_FEATURE(nic_io->hwdev, HTN_CMDQ))
		nic_io->cmdq_ops = hinic5_nic_cmdq_get_182x_ops();
	else
		nic_io->cmdq_ops = hinic5_nic_cmdq_get_187x_ops();
}
