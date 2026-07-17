/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */

#ifndef BIFUR_MAIN_H__
#define BIFUR_MAIN_H__
#include "asm-generic/int-ll64.h"

#include <linux/mutex.h>
#include <linux/types.h>
#include <linux/timer.h>
#include "securec.h"
#ifndef HINIC5_EVENT_DATA_SIZE
#define HINIC5_EVENT_DATA_SIZE 104
#endif
#define PF_MAX_QPS_NUM 256    // PF最大的队列数
#define HINIC5_SRV_EVENT_TYPE(svc, type)    ((((u32)(svc)) << 16) | (type))

enum bifur_func_type {
    BIFUR_EXCLUSIVE_PF = 1,
    BIFUR_SHARED_PF,
    BIFUR_RESOURCE_PF,
    BIFUR_RESOURCE_VF,
    BIFUR_FUNC_TYPE_MAX
};

struct bifur_flow_dev {
    atomic_t bifur_dev_ref;
    bool has_created; /* bifur dev created or not */
};

struct bifur_adapter {
    struct list_head lld_dev_head; /* pcie device list head */
    struct mutex bifur_dev_mutex;   /* lock for bifur dev list */
    struct workqueue_struct *event_workq; /* global work queue */
    u8 bifur_enabled;               /* used for mark whether to enable traffic bifurcation */
    u8 bond_bifur_enabled;          /* used for mark whether to enable bond status of bifurcation vfs */
    u16 bond_id;
    u8 bond_link_status;
};

struct bifur_vf_mgr;
struct bifur_cdev_data;
struct flow_proc_file_list_t;
struct bifur_qpool_dpdk_ops;

struct bifur_user_txrxq_params {
    DECLARE_BITMAP(qid_map_bitmap, PF_MAX_QPS_NUM);
    u16 qp_start;
    u16 num_qps;
};

struct bifur_qpool_group {
    u8 qg_num;
    u8 qg_bitmap;
    u8 qg_valid_cnt;
};

struct bifur_qpool_dev {
    struct bifur_cdev_data *cdev_data;
    struct flow_proc_file_list_t *flow_proc_file_list;
    struct bifur_qpool_dpdk_ops *qpool_dpdk_ops;
    struct bifur_user_txrxq_params u_params;
    struct bifur_qpool_group qpool_group;
};
struct bifur_lld_dev {
    struct list_head list;
    struct hinic5_lld_dev *lld_dev;
    struct bifur_vf_mgr *vf_mgr;
    struct bifur_flow_dev bifur_dev; /* bifur device */
    enum bifur_func_type pf_type;
    struct work_struct netdev_link;
    u8 link_status;
    u32 dbdf;
    struct bifur_qpool_dev *qpool_dev;
};

struct bifur_event_info {
    u16 service;
    u16 type;
    u8 event_data[HINIC5_EVENT_DATA_SIZE];
};

void bifur_dev_hold(struct bifur_lld_dev *bifur_dev);
void bifur_dev_put(struct bifur_lld_dev *bifur_dev);
void bifur_dev_list_lock(void);
void bifur_dev_list_unlock(void);
struct bifur_lld_dev *bifur_get_resource_dev(void);
struct bifur_adapter *bifur_get_adp(void);
struct bifur_lld_dev *bifur_get_shared_dev_by_dbdf(u32 dbdf);
unsigned int bifur_get_dbdf_by_lld(struct hinic5_lld_dev *lld_dev);
struct hinic5_nic_dev *bifur_get_hinic5_nic_by_bifur_dev(struct bifur_lld_dev *bifur_dev);
#endif