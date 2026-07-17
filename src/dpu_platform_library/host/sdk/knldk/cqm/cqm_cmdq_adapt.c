// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#include "cqm_cmdq.h"
#include "hinic5_hwdev.h"

void cqm_cmdq_adapt_init(struct tag_cqm_handle *cqm_handle)
{
	if (!COMM_SUPPORT_HTN_CMD(cqm_handle->ex_handle)) {
		cqm_handle->cmdq_ops = cqm_cmdq_get_182x_ops();
	} else {
		cqm_handle->cmdq_ops = cqm_cmdq_get_187x_ops();
	}
}
