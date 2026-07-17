/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026-2026. All rights reserved.
 */
#ifndef HTN_QPOOL_OPS_H
#define HTN_QPOOL_OPS_H

#include "bifur_qpool_main.h"

struct bifur_qpool_qbuf {
    struct list_head qpool_buf_list;
    unsigned long vm_start;
    unsigned long vma_size;
    void *buf_vaddr;
    dma_addr_t buf_paddr;
};

struct bifur_qpool_dpdk_ops *bifur_qpool_dpdk_get_187x_ops(void);
#endif
