/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2025. All rights reserved.
 *
 * File Name     : hihtr_dfx_bypass_set.c
 * Version       : v0.1
 * Created       : 2025/1/16
 * Last Modified : 2025/1/16
 * Description   : The definition of callback functions and internal functions of 1872 RoCE driver dfx features on bypass set.
 */

#ifdef __ROCE_DFX__

#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <rdma/ib_verbs.h>
#include "comm_defs.h"
#include "hinic5_hw.h"
#include "hihtr_dfx.h"

int hihtr_bypass_func_cnt_change(struct hihtr_dev *hr_dev, u8 func_add)
{
    struct hihtr_v3_priv *priv = hr_dev->priv;
    roce_config_bypass_cmd_s buf_in = {0};
    u16 in_size = sizeof(buf_in);
    int ret;

    buf_in.func_add = func_add;
    buf_in.op_type = hr_dev->is_vf ? HIHTR_CHANGE_VF_CNT : HIHTR_CHANGE_PF_CNT;
    ret = hihtr_v3_msg_to_mgmt_sync(priv->lld_dev->hwdev, ROCE_MPU_CMD_CFG_BYPASS, &buf_in, in_size, NULL, NULL);
    if (ret != 0) {
        dev_err(hr_dev->dev, "change bypass func cnt failed.(ret:%d)", ret);
        return -EINVAL;
    }

    return ret;
}

static int hihtr_dfx_bypass_set(struct hihtr_dev *hr_dev, const struct hihtr_dfx_bypass_inbuf *inbuf)
{
    struct hihtr_v3_priv *priv = hr_dev->priv;
    roce_config_bypass_cmd_s buf_in = {0};
    roce_config_bypass_cmd_s buf_out = {0};
    u16 in_size = sizeof(buf_in);
    u16 out_size = sizeof(buf_out);
    int ret;

    buf_in.bypass_ctrl = inbuf->enable;
    buf_in.op_type = HIHTR_CTRL_BYPASS_MANUAL;
    ret = hihtr_v3_msg_to_mgmt_sync(priv->lld_dev->hwdev, ROCE_MPU_CMD_CFG_BYPASS, &buf_in, in_size, &buf_out, &out_size);
    if (ret != 0) {
        dev_err(hr_dev->dev, "set bypass status manually failed.(ret:%d)", ret);
        return -EINVAL;
    }

    if (buf_out.bypass_ctrl != inbuf->enable) {
        ret = -EINVAL;
    }
    return ret;
}

static int hihtr_check_bypass_cmd_buf(struct hihtr_dev *hr_dev, const struct hihtr_dfx_bypass_inbuf *inbuf,
                               u32 in_size, void *buf_out, u32 *out_size)
{
    if (inbuf == NULL)
        return -ENOMEM;

    if (in_size < sizeof(struct hihtr_dfx_bypass_inbuf)) {
        dev_err(hr_dev->dev, "Input parameter(in_size: %dB) is shorter than input buffer(allocated: %dB)",
                (int)in_size, (int)sizeof(struct hihtr_dfx_bypass_inbuf));
        return -ENOMEM;
    }
    if (*out_size < sizeof(struct hihtr_dfx_bypass_outbuf)) {
        dev_err(hr_dev->dev, "Input parameter(out_size: %dB) is shorter than allocated buffer(allocated: %dB)",
                (int)*out_size, (int)sizeof(struct hihtr_dfx_bypass_outbuf));
        return -ENOMEM;
    }
    memset((void *)buf_out, 0, sizeof(struct hihtr_dfx_bypass_outbuf));
    *out_size = (u32)sizeof(struct hihtr_dfx_bypass_outbuf);

    if (inbuf->cmd_type >= HIHTR_CMD_INVALID) {
        hihtr_dfx_print("Not support this type(%d)", inbuf->cmd_type);
        return -EINVAL;
    }
    return 0;
}

int hihtr_dfx_cmd_set_bypass(struct hihtr_dev *hr_dev, const void *buf_in, u32 in_size,
                                void *buf_out, u32 *out_size)
{
    int ret;
    const struct hihtr_dfx_bypass_inbuf *inbuf = (struct hihtr_dfx_bypass_inbuf *)buf_in;

    ret = hihtr_check_bypass_cmd_buf(hr_dev, inbuf, in_size, buf_out, out_size);
    if (ret != 0)
        return ret;

    return hihtr_dfx_bypass_set(hr_dev, inbuf);
}

static int hihtr_dfx_bypass_query(struct hihtr_dev *hr_dev, struct hihtr_dfx_bypass_outbuf *outbuf)
{
    struct hihtr_v3_priv *priv = hr_dev->priv;
    roce_config_bypass_cmd_s buf_in = {0};
    roce_config_bypass_cmd_s buf_out = {0};
    u16 in_size = sizeof(buf_in), out_size = sizeof(buf_out);
    int ret;

    buf_in.op_type = HIHTR_QUERY_BYPASS_STATUS;
    ret = hihtr_v3_msg_to_mgmt_sync(priv->lld_dev->hwdev, ROCE_MPU_CMD_CFG_BYPASS, &buf_in, in_size, &buf_out, &out_size);
    if (ret != 0) {
        dev_err(hr_dev->dev, "query bypass status failed.(ret:%d)", ret);
        return -EINVAL;
    }

    outbuf->enable = buf_out.bypass_ctrl;
    return ret;
}

int hihtr_dfx_cmd_query_bypass(struct hihtr_dev *hr_dev, const void *buf_in, u32 in_size,
                                void *buf_out, u32 *out_size)
{
    int ret;
    const struct hihtr_dfx_bypass_inbuf *inbuf = (struct hihtr_dfx_bypass_inbuf *)buf_in;
    struct hihtr_dfx_bypass_outbuf *outbuf = (struct hihtr_dfx_bypass_outbuf *)buf_out;

    ret = hihtr_check_bypass_cmd_buf(hr_dev, inbuf, in_size, buf_out, out_size);
    if (ret != 0)
        return ret;

    return hihtr_dfx_bypass_query(hr_dev, outbuf);
}

#endif /* __ROCE_DFX__ */