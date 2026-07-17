/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2025. All rights reserved.
 *
 * File Name     : hihtr_dfx.c
 * Version       : v0.1
 * Created       : 2024/10/30
 * Last Modified : 2024/10/30
 * Description   : The definition of callback functions and internal functions of 1872 RoCE driver dfx features.
 */

#ifdef __ROCE_DFX__

#include <linux/fs.h>
#include <linux/slab.h>
#include "hihtr_dfx.h"

int hihtr_get_drv_version(void *buf_out, u32 *out_size, struct hihtr_dev *hr_dev)
{
    struct drv_version_info *ver_info = buf_out;

    if (!buf_out) {
        dev_err(hr_dev->dev, "Buf_out is NULL.");
        return -EINVAL;
    }

    if (*out_size != sizeof(*ver_info)) {
        dev_err(hr_dev->dev, "Unexpect out buf size from user :%u, expect: %lu", *out_size, sizeof(*ver_info));
        return -EINVAL;
    }

    snprintf(ver_info->ver, sizeof(ver_info->ver), "%s  %s", HIHTR_DRV_VERSION, __TIME_STR__);

    return 0;
}

void *hihtr_adm_generic_find_cmd_func(const hihtr_adm_generic_cmd_proc_t *cmd_array, int elem_count, u32 target_cmd)
{
    int i;

    if (cmd_array == NULL || elem_count <= 0) {
        return NULL;
    }

    for (i = 0; i < elem_count; i++) {
        if (cmd_array[i].cmd_type == target_cmd) {
            return cmd_array[i].proc_func;
        }
    }
    return NULL;
}

#endif /* __ROCE_DFX__ */
