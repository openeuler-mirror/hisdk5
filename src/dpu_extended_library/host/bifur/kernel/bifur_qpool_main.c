/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026-2026. All rights reserved.
 * Description: queue pool main function
 * Create: 2026/02/04
 */

#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/cdev.h>
#include <linux/netdevice.h>
#include "linux/hashtable.h"
#include <linux/types.h>
#include "hinic5_hw.h"
#include "hinic5_nic.h"
#include "hinic5_nic_cfg.h"
#include "hinic5_nic_dev.h"
#include "drv_nic_api.h"
#include "comm_defs.h"
#include "nic_cfg_comm.h"
#include "nic_drv_ext_cmd_defs.h"
#include "nic_npu_ext_cmd_defs.h"
#include "187x_qpool_ops.h"
#include "bifur_main.h"

#include "bifur_qpool_main.h"

static inline bool qg_bitmap_is_zero(u8 qg_bitmap)
{
    return qg_bitmap == 0;
}

static bool qps_allocation_valid(struct hinic5_nic_dev *nic_dev, struct bifur_cmd_queue_pooling *pooling)
{
    u32 kernel_qps_size = nic_dev->max_qps - pooling->number;
    if (pooling->number >= nic_dev->max_qps) {
        pr_err("Requested pooling number (%u) exceeds device max QPS (%u)", pooling->number, nic_dev->max_qps);
        return false;
    }
    /* kernel_group_size 必须是 max_qps 的因子 */
    if (nic_dev->max_qps % kernel_qps_size != 0) {
        pr_err("Kernel QPS (%u) must be a divisor of device max QPS (%u)", kernel_qps_size, nic_dev->max_qps);
        return false;
    }

    if (nic_dev->q_params.num_qps > kernel_qps_size) {
        pr_err("Used QPS (%u) exceeds kernel allocated QPS (%u)", nic_dev->q_params.num_qps, kernel_qps_size);
        return false;
    }

    pooling->group = (pooling->number / kernel_qps_size) + 1; // 内核的分组也包括在这里
    if (pooling->group > BIFUR_FUNC_MAX_QUEUE_GROUP) {
        pr_err("Calculated group number (%u) is out of range [%d, %d]", pooling->group, BIFUR_FUNC_MIN_QUEUE_GROUP, BIFUR_FUNC_MAX_QUEUE_GROUP);
        return false;
    }

    // 判断分组数是否满足2的幂次要求
    if (pooling->group != roundup_pow_of_two(pooling->group)) {
        pr_err("Invalid group size %u is not a power of two\n", pooling->group);
        return -EINVAL;
    }

    pr_info("Successfully calculated pooling group: %u", pooling->group);
    return true;
}
/**
 * @brief 工具使能group数量的接口
 *
 * @details 范围为1~8。1：关闭流分叉，其他值：开启流分叉。
 * @attention 开启流分叉时，group_num实际生效值会向上取整到2的幂次。
 *
 * @return: 命令执行结果.
 *    @retval 0 成功
 *    @retval 非0 失败
 */
int bifur_set_queue_group_pooling(struct bifur_lld_dev *bifur_dev, struct bifur_cmd_queue_pooling *pooling, struct bifur_cmd_queue_pooling *pooling_out)
{
    struct hinic5_nic_dev *nic_dev = bifur_get_hinic5_nic_by_bifur_dev(bifur_dev);
    int ret;
    // 用户态未释放分组资源时 命令禁用
    if(!qg_bitmap_is_zero(bifur_dev->qpool_dev->qpool_group.qg_bitmap)) {
        pr_err("Queue pooling configuration failed: Grouping resources are currently in use");
        return -EBUSY;
    }

    // UP时使能流分叉 会导致内核maxqp数量不匹配 命令禁用
    if (nic_dev->netdev->flags & IFF_UP) {
        pr_err("Queue pooling configuration failed: Cannot modify queue pooling while device is active");
        return -EBUSY;
    }

    if (!qps_allocation_valid(nic_dev, pooling)) {
        pr_err("Queue pooling configuration failed: Invalid QPS allocation parameters");
        return -EINVAL;
    }

    // group为1代表仅有内核分组 关闭流量分叉
    if (pooling->group == BIFUR_FUNC_MIN_QUEUE_GROUP) {
        bifur_dev->qpool_dev->qpool_group.qg_valid_cnt = 0;
        bifur_dev->qpool_dev->qpool_group.qg_num = 0;
        bifur_dev->qpool_dev->u_params.qp_start = nic_dev->max_qps;
        ret = hinic5_set_flow_bifurcation_group_num(nic_dev->netdev, pooling->group);
        if (ret != 0) {
            pr_err("close queue grouping failed");
            return ret;
        }
        return 0;
    }

    bifur_dev->qpool_dev->qpool_group.qg_valid_cnt = pooling->group - 1; // qg_valid_cnt为group - 1; 1是内核使用的group
    bifur_dev->qpool_dev->qpool_group.qg_num = pooling->group;
    ret = hinic5_set_flow_bifurcation_group_num(nic_dev->netdev, pooling->group);
    if (ret != 0) {
        pr_err("set flow bifurcation group num fail");
        return ret;
    }
    bifur_dev->qpool_dev->u_params.qp_start = nic_dev->max_qps - pooling->number;
    pooling_out->group = pooling->group;
    return 0;
}

int bifur_get_queue_group_pooling(struct bifur_lld_dev *bifur_dev, struct bifur_cmd_queue_pooling *pooling)
{
    pooling->group = bifur_dev->qpool_dev->qpool_group.qg_valid_cnt; // 查询接口，返回实际可用组数
    return 0;
}


int bifur_qpool_adapt_init(struct bifur_lld_dev *bifur_dev)
{
    struct bifur_qpool_dev *qpool_dev = bifur_dev->qpool_dev;
    if (qpool_dev->qpool_dpdk_ops) {
        return 0;
    }
    qpool_dev->qpool_dpdk_ops = hinic5_support_htn(bifur_dev->lld_dev->hwdev)
                                     ? bifur_qpool_dpdk_get_187x_ops()
                                     : NULL;

    if (!qpool_dev->qpool_dpdk_ops) {
        pr_err("Failed to initialize qpool DPDK operations, HTN support: %s\n",
                hinic5_support_htn(bifur_dev->lld_dev->hwdev) ? "enabled" : "disabled");
        return -EINVAL;
    }
    return 0;
}

void bifur_qpool_adapt_deinit(struct bifur_lld_dev *bifur_dev)
{
    struct bifur_qpool_dev *qpool_dev = bifur_dev->qpool_dev;
    if (!qpool_dev->qpool_dpdk_ops) {
        return;
    }
    qpool_dev->qpool_dpdk_ops = NULL;
}

int bifur_release_all_queue(struct bifur_fpriv_data *fpriv, u16 func_id, u16 lcore_id)
{
    struct bifur_qpool_dev *qpool_dev = fpriv->cdev_data->bifur_dev->qpool_dev;
    if (qpool_dev->qpool_dpdk_ops->release_all_queue == NULL) {
        return -EINVAL;
    }
    return qpool_dev->qpool_dpdk_ops->release_all_queue(fpriv, func_id, lcore_id);
}

/* qgrp映射表：lcore_id -> q_grp_id, -- key: func_id + lcore_id */
DEFINE_HASHTABLE(qgrp_mapping_ht, QGRP_MAPPING_HT_SIZE);

int qgrp_mapping_add_ht_entry(const struct qgrp_mapping_ht_key *key, u16 q_grp_id)
{
    struct qgrp_mapping_hn *hash_n = NULL;
    struct hlist_node *tmp = NULL;

    hash_for_each_possible_safe(qgrp_mapping_ht, hash_n, tmp, node, QGRP_MAPPING_HASH_KEY(key->func_id, key->lcore_id)) {
        if ((hash_n->func_id == key->func_id) && (hash_n->lcore_id == key->lcore_id)) {
            hash_n->q_grp_id = q_grp_id;
            return 0;
        }
    }

    hash_n = (struct qgrp_mapping_hn *)kzalloc(sizeof(struct qgrp_mapping_hn), GFP_KERNEL);
    if (hash_n == NULL) {
        return -ENOMEM;
    }

    hash_n->func_id = key->func_id;
    hash_n->lcore_id = key->lcore_id;
    hash_n->q_grp_id = q_grp_id;
    hash_n->queue_n = 1;

    hash_add(qgrp_mapping_ht, &hash_n->node, QGRP_MAPPING_HASH_KEY(key->func_id, key->lcore_id));

    return 0;
}

static int qgrp_mapping_get_ht_entry(const struct qgrp_mapping_ht_key *key, u16 *q_grp_id)
{
    struct qgrp_mapping_hn *hash_n = NULL;
    struct hlist_node *tmp = NULL;

    hash_for_each_possible_safe(qgrp_mapping_ht, hash_n, tmp, node, QGRP_MAPPING_HASH_KEY(key->func_id, key->lcore_id)) {
        if ((hash_n->func_id == key->func_id) && (hash_n->lcore_id == key->lcore_id)) {
            if (q_grp_id) {
                *q_grp_id = hash_n->q_grp_id;
            }
            return 0;
        }
    }

    return -ENOENT;
}

int qgrp_mapping_increase_queue_cnt(u16 func_id, u16 lcore_id, u16 q_grp_id)
{
    struct qgrp_mapping_hn *hash_n = NULL;
    struct hlist_node *tmp = NULL;

    hash_for_each_possible_safe(qgrp_mapping_ht, hash_n, tmp, node, QGRP_MAPPING_HASH_KEY(func_id, lcore_id)) {
        if ((hash_n->func_id == func_id) && (hash_n->lcore_id == lcore_id)) {
            hash_n->queue_n++;
            return 0;
        }
    }

    hash_n = (struct qgrp_mapping_hn *)kzalloc(sizeof(struct qgrp_mapping_hn), GFP_KERNEL);
    if (hash_n == NULL) {
        return -ENOMEM;
    }

    hash_n->func_id = func_id;
    hash_n->lcore_id = lcore_id;
    hash_n->q_grp_id = q_grp_id;
    hash_n->queue_n = 1;

    hash_add(qgrp_mapping_ht, &hash_n->node, QGRP_MAPPING_HASH_KEY(func_id, lcore_id));

    return 0;
}

int qgrp_mapping_decrease_queue_cnt(u16 func_id, u16 lcore_id)
{
    struct qgrp_mapping_hn *hash_n = NULL;
    struct hlist_node *tmp = NULL;
    int ret;

    hash_for_each_possible_safe(qgrp_mapping_ht, hash_n, tmp, node, QGRP_MAPPING_HASH_KEY(func_id, lcore_id)) {
        if ((hash_n->func_id == func_id) && (hash_n->lcore_id == lcore_id)) {
            if (hash_n->queue_n > 0) {
                hash_n->queue_n--;
            }
            ret = hash_n->queue_n;
            if (hash_n->queue_n == 0) {
                hash_del(&hash_n->node);
                kfree(hash_n);
            }
            return ret;
        }
    }

    return -ENOENT;
}

int bifur_get_qgrp_id_by_ht_key(u16 func_id, u16 lcore_id, u16 *q_grp_id)
{
    struct qgrp_mapping_ht_key key = { 0 };
    int ret;

    key.func_id = func_id;
    key.lcore_id = lcore_id;

    ret = qgrp_mapping_get_ht_entry(&key, q_grp_id);
    if (ret != 0 || *q_grp_id < Q_GRP_FUNC_INDX_OFFSET) {
        pr_info("[QUEUE_POOLING] hashtable entry q_grp_id does not exit, func_id: %u, lcore_id: %u.\n",
                func_id, lcore_id);
        return ret;
    }

    return 0;
}