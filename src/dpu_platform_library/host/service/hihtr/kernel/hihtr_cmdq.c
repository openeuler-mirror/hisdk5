/*
 * Copyright (c) 2016 Hisilicon Limited.
 * Copyright (c) 2007, 2008 Mellanox Technologies. All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <linux/module.h>
#include "hihtr_hw_v3.h"
#include "hihtr_common.h"
#include "hihtr_cmdq.h"

#define CMDQ_TIMEOUT 60000
static uint cmdq_timeout = CMDQ_TIMEOUT;
module_param(cmdq_timeout, uint, 0444);
MODULE_PARM_DESC(cmdq_timeout, "cmdq timeout for RoCE");

void hihtr_cmq_setup_basic_desc(struct hihtr_cmq_desc *desc,
					  enum hihtr_opcode_type opcode,
					  bool is_read)
{
	memset((void *)desc, 0, sizeof(struct hihtr_cmq_desc));
	desc->opcode = cpu_to_le16(opcode);
	desc->flag = cpu_to_le16(HIHTR_CMD_FLAG_IN);
	if (is_read)
		desc->flag |= cpu_to_le16(HIHTR_CMD_FLAG_WR);
	else
		desc->flag &= cpu_to_le16(~HIHTR_CMD_FLAG_WR);
}

int hihtr5_cmq_send(struct hihtr_dev *hr_dev,
	struct hihtr_cmq_desc *desc, int num)
{
    int ret;
    u64 out_param = 0; // 预埋：该参数当前输出值等于输入值，未使用
    struct hinic5_cmd_buf *cmd_buf = NULL;
    struct hinic5_cmdq_cmd_param cmd_param;
    struct hihtr_v3_priv *priv = hr_dev->priv;
    size_t cmdq_size = sizeof(struct hihtr_cmq_desc) * num;

    if (HIHTR_CMDQ_MAX_SIZE < cmdq_size) {
        return -EINVAL;
    }

    cmd_buf = hinic5_alloc_cmd_buf(priv->lld_dev->hwdev);
    if (!cmd_buf) {
        ibdev_err(&hr_dev->ib_dev,
            "failed to alloc cmd buffer in %s.\n", __func__);
        return -ENOMEM;
    }

    memcpy(cmd_buf->buf, desc, cmdq_size);
    cmd_buf->size = (u16)cmdq_size;

    cmd_param.mod = HINIC5_MOD_HIHTR;
    cmd_param.cmd = (u8)(desc->opcode);
    cmd_param.buf_in = cmd_buf;
    cmd_param.buf_out = cmd_buf;
    cmd_param.out_param = &out_param;

    /* 通过NIC接口发送cmdq */
	ret = hinic5_cmdq_inline_data(priv->lld_dev->hwdev, &cmd_param,
		cmdq_timeout, HINIC5_CHANNEL_HIHTR);
	if (ret) {
		ibdev_err(&hr_dev->ib_dev,
			"failed to send cmdq, ret = %d, out param = %lld.\n", ret,
			out_param);
	}

	/* inline回写值覆盖原desc输出 */
	memcpy(desc, cmd_buf->buf, cmdq_size);
	if (ret) {
		ibdev_err(&hr_dev->ib_dev,
			"ret_opcode = 0x%x, ret_val = 0x%x, vf_id = 0x%x, tag = 0x%x.\n",
			desc->response.ret_opcode, desc->response.ret_val,
			desc->response.vf_id, desc->response.resp_tag); // 错误码含义详见 EN_CMD_ERR_CODE
	}
	hinic5_free_cmd_buf(priv->lld_dev->hwdev, cmd_buf);

	return ret;
}

static inline void mbox_desc_data_init(struct hihtr_cmq_desc *desc, u64 in_param, u64 out_param,
                       u8 cmd, unsigned long tag)
{
    struct hihtr_cmq_req *cfg_mb =
				    (struct hihtr_cmq_req *)desc->data;

    hr_reg_write(cfg_mb, CFG_MB_IN_PARAM0, lower_32_bits(in_param));
	hr_reg_write(cfg_mb, CFG_MB_IN_PARAM1, upper_32_bits(in_param));
	hr_reg_write(cfg_mb, CFG_MB_OUT_PARAM0, lower_32_bits(out_param));
	hr_reg_write(cfg_mb, CFG_MB_OUT_PARAM1, upper_32_bits(out_param));
	hr_reg_write(cfg_mb, CFG_MB_CMD, cmd);
	hr_reg_write(cfg_mb, CFG_MB_TAG, tag);
	hr_reg_write(cfg_mb, CFG_MB_EVENT_EN, 0);
}

int hihtr_cmq_mbox(struct hihtr_dev *hr_dev, u64 in_param, u64 out_param,
                       u8 cmd, unsigned long tag)
{
	struct hihtr_cmq_desc desc;

	hihtr_cmq_setup_basic_desc(&desc, HIHTR_OPC_POST_MB, false);
	mbox_desc_data_init(&desc, in_param, out_param, cmd, tag);
	return hihtr5_cmq_send(hr_dev, &desc, 1);
}

EXPORT_SYMBOL(hihtr5_cmq_send);
