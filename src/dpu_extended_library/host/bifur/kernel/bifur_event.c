/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */
#include <net/bonding.h>
#include "hinic5_srv_nic.h"
#include "hinic5_lld.h"
#include "hinic5_bond.h"
#include "hinic5_mt.h"
#include "hinic5_hw.h"
#include "nic_mpu_cmd.h"
#include "mag_mpu_cmd.h"
#include "comm_defs.h"

#include "bifur_common.h"
#include "bifur_vf_mgr.h"
#include "bifur_main.h"
#include "bifur_event.h"

void bifur_notify_vf_link_status(struct hinic5_lld_dev *lld_dev, u8 port_id, u16 vf_id, u8 link_status)
{
    struct mag_cmd_get_link_status link;
    u16 out_size = sizeof(link);
    int err;

    (void)memset_s(&link, sizeof(link), 0, sizeof(link));
    link.status = link_status;
    link.port_id = port_id;

    err = hinic5_mbox_to_vf_no_ack(lld_dev->hwdev, vf_id, HINIC5_MOD_HILINK, MAG_CMD_GET_LINK_STATUS,
        &link, sizeof(link), &link, &out_size, HINIC5_CHANNEL_NIC);
    if (err == MBOX_ERRCODE_UNKNOWN_DES_FUNC) {
        pr_err_ratelimited("Vf%d not initialized, disconnect it\n", HW_VF_ID_TO_OS(vf_id));
        return;
    }

    if (err || !out_size || link.head.status) {
        pr_err_ratelimited("Send link change event to VF %d failed, err: %d, status: 0x%x, out_size: 0x%x\n",
            HW_VF_ID_TO_OS(vf_id), err, link.head.status, out_size);
    }
}

void bifur_notify_all_vfs_link_changed(struct hinic5_lld_dev *lld_dev, u32 dbdf, u8 link_status)
{
    struct bifur_lld_dev *bifur_src_dev = NULL;
    struct bifur_vf_mgr *vf_mgr = NULL;
    struct bifur_vf_info *vf_info = NULL;
    u16 i;
    u8 port_id;

    bifur_src_dev = bifur_get_resource_dev();
    if (!bifur_src_dev) {
        return;
    }

    vf_mgr = bifur_src_dev->vf_mgr;
    port_id = hinic5_physical_port_id(lld_dev->hwdev);

    mutex_lock(&vf_mgr->vf_mgr_mutex);
    for (i = 0; i < vf_mgr->vf_sum; ++i) {
        vf_info = &(vf_mgr->vf_info[i]);
        if ((vf_info->pf_dbdf == dbdf) && vf_info->in_use) {
            bifur_notify_vf_link_status(bifur_src_dev->lld_dev, port_id, OS_VF_ID_TO_HW(i), link_status);
        }
    }
    mutex_unlock(&vf_mgr->vf_mgr_mutex);

    bifur_dev_put(bifur_src_dev);

    return;
}

void bifur_netdev_event(struct work_struct *work)
{
    struct bifur_lld_dev *bifur_dev = container_of(work, struct bifur_lld_dev, netdev_link);
    struct bifur_adapter *adp = bifur_get_adp();

    if (adp->bond_bifur_enabled == BIFUR_DISABLED) {
        bifur_notify_all_vfs_link_changed(bifur_dev->lld_dev, bifur_dev->dbdf, bifur_dev->link_status);
    } else {
        bifur_notify_all_vfs_link_changed(bifur_dev->lld_dev, bifur_dev->dbdf, adp->bond_link_status);
    }

    return;
}

static void bifur_bond_net_link_event(struct bifur_adapter *adp, unsigned long event)
{
    struct bifur_lld_dev *bifur_dev = NULL;
    struct bifur_lld_dev *tmp_dev = NULL;
    struct list_head *head = &adp->lld_dev_head;
    u8 bond_link_status = 0;
    list_for_each_entry_safe(bifur_dev, tmp_dev, head, list)
    {
        if (bifur_dev->pf_type == BIFUR_SHARED_PF) {
            bond_link_status |= bifur_dev->link_status;
        }
    }

    adp->bond_link_status = bond_link_status;

    list_for_each_entry_safe(bifur_dev, tmp_dev, head, list)
    {
        if (bifur_dev->pf_type != BIFUR_SHARED_PF) {
            continue;
        }

        if (event == NETDEV_UP || ((event == NETDEV_DOWN) && !bond_link_status)) {
            bifur_dev_hold(bifur_dev);
            (void)queue_work(adp->event_workq, &bifur_dev->netdev_link);
        }
    }

    return;
}

int bifur_net_link_event(struct bifur_adapter *adp, unsigned long event, struct net_device *dev)
{
    u32 dbdf;
    struct bifur_lld_dev *bifur_dev = NULL;
    struct hinic5_nic_dev *nic_dev = NULL;
    struct hinic5_lld_dev *lld_dev = NULL;

    lld_dev = hinic5_get_lld_dev_by_netdev(dev);
    if (!lld_dev) {
        pr_err_ratelimited("lld dev null\n");
        return NOTIFY_DONE;
    }
    nic_dev = netdev_priv(dev);
    dbdf = bifur_get_dbdf_by_lld(nic_dev->lld_dev);
    bifur_dev = bifur_get_shared_dev_by_dbdf(dbdf);
    if (!bifur_dev) {
        return NOTIFY_DONE;
    }

    bifur_dev->link_status = (event == NETDEV_UP ? 1 : 0);
    if (adp->bond_bifur_enabled == BIFUR_DISABLED) {
        (void)queue_work(adp->event_workq, &bifur_dev->netdev_link);
        bifur_dev_put(bifur_dev);
        return NOTIFY_OK;
    }
    bifur_bond_net_link_event(adp, event);
    bifur_dev_put(bifur_dev);

    return NOTIFY_OK;
}

int bifur_net_event_callback(struct notifier_block *nb, unsigned long event, void *ptr)
{
    struct net_device *dev = netdev_notifier_info_to_dev(ptr);
    struct bifur_adapter *adp = bifur_get_adp();

    if (unlikely(dev == NULL)) {
        pr_err_ratelimited("bifur notify dev null\n");
        return NOTIFY_DONE;
    }
    if (unlikely(adp == NULL)) {
        pr_err_ratelimited("bifur notify adp null\n");
        return NOTIFY_DONE;
    }
    /* only self-developed NICs can be processed */
    if (!hinic5_get_lld_dev_by_netdev(dev)) {
        return NOTIFY_DONE;
    }

    switch (event) {
        case NETDEV_UP:
        case NETDEV_DOWN:
            return bifur_net_link_event(adp, event, dev);
        default:
            break;
    }

    return NOTIFY_OK;
}

static struct notifier_block g_bifur_event_nb = {
    .notifier_call = bifur_net_event_callback
};

int bifur_register_net_event(void)
{
    int ret;
    ret = register_netdevice_notifier(&g_bifur_event_nb);
    return ret;
}

void bifur_unregister_net_event(void)
{
    (void)unregister_netdevice_notifier(&g_bifur_event_nb);
    return;
}
