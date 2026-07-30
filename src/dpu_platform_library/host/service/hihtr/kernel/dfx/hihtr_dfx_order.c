/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2025. All rights reserved.
 *
 * File Name     : hihtr_dfx_order.c
 * Version       : v0.1
 * Created       : 2026/4/15
 * Last Modified : 2026/4/15
 * Description   : Set the 1872 ROCEE write payload order configuration.
 */

#ifdef __ROCE_DFX__

#include "hihtr_dfx.h"

int hihtr_cmq_set_so_cfg(struct hihtr_dev *hr_dev, payload_so_type so_type)
{
	struct hihtr_cmq_desc desc;
	struct hihtr_order_cmd *so_info = (struct hihtr_order_cmd*)desc.data;
	int ret;

	if (hr_dev->is_vf)
		return -EOPNOTSUPP;

	hihtr_cmq_setup_basic_desc(&desc, HIHTR_SET_ROCEE_SO_TYPE, false);
	so_info->so_type = so_type;
	ret = hihtr5_cmq_send(hr_dev, &desc, 1);
	if (ret) {
		ibdev_err(&hr_dev->ib_dev, "failed to set rocee so_type, ret = %d.\n", ret);
	}
	return ret;
}

int check_set_so_buf(struct hihtr_order_cmd *so_info)
{
	if (so_info->so_type != ALWAYS_SO && so_info->so_type != RO) {
		return -EINVAL;
	}
	return 0;
}

int hihtr_dfx_set_so_type(struct hihtr_dev *hr_dev, const void *buf_in, u32 in_size,
                             void *buf_out, u32 *out_size)
{
    int ret;
    struct hihtr_order_cmd *so_info = (struct hihtr_order_cmd *)buf_in;

	if (HIHTR_GET_PCI_REVISION(hr_dev) <= PCI_REVISION_ID_HIP08)
		return -EOPNOTSUPP;

	ret = check_set_so_buf(so_info);
	if (ret != 0)
		return ret;

	return hihtr_cmq_set_so_cfg(hr_dev, so_info->so_type);
}

int hihtr_dfx_query_so_type(struct hihtr_dev *hr_dev, const void *buf_in, u32 in_size,
                             void *buf_out, u32 *out_size)
{
	int ret = 0;
	struct hihtr_cmq_desc desc;
	struct hihtr_order_cmd *so_info;

	if (HIHTR_GET_PCI_REVISION(hr_dev) <= PCI_REVISION_ID_HIP08)
		return -EOPNOTSUPP;

	hihtr_cmq_setup_basic_desc(&desc, HIHTR_QUERY_ROCEE_SO_TYPE, true);
	ret = hihtr5_cmq_send(hr_dev, &desc, 1);
	if (ret) {
		ibdev_err(&hr_dev->ib_dev, "failed to query rocee so_type, ret = %d.\n", ret);
		return ret;
	}

	memcpy(buf_out, desc.data, sizeof(struct hihtr_order_cmd));
	so_info = (struct hihtr_order_cmd*)desc.data;
	return ret;
}

#endif /* __ROCE_DFX__ */
