/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026-2026. All rights reserved.
 * Description: bifur ioctl function
 * Create: 2026/02/05
 */
#include <linux/cdev.h>
#include "securec.h"
#include "comm_defs.h"
#include "ossl_knl.h"
#include "drv_nic_api.h"
#include "hinic5_hw.h"
#include "hinic5_mt.h"
#include "mpu_inband_cmd.h"
#include "nic_drv_ext_cmd_defs.h"
#include "bifur_main.h"
#include "hinic5_srv_nic.h"
#include "hinic5_nic_cfg.h"
#include "bifur_common.h"
#include "bifur_qpool_main.h"

#include "bifur_cdev.h"

static struct bifur_root_cdev g_root_cdev;

static inline u16 flow_global_file_del(struct flow_proc_file_list_t *flow_proc_file_list, struct flow_proc_file_t *proc_file)
{
    u16 cnt = 0;
    mutex_lock(&flow_proc_file_list->lock);
    cnt = --proc_file->cnt;
    if (proc_file->cnt == 0) {
        list_del(&proc_file->node);
        if (proc_file->lcore_id < BIFUR_FUNC_MAX_QUEUE_GROUP)
            flow_proc_file_list->lcore_bitmap &=~ BIT(proc_file->lcore_id);
        kfree(proc_file);
    }
    mutex_unlock(&flow_proc_file_list->lock);
    return cnt;
}

static int alloc_proc_bitmap_slot(struct bifur_cdev_data *cdev_data)
{
    // 加锁，防止event快照的时候proc_map被修改
    struct iface_notify_dev *iface_notify = &cdev_data->notify_dev;
    int i;
    mutex_lock(&iface_notify->lock);
    for (i = 0; i < BIFUR_FUNC_MAX_QUEUE_GROUP - 1; i++) {
        if (!(cdev_data->proc_map & (1U << i))) {
            cdev_data->proc_map |= (1U << i);
            mutex_unlock(&iface_notify->lock);
            return i;
        }
    }
    mutex_unlock(&iface_notify->lock);
    return -ENOSPC;
}

static inline int flow_global_file_add(struct bifur_cdev_data *cdev_data,
                                        struct flow_proc_file_list_t *flow_proc_file_list, struct flow_proc_file_t *proc_file)
{
    u8 id;

    mutex_lock(&flow_proc_file_list->lock);
    for (id = 0; id < BIFUR_FUNC_MAX_QUEUE_GROUP; id++) {
        if (!(flow_proc_file_list->lcore_bitmap & BIT(id)))
            break;
    }
    if (id == BIFUR_FUNC_MAX_QUEUE_GROUP) {
        mutex_unlock(&flow_proc_file_list->lock);
        pr_err("No available queue group (max %d)\n", BIFUR_FUNC_MAX_QUEUE_GROUP);
        return -ENOSPC;
    }
    flow_proc_file_list->lcore_bitmap |= BIT(id);

    ++proc_file->cnt;
    proc_file->lcore_id = id;
    list_add_tail(&proc_file->node, &flow_proc_file_list->list);
    atomic_inc(&cdev_data->thread_cnt);
    init_completion(&cdev_data->thread_comp);
    mutex_unlock(&flow_proc_file_list->lock);

    return 0;
}

struct flow_proc_file_t *flow_alloc_proc_file(struct bifur_cdev_data *cdev_data)
{
    struct flow_proc_file_t *proc_file = kzalloc(sizeof(struct flow_proc_file_t), GFP_KERNEL);
    if (proc_file == NULL) {
        pr_err("Alloc proc file failed.");
        return NULL;
    }

    proc_file->pid = current->pid;
    proc_file->slot = alloc_proc_bitmap_slot(cdev_data);
    if (proc_file->slot < 0) {
        pr_err("No available proc map, max %d\n", BIFUR_FUNC_MAX_QUEUE_GROUP - 1);
        return NULL;
    }
    return proc_file;
}

static inline void flow_global_file_uninit(struct bifur_cdev_data *cdata,
                                            struct flow_proc_file_list_t *flow_proc_file_list)
{
    struct flow_proc_file_t *tmp = NULL;
    struct flow_proc_file_t *proc_file = NULL;
    if (atomic_read(&cdata->thread_cnt) > 0) {
        pr_info("wait all dpdk threads to be closed.\n");
        wait_for_completion(&cdata->thread_comp);
    }

    mutex_lock(&flow_proc_file_list->lock);
    list_for_each_entry_safe(proc_file, tmp, &flow_proc_file_list->list, node) {
        list_del(&proc_file->node);
        kfree(proc_file);
    }
    mutex_unlock(&flow_proc_file_list->lock);
}

static inline void flow_global_file_init(struct flow_proc_file_list_t *flow_proc_file_list)
{
    mutex_init(&flow_proc_file_list->lock);
    INIT_LIST_HEAD(&flow_proc_file_list->list);
}

static int bifur_cdev_open(struct inode *inode, struct file *file)
{
    struct bifur_cdev_data *cdev_data = NULL;
    struct bifur_fpriv_data *fpriv = NULL;
    struct flow_proc_file_t *proc_file = NULL;

    cdev_data = container_of(inode->i_cdev, struct bifur_cdev_data, cdev);
    fpriv = (struct bifur_fpriv_data *)kzalloc(sizeof(struct bifur_fpriv_data), GFP_KERNEL);
    if (fpriv == NULL) {
        pr_err("failed to allocate file private data\n");
        return -ENOMEM;
    }

    xa_init(&fpriv->flow_xa);
    xa_init(&fpriv->block_xa);
    fpriv->cdev_data = cdev_data;

    proc_file = flow_alloc_proc_file(cdev_data);
    if (proc_file == NULL) {
        kfree(fpriv);
        return -ENOMEM;
    }

    flow_global_file_add(cdev_data, cdev_data->bifur_dev->qpool_dev->flow_proc_file_list, proc_file);
    fpriv->proc_file = proc_file;
    fpriv->lcore_id = proc_file->lcore_id;
    fpriv->func_id = hinic5_global_func_id(cdev_data->bifur_dev->lld_dev->hwdev);
    pr_info("cdev open, func id:%u, lcore id:%u, pid:%u, proc:0x%02x \n",
            fpriv->func_id, fpriv->lcore_id, fpriv->proc_file->pid, cdev_data->proc_map);
    file->private_data = fpriv;

    return 0;
}

static int bifur_cleanup_flows(struct bifur_fpriv_data *fpriv)
{
    LIST_HEAD(flow_list);
    struct bifur_flow_entry *entry = NULL, *tmp = NULL;
    unsigned long index;

    struct bifur_fdir_del_rule tcam_cmd;
    u16 out_size = sizeof(tcam_cmd);
    int err;

    xa_lock(&fpriv->flow_xa);
    xa_for_each(&fpriv->flow_xa, index, entry) {
        list_add_tail(&entry->node, &flow_list);
    }
    xa_unlock(&fpriv->flow_xa);

    list_for_each_entry_safe(entry, tmp, &flow_list, node) {
        (void)memset_s(&tcam_cmd, sizeof(tcam_cmd), 0, sizeof(tcam_cmd));
        out_size = sizeof(tcam_cmd);

        tcam_cmd.func_id = fpriv->func_id;
        tcam_cmd.type = entry->type;
        tcam_cmd.index_start = entry->index;
        tcam_cmd.index_num = 1;

        err = hinic5_msg_to_mgmt_sync(fpriv->cdev_data->bifur_dev->lld_dev->hwdev, HINIC5_MOD_L2NIC, HINIC5_NIC_CMD_DEL_TC_FLOW, 
                        &tcam_cmd, sizeof(tcam_cmd),
                        &tcam_cmd, &out_size, 0, HINIC5_CHANNEL_NIC);
        if (err || tcam_cmd.msg_head.status || !out_size) {
            pr_err("Cleanup flow entry failed, err: %d, status: 0x%x, out size: 0x%x, index: %u, type: %u\n",
                    err, tcam_cmd.msg_head.status, out_size, entry->index, entry->type);
            return -EIO;
        }
        xa_erase(&fpriv->flow_xa, entry->index);
        list_del(&entry->node);
        pr_info("Cleanup flow entry: index = %u, type = %u\n", entry->index, entry->type);
        kfree(entry);
        entry = NULL;
    }
    return 0;
}

static int bifur_cleanup_blocks(struct bifur_fpriv_data *fpriv)
{
    LIST_HEAD(block_list);
    struct bifur_block_entry *entry = NULL, *tmp = NULL;
    unsigned long index;
    struct bifur_tcam_block tcam_cmd;
    u16 out_size = sizeof(tcam_cmd);
    int err;

    xa_lock(&fpriv->block_xa);
    xa_for_each(&fpriv->block_xa, index, entry) {
        list_add_tail(&entry->node, &block_list);
    }
    xa_unlock(&fpriv->block_xa);

    list_for_each_entry_safe(entry, tmp, &block_list, node) {
        (void)memset_s(&tcam_cmd, sizeof(tcam_cmd), 0, sizeof(tcam_cmd));
        out_size = sizeof(tcam_cmd);

        tcam_cmd.func_id = fpriv->func_id;
        tcam_cmd.alloc_en = 0;
        tcam_cmd.tcam_type = entry->tcam_type;
        tcam_cmd.tcam_block_index = entry->block_index;

        err = hinic5_msg_to_mgmt_sync(fpriv->cdev_data->bifur_dev->lld_dev->hwdev, HINIC5_MOD_L2NIC, HINIC5_NIC_CMD_CFG_TCAM_BLOCK, 
                        &tcam_cmd, sizeof(tcam_cmd),
                        &tcam_cmd, &out_size, 0, HINIC5_CHANNEL_NIC);
        if (err || tcam_cmd.msg_head.status || !out_size) {
            pr_err("Cleanup tcam block failed, err: %d, status: 0x%x, out size: 0x%x, block index: %u, tcam type: %u\n",
                    err, tcam_cmd.msg_head.status, out_size, entry->block_index, entry->tcam_type);
            return -EIO;
        }

        xa_erase(&fpriv->block_xa, entry->block_index);
        list_del(&entry->node);
        pr_info("Cleanup block index = %u, type = %u\n", entry->block_index, entry->tcam_type);
        kfree(entry);
        entry = NULL;
    }

    return 0;
}

void bifur_cleanup_event(struct bifur_fpriv_data *fpriv)
{
    struct bifur_cdev_data *cdev_data = fpriv->cdev_data;
    struct iface_notify_dev *iface_dev = &cdev_data->notify_dev;
    int event_read_pos = 0;
    int count = 0;
    int pos = 0;
    int i = 0;
    u8 slot_mask = (1U << fpriv->proc_file->slot);
    mutex_lock(&iface_dev->lock);
    event_read_pos = iface_dev->event_read_pos;
    count = iface_dev->event_count;
    /*
     * 只清理当前进程对应bit
     */
    for (i = 0; i < count; i++) {
        pos = (event_read_pos + i) % IFCFG_MAX_EVENTS;
        if (iface_dev->events_proc_map[pos] & slot_mask) {
            iface_dev->events_proc_map[pos] &= ~slot_mask;
            pr_info("[Event cleanup done, pos=%d after=0x%x\n", pos, iface_dev->events_proc_map[pos]);
        }
    }
    mutex_unlock(&iface_dev->lock);
}

void bifur_cleanup_proc_map(struct bifur_fpriv_data *fpriv)
{
    struct bifur_cdev_data *cdev_data = fpriv->cdev_data;
    struct iface_notify_dev *iface_notify = &cdev_data->notify_dev;
    mutex_lock(&iface_notify->lock);
    cdev_data->proc_map &= ~(1U << fpriv->proc_file->slot);
    mutex_unlock(&iface_notify->lock);
}

static int bifur_cdev_close(struct inode *inode, struct file *file)
{
    struct bifur_fpriv_data *fpriv = file->private_data;
    uint16_t func_id;
    uint16_t lcore_id;
    pid_t pid;
    struct bifur_qpool_dev *qpool_dev = NULL;
    u16 cnt = 0;
    int ret;

    if (fpriv == NULL) {
        pr_err_ratelimited("fpriv is NULL\n");
        return 0;
    }
    func_id = fpriv->func_id;
    lcore_id = fpriv->lcore_id;
    pid = fpriv->proc_file->pid;

    ret = bifur_release_all_queue(fpriv, func_id, lcore_id);
    if (ret != 0) {
        pr_err("Failed to release all queue, ret: %d\n", ret);
        return ret;
    }
    /* 清理 flow */
    bifur_cleanup_flows(fpriv);
    /* 清理 block */
    bifur_cleanup_blocks(fpriv);
    xa_destroy(&fpriv->flow_xa);
    xa_destroy(&fpriv->block_xa);
    /* 清理未处理的event*/
    bifur_cleanup_event(fpriv);
    /* 清理bitmap*/
    bifur_cleanup_proc_map(fpriv);
    if (atomic_dec_and_test(&fpriv->cdev_data->thread_cnt)) {
        complete(&fpriv->cdev_data->thread_comp);
    }
    qpool_dev = fpriv->cdev_data->bifur_dev->qpool_dev;
    cnt = flow_global_file_del(qpool_dev->flow_proc_file_list, fpriv->proc_file);
    if (cnt != 0) { // cnt != 0 表示还有其他句柄打开
        return 0;
    }
    fpriv->proc_file = NULL;

    pr_info("cdev close, func id: %u, lcore id: %u, pid: %u\n", func_id, lcore_id, pid);

    if (fpriv) {
        kfree(fpriv);
    }
    return 0;
}

static ssize_t bifur_cdev_read(struct file *file, char __user *ubuf, size_t size, loff_t *ppos)
{
    struct bifur_fpriv_data *fpriv = file->private_data;
    struct bifur_cdev_data *cdev_data = fpriv->cdev_data;
    struct iface_notify_dev *iface_dev = &cdev_data->notify_dev;
    int event_read_pos = 0;
    struct netdev_event local_event = {0};

    mutex_lock(&iface_dev->lock);
    event_read_pos = iface_dev->event_read_pos;

    if (iface_dev->event_count == 0) {
        mutex_unlock(&iface_dev->lock);
        return 0;
    }

    if (!(iface_dev->events_proc_map[event_read_pos] & (1U << fpriv->proc_file->slot))) { // 不需要当前进程处理
        if (iface_dev->events_proc_map[event_read_pos] == 0) { // 如果当前event已经无人消费, 则丢弃当前事件
            pr_warn("Event reclaim pos=%d\n", event_read_pos);
            iface_dev->event_read_pos = (event_read_pos + 1) % IFCFG_MAX_EVENTS;
            iface_dev->event_count--;
        }
        mutex_unlock(&iface_dev->lock);
        return -EAGAIN;
    }

    if (memcpy_s(&local_event, sizeof(local_event), &iface_dev->events[event_read_pos], sizeof(iface_dev->events[event_read_pos])) != 0) {
        mutex_unlock(&iface_dev->lock);
        return -ENOMEM;
    }

    iface_dev->events_proc_map[event_read_pos] &= ~(1U << fpriv->proc_file->slot);

    if (iface_dev->events_proc_map[event_read_pos] == 0) {
        iface_dev->event_read_pos = (event_read_pos + 1) % IFCFG_MAX_EVENTS;
        iface_dev->event_count--;
    }

    mutex_unlock(&iface_dev->lock);
    if (copy_to_user(ubuf, &local_event, sizeof(local_event))) {
        return -EFAULT;
    }
    return sizeof(local_event);
}

static unsigned int bifur_cdev_poll(struct file *filp, poll_table *wait)
{
    struct bifur_fpriv_data *fpriv = filp->private_data;
    struct bifur_cdev_data *cdev_data = fpriv->cdev_data;
    struct iface_notify_dev *iface_dev = &cdev_data->notify_dev;
    unsigned int mask = 0;

    if (!iface_dev) {
        pr_err("notify_dev is NULL\n");
        return 0;
    }

    poll_wait(filp, &iface_dev->wait_queue, wait);
    if (iface_dev->event_count > 0) {
        mask |= POLLIN | POLLRDNORM;
    }

    return mask;
}

static void write_event(struct bifur_lld_dev *bifur_dev, struct iface_notify_dev *iface_notify, struct netdev_event *event)
{
    int write_pos = 0;
    write_pos = (iface_notify->event_read_pos + iface_notify->event_count) % IFCFG_MAX_EVENTS;
    if (iface_notify->event_count < IFCFG_MAX_EVENTS) {
        iface_notify->events[write_pos] = *event;
        iface_notify->events_proc_map[write_pos] = bifur_dev->qpool_dev->cdev_data->proc_map;
        iface_notify->event_count++;
    } else {
        pr_err_ratelimited("event queue full\n");
    }
}

static int netdevice_event_handler(struct notifier_block *nb,
                                  unsigned long event, void *ptr)
{
    struct net_device *ndev = netdev_notifier_info_to_dev(ptr);
    u32 dbdf = 0;
    struct bifur_lld_dev *bifur_dev = NULL;
    struct hinic5_nic_dev *nic_dev = NULL;
    struct iface_notify_dev *iface_notify = NULL;
    struct netdev_event new_event;

    /* only self-developed NICs can be processed */
    if (!hinic5_get_lld_dev_by_netdev(ndev)) {
        return NOTIFY_DONE;
    }

    nic_dev = netdev_priv(ndev);
    dbdf = bifur_get_dbdf_by_lld(nic_dev->lld_dev);
    bifur_dev = bifur_get_shared_dev_by_dbdf(dbdf);
    if (!bifur_dev) {
        pr_err("%s, bifur dev cannot be found\n", ndev->name);
        return -EINVAL;
    }
    dev_hold(ndev);

    if (bifur_dev->qpool_dev == NULL || bifur_dev->qpool_dev->cdev_data == NULL) {
        pr_err("%s, cdev_data or nic_extend cannot be found\n", ndev->name);
        dev_put(ndev);
        bifur_dev_put(bifur_dev);
        return NOTIFY_DONE;
    }
    iface_notify = &bifur_dev->qpool_dev->cdev_data->notify_dev;

    new_event.type = event;
    switch (event) {
        case NETDEV_CHANGEADDR:
            ether_addr_copy(new_event.dev_addr, ndev->dev_addr);
            break;
        case NETDEV_CHANGEMTU:
            new_event.new_mtu = ndev->mtu;
            break;
        default:
            break;
    }

    mutex_lock(&iface_notify->lock);
    if (bifur_dev->qpool_dev->cdev_data->proc_map == 0) {
        goto out;
    }
    write_event(bifur_dev, iface_notify, &new_event);
    wake_up_interruptible(&iface_notify->wait_queue);
out:
    mutex_unlock(&iface_notify->lock);
    dev_put(ndev);
    bifur_dev_put(bifur_dev);

    return NOTIFY_DONE;
}

static struct notifier_block netdevice_nb = {
    .notifier_call = netdevice_event_handler,
};

static int add_tc_flow_pre_handle(struct bifur_fpriv_data *fpriv, struct bifur_ioctl_msg *ctl_msg,
    void *buf_in, u32 in_size, void *buf_out, u32 *out_size)
{
    struct bifur_fdir_add_rule *tcam_cmd = (struct bifur_fdir_add_rule *)buf_in;
    struct bifur_qpool_dev *qpool_dev = fpriv->cdev_data->bifur_dev->qpool_dev;
    if (qpool_dev->qpool_dpdk_ops->add_tc_flow_pre_handle == NULL) {
        pr_err("Pre-handle operation is not supported, func_id: %u, lcore_id: %u, cmd: %u\n",
                fpriv->func_id, ctl_msg->lcore_id, ctl_msg->mpu_cmd.cmd);
        return -EINVAL;
    }
    return qpool_dev->qpool_dpdk_ops->add_tc_flow_pre_handle(fpriv, &(ctl_msg->mpu_cmd), ctl_msg->lcore_id, tcam_cmd);
}

static int add_tc_flow_commit_handle(struct bifur_fpriv_data *fpriv, struct bifur_ioctl_msg *ctl_msg,
    void *buf_in, u32 in_size, void *buf_out, u32 *out_size)
{
    struct bifur_fdir_add_rule *tcam_cmd = (struct bifur_fdir_add_rule *)buf_out;
    struct bifur_qpool_dev *qpool_dev = fpriv->cdev_data->bifur_dev->qpool_dev;
    if (qpool_dev->qpool_dpdk_ops->add_tc_flow_commit_handle == NULL) {
        pr_err("Commit-handle operation is not supported, func_id: %u, lcore_id: %u, cmd: %u\n",
                fpriv->func_id, ctl_msg->lcore_id, ctl_msg->mpu_cmd.cmd);
        return -EINVAL;
    }
    return qpool_dev->qpool_dpdk_ops->add_tc_flow_commit_handle(fpriv, &(ctl_msg->mpu_cmd), ctl_msg->lcore_id, tcam_cmd);
}

static int del_tc_flow_commit_handle(struct bifur_fpriv_data *fpriv, struct bifur_ioctl_msg *ctl_msg,
    void *buf_in, u32 in_size, void *buf_out, u32 *out_size)
{
    struct bifur_fdir_del_rule *tcam_cmd = (struct bifur_fdir_del_rule *)buf_out;
    struct bifur_qpool_dev *qpool_dev = fpriv->cdev_data->bifur_dev->qpool_dev;
    if (qpool_dev->qpool_dpdk_ops->del_tc_flow_commit_handle == NULL) {
        pr_err("Commit-handle operation is not supported, func_id: %u, lcore_id: %u, cmd: %u\n",
                fpriv->func_id, ctl_msg->lcore_id, ctl_msg->mpu_cmd.cmd);
        return -EINVAL;
    }
    return qpool_dev->qpool_dpdk_ops->del_tc_flow_commit_handle(fpriv, &(ctl_msg->mpu_cmd), ctl_msg->lcore_id, tcam_cmd);
}

static int cfg_tcam_block_commit_handle(struct bifur_fpriv_data *fpriv, struct bifur_ioctl_msg *ctl_msg,
    void *buf_in, u32 in_size, void *buf_out, u32 *out_size)
{
    struct bifur_tcam_block *tcam_cmd = (struct bifur_tcam_block *)buf_out;
    struct bifur_qpool_dev *qpool_dev = fpriv->cdev_data->bifur_dev->qpool_dev;
    if (qpool_dev->qpool_dpdk_ops->cfg_tcam_block_commit_handle == NULL) {
        pr_err("Commit-handle operation is not supported, func_id: %u, lcore_id: %u, cmd: %u\n",
                fpriv->func_id, ctl_msg->lcore_id, ctl_msg->mpu_cmd.cmd);
        return -EINVAL;
    }
    return qpool_dev->qpool_dpdk_ops->cfg_tcam_block_commit_handle(fpriv, &(ctl_msg->mpu_cmd), ctl_msg->lcore_id, tcam_cmd);
}

struct hinic5_cmd_to_mpu_pre_handle g_cmd_pre_handle[] = {
    {
        .cmd_type = HINIC5_NIC_CMD_ADD_TC_FLOW,
        .pre_send = add_tc_flow_pre_handle,
        .commit = add_tc_flow_commit_handle,
    },
    {
        .cmd_type = HINIC5_NIC_CMD_DEL_TC_FLOW,
        .pre_send = NULL,
        .commit = del_tc_flow_commit_handle,
    },
    {
        .cmd_type = HINIC5_NIC_CMD_CFG_TCAM_BLOCK,
        .pre_send = NULL,
        .commit = cfg_tcam_block_commit_handle,
    },
};

static struct hinic5_cmd_to_mpu_pre_handle *bifur_find_cmd_handler(u32 cmd)
{
    int index, num_cmds = (int)(sizeof(g_cmd_pre_handle) / sizeof(g_cmd_pre_handle[0]));
    for (index = 0; index < num_cmds; index++) {
        if (g_cmd_pre_handle[index].cmd_type == cmd)
            return &g_cmd_pre_handle[index];
    }

    return NULL;
}

static u32 get_up_timeout_val(enum hinic5_mod_type mod, u16 cmd)
{
    if (mod == HINIC5_MOD_COMM &&
        (cmd == COMM_MGMT_CMD_UPDATE_FW || cmd == COMM_MGMT_CMD_UPDATE_BIOS || cmd == COMM_MGMT_CMD_ACTIVE_FW ||
        cmd == COMM_MGMT_CMD_SWITCH_CFG || cmd == COMM_MGMT_CMD_HOT_ACTIVE_FW)) {
        return 200000U;
    }

    return 0; /* use default mbox/apichain timeout time */
}

static int bifur_send_to_mpu(struct bifur_fpriv_data *fpriv, struct bifur_ioctl_msg *ctl_msg,
    void *buf_in, u32 in_size, void *buf_out, u32 *out_size)
{
    enum hinic5_mod_type mod;
    u32 timeout;
    int ret = 0;
    u16 cmd;
    void *hwdev = fpriv->cdev_data->bifur_dev->lld_dev->hwdev;
    struct hinic5_cmd_to_mpu_pre_handle *handler = NULL;

    mod = (enum hinic5_mod_type)ctl_msg->mpu_cmd.mod;
    cmd = ctl_msg->mpu_cmd.cmd;

    handler = bifur_find_cmd_handler(cmd);
    if (handler && handler->pre_send) {
        ret = handler->pre_send(fpriv, ctl_msg, buf_in, in_size,
                                buf_out, out_size);
        if (ret != 0)
            return ret;
    }

    if (ctl_msg->mpu_cmd.api_type != API_TYPE_MBOX) {
        pr_err("unsupported api_type %u\n", ctl_msg->mpu_cmd.api_type);
        return -EINVAL;
    }

    timeout = get_up_timeout_val(mod, cmd);
    ret = hinic5_msg_to_mgmt_sync(hwdev, mod, cmd, buf_in, (u16)in_size,
                        buf_out, (u16 *)(u8 *)out_size, timeout,
                        HINIC5_CHANNEL_DEFAULT);
    if (ret != 0) {
        pr_err("message to mgmt cpu failed, mod: %d, cmd: %u, err: %d\n", mod, cmd, ret);
        return ret;
    }

    if (handler && handler->commit) {
        ret = handler->commit(fpriv, ctl_msg, buf_in, in_size,
                                buf_out, out_size);
        if (ret != 0)
            return ret;
    }

    return 0;
}

int get_user_queue_id(struct bifur_fpriv_data *fpriv, const void *buf_in,
            u32 in_size, void *buf_out, u32 *out_size)
{
    struct mag_cmd_user_queue_get *queue_in = (struct mag_cmd_user_queue_get *)buf_in;
    struct mag_cmd_user_queue_get *queue_out = (struct mag_cmd_user_queue_get *)buf_out;
    struct bifur_lld_dev *bifur_dev = fpriv->cdev_data->bifur_dev;

    if (*out_size != sizeof(*queue_out) || in_size != sizeof(*queue_in)) {
        BIFUR_DEV_ERR(bifur_dev->lld_dev, "Unexpect buf size from user, in_size: %u, out_size: %u, expect: %zu\n",
            in_size, *out_size, sizeof(*queue_in));
        return -EINVAL;
    }

    if (memcpy_s(queue_out, sizeof(*queue_out), queue_in, sizeof(*queue_in)) != 0) {
        BIFUR_DEV_ERR(bifur_dev->lld_dev, "Failed to copy user queue data, src_size:%zu, dst_size:%zu\n",
            sizeof(*queue_in), sizeof(*queue_out));
        return -ENOMEM;
    }

    if (bifur_dev->qpool_dev->qpool_dpdk_ops->get_user_queue_id == NULL) {
        return -EINVAL;
    }
    return bifur_dev->qpool_dev->qpool_dpdk_ops->get_user_queue_id(fpriv, queue_out);
}

int del_user_queue_id(struct bifur_fpriv_data *fpriv, const void *buf_in,
            u32 in_size, void *buf_out, u32 *out_size)
{
    struct mag_cmd_user_queue_get *queue_in = (struct mag_cmd_user_queue_get *)buf_in;
    struct mag_cmd_user_queue_get *queue_out = (struct mag_cmd_user_queue_get *)buf_out;
    struct bifur_lld_dev *bifur_dev = fpriv->cdev_data->bifur_dev;

    if (*out_size != sizeof(*queue_out) || in_size != sizeof(*queue_in)) {
        BIFUR_DEV_ERR(bifur_dev->lld_dev, "Unexpect buf size from user, in_size: %u, out_size: %u, expect: %zu\n",
            in_size, *out_size, sizeof(*queue_in));
        return -EINVAL;
    }

    if (memcpy_s(queue_out, sizeof(*queue_out), queue_in, sizeof(*queue_in)) != 0) {
        BIFUR_DEV_ERR(bifur_dev->lld_dev, "Failed to copy user queue data, src_size:%zu, dst_size:%zu\n",
            sizeof(*queue_in), sizeof(*queue_out));
        return -ENOMEM;
    }

    if (bifur_dev->qpool_dev->qpool_dpdk_ops->del_user_queue_id == NULL) {
        return -EINVAL;
    }
    return bifur_dev->qpool_dev->qpool_dpdk_ops->del_user_queue_id(fpriv, queue_out);
}

int cfg_rss_template(struct bifur_fpriv_data *fpriv, const void *buf_in,
            u32 in_size, void *buf_out, u32 *out_size)
{
    struct mag_cmd_cfg_rss_temp *rss_in = (struct mag_cmd_cfg_rss_temp *)buf_in;
    struct mag_cmd_cfg_rss_temp *rss_out = (struct mag_cmd_cfg_rss_temp *)buf_out;
    struct bifur_lld_dev *bifur_dev = fpriv->cdev_data->bifur_dev;

    if (*out_size != sizeof(*rss_out) || in_size != sizeof(*rss_in)) {
        BIFUR_DEV_ERR(bifur_dev->lld_dev, "Unexpect buf size from user, in_size: %u, out_size: %u, expect: %zu\n",
            in_size, *out_size, sizeof(*rss_in));
        return -EINVAL;
    }

    if (memcpy_s(rss_out, sizeof(*rss_out), rss_in, sizeof(*rss_in)) != 0) {
        BIFUR_DEV_ERR(bifur_dev->lld_dev, "Failed to copy user queue data, src_size:%zu, dst_size:%zu\n",
            sizeof(*rss_in), sizeof(*rss_out));
        return -ENOMEM;
    }

    if (bifur_dev->qpool_dev->qpool_dpdk_ops->cfg_rss_template == NULL) {
        return -EINVAL;
    }
    return bifur_dev->qpool_dev->qpool_dpdk_ops->cfg_rss_template(fpriv, rss_out);
}

int set_rss_indir_table(struct bifur_fpriv_data *fpriv, const void *buf_in,
            u32 in_size, void *buf_out, u32 *out_size)
{
    struct mag_cmd_rss_indir_tbl *indir_in = (struct mag_cmd_rss_indir_tbl *)buf_in;
    struct mag_cmd_rss_indir_tbl *indir_out = (struct mag_cmd_rss_indir_tbl *)buf_out;
    struct bifur_lld_dev *bifur_dev = fpriv->cdev_data->bifur_dev;

    if (*out_size != sizeof(*indir_out) || in_size != sizeof(*indir_in)) {
        BIFUR_DEV_ERR(bifur_dev->lld_dev, "Unexpect buf size from user, in_size: %u, out_size: %u, expect: %zu\n",
            in_size, *out_size, sizeof(*indir_in));
        return -EINVAL;
    }

    if (memcpy_s(indir_out, sizeof(*indir_out), indir_in, sizeof(*indir_in)) != 0) {
        BIFUR_DEV_ERR(bifur_dev->lld_dev, "Failed to copy user queue data, src_size:%zu, dst_size:%zu\n",
            sizeof(*indir_in), sizeof(*indir_out));
        return -ENOMEM;
    }

    if (bifur_dev->qpool_dev->qpool_dpdk_ops->set_rss_indir_table == NULL) {
        return -EINVAL;
    }
    return bifur_dev->qpool_dev->qpool_dpdk_ops->set_rss_indir_table(fpriv, indir_out);
}

int get_rss_indir_table(struct bifur_fpriv_data *fpriv, const void *buf_in,
            u32 in_size, void *buf_out, u32 *out_size)
{
    struct mag_cmd_rss_indir_tbl *indir_in = (struct mag_cmd_rss_indir_tbl *)buf_in;
    struct mag_cmd_rss_indir_tbl *indir_out = (struct mag_cmd_rss_indir_tbl *)buf_out;
    struct bifur_lld_dev *bifur_dev = fpriv->cdev_data->bifur_dev;

    if (*out_size != sizeof(*indir_out) || in_size != sizeof(*indir_in)) {
        BIFUR_DEV_ERR(bifur_dev->lld_dev, "Unexpect buf size from user, in_size: %u, out_size: %u, expect: %zu\n",
            in_size, *out_size, sizeof(*indir_in));
        return -EINVAL;
    }

    if (memcpy_s(indir_out, sizeof(*indir_out), indir_in, sizeof(*indir_in)) != 0) {
        BIFUR_DEV_ERR(bifur_dev->lld_dev, "Failed to copy user queue data, src_size:%zu, dst_size:%zu\n",
            sizeof(*indir_in), sizeof(*indir_out));
        return -ENOMEM;
    }

    if (bifur_dev->qpool_dev->qpool_dpdk_ops->get_rss_indir_table == NULL) {
        return -EINVAL;
    }
    return bifur_dev->qpool_dev->qpool_dpdk_ops->get_rss_indir_table(fpriv, indir_out);
}

int get_queue_start_paddr(struct bifur_fpriv_data *fpriv, const void *buf_in, u32 in_size,
    void *buf_out, u32 *out_size)
{
    struct bifur_qpool_dev *qpool_dev = NULL;
    if (*out_size != sizeof(dma_addr_t)) {
        pr_err("unexpect out buf size from user: %u, expect: %lu\n", *out_size, sizeof(dma_addr_t));
        return -EINVAL;
    }
    qpool_dev = fpriv->cdev_data->bifur_dev->qpool_dev;
    if (qpool_dev->qpool_dpdk_ops->get_queue_start_paddr == NULL) {
        return -EINVAL;
    }
    return qpool_dev->qpool_dpdk_ops->get_queue_start_paddr(fpriv, (struct drv_cmd_paddr_get *)buf_in, buf_out);
}

int get_kernel_nic_data(struct bifur_fpriv_data *fpriv, const void *buf_in,
                         u32 in_size, void *buf_out, u32 *out_size)
{
    struct drv_cmd_kernel_nic_data *indir_in = (struct drv_cmd_kernel_nic_data *)buf_in;
    struct drv_cmd_kernel_nic_data *indir_out = (struct drv_cmd_kernel_nic_data *)buf_out;
    struct bifur_lld_dev *bifur_dev = fpriv->cdev_data->bifur_dev;
    struct hinic5_nic_dev *nic_dev = bifur_get_hinic5_nic_by_bifur_dev(bifur_dev);
    if (nic_dev == NULL) {
        pr_err("nic dev cannot be found\n");
        return -EINVAL;
    }

    if (*out_size != sizeof(*indir_out) || in_size != sizeof(*indir_in)) {
        BIFUR_DEV_ERR(bifur_dev->lld_dev, "Unexpect buf size from user, in_size: %u, out_size: %u, expect: %zu\n",
            in_size, *out_size, sizeof(*indir_in));
        return -EINVAL;
    }

    if (memcpy_s(indir_out, sizeof(*indir_out), indir_in, sizeof(*indir_in)) != 0) {
        BIFUR_DEV_ERR(bifur_dev->lld_dev, "Failed to copy user queue data, src_size:%zu, dst_size:%zu\n",
            sizeof(*indir_in), sizeof(*indir_out));
        return -ENOMEM;
    }

    indir_out->rx_q_depth = nic_dev->rxqs[0].q_depth;
    indir_out->tx_q_depth = nic_dev->txqs[0].q_depth;
    indir_out->mtu = nic_dev->netdev->mtu;
    indir_out->link_status = netif_carrier_ok(nic_dev->netdev) ? 1 : 0;
    indir_out->group_num = bifur_dev->qpool_dev->qpool_group.qg_num;
    return 0;
}

static const struct file_bifur_drv_module_handle bifur_driv_module_cmd_handle[] = {
    {GET_USER_QUEUE_ID,     get_user_queue_id},
    {DEL_USER_QUEUE_ID,     del_user_queue_id},
    {CFG_RSS_TEMPLATE,      cfg_rss_template},
    {SET_RSS_INDIR_TBL,     set_rss_indir_table},
    {GET_RSS_INDIR_TBL,     get_rss_indir_table},
    {GET_QUEUE_START_PA,    get_queue_start_paddr},
    {GET_KERN_DEV_DATA,     get_kernel_nic_data}
};

static int bifur_send_to_driver(struct bifur_fpriv_data *fpriv, const struct bifur_ioctl_msg *nt_msg,
    const void *buf_in, u32 in_size, void *buf_out, u32 *out_size)
{
    int index, num_cmds = (int)(sizeof(bifur_driv_module_cmd_handle) /
                sizeof(bifur_driv_module_cmd_handle[0]));
    enum bifur_dpdk_cmd_type cmd_type = (enum bifur_dpdk_cmd_type)nt_msg->msg_format;
    int err = 0;

    for (index = 0; index < num_cmds; index++) {
        if (cmd_type ==
            bifur_driv_module_cmd_handle[index].driv_cmd_name) {
            err = bifur_driv_module_cmd_handle[index].driv_func
                    (fpriv, buf_in, in_size, buf_out, out_size);
            break;
        }
    }

    if (index == num_cmds) {
        pr_err("Can't find callback for %d\n", cmd_type);
        return -EINVAL;
    }

    return err;
}

int bifur_send_to_npu(struct bifur_fpriv_data *fpriv, struct bifur_ioctl_msg *ctl_msg,
        void *buf_in, u32 in_size, void *buf_out, u32 *out_size)
{
    int ret;
    u8 cmd;
    enum hinic5_mod_type mod;
    void *hwdev = fpriv->cdev_data->bifur_dev->lld_dev->hwdev;

    mod = (enum hinic5_mod_type)ctl_msg->npu_cmd.mod;
    cmd = ctl_msg->npu_cmd.cmd;

    if (ctl_msg->npu_cmd.direct_resp) {
        ret = hinic5_cmdq_direct_resp(hwdev, mod, cmd, buf_in, buf_out, 0, HINIC5_CHANNEL_DEFAULT);
        if (ret != 0) {
            pr_err("send direct cmdq failed, err: %d\n", ret);
        }
    } else {
        ret = hinic5_cmdq_detail_resp(hwdev, mod, cmd, buf_in, buf_out, NULL, 0, HINIC5_CHANNEL_DEFAULT);
        if (ret != 0) {
            pr_err("send detail cmdq failed, err: %d\n", ret);
        }
    }

    return ret;
}

int bifur_ioctl_exec_cmd(struct bifur_fpriv_data *fpriv, struct bifur_ioctl_msg *ctl_msg,
    void *buf_in, u32 in_size, void *buf_out, u32 *out_size)
{
    int ret = 0;

    switch (ctl_msg->module) {
        case SEND_TO_MPU:
            ret = bifur_send_to_mpu(fpriv, ctl_msg, buf_in, in_size, buf_out, out_size);
            break;
        case SEND_TO_BIFUR_DRIVER:
            ret = bifur_send_to_driver(fpriv, ctl_msg, buf_in, in_size, buf_out, out_size);
            break;
        case SEND_TO_NPU:
            ret = bifur_send_to_npu(fpriv, ctl_msg, buf_in, in_size, buf_out, out_size);
            break;
        default:
            pr_err("unsupport moudle:%u\n", ctl_msg->module);
            ret = -EOPNOTSUPP;
            break;
    }

    return ret;
}

static int cmd_parameter_valid(struct file *file, struct bifur_ioctl_msg *nt_msg,
    unsigned long arg, u32 *out_size_expect, u32 *in_size)
{
    struct bifur_fpriv_data *fpriv = file->private_data;
    struct bifur_cdev_data *cdev_data = NULL;

    if (!fpriv) {
        pr_err_ratelimited("cmd para check, file private_data is null\n");
        return -EINVAL;
    }

    cdev_data = fpriv->cdev_data;
    if (!cdev_data || !(cdev_data->bifur_dev)) {
        pr_err_ratelimited("cmd para check, nic dev is null\n");
        return -EINVAL;
    }

    if (copy_from_user(nt_msg, (void *)arg, sizeof(*nt_msg))) {
        pr_err_ratelimited("copy information from user failed\n");
        return -EFAULT;
    }

    *out_size_expect = nt_msg->buf_out_size;
    *in_size = nt_msg->buf_in_size;
    if (*out_size_expect > BIFUR_MSG_MAX_BUF_SIZE || *in_size > BIFUR_MSG_MAX_BUF_SIZE) {
        pr_err("invalid in size: %u or out size: %u\n", *in_size, *out_size_expect);
        return -EFAULT;
    }

    return 0;
}

static void bifur_free_buff_in(void *hwdev, const struct bifur_ioctl_msg *ctl_msg, void *buf_in)
{
    if (!buf_in) {
        return;
    }

    if (ctl_msg->module == SEND_TO_NPU) {
        hinic5_free_cmd_buf(hwdev, buf_in);
    } else {
        kfree(buf_in);
    }
}

static void bifur_free_buff_out(void *hwdev, const struct bifur_ioctl_msg *ctl_msg, void *buf_out)
{
    if (!buf_out) {
        return;
    }

    if (ctl_msg->module == SEND_TO_NPU && (ctl_msg->npu_cmd.direct_resp == 0)) {
        hinic5_free_cmd_buf(hwdev, buf_out);
    } else {
        kfree(buf_out);
    }
}

static void bifur_free_buf(void *hwdev, const struct bifur_ioctl_msg *ctl_msg, void *buf_in, void *buf_out)
{
    bifur_free_buff_out(hwdev, ctl_msg, buf_out);
    bifur_free_buff_in(hwdev, ctl_msg, buf_in);
}

static int bifur_alloc_buff_in(void *hwdev, const struct bifur_ioctl_msg *ctl_msg, u32 in_size, void **buf_in)
{
    void *msg_buf = NULL;

    if (in_size == 0 || !ctl_msg->in_buf) {
        return 0;
    }

    if (ctl_msg->module == SEND_TO_NPU) {
        struct hinic5_cmd_buf *cmd_buf = NULL;

        if (in_size > BIFUR_CMDQ_BUF_MAX_SIZE) {
            pr_err("cmdq in size(%u) more than %u\n", in_size, BIFUR_CMDQ_BUF_MAX_SIZE);
            return -EINVAL;
        }

        cmd_buf = hinic5_alloc_cmd_buf(hwdev);
        if (!cmd_buf) {
            pr_err("alloc cmdq cmd buffer failed in\n");
            return -ENOMEM;
        }
        msg_buf = cmd_buf->buf;
        *buf_in = (void *)cmd_buf;
        cmd_buf->size = (u16)in_size;
    } else {
        if (in_size > BIFUR_MSG_MAX_BUF_SIZE) {
            pr_err("in size(%u) more than %u\n", in_size, BIFUR_MSG_MAX_BUF_SIZE);
            return -ENOMEM;
        }
        msg_buf = kzalloc(in_size, GFP_KERNEL);
        *buf_in = msg_buf;
    }
    if (!(*buf_in)) {
        pr_err("alloc buffer in failed\n");
        return -ENOMEM;
    }
    if (!msg_buf) {
        pr_err("alloc msg_buf failed\n");
        return -ENOMEM;
    }
    if (copy_from_user(msg_buf, ctl_msg->in_buf, in_size)) {
        pr_err("copy from user failed\n");
        bifur_free_buff_in(hwdev, ctl_msg, *buf_in);
        return -EFAULT;
    }

    return 0;
}

static int bifur_alloc_buff_out(void *hwdev, const struct bifur_ioctl_msg *ctl_msg, u32 out_size, void **buf_out)
{
    if (out_size == 0) {
        return 0;
    }

    if (ctl_msg->module == SEND_TO_NPU && (ctl_msg->npu_cmd.direct_resp == 0)) {
        struct hinic5_cmd_buf *cmd_buf = NULL;

        if (out_size > BIFUR_CMDQ_BUF_MAX_SIZE) {
            pr_err("cmdq out size(%u) more than %u\n", out_size, BIFUR_CMDQ_BUF_MAX_SIZE);
            return -EINVAL;
        }

        cmd_buf = hinic5_alloc_cmd_buf(hwdev);
        *buf_out = (void *)cmd_buf;
    } else {
        if (out_size > BIFUR_MSG_MAX_BUF_SIZE) {
            pr_err("out size(%u) more than %u\n", out_size, BIFUR_MSG_MAX_BUF_SIZE);
            return -ENOMEM;
        }
        *buf_out = kzalloc(out_size, GFP_KERNEL);
    }
    if (!(*buf_out)) {
        pr_err("alloc buffer out failed\n");
        return -ENOMEM;
    }

    return 0;
}

static int bifur_alloc_buf(void *hwdev, const struct bifur_ioctl_msg *ctl_msg, u32 in_size,
    void **buf_in, u32 out_size, void **buf_out)
{
    int ret;

    ret = bifur_alloc_buff_in(hwdev, ctl_msg, in_size, buf_in);
    if (ret != 0) {
        pr_err("alloc cdev cmd buff in failed\n");
        return ret;
    }

    ret = bifur_alloc_buff_out(hwdev, ctl_msg, out_size, buf_out);
    if (ret != 0) {
        pr_err("alloc cdev cmd buff out failed\n");
        goto out_free_buf_in;
    }

    return 0;

out_free_buf_in:
    bifur_free_buff_in(hwdev, ctl_msg, *buf_in);

    return ret;
}

static int bifur_copy_buf_out_to_user(struct bifur_ioctl_msg *ctl_msg, u32 out_size, void *buf_out)
{
    void *msg_out = NULL;

    if (out_size == 0 || !buf_out) {
        return 0;
    }

    if (ctl_msg->module == SEND_TO_NPU && (ctl_msg->npu_cmd.direct_resp == 0)) {
        msg_out = ((struct hinic5_cmd_buf *)buf_out)->buf;
    } else {
        msg_out = buf_out;
    }

    if (copy_to_user(ctl_msg->out_buf, msg_out, out_size)) {
        return -EFAULT;
    }

    return 0;
}

static long bifur_cdev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct bifur_cdev_data *cdev_data = NULL;
    struct bifur_ioctl_msg ctl_msg = {};
    struct bifur_fpriv_data *fpriv = file->private_data;
    void *buf_in = NULL, *buf_out = NULL, *hwdev = NULL;
    u32 out_size_expect = 0, in_size = 0, out_size = 0;
    int ret;

    ret = cmd_parameter_valid(file, &ctl_msg, arg, &out_size_expect, &in_size);
    if (ret != 0) {
        return ret;
    }

    cdev_data = fpriv->cdev_data;
    hwdev = cdev_data->bifur_dev->lld_dev->hwdev;
    if (!hwdev) {
        pr_info("hwdev is null\n");
        return -EFAULT;
    }

    ret = bifur_alloc_buf(hwdev, &ctl_msg, in_size, &buf_in, out_size_expect, &buf_out);
    if (ret != 0) {
        pr_err("alloc tmp buff failed\n");
        return ret;
    }

    out_size = out_size_expect;
    ret = bifur_ioctl_exec_cmd(fpriv, &ctl_msg, buf_in, in_size, buf_out, &out_size);
    if (ret != 0) {
        pr_err("ioctl exec cmd failed, module: %u, ret: %d.\n", ctl_msg.module, ret);
        goto out_free_buf;
    }

    if (out_size > out_size_expect) {
        ret = -EFAULT;
        pr_err("out size is greater than expected out size from user: %u, out size: %u\n", out_size_expect, out_size);
        goto out_free_buf;
    }

    ret = bifur_copy_buf_out_to_user(&ctl_msg, out_size, buf_out);
    if (ret != 0) {
        pr_err("copy information to user failed\n");
    }

out_free_buf:
    bifur_free_buf(hwdev, &ctl_msg, buf_in, buf_out);

    return ret;
}

static int bifur_cdev_mmap(struct file *file, struct vm_area_struct *vma)
{
    struct bifur_fpriv_data *fpriv = NULL;
    struct bifur_qpool_dev *qpool_dev= NULL;
    if (file == NULL || vma == NULL) {
        pr_err("mmap: invalid file(%d) or vma(%d).\n", !!file, !!vma);
        return -EINVAL;
    }

    fpriv = file->private_data;
    qpool_dev = fpriv->cdev_data->bifur_dev->qpool_dev;

    if (qpool_dev->qpool_dpdk_ops->qpool_mmap == NULL) {
        return -EINVAL;
    }
    return qpool_dev->qpool_dpdk_ops->qpool_mmap(fpriv, vma);
}

static const struct file_operations g_bifur_qpool_cdev_fops = {
    .owner = THIS_MODULE,
    .open = bifur_cdev_open,
    .release = bifur_cdev_close,
    .read = bifur_cdev_read,
    .write = NULL,
    .unlocked_ioctl = bifur_cdev_ioctl,
    .poll = bifur_cdev_poll,
    .mmap = bifur_cdev_mmap,
};

static int bifur_get_bdf_name(struct bifur_lld_dev *bifur_dev, char *name, size_t len)
{
    int ret;
    struct pci_dev *pdev = to_pci_dev(bifur_dev->lld_dev->dev);

    ret = snprintf_s(name, len, len - 1, "%.4x:%.2x:%.2x.%x",
        pci_domain_nr(pdev->bus), pdev->bus->number, PCI_SLOT(pdev->devfn), PCI_FUNC(pdev->devfn));
    if ((ret < 0) || (ret >= len)) {
        pr_err("get bdf name failed.%.4x:%.2x:%.2x.%x\n",
            pci_domain_nr(pdev->bus), pdev->bus->number, PCI_SLOT(pdev->devfn), PCI_FUNC(pdev->devfn));
        return -EINVAL;
    }
    return 0;
}

static int bifur_cdev_create_device(struct bifur_cdev_data *cdata, const char *node_name)
{
    cdata->dev = device_create(cdata->cdev_class, NULL, cdata->cdev_major, "%s", node_name);
    if (!cdata->dev) {
        pr_err("failed to create device!\n");
        return (int)PTR_ERR(cdata->dev);
    }
    return 0;
}

static void init_notify_dev(struct bifur_cdev_data *cdata)
{
    cdata->notify_dev.event_count = 0;
    cdata->notify_dev.event_read_pos = 0;
    mutex_init(&cdata->notify_dev.lock);
    init_waitqueue_head(&(cdata->notify_dev.wait_queue));
}
static int bifur_cdev_init(struct bifur_lld_dev *bifur_dev, struct bifur_cdev_data *cdata)
{
    int ret;
    char node_name[BIFUR_CDEV_NAME_LEN] = {};

    cdata->bifur_dev = bifur_dev;
    init_notify_dev(cdata);
    ret = bifur_get_bdf_name(bifur_dev, cdata->name, sizeof(cdata->name));
    if (ret < 0) {
        return ret;
    }

    mutex_lock(&g_root_cdev.cdev_mutex);
    cdata->cdev_idx = (int)find_first_zero_bit(g_root_cdev.dev_map, BIFUR_MAX_CDEV_NUM);
    if (cdata->cdev_idx >= BIFUR_MAX_CDEV_NUM) {
        pr_err("failed to get cdev idx\n");
        mutex_unlock(&g_root_cdev.cdev_mutex);
        return -ENOMEM;
    }

    cdata->cdev_major = MKDEV(MAJOR(g_root_cdev.cdev_base), cdata->cdev_idx);
    set_bit(cdata->cdev_idx, g_root_cdev.dev_map);
    mutex_unlock(&g_root_cdev.cdev_mutex);

    cdev_init(&cdata->cdev, &g_bifur_qpool_cdev_fops);
    cdata->cdev.owner = THIS_MODULE;

    ret = cdev_add(&cdata->cdev, cdata->cdev_major, 1);
    if (ret != 0) {
        pr_err("failed to add cdev!\n");
        goto cdev_add_fail;
    }

    cdata->cdev_class = g_root_cdev.cdev_class;
    ret = snprintf_s(node_name, BIFUR_CDEV_NAME_LEN, BIFUR_CDEV_NAME_LEN - 1, "%s/%s", PARENT_CDEV_PATH_NAME, cdata->name);
    if ((ret < 0) || (ret >= BIFUR_CDEV_NAME_LEN)) {
        pr_err("set node name fail\n");
        goto node_name_fail;
    }

    ret = bifur_cdev_create_device(cdata, node_name);
    if (ret != 0) {
        goto device_create_fail; // 错误日志函数内打印
    }

    atomic_inc(&g_root_cdev.refcnt);
    atomic_set(&cdata->thread_cnt, 0);
    init_completion(&cdata->thread_comp);

    return 0;

device_create_fail:
    cdata->cdev_class = NULL;

node_name_fail:
    cdev_del(&cdata->cdev);

cdev_add_fail:
    clear_bit(cdata->cdev_idx, g_root_cdev.dev_map);
    return ret;
}

static int bifur_cdev_root_create(void)
{
    int ret;

    if (g_root_cdev.cdev_class) {
        return 0;
    }

    ret = register_netdevice_notifier(&netdevice_nb);
    if (ret != 0) {
        pr_err("failed to register netdevice notifier\n");
        return ret;
    }

    ret = alloc_chrdev_region(&g_root_cdev.cdev_base, 0, BIFUR_MAX_CDEV_NUM, PARENT_CDEV_PATH_NAME);
    if (ret < 0) {
        pr_err("failed to allocate device numbers\n");
        unregister_netdevice_notifier(&netdevice_nb);
        return ret;
    }

    g_root_cdev.cdev_class = class_create(THIS_MODULE, PARENT_CDEV_PATH_NAME);
    if (!g_root_cdev.cdev_class) {
        /* try again */
        g_root_cdev.cdev_class = class_create(THIS_MODULE, PARENT_CDEV_PATH_NAME);
        if (!g_root_cdev.cdev_class) {
            pr_err("[qpool]create nic cdev dir failed!");
            unregister_chrdev_region(g_root_cdev.cdev_base, BIFUR_MAX_CDEV_NUM);
            unregister_netdevice_notifier(&netdevice_nb);
            return -EPERM;
        }
    }

    mutex_init(&g_root_cdev.cdev_mutex);
    atomic_set(&g_root_cdev.refcnt, 0);

    return 0;
}

static void bifur_cdev_root_remove(struct bifur_lld_dev *bifur_dev)
{
    struct bifur_qpool_dev *qpool_dev = bifur_dev->qpool_dev;
    mutex_lock(&g_root_cdev.cdev_mutex);
    if (g_root_cdev.cdev_class && atomic_read(&g_root_cdev.refcnt) == 0) {
        class_destroy(g_root_cdev.cdev_class);
        g_root_cdev.cdev_class = NULL;
        unregister_netdevice_notifier(&netdevice_nb);
    }
    if (qpool_dev && qpool_dev->cdev_data) {
        clear_bit(qpool_dev->cdev_data->cdev_idx, g_root_cdev.dev_map);
    }

    mutex_unlock(&g_root_cdev.cdev_mutex);
    return;
}

int bifur_cdev_create(struct bifur_lld_dev *bifur_dev)
{
    int ret = 0;
    struct bifur_qpool_dev *qpool_dev = bifur_dev->qpool_dev;
    struct flow_proc_file_list_t *flow_proc_file_list = NULL;

    ret = bifur_qpool_adapt_init(bifur_dev);
    if (ret != 0) {
        goto qpool_adapt_init_err;
    }

    ret = bifur_cdev_root_create();
    if (ret != 0) {
        goto cdev_root_err;
    }

    qpool_dev->cdev_data = (struct bifur_cdev_data *)kzalloc(sizeof(struct bifur_cdev_data), GFP_KERNEL);
    if (qpool_dev->cdev_data == NULL) {
        pr_err("failed to allocate cdev data\n");
        ret = -ENOMEM;
        goto cdev_data_err;
    }

    flow_proc_file_list = (struct  flow_proc_file_list_t *)kzalloc(sizeof(struct flow_proc_file_list_t), GFP_KERNEL);
    if (flow_proc_file_list == NULL) {
        pr_err("failed to proc file list\n");
        ret = -ENOMEM;
        goto proc_file_err;
    }

    flow_global_file_init(flow_proc_file_list);
    qpool_dev->flow_proc_file_list = flow_proc_file_list;

    ret = bifur_cdev_init(bifur_dev, qpool_dev->cdev_data);
    if (ret != 0) {
        pr_err("failed to init cdev info\n");
        goto cdev_init_err;
    }

    return ret;

cdev_init_err:
    kfree(flow_proc_file_list);
    flow_proc_file_list = NULL;
proc_file_err:
    kfree(qpool_dev->cdev_data);
    qpool_dev->cdev_data = NULL;
cdev_data_err:
    bifur_cdev_root_remove(bifur_dev);
cdev_root_err:
qpool_adapt_init_err:
    bifur_qpool_adapt_deinit(bifur_dev);
    return ret;
}

void bifur_cdev_remove(struct bifur_lld_dev *bifur_dev)
{
    struct bifur_cdev_data *cdata = NULL;
    struct bifur_qpool_dev *qpool_dev = NULL;

    if (bifur_dev == NULL || bifur_dev->qpool_dev == NULL) {
        pr_err("some parameters is null\n");
        return;
    }

    qpool_dev = bifur_dev->qpool_dev;
    cdata = qpool_dev->cdev_data;

    flow_global_file_uninit(cdata, qpool_dev->flow_proc_file_list);
    kfree(qpool_dev->flow_proc_file_list);
    qpool_dev->flow_proc_file_list = NULL;

    device_destroy(cdata->cdev_class, cdata->cdev_major);

    cdev_del(&cdata->cdev);

    unregister_chrdev_region(cdata->cdev_major, 1);

    atomic_dec(&g_root_cdev.refcnt);
    bifur_cdev_root_remove(bifur_dev);

    kfree(cdata);
    qpool_dev->cdev_data = NULL;

    bifur_qpool_adapt_deinit(bifur_dev);
}