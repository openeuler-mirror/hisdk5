/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2025. All rights reserved.
 *
 * File Name     : hihtr_dfx_scc.c
 * Version       : v0.1
 * Created       : 2025/8/5
 * Last Modified : 2025/8/5
 * Description   : The definition of callback functions and internal functions of RoCE driver dfx features on scc set.
 */

#ifdef __ROCE_DFX__

#include "hihtr_dfx.h"

static int hihtr_dfx_scc_set(struct hihtr_dev *hr_dev,
        const struct hihtr_dfx_scc_buf *inbuf)
{
    int ret;

    if (!hr_dev->hw->set_scc) {
        dev_err(hr_dev->dev, "set scc not supportted\n");
        return EINVAL;
    }
    ret = hr_dev->hw->set_scc(hr_dev, inbuf->scc_enable, inbuf->algo_type,
                inbuf->template_id, inbuf->scc_mask);
    if (ret != 0) {
        dev_err(hr_dev->dev, "failed to set scc, ret = %d\n", ret);
        return ret;
    }
    return 0;
}

#define SCC_CTRL_FULL_MASK 0x7

static int hihtr_dfx_query_scc(struct hihtr_dev *hr_dev,
        struct hihtr_dfx_scc_buf *outbuf)
{
    int ret;
    struct scc_info card_scc_info;

    if (!hr_dev->hw->query_scc) {
        dev_err(hr_dev->dev, "query scc not supported\n");
        return EINVAL;
    }
    ret = hr_dev->hw->query_scc(hr_dev, &card_scc_info);
    if (ret != 0) {
        dev_err(hr_dev->dev, "failed to query scc, ret = %d\n", ret);
        return ret;
    }
    outbuf->scc_enable = card_scc_info.scc_enable;
    outbuf->algo_type = card_scc_info.scc_algo_id;
    outbuf->template_id = card_scc_info.scc_template_id;
    outbuf->scc_mask = SCC_CTRL_FULL_MASK;

    return 0;
}

static int hihtr_dfx_set_scc_template(struct hihtr_dev *hr_dev,
        struct hihtr_dfx_scc_buf *inbuf)
{
    int ret;

    if (!hr_dev->hw->set_scc_temp) {
        dev_err(hr_dev->dev, "query scc template not supported\n");
        return EINVAL;
    }
    ret = hr_dev->hw->set_scc_temp(hr_dev, inbuf);
    if (ret != 0) {
        dev_err(hr_dev->dev, "failed to query scc template, ret = %d\n", ret);
        return ret;
    }
    return 0;
}

static int hihtr_dfx_query_scc_template(struct hihtr_dev *hr_dev,
        struct hihtr_dfx_scc_buf *outbuf)
{
    int ret;

    if (!hr_dev->hw->query_scc_temp) {
        dev_err(hr_dev->dev, "query scc template not supported\n");
        return EINVAL;
    }
    ret = hr_dev->hw->query_scc_temp(hr_dev, outbuf);
    if (ret != 0) {
        dev_err(hr_dev->dev, "failed to query scc template, ret = %d\n", ret);
        return ret;
    }
    return 0;
}

static int check_scc_cmd_buf(struct hihtr_dev *hr_dev,
                const struct hihtr_dfx_scc_buf *inbuf,
                u32 in_size, void *buf_out, u32 *out_size)
{
    if (inbuf == NULL)
        return -ENOMEM;
    if (in_size < sizeof(struct hihtr_dfx_scc_buf)) {
        dev_err(hr_dev->dev, "Input parameter(in_size: %dB) is shorter than input buffer(allocated: %dB)",
               (int)in_size, (int)sizeof(struct hihtr_dfx_scc_buf));
        return -ENOMEM;
    }
    if (*out_size < sizeof(struct hihtr_dfx_scc_buf)) {
        dev_err(hr_dev->dev, "Input parameter(out_size: %dB) is shorter than allocated buffer(allocated: %dB)",
               (int)*out_size, (int)sizeof(struct hihtr_dfx_scc_buf));
        return -ENOMEM;
    }
    memset((void *)buf_out, 0, sizeof(struct hihtr_dfx_scc_buf));
    *out_size = (u32)sizeof(struct hihtr_dfx_scc_buf);

    if (inbuf->cmd_type >= HIHTR_CMD_INVALID) {
        hihtr_dfx_print("Not support this type(%d)", inbuf->cmd_type);
        return -EINVAL;
    }
    return 0;
}

int hihtr_dfx_cmd_set_scc(struct hihtr_dev *hr_dev, const void *buf_in, u32 in_size,
                             void *buf_out, u32 *out_size)
{
    int ret;
    const struct hihtr_dfx_scc_buf *inbuf = (struct hihtr_dfx_scc_buf *)buf_in;

    ret = check_scc_cmd_buf(hr_dev, inbuf, in_size, buf_out, out_size);
    if (ret != 0)
        return ret;

    return hihtr_dfx_scc_set(hr_dev, inbuf);
}

int hihtr_dfx_cmd_query_scc(struct hihtr_dev *hr_dev, const void *buf_in, u32 in_size,
    void *buf_out, u32 *out_size)
{
    int ret;
    struct hihtr_dfx_scc_buf *outbuf = (struct hihtr_dfx_scc_buf *)buf_out;
    const struct hihtr_dfx_scc_buf *inbuf = (struct hihtr_dfx_scc_buf *)buf_in;

    ret = check_scc_cmd_buf(hr_dev, inbuf, in_size, buf_out, out_size);
    if (ret != 0)
        return ret;

    return hihtr_dfx_query_scc(hr_dev, outbuf);
}

int hihtr_dfx_cmd_set_scc_template(struct hihtr_dev *hr_dev, const void *buf_in, u32 in_size,
    void *buf_out, u32 *out_size)
{
    int ret;
    struct hihtr_dfx_scc_buf *inbuf = (struct hihtr_dfx_scc_buf *)buf_in;

    ret = check_scc_cmd_buf(hr_dev, inbuf, in_size, buf_out, out_size);
    if (ret != 0)
        return ret;

    return hihtr_dfx_set_scc_template(hr_dev, inbuf);
}

int hihtr_dfx_cmd_query_scc_template(struct hihtr_dev *hr_dev,
    const void *buf_in, u32 in_size, void *buf_out, u32 *out_size)
{
    int ret;
    const struct hihtr_dfx_scc_buf *inbuf = (struct hihtr_dfx_scc_buf *)buf_in;
    struct hihtr_dfx_scc_buf *outbuf = (struct hihtr_dfx_scc_buf *)buf_out;

    ret = check_scc_cmd_buf(hr_dev, inbuf, in_size, buf_out, out_size);
    if (ret != 0)
        return ret;

    outbuf->template_id = inbuf->template_id;
    return hihtr_dfx_query_scc_template(hr_dev, outbuf);
}

#endif