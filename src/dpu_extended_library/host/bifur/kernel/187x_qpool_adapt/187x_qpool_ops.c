/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026-2026. All rights reserved.
 */
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/cdev.h>
#include <linux/netdevice.h>
#include <linux/bitops.h>
#include "hinic5_hw.h"
#include "hinic5_nic.h"
#include "hinic5_nic_cfg.h"
#include "drv_nic_api.h"
#include "comm_defs.h"
#include "nic_cfg_comm.h"
#include "nic_drv_ext_cmd_defs.h"
#include "nic_npu_ext_cmd_defs.h"
#include "drv_nic_api.h"
#include "187x_qpool_ops.h"
#include "bifur_main.h"

#define HINIC5_SET_INDIR_TBL 1
#define HINIC5_GET_INDIR_TBL 0

static int set_first_zero_bit(uint8_t *value) {
    uint8_t mask;
    int i;

    if (*value == 0xFF) {
        return -1;
    }

    mask = 1;
    for (i = 0; i < BIFUR_FUNC_MAX_QUEUE_GROUP; ++i) {
        if ((*value & mask) == 0) {
            *value |= mask;
            return i;       // 返回位索引（0 ~ BIFUR_FUNC_MAX_QUEUE_GROUP - 1）
        }
        mask <<= 1;
    }

    return -1;
}

static inline bool qg_bitmap_exceed_valid_cnt(u8 qg_bitmap, u8 qg_valid_cnt)
{
    return hweight8(qg_bitmap) >= qg_valid_cnt;
}

int bifur_187x_release_queue_group(struct bifur_qpool_dev *qpool_dev, u16 q_grp_id)
{
    qpool_dev->qpool_group.qg_bitmap &= ~(1U << q_grp_id);
    pr_info("[QUEUE_POOLING] release queue group: %u.\n", q_grp_id);
    return 0;
}

int bifur_187x_alloc_queue_group(struct bifur_qpool_dev *qpool_dev)
{
    int ret = 0;

    if (qg_bitmap_exceed_valid_cnt(qpool_dev->qpool_group.qg_bitmap, qpool_dev->qpool_group.qg_valid_cnt)) {
        return -ENOMEM;
    }
    ret = set_first_zero_bit(&(qpool_dev->qpool_group.qg_bitmap));
    if (ret < 0) {
        pr_err("[QUEUE_POOLING] Failed to allocate queue group, bitmap state: 0x%02X, valid cnt: %u\n",
               qpool_dev->qpool_group.qg_bitmap, qpool_dev->qpool_group.qg_valid_cnt);
        return -ENOMEM;
    }
    pr_info("[QUEUE_POOLING] alloc queue group: %d.\n", ret);
    return ret;
}

static int validate_queue_allocation(struct hinic5_nic_dev *nic_dev,
                                    struct bifur_qpool_dev *qpool_dev,
                                    struct mag_cmd_user_queue_get *queue_get) {
    u16 qp_per_group = qpool_dev->u_params.qp_start;
    if (nic_dev->q_params.num_qps > qp_per_group) {
        pr_err("[QUEUE_POOLING] NIC qps exceed qpool grouping qps, nic qps : %u. qpool grouping qps : %u\n",
                nic_dev->q_params.num_qps, qp_per_group);
        return -EINVAL;
    }

    if (queue_get->qid >= qp_per_group) {
        pr_err("[QUEUE_POOLING] alloc qp exceed qpool grouping qps, alloc qp : %u, qpool grouping qps : %u\n",
                queue_get->qid, qp_per_group);
        return -EINVAL;
    }
    return 0;
}

static int process_queue_group(struct bifur_fpriv_data *fpriv, 
                              struct qgrp_mapping_ht_key *key,
                              struct bifur_qpool_dev *qpool_dev,
                              u16 *q_grp_id)
{
    int ret;

    ret = bifur_get_qgrp_id_by_ht_key(key->func_id, key->lcore_id, q_grp_id);
    if (ret != 0) {
        ret = bifur_187x_alloc_queue_group(qpool_dev);
        if (ret < 0) {
            pr_err("[QUEUE_POOLING] get queue failed no more queue group, func_id: %u, lcore_id: %u.\n",
                    key->func_id, key->lcore_id);
            return ret;
        }
        *q_grp_id = ret;
        ret = qgrp_mapping_add_ht_entry(key, *q_grp_id);
        if (ret != 0) {
            pr_err("[QUEUE_POOLING] hashtable entry insert failed, func_id: %u, lcore_id: %u.\n",
                    key->func_id, key->lcore_id);
            bifur_187x_release_queue_group(qpool_dev, *q_grp_id);
            return ret;
        }
    } else if (*q_grp_id > BIFUR_FUNC_MAX_QUEUE_GROUP - 1) {
        pr_err("[QUEUE_POOLING] hashtable entry q_grp_id invalid, func_id: %u, lcore_id: %u.\n",
                key->func_id, key->lcore_id);
        return -EINVAL;
    } else {
        ret = qgrp_mapping_increase_queue_cnt(key->func_id, key->lcore_id, *q_grp_id);
        if (ret != 0) {
            pr_err("[QUEUE_POOLING] hashtable entry insert failed, func_id: %u, lcore_id: %u.\n",
                    key->func_id, key->lcore_id);
            return ret;
        }
    }

    return 0;
}

static u16 calculate_local_qid(struct bifur_qpool_dev *qpool_dev,
                              struct hinic5_nic_dev *nic_dev,
                              u16 qid, u16 q_grp_id)
{
    struct bifur_user_txrxq_params *u_params = &qpool_dev->u_params;

    return u_params->qp_start + 
           q_grp_id * (nic_dev->max_qps / qpool_dev->qpool_group.qg_num) + 
           qid;
}

static u16 reverse_calculate_qid(struct bifur_qpool_dev *qpool_dev,
                               struct hinic5_nic_dev *nic_dev,
                               u16 local_qid, u16 q_grp_id)
{
    struct bifur_user_txrxq_params *u_params = &qpool_dev->u_params;
    u16 group_size = nic_dev->max_qps / qpool_dev->qpool_group.qg_num;

    return local_qid - u_params->qp_start - q_grp_id * group_size;
}

int bifur_187x_get_user_queue_id(struct bifur_fpriv_data *fpriv, struct mag_cmd_user_queue_get *queue_get)
{
    int ret = 0;
    u16 q_grp_id, local_qid;
    struct qgrp_mapping_ht_key key = { 0 };
    struct bifur_lld_dev *bifur_dev = fpriv->cdev_data->bifur_dev;
    struct bifur_qpool_dev *qpool_dev = bifur_dev->qpool_dev;

    struct hinic5_nic_dev *nic_dev = bifur_get_hinic5_nic_by_bifur_dev(bifur_dev);
    if (!nic_dev) {
        pr_err("nic dev cannot be found\n");
        return -EINVAL;
    }

    ret = validate_queue_allocation(nic_dev, qpool_dev, queue_get);
    if (ret != 0) {
        return ret; // 错误日志在函数内打印
    }

    key.func_id = fpriv->func_id;
    key.lcore_id = fpriv->lcore_id;

    ret = process_queue_group(fpriv, &key, qpool_dev, &q_grp_id);
    if (ret != 0) {
        return ret; // 错误日志在函数内打印
    }

    local_qid = calculate_local_qid(qpool_dev, nic_dev, queue_get->qid, q_grp_id);
    if (local_qid >= nic_dev->max_qps) {
        pr_err("[QUEUE_POOLING] qpool grouping start qps exceed nic max qps, qpool grouping start qp : %u, nic max qps : %u\n",
                local_qid , nic_dev->max_qps);
        return -EINVAL;
    }

    queue_get->local_qid = local_qid;
    pr_info("[QUEUE_POOLING] qid map qid:%u, local_qid:%u...... func_id: %u, lcore_id: %u, q_grp_id:%u.\n",
        queue_get->qid, queue_get->local_qid, key.func_id, key.lcore_id, q_grp_id);
    return 0;
}


int bifur_187x_del_user_queue_id(struct bifur_fpriv_data *fpriv, struct mag_cmd_user_queue_get *queue_get)
{
    int ret;
    u16 q_grp_id;
    struct qgrp_mapping_ht_key key = { 0 };

    struct bifur_lld_dev *bifur_dev = fpriv->cdev_data->bifur_dev; 
    struct bifur_qpool_dev *qpool_dev = bifur_dev->qpool_dev;

    key.func_id = fpriv->func_id;
    key.lcore_id = fpriv->lcore_id;

    ret = bifur_get_qgrp_id_by_ht_key(key.func_id, key.lcore_id, &q_grp_id);
    if (ret != 0) {
        pr_info("[QUEUE_POOLING] hashtable entry does not exist, func_id: %u, lcore_id: %u.\n",
                 key.func_id, key.lcore_id);
        return ret;
    } else if (q_grp_id > BIFUR_FUNC_MAX_QUEUE_GROUP - 1) { // q_grp_id是dpdk参数, 跳过内核的1个队列
        pr_err("[QUEUE_POOLING] hashtable entry q_grp_id invalid, func_id: %u, lcore_id: %u.\n",
                key.func_id, key.lcore_id);
        return -EINVAL;
    } else {
        ret = qgrp_mapping_decrease_queue_cnt(key.func_id, key.lcore_id);
        if (ret < 0) {
            pr_err("[QUEUE_POOLING] user queues counter sub failed, lcore_id: %u.\n", key.lcore_id);
            return ret;
        } else if (ret == 0) {
            bifur_187x_release_queue_group(qpool_dev, q_grp_id);
            return ret;
        }
    }

    return 0;
}

int bifur_187x_set_rss_indir_table(struct bifur_fpriv_data *fpriv, struct mag_cmd_rss_indir_tbl *indir_tbl)
{
    int ret = 0;
    struct qgrp_mapping_ht_key key = {0};
    u16 i, q_grp_id;
    struct bifur_lld_dev *bifur_dev = fpriv->cdev_data->bifur_dev; 
    struct bifur_qpool_dev *qpool_dev = bifur_dev->qpool_dev;
    struct hinic5_nic_dev *nic_dev = bifur_get_hinic5_nic_by_bifur_dev(bifur_dev);
    struct bifur_user_txrxq_params *u_params = &qpool_dev->u_params;
    u16 indir_length = NIC_RSS_INDIR_SIZE / qpool_dev->qpool_group.qg_num;
    u32 *rss_indir = kzalloc((indir_length) * sizeof(u32), GFP_KERNEL);
    if (!rss_indir) {
        pr_err("[QUEUE_POOLING] Failed to allocate memory for rss_indir table\n");
        return -ENOMEM;
    }
    key.func_id = fpriv->func_id;
    key.lcore_id = fpriv->lcore_id;

    ret = bifur_get_qgrp_id_by_ht_key(key.func_id, key.lcore_id, &q_grp_id);
    if (ret != 0 || q_grp_id > BIFUR_FUNC_MAX_QUEUE_GROUP - 1) {
        pr_info("[QUEUE_POOLING] hashtable entry q_grp_id does not exit, func_id: %u, lcore_id: %u, q_grp_id:%u.\n",
                key.func_id, key.lcore_id, q_grp_id);
        goto free_rss_indir;
    }
    pr_info("[QUEUE_POOLING] set indirect table, func_id: %u, lcore_id: %u, q_grp_id:%u, q_grp_num:%u, qp_start:%u.\n",
                key.func_id, key.lcore_id, q_grp_id, qpool_dev->qpool_group.qg_num, u_params->qp_start);
    for (i = 0; i < indir_length; i++) {
        rss_indir[i] = u_params->qp_start +
            q_grp_id * (nic_dev->max_qps / qpool_dev->qpool_group.qg_num) +
            indir_tbl->rss_indir.entry[i];
    }

    ret = hinic5_cfg_flow_bifurcation_paras(nic_dev->netdev, HINIC5_SET_INDIR_TBL, q_grp_id + 1, rss_indir, indir_length); // q_grp_id未计算内核group, 调用海思接口需+1
    if (ret != 0) {
        pr_err("[QUEUE_POOLING] Failed to set indirect table for q_grp_id %u, ret:%d\n", q_grp_id + 1, ret);
        goto free_rss_indir;
    }
free_rss_indir:
    kfree(rss_indir);  // 释放内存
    return ret;
}

int bifur_187x_get_rss_indir_table(struct bifur_fpriv_data *fpriv, struct mag_cmd_rss_indir_tbl *indir_tbl)
{
    int ret = 0, i;
    struct qgrp_mapping_ht_key key = {0};
    u16 q_grp_id;
    struct bifur_lld_dev *bifur_dev = fpriv->cdev_data->bifur_dev; 
    struct bifur_qpool_dev *qpool_dev = bifur_dev->qpool_dev;
    struct hinic5_nic_dev *nic_dev = bifur_get_hinic5_nic_by_bifur_dev(bifur_dev);
    struct bifur_user_txrxq_params *u_params = &qpool_dev->u_params;
    u16 indir_length = NIC_RSS_INDIR_SIZE / qpool_dev->qpool_group.qg_num;

    u32 *rss_indir = kzalloc((indir_length) * sizeof(u32), GFP_KERNEL);
    if (!rss_indir) {
        pr_err("[QUEUE_POOLING] Failed to allocate memory for rss_indir table\n");
        return -ENOMEM;
    }

    key.func_id = fpriv->func_id;
    key.lcore_id = fpriv->lcore_id;
    ret = bifur_get_qgrp_id_by_ht_key(key.func_id, key.lcore_id, &q_grp_id);
    if (ret != 0 || q_grp_id > BIFUR_FUNC_MAX_QUEUE_GROUP - 1) {
        pr_info("[QUEUE_POOLING] hashtable entry q_grp_id does not exit, func_id: %u, lcore_id: %u, q_grp_id:%u.\n",
                key.func_id, key.lcore_id, q_grp_id);
        goto free_rss_indir;
    }
    pr_info("[QUEUE_POOLING] get indirect table, func_id: %u, lcore_id: %u, q_grp_id:%u, q_grp_num:%u, qp_start:%u.\n",
                key.func_id, key.lcore_id, q_grp_id, qpool_dev->qpool_group.qg_num, u_params->qp_start);
    ret = hinic5_cfg_flow_bifurcation_paras(nic_dev->netdev, HINIC5_GET_INDIR_TBL, q_grp_id + 1, // q_grp_id未计算内核group, 调用海思接口需+1
                                            rss_indir, indir_length);
    if (ret != 0) {
        pr_err("[QUEUE_POOLING] Failed to get indirect table for q_grp_id %u, ret:%d\n", q_grp_id + 1, ret);
        goto free_rss_indir;
    }

    for (i = 0; i < indir_length; i++) {
        indir_tbl->rss_indir.entry[i] = reverse_calculate_qid(qpool_dev, nic_dev, rss_indir[i], q_grp_id);
    }
    indir_tbl->indir_length = indir_length;
free_rss_indir:
    kfree(rss_indir);  // 释放内存
    return ret;
}

static int bifur_flow_index_add(struct bifur_fpriv_data *fpriv, u32 index, u32 type)
{
    struct bifur_flow_entry *entry = NULL;
    int ret;

    entry = kmalloc(sizeof(*entry), GFP_KERNEL);
    if (entry == NULL) {
        pr_err("Failed to allocate memory for flow entry\n");
        return -ENOMEM;
    }

    entry->index = index;
    entry->type = type;

    ret = xa_insert(&fpriv->flow_xa, index, entry, GFP_KERNEL);
    if (ret != 0) {
        pr_err("Failed to insert flow entry with index %u, type %u\n", index, type);
        kfree(entry);
        entry = NULL;
    }

    pr_info("Added flow entry: index = %u, type = %u, func_id: %u, lcore_id: %u\n", index, type, fpriv->func_id, fpriv->lcore_id);
    return ret;
}

static void bifur_flow_index_del(struct bifur_fpriv_data *fpriv, u32 index)
{
    struct bifur_flow_entry *entry = NULL;

    entry = xa_erase(&fpriv->flow_xa, index);
    if (entry == NULL) {
        pr_warn("Flow entry with index %u not found, func_id: %u, lcore_id: %u\n",
                index, fpriv->func_id, fpriv->lcore_id);
        return;
    }
    pr_info("Deleted flow entry: index = %u, type = %u, func_id: %u, lcore_id: %u\n",
            entry->index, entry->type, fpriv->func_id, fpriv->lcore_id);
    kfree(entry);
    entry = NULL;
}

static int bifur_block_add(struct bifur_fpriv_data *fpriv, u16 block_index, u8 tcam_type)
{
    struct bifur_block_entry *entry;
    int ret;

    entry = kmalloc(sizeof(*entry), GFP_KERNEL);
    if (entry == NULL) {
        pr_err("Failed to allocate memory for block entry\n");
        return -ENOMEM;
    }

    entry->block_index = block_index;
    entry->tcam_type = tcam_type;

    ret = xa_insert(&fpriv->block_xa, block_index, entry, GFP_KERNEL);
    if (ret != 0) {
        pr_err("Failed to insert block entry with index %u, tcam_type %u\n", block_index, tcam_type);
        kfree(entry);
        entry = NULL;
    }

    pr_info("Added block entry: block_index = %u, tcam_type = %u, func_id: %u, lcore_id: %u\n",
            block_index, tcam_type, fpriv->func_id, fpriv->lcore_id);
    return ret;
}

static void bifur_block_del(struct bifur_fpriv_data *fpriv, u16 block_index)
{
    struct bifur_block_entry *entry = NULL;

    entry = xa_erase(&fpriv->block_xa, block_index);
    if (entry == NULL) {
        pr_warn("Block entry with block_index %u not found, func_id: %u, lcore_id: %u\n",
                block_index, fpriv->func_id, fpriv->lcore_id);
        return;
    }
    pr_info("Deleted block entry: block_index = %u, tcam_type = %u, func_id: %u, lcore_id: %u\n",
            entry->block_index, entry->tcam_type, fpriv->func_id, fpriv->lcore_id);
    kfree(entry);
    entry = NULL;
}

int bifur_187x_add_tc_flow_pre_handle(struct bifur_fpriv_data *fpriv, struct drv_mpu_cmd_st *cmd, uint32_t lcore_id,
    struct bifur_fdir_add_rule *tcam_cmd)
{
    int ret;
    u16 q_grp_id, queue_num;
    struct hinic5_nic_dev *nic_dev = bifur_get_hinic5_nic_by_bifur_dev(fpriv->cdev_data->bifur_dev);
    struct qgrp_mapping_ht_key key = { 0 };
    struct bifur_qpool_dev *qpool_dev = fpriv->cdev_data->bifur_dev->qpool_dev;

    queue_num = tcam_cmd->rule.data.ext;
    key.func_id = fpriv->func_id;
    key.lcore_id = fpriv->lcore_id;
    ret = bifur_get_qgrp_id_by_ht_key(key.func_id, key.lcore_id, &q_grp_id);
    if (ret != 0 || q_grp_id > BIFUR_FUNC_MAX_QUEUE_GROUP - 1) { // q_grp_id是dpdk参数, 跳过内核的1个队列
        pr_info("[QUEUE_POOLING] hashtable entry q_grp_id does not exit, func_id: %u, lcore_id: %u.\n",
                key.func_id, lcore_id);
        return ret;
    }

    if (queue_num > 1) {
        tcam_cmd->rule.data.fdir_info.qid_htn.qid = q_grp_id + 1;
        tcam_cmd->rule.data.fdir_info.qid_htn.flag = 1;
    } else {
        tcam_cmd->rule.data.fdir_info.qid_htn.qid = calculate_local_qid(qpool_dev, nic_dev, tcam_cmd->rule.data.qid, q_grp_id);
        tcam_cmd->rule.data.fdir_info.qid_htn.flag = 0;
    }

    pr_info("[QUEUE_POOLING] add tc flow, func_id: %u, lcore_id: %u, queue_id: %u, queue_num: %u.\n",
                key.func_id, key.lcore_id, tcam_cmd->rule.data.fdir_info.qid_htn.qid, queue_num);
    return 0;
}

int bifur_187x_add_tc_flow_commit_handle(struct bifur_fpriv_data *fpriv, struct drv_mpu_cmd_st *cmd, uint32_t lcore_id,
    struct bifur_fdir_add_rule *tcam_cmd)
{
    return bifur_flow_index_add(fpriv, tcam_cmd->rule.index, tcam_cmd->type);
}

int bifur_187x_del_tc_flow_commit_handle(struct bifur_fpriv_data *fpriv, struct drv_mpu_cmd_st *cmd, uint32_t lcore_id,
    struct bifur_fdir_del_rule *tcam_cmd)
{
    u32 i, index;
    for (i = 0; i < tcam_cmd->index_num; i++) {
        index = tcam_cmd->index_start + i;
        bifur_flow_index_del(fpriv, index);
    }
    return 0;
}

int bifur_187x_cfg_tcam_block_commit_handle(struct bifur_fpriv_data *fpriv, struct drv_mpu_cmd_st *cmd, uint32_t lcore_id,
    struct bifur_tcam_block *tcam_cmd)
{
    int ret = 0;
    if (tcam_cmd->alloc_en == 0) {
        bifur_block_del(fpriv, tcam_cmd->tcam_block_index);
    } else {
        ret = bifur_block_add(fpriv, tcam_cmd->tcam_block_index, tcam_cmd->tcam_type);
    }
    return ret;
}

static void set_vm_flags(struct vm_area_struct *vma, unsigned long flags)
{
#ifdef HAVE_VM_FLAGS_SET
    vm_flags_set(vma, flags);
#else
    vma->vm_flags |= flags;
#endif
}

static int bifur_check_dev_valid(struct bifur_fpriv_data *fpriv)
{
    struct bifur_cdev_data *cdev_data = NULL;

    if (!fpriv) {
        pr_err("[qpool file private_data is null\n");
        return -EINVAL;
    }

    cdev_data = fpriv->cdev_data;
    if (!cdev_data || !(cdev_data->bifur_dev) ||
        !(cdev_data->bifur_dev->lld_dev) || !(to_pci_dev(cdev_data->bifur_dev->lld_dev->dev))) {
        pr_err("[qpool]qpool dev is null\n");
        return -EINVAL;
    }

    return 0;
}

int bifur_187x_qpool_mmap(struct bifur_fpriv_data *fpriv, struct vm_area_struct *vma)
{
    int ret;
    unsigned long pfn, vma_size, bar_size, page_offset, check_size;
    struct pci_dev *pdev = NULL;
    struct bifur_cdev_data *cdev_data = NULL;

    ret = bifur_check_dev_valid(fpriv);
    if (ret != 0) {
        return ret;
    }

    cdev_data = fpriv->cdev_data;
    pdev = to_pci_dev(cdev_data->bifur_dev->lld_dev->dev);

    page_offset = vma->vm_pgoff & QPOOL_BAR_MMAP_MASK;
    if (page_offset >= HINIC5_VF_PCI_CFG_REG_BAR && page_offset <= HINIC5_PCI_DB_BAR) {
        pfn = pci_resource_start(pdev, page_offset) >> PAGE_SHIFT;
        bar_size = pci_resource_len(pdev, page_offset);
    } else {
        pr_err("[qpool]invalid offset:0x%lx\n", page_offset);
        return -EINVAL;
    }

    vma_size = vma->vm_end - vma->vm_start;
    /* bar_size align pagesize, check vma_size */
    check_size =  ((bar_size + PAGE_SIZE - 1) / PAGE_SIZE) * PAGE_SIZE;
    if (vma_size > check_size) {
        pr_err("[qpool]invalid vma_size:0x%lx, map_size:0x%lx, bar_size:0x%lx.\n", vma_size, check_size, bar_size);
        return -EINVAL;
    }

    set_vm_flags(vma, VM_IO);
    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
    ret = remap_pfn_range(vma, vma->vm_start, pfn, vma_size, vma->vm_page_prot);
    if (ret != 0) {
        pr_err("[qpool]remap bar failed, ret %d\n", ret);
        return ret;
    }

    return 0;
}

int bifur_187x_release_all_queue(struct bifur_fpriv_data *fpriv, u16 func_id, u16 lcore_id)
{
    int ret;
    u16 q_grp_id;
    struct qgrp_mapping_ht_key key = { 0 };

    key.func_id = func_id;
    key.lcore_id = lcore_id;

    ret = bifur_get_qgrp_id_by_ht_key(key.func_id, key.lcore_id, &q_grp_id);
    if (ret != 0) {
        pr_info("[QUEUE_POOLING] All queue groups released for func_id: %u, lcore_id: %u\n", 
            func_id, lcore_id);
        return 0;
    } else if (q_grp_id > BIFUR_FUNC_MAX_QUEUE_GROUP - 1) {
        pr_err("[QUEUE_POOLING] hashtable entry q_grp_id invalid, func_id: %u, lcore_id: %u.\n",
                key.func_id, key.lcore_id);
        return -EINVAL;
    } else {
        do {
            ret = qgrp_mapping_decrease_queue_cnt(key.func_id, key.lcore_id);
        } while (ret > 0);
        if (ret < 0) {
            pr_err("[QUEUE_POOLING] user queues counter sub failed, lcore_id: %u.\n", key.lcore_id);
            return ret;
        } else if (ret == 0) {
            bifur_187x_release_queue_group(fpriv->cdev_data->bifur_dev->qpool_dev, q_grp_id);
            return ret;
        }
    }

    return ret;
}

struct bifur_qpool_dpdk_ops *bifur_qpool_dpdk_get_187x_ops(void)
{
    static struct bifur_qpool_dpdk_ops qpool_dpdk_187x_ops = {
        .get_user_queue_id = bifur_187x_get_user_queue_id,
        .del_user_queue_id = bifur_187x_del_user_queue_id,
        .cfg_rss_template = NULL,
        .set_rss_indir_table = bifur_187x_set_rss_indir_table,
        .get_rss_indir_table = bifur_187x_get_rss_indir_table,
        .get_queue_start_paddr = NULL,
        .add_tc_flow_pre_handle = bifur_187x_add_tc_flow_pre_handle,
        .add_tc_flow_commit_handle = bifur_187x_add_tc_flow_commit_handle,
        .del_tc_flow_commit_handle = bifur_187x_del_tc_flow_commit_handle,
        .cfg_tcam_block_commit_handle = bifur_187x_cfg_tcam_block_commit_handle,
        .qpool_mmap = bifur_187x_qpool_mmap,
        .release_all_queue = bifur_187x_release_all_queue,
    };
    return &qpool_dpdk_187x_ops;
}
