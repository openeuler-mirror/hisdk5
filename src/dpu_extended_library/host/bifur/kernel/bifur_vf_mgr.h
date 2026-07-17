/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */

#ifndef BIFUR_VF_MGR_H__
#define BIFUR_VF_MGR_H__

#include <linux/module.h>
#include <linux/init.h>
#include <linux/etherdevice.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/mm_types.h>
#include <linux/kdev_t.h>
#include <linux/netdevice.h>
#include <linux/notifier.h>
#include <net/net_namespace.h>
#include "asm-generic/int-ll64.h"
#include "linux/pci.h"
#include "securec.h"

#ifndef BIFUR_DT_TEST
#include "ossl_knl_linux.h"
#include "hinic5_nic_dev.h"
#else
#include <linux/version.h>
#include <linux/fcntl.h>
#include <linux/u64_stats_sync.h>
#include <linux/bitops.h>
#include <linux/class.h>
#include <hinic5/hinic5_nic_def.h>
#endif

#include "ossl_knl.h"
#include "hinic5_nic_cfg.h"
#include "hinic5_srv_nic.h"

#define BIFURNAMSIZ 20
#define BIFUR_PROC_FILE_MOD 0640
#define BIFUR_CDEV_PROC_NAME "bifur_vdev"
    
typedef struct {
    struct mutex lock;
    struct list_head list;
} bifur_dev_file_mgt_t;
struct bifur_lld_dev;
struct bifur_vf_mgr;

struct bifur_cdev {
    struct cdev cdev;
    dev_t cdev_id;
    struct proc_dir_entry *proc_dir;
};

struct bifur_vf_info {
    char name[BIFURNAMSIZ];
    u32 vf_dbdf;
    u32 pf_dbdf;
    u16 glb_func_id;
    bool in_use;
    struct bifur_cdev cdev;
    bifur_dev_file_mgt_t dev_file_mgt;
    struct bifur_vf_mgr *vf_mgr;
    atomic_t refcount;
};

struct bifur_vf_mgr {
    u32 vf_sum;
    u32 vf_in_use;
    struct bifur_vf_info *vf_info;
    struct mutex vf_mgr_mutex;
    struct proc_dir_entry *bifur_proc_root;
};

typedef struct bifur_dev_file {
    struct list_head node;
    struct bifur_vf_info *dev;

    atomic_t refcount;
} bifur_dev_file_t;

int bifur_alloc_vf_mgr(struct bifur_lld_dev *bifur_dev);

void bifur_free_vf_mgr(struct bifur_lld_dev *bifur_dev);

struct bifur_vf_info *bifur_find_vf(struct bifur_vf_mgr *vf_mgr, u32 dbdf);

int bifur_find_pf_by_vf(struct bifur_vf_mgr *vf_mgr, u32 vf_dbdf, u32 *pf_dbdf);

int bifur_vf_cdev_init(struct bifur_vf_info *dev);
void bifur_vf_cdev_uninit(struct bifur_vf_info *dev);

#ifdef BIFUR_DT_TEST
int bifur_proc_open(struct inode *inode, struct file *filp);
int bifur_proc_close(struct inode *inode, struct file *filp);
#endif // BIFUR_DT_TEST
#endif