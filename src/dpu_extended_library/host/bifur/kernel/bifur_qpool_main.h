/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026-2026. All rights reserved.
 * Description: queue pool related structure and macro defined here
 * Create: 2026/02/04
 */

#ifndef BIFUR_QPOOL_MAIN_H
#define BIFUR_QPOOL_MAIN_H

#include <linux/types.h>

#include "nic_mpu_cmd_structs.h"
#include "hinic5_nic_dev.h"
#include "bifur_cdev_cmd.h"
#include "bifur_cdev.h"
#include "nic_drv_ext_cmd_defs.h"
#include "bifur_common.h"

#define QID_MAPPING_HT_SIZE 17
#define QGRP_MAPPING_HT_SIZE 8

#define QID_MAPPING_HASH_KEY(func_id, lcore_id, qid) ((unsigned int)((func_id) ^ (lcore_id) ^ (qid)))
#define QGRP_MAPPING_HASH_KEY(func_id, lcore_id) ((unsigned int)((func_id) ^ (lcore_id)))

#define HINIC5_VF_PCI_CFG_REG_BAR	0
#define HINIC5_PF_PCI_CFG_REG_BAR	1
#define HINIC5_PCI_INTR_REG_BAR		2
#define HINIC5_PCI_MGMT_REG_BAR		3 /* Only PF have mgmt bar */
#define HINIC5_PCI_DB_BAR		4

#define QPOOL_BAR_MMAP_MASK 0xff
struct bifur_user_txrxq_params;

struct qgrp_mapping_ht_key {
    u16 lcore_id;
    u16 func_id;
};

struct qgrp_mapping_hn {
    u16 func_id;
    u16 lcore_id;
    u16 q_grp_id;
    u16 queue_n;
    struct hlist_node node;
};

// hinicadm5 工具接口
int bifur_set_queue_group_pooling(struct bifur_lld_dev *bifur_dev, struct bifur_cmd_queue_pooling *pooling, struct bifur_cmd_queue_pooling *pooling_out);
int bifur_get_queue_group_pooling(struct bifur_lld_dev *bifur_dev, struct bifur_cmd_queue_pooling *pooling);

// 1872/1825挂载初始化
int bifur_qpool_adapt_init(struct bifur_lld_dev *bifur_dev);
void bifur_qpool_adapt_deinit(struct bifur_lld_dev *bifur_dev);

int bifur_release_all_queue(struct bifur_fpriv_data *fpriv, u16 func_id, u16 lcore_id);

// 1872/1825挂载函数接口
struct bifur_qpool_dpdk_ops {
    int (*get_user_queue_id)(struct bifur_fpriv_data *fpriv, struct mag_cmd_user_queue_get *queue_get);
    int (*del_user_queue_id)(struct bifur_fpriv_data *fpriv, struct mag_cmd_user_queue_get *queue_get);
    int (*cfg_rss_template)(struct bifur_fpriv_data *fpriv, struct mag_cmd_cfg_rss_temp *rss_temp);
    int (*set_rss_indir_table)(struct bifur_fpriv_data *fpriv, struct mag_cmd_rss_indir_tbl *indir_tbl);
    int (*get_rss_indir_table)(struct bifur_fpriv_data *fpriv, struct mag_cmd_rss_indir_tbl *indir_tbl);
    int (*get_queue_start_paddr)(struct bifur_fpriv_data *fpriv, struct drv_cmd_paddr_get *cmd_pa_get, void *buf_out);
    int (*add_tc_flow_pre_handle)(struct bifur_fpriv_data *fpriv, struct drv_mpu_cmd_st *cmd, uint32_t lcore_id,
                                    struct bifur_fdir_add_rule *tcam_cmd);
    int (*add_tc_flow_commit_handle)(struct bifur_fpriv_data *fpriv, struct drv_mpu_cmd_st *cmd, uint32_t lcore_id,
                                    struct bifur_fdir_add_rule *tcam_cmd);
    int (*del_tc_flow_commit_handle)(struct bifur_fpriv_data *fpriv, struct drv_mpu_cmd_st *cmd, uint32_t lcore_id,
                                    struct bifur_fdir_del_rule *tcam_cmd);
    int (*cfg_tcam_block_commit_handle)(struct bifur_fpriv_data *fpriv, struct drv_mpu_cmd_st *cmd, uint32_t lcore_id,
                                    struct bifur_tcam_block *tcam_cmd);
    int (*qpool_mmap)(struct bifur_fpriv_data *fpriv, struct vm_area_struct *vma);
    int (*release_all_queue)(struct bifur_fpriv_data *fpriv, u16 func_id, u16 lcore_id);
};

int qgrp_mapping_add_ht_entry(const struct qgrp_mapping_ht_key *key, u16 q_grp_id);
int bifur_get_qgrp_id_by_ht_key(u16 func_id, u16 lcore_id, u16 *q_grp_id);
int qgrp_mapping_increase_queue_cnt(u16 func_id, u16 lcore_id, u16 q_grp_id);
int qgrp_mapping_decrease_queue_cnt(u16 func_id, u16 lcore_id);
#endif