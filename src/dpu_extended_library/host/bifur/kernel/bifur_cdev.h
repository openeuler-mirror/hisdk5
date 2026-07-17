/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026-2026. All rights reserved.
 * Description: bifur internal interface definition of cdev
 * Create: 2026/02/05
 */

#ifndef BIFUR_CDEV_H
#define BIFUR_CDEV_H

#ifndef BIFUR_DT_TEST
#include <linux/cdev.h>
#endif
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/netdevice.h>
#include <linux/xarray.h>
#include "hinic5_nic_dev.h"
#include "nic_mpu_cmd.h"
#include "bifur_cdev_cmd.h"

#define BIFUR_CDEV_NAME_LEN 64

#define BIFUR_MAX_CDEV_NUM 1024
#define PARENT_CDEV_PATH_NAME "bifur_cdev"

#define IFCFG_MAX_EVENTS 10

struct bifur_flow_entry {
    u32 index;
    u32 type;
    struct list_head node;
};

struct bifur_block_entry {
    u16 block_index;
    u8  tcam_type;
    struct list_head node;
};

struct bifur_fpriv_data {
    struct bifur_cdev_data *cdev_data;
    struct flow_proc_file_t *proc_file;
    u16 lcore_id;
    u16 func_id;
    struct xarray flow_xa;
    struct xarray block_xa;
};

typedef int (*file_bifur_driv_module)(struct bifur_fpriv_data *fpriv, const void *buf_in, u32 in_size,
    void *buf_out, u32 *out_size);

struct file_bifur_drv_module_handle {
    u32 driv_cmd_name;
    file_bifur_driv_module driv_func;
};


struct netdev_event {
    enum netdev_cmd type;
    u8 dev_addr[MAX_ADDR_LEN];
    u16 new_mtu;
};

struct iface_notify_dev {
    wait_queue_head_t wait_queue;
    struct mutex lock;
    struct netdev_event events[IFCFG_MAX_EVENTS];
    int event_count;
    int event_read_pos;
    u8 events_proc_map[IFCFG_MAX_EVENTS];                       // event应当被哪些proc读取
};
#ifndef BIFUR_DT_TEST
struct bifur_cdev_data {
    struct bifur_lld_dev *bifur_dev;
    struct cdev cdev;
    char name[BIFUR_CDEV_NAME_LEN];
    dev_t cdev_major;
    u32 cdev_idx;
    struct class *cdev_class;
    struct device *dev;
    struct iface_notify_dev notify_dev;
    atomic_t thread_cnt;
    struct completion thread_comp;
    u8 proc_map; // 标识dpdk进程
};

struct bifur_root_cdev {
    struct class *cdev_class;
    dev_t cdev_base;
    DECLARE_BITMAP(dev_map, BIFUR_MAX_CDEV_NUM);
    struct mutex cdev_mutex;
    atomic_t refcnt;
};
#endif

typedef int (*cmd_handle)(struct bifur_fpriv_data *fpriv, struct bifur_ioctl_msg *ctl_msg,
    void *buf_in, u32 in_size, void *buf_out, u32 *out_size);

struct hinic5_cmd_to_mpu_pre_handle {
    u32 cmd_type;
    cmd_handle pre_send;
    cmd_handle commit;
};

struct bifur_lld_dev;
int bifur_cdev_create(struct bifur_lld_dev *bifur_dev);
void bifur_cdev_remove(struct bifur_lld_dev *bifur_dev);

#endif