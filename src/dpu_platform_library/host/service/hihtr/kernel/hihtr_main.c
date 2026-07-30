/*
 * Copyright (c) 2016 Hisilicon Limited.
 * Copyright (c) 2007, 2008 Mellanox Technologies. All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include <linux/acpi.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/list.h>
#include <linux/jhash.h>
#include <linux/vmalloc.h>
#include <rdma/ib_addr.h>
#include <rdma/ib_mad.h>
#include <rdma/ib_user_verbs.h>
#include <rdma/ib_cache.h>
#include <rdma/uverbs_ioctl.h>
#include <rdma/ib_verbs.h>

#include "hihtr_common.h"
#include "hihtr_device.h"
#include "hihtr_hem.h"
#include "hihtr_dca.h"
#include "hihtr_debugfs.h"
#include "hihtr_cmdq.h"
#include "hihtr_hw_v3.h"
#include "hihtr_mcast.h"
#include "dfx/hihtr_dfx.h"

static void hihtr_init_mcast_gmv_table(struct hihtr_dev *hr_dev)
{
    u32 i;

    for (i = 0; i < HIHTR_MCAST_HASH_SIZE; i++) {
        INIT_HLIST_HEAD(&hr_dev->gmv_hash_table[i]);
    }
	hr_dev->mcast_count = 0;
    spin_lock_init(&hr_dev->gmv_hash_lock);
	dev_info(hr_dev->dev, "mcast gmv table init success!\n");
}

static void hihtr_destroy_mcast_gmv_table(struct hihtr_dev *hr_dev)
{
    u32 i;
    struct hihtr_mgid_hash_node *entry;
	struct hlist_node *tmp;

    spin_lock(&hr_dev->gmv_hash_lock);
    for (i = 0; i < HIHTR_MCAST_HASH_SIZE; i++) {
        hlist_for_each_entry_safe(entry, tmp, &hr_dev->gmv_hash_table[i], node) {
            hlist_del(&entry->node);
            kfree(entry);
        }
    }
    spin_unlock(&hr_dev->gmv_hash_lock);

	for (i = 0; i < HIHTR_MCAST_HASH_SIZE; i++) {
		INIT_HLIST_HEAD(&hr_dev->gmv_hash_table[i]);
	}
	return;
}

#if defined(HAVE_ALL_ROCE_FUNCS_U32_PORT)
static struct net_device *hihtr_get_netdev(struct ib_device *ib_dev, u32 port_num)
#else
STATIC struct net_device *hihtr_get_netdev(struct ib_device *ib_dev, u8 port_num)
#endif
{
	struct hihtr_dev *hr_dev = to_hr_dev(ib_dev);
	struct net_device *ndev;

	if (port_num < 1 || port_num > hr_dev->caps.num_ports)
		return NULL;

	rcu_read_lock();
	ndev = hr_dev->iboe.netdevs[port_num - 1];

	if (ndev)
		dev_hold(ndev);
	rcu_read_unlock();

	return ndev;
}

static int hihtr_set_mac(struct hihtr_dev *hr_dev, u32 port,
			    const u8 *addr)
{
	u8 phy_port;
	u32 i;

	if (HIHTR_GET_PCI_REVISION(hr_dev) >= PCI_REVISION_ID_HIP09)
		return 0;

	if (!memcmp(hr_dev->dev_addr[port], addr, ETH_ALEN))
		return 0;

	for (i = 0; i < ETH_ALEN; i++)
		hr_dev->dev_addr[port][i] = addr[i];

	phy_port = hr_dev->iboe.phy_port[port];
	return hr_dev->hw->set_mac(hr_dev, phy_port, addr);
}

STATIC int hihtr_add_gid(const struct ib_gid_attr *attr, void **context)
{
	struct hihtr_dev *hr_dev = to_hr_dev(attr->device);
	u8 port = attr->port_num - 1;
	int ret;

	if (port >= hr_dev->caps.num_ports)
		return -EINVAL;

	ret = hr_dev->hw->set_gid(hr_dev, attr->index, &attr->gid, attr);

	return ret;
}

int hihtr_del_gid(const struct ib_gid_attr *attr, void **context)
{
	struct hihtr_dev *hr_dev = to_hr_dev(attr->device);
	u8 port = attr->port_num - 1;
	int ret;

	if (port >= hr_dev->caps.num_ports)
		return -EINVAL;

	ret = hr_dev->hw->set_gid(hr_dev, attr->index, NULL, NULL);

	return ret;
}

#if defined(HAVE_ALL_ROCE_FUNCS_U32_PORT)
int hihtr_query_gid(struct ib_device *device, u32 port_num, int index, union ib_gid *gid)
#else
int hihtr_query_gid(struct ib_device *device, u8 port_num, int index, union ib_gid *gid)
#endif
{
	return rdma_query_gid(device, port_num, index, gid);
}

static int handle_en_event(struct hihtr_dev *hr_dev, u8 port,
			   unsigned long dev_event)
{
	struct device *dev = hr_dev->dev;
	enum ib_port_state port_state;
	struct net_device *netdev;
	struct ib_event event;
	unsigned long flags;
	int ret = 0;

	netdev = hr_dev->iboe.netdevs[port];
	if (!netdev) {
		dev_err(dev, "Can't find netdev on port(%u)!\n", port);
		return -ENODEV;
	}

	switch (dev_event) {
	case NETDEV_REGISTER:
	case NETDEV_CHANGEADDR:
		ret = hihtr_set_mac(hr_dev, port, netdev->dev_addr);
		break;
	case NETDEV_UP:
	case NETDEV_CHANGE:
		ret = hihtr_set_mac(hr_dev, port, netdev->dev_addr);
		if (ret)
			return ret;
		fallthrough;
	case NETDEV_DOWN:
		port_state = get_port_state(netdev);

		spin_lock_irqsave(&hr_dev->iboe.lock, flags);
		if (hr_dev->iboe.port_state[port] == port_state) {
			spin_unlock_irqrestore(&hr_dev->iboe.lock, flags);
			return NOTIFY_DONE;
		}
		hr_dev->iboe.port_state[port] = port_state;
		spin_unlock_irqrestore(&hr_dev->iboe.lock, flags);

		event.device = &hr_dev->ib_dev;
		event.event = (port_state == IB_PORT_ACTIVE) ?
			      IB_EVENT_PORT_ACTIVE : IB_EVENT_PORT_ERR;
		event.element.port_num = to_rdma_port_num(port);
		hihtr_dfx_record_err_event_count(hr_dev, event.event);
		ib_dispatch_event(&event);
		break;
	case NETDEV_UNREGISTER:
		break;
	default:
		dev_dbg(dev, "NETDEV event = 0x%x!\n", (u32)(dev_event));
		break;
	}

	return ret;
}

static int hihtr_netdev_event(struct notifier_block *self,
				 unsigned long event, void *ptr)
{
	struct net_device *dev = netdev_notifier_info_to_dev(ptr);
	struct hihtr_ib_iboe *iboe = NULL;
	struct hihtr_dev *hr_dev = NULL;
	int ret;
	u8 port;

	hr_dev = container_of(self, struct hihtr_dev, iboe.nb);
	iboe = &hr_dev->iboe;

	for (port = 0; port < hr_dev->caps.num_ports; port++) {
		if (dev == iboe->netdevs[port]) {
			ret = handle_en_event(hr_dev, port, event);
			if (ret)
				return NOTIFY_DONE;
			break;
		}
	}

	return NOTIFY_DONE;
}

static int hihtr_setup_mtu_mac(struct hihtr_dev *hr_dev)
{
	int ret;
	u8 i;

	for (i = 0; i < hr_dev->caps.num_ports; i++) {
		hr_dev->iboe.port_state[i] = IB_PORT_DOWN;

		ret = hihtr_set_mac(hr_dev, i,
				       hr_dev->iboe.netdevs[i]->dev_addr);
		if (ret)
			return ret;
	}

	return 0;
}

static int hihtr_query_device(struct ib_device *ib_dev,
				 struct ib_device_attr *props,
				 struct ib_udata *uhw)
{
	struct hihtr_dev *hr_dev = to_hr_dev(ib_dev);

	memset(props, 0, sizeof(*props));

	props->fw_ver = hr_dev->caps.fw_ver;
	props->sys_image_guid = cpu_to_be64(hr_dev->sys_image_guid);
	props->max_mr_size = (u64)(~(0ULL));
	props->page_size_cap = hr_dev->caps.page_size_cap;
	props->vendor_id = hr_dev->vendor_id;
	props->vendor_part_id = hr_dev->vendor_part_id;
	props->hw_ver = hr_dev->hw_rev;
	props->max_qp = hr_dev->caps.num_qps;
	props->max_qp_wr = hr_dev->caps.max_rq_wqes;
	props->device_cap_flags = IB_DEVICE_PORT_ACTIVE_EVENT |
				  IB_DEVICE_RC_RNR_NAK_GEN;
	props->max_send_sge = hr_dev->caps.max_sq_sg;
	props->max_recv_sge = hr_dev->caps.max_rq_sg;
	props->max_sge_rd = hr_dev->caps.max_sq_sg;
	props->max_cq = hr_dev->caps.num_cqs;
	props->max_cqe = hr_dev->caps.max_cqes;
	props->max_mr = hr_dev->caps.num_mtpts;
	props->max_pd = hr_dev->caps.num_pds;
	props->max_qp_rd_atom = hr_dev->caps.max_qp_dest_rdma;
	props->max_qp_init_rd_atom = hr_dev->caps.max_qp_init_rdma;
	props->atomic_cap = hr_dev->caps.flags & HIHTR_CAP_FLAG_ATOMIC ?
			    IB_ATOMIC_HCA : IB_ATOMIC_NONE;
	props->max_pkeys = 1;
	props->local_ca_ack_delay = hr_dev->caps.local_ca_ack_delay;
	if (hr_dev->caps.flags & HIHTR_CAP_FLAG_SRQ) {
		props->max_srq = hr_dev->caps.num_srqs;
		props->max_srq_wr = hr_dev->caps.max_srq_wrs;
		props->max_srq_sge = hr_dev->caps.max_srq_sges;
	}

	if ((hr_dev->caps.flags & HIHTR_CAP_FLAG_FRMR) &&
    	(HIHTR_GET_PCI_REVISION(hr_dev) >= PCI_REVISION_ID_HIP09)) {
		props->device_cap_flags |= IB_DEVICE_MEM_MGT_EXTENSIONS;
		props->max_fast_reg_page_list_len = HIHTR_FRMR_MAX_PA;
	}

	if (hr_dev->caps.flags & HIHTR_CAP_FLAG_XRC)
		props->device_cap_flags |= IB_DEVICE_XRC;

	return 0;
}

#if defined(HAVE_ALL_ROCE_FUNCS_U32_PORT)
static int hihtr_query_port(struct ib_device *ib_dev, u32 port_num, struct ib_port_attr *props)
#else
static int hihtr_query_port(struct ib_device *ib_dev, u8 port_num, struct ib_port_attr *props)
#endif
{
	struct hihtr_dev *hr_dev = to_hr_dev(ib_dev);
	struct device *dev = hr_dev->dev;
	struct net_device *net_dev;
	unsigned long flags;
	enum ib_mtu mtu;
	u8 port;

	port = port_num - 1;

	/* props being zeroed by the caller, avoid zeroing it here */

	props->max_mtu = hr_dev->caps.max_mtu;
	props->gid_tbl_len = hr_dev->caps.gid_table_len[port];
	props->port_cap_flags = IB_PORT_CM_SUP | IB_PORT_REINIT_SUP |
				IB_PORT_VENDOR_CLASS_SUP |
				IB_PORT_BOOT_MGMT_SUP;
	props->max_msg_sz = HIHTR_MAX_MSG_LEN;
	props->pkey_tbl_len = 1;
	props->active_width = IB_WIDTH_4X;
	props->active_speed = 1;

	spin_lock_irqsave(&hr_dev->iboe.lock, flags);

	net_dev = hr_dev->iboe.netdevs[port];
	if (!net_dev) {
		spin_unlock_irqrestore(&hr_dev->iboe.lock, flags);
		dev_err(dev, "Find netdev %u failed!\n", port);
		return -EINVAL;
	}

	mtu = iboe_get_mtu(net_dev->mtu);
	props->active_mtu = mtu ? min(props->max_mtu, mtu) : IB_MTU_256;
	props->state = get_port_state(net_dev);
	props->phys_state = props->state == IB_PORT_ACTIVE ?
				    IB_PORT_PHYS_STATE_LINK_UP :
				    IB_PORT_PHYS_STATE_DISABLED;

	spin_unlock_irqrestore(&hr_dev->iboe.lock, flags);

	return 0;
}

#if defined(HAVE_ALL_ROCE_FUNCS_U32_PORT)
static enum rdma_link_layer hihtr_get_link_layer(struct ib_device *device, u32 port_num)
#else
static enum rdma_link_layer hihtr_get_link_layer(struct ib_device *device, u8 port_num)
#endif
{
	return IB_LINK_LAYER_ETHERNET;
}

#if defined(HAVE_ALL_ROCE_FUNCS_U32_PORT)
static int hihtr_query_pkey(struct ib_device *ib_dev, u32 port, u16 index, u16 *pkey)
#else
static int hihtr_query_pkey(struct ib_device *ib_dev, u8 port, u16 index, u16 *pkey)
#endif
{
	if (index > 0)
		return -EINVAL;

	*pkey = PKEY_ID;

	return 0;
}

static int hihtr_modify_device(struct ib_device *ib_dev, int mask,
				  struct ib_device_modify *props)
{
	unsigned long flags;

	if (mask & ~IB_DEVICE_MODIFY_NODE_DESC)
		return -EOPNOTSUPP;

	if (mask & IB_DEVICE_MODIFY_NODE_DESC) {
		spin_lock_irqsave(&to_hr_dev(ib_dev)->sm_lock, flags);
		memcpy(ib_dev->node_desc, props->node_desc, NODE_DESC_SIZE);
		spin_unlock_irqrestore(&to_hr_dev(ib_dev)->sm_lock, flags);
	}

	return 0;
}

static void hihtr_dealloc_uar_entry(struct hihtr_ucontext *context)
{
	if (context->uar_mmap_entry)
		rdma_user_mmap_entry_remove(
			&context->uar_mmap_entry->rdma_entry);
	if (context->dca_ctx.dca_mmap_entry)
		rdma_user_mmap_entry_remove(
			&context->dca_ctx.dca_mmap_entry->rdma_entry);

/* Only destroy mmap_xa if we define it in hihtr_ucontext */
#if defined(RDMA_HAVE_XARRAY) && !defined(IB_UCONTEXT_HAVE_MMAP_XA)
	xa_destroy(&context->mmap_xa);
#endif
}

static int hihtr_alloc_uar_entry(struct ib_ucontext *uctx)
{
	struct hihtr_ucontext *context = to_hr_ucontext(uctx);
	u64 address;
	struct hihtr_dev *hr_dev = to_hr_dev(uctx->device);

/* Only initialize mmap_xa if we define it in hihtr_ucontext */
#if defined(RDMA_HAVE_XARRAY) && !defined(IB_UCONTEXT_HAVE_MMAP_XA)
	xa_init_flags(&context->mmap_xa, XA_FLAGS_ALLOC);
#endif

	address = (u64)hr_dev->mem_base;
	context->uar_mmap_entry = hihtr_user_mmap_entry_insert(
		uctx, address, PAGE_SIZE, HIHTR_MMAP_TYPE_DB);
	if (!context->uar_mmap_entry)
		return -ENOMEM;

	return 0;
}

static void hihtr_dealloc_reset_entry(struct hihtr_ucontext *context)
{
	if (context->reset_mmap_entry)
		rdma_user_mmap_entry_remove(&context->reset_mmap_entry->rdma_entry);
}

static int hihtr_alloc_reset_entry(struct ib_ucontext *uctx)
{
	struct hihtr_ucontext *context = to_hr_ucontext(uctx);
	struct hihtr_dev *hr_dev = to_hr_dev(uctx->device);

	context->reset_mmap_entry = hihtr_user_mmap_entry_insert(uctx,
		(u64)hr_dev->reset_kaddr, PAGE_SIZE, HIHTR_MMAP_TYPE_RESET);

	if (!context->reset_mmap_entry)
		return -ENOMEM;

	return 0;
}

static void hihtr_dealloc_stats_entry(struct hihtr_ucontext *context)
{
	if (context->stats_mmap_entry)
		rdma_user_mmap_entry_remove(&context->stats_mmap_entry->rdma_entry);
}

static int hihtr_alloc_stats_entry(struct ib_ucontext *uctx)
{
	struct hihtr_ucontext *context = to_hr_ucontext(uctx);
	struct hihtr_dev *hr_dev = to_hr_dev(uctx->device);

	context->stats_mmap_entry = hihtr_user_mmap_entry_insert(uctx,
		(u64)hr_dev->user_count_buf, PAGE_SIZE, HIHTR_MMAP_TYPE_STATS);

	if (!context->stats_mmap_entry) {
		return -ENOMEM;
	}

	return 0;
}

void hihtr_dealloc_db_entry(struct hihtr_ucontext *context)
{
	struct hihtr_dev *hr_dev = to_hr_dev(context->ibucontext.device);
	struct hihtr_v3_priv *priv = (struct hihtr_v3_priv *)(hr_dev->priv);
	if (context->db_mmap_entry) {
		hinic5_free_db_phy_addr(priv->lld_dev->hwdev, context->db_mmap_entry->address, 0);
		rdma_user_mmap_entry_remove(&context->db_mmap_entry->rdma_entry);
	}
}

int hihtr_alloc_db_entry(struct ib_ucontext *uctx)
{
	struct hihtr_ucontext *context = to_hr_ucontext(uctx);
	struct hihtr_dev *hr_dev = to_hr_dev(uctx->device);
	struct hihtr_v3_priv *priv = (struct hihtr_v3_priv *)(hr_dev->priv);
	u64 udb_base;
	u64 udb_dweq_base;
	int ret;

	ret = hinic5_alloc_db_phy_addr(priv->lld_dev->hwdev, &udb_base, &udb_dweq_base);
    if (ret) {
        ibdev_err(&hr_dev->ib_dev, "Failed to allocate doorbell page, ret %d.\n", ret);
        return ret;
    }

	context->db_mmap_entry = hihtr_user_mmap_entry_insert(uctx, udb_base, HIHTR_DWQE_SIZE, HIHTR_MMAP_TYPE_DWQE);
	if (!context->db_mmap_entry) {
		hinic5_free_db_phy_addr(priv->lld_dev->hwdev, context->db_mmap_entry->address, 0);
		return -ENOMEM;
	}

	return 0;
}

static void ucontext_set_resp(struct ib_ucontext *uctx,
			      struct hihtr_ib_alloc_ucontext_resp *resp)
{
	struct hihtr_ucontext *context = to_hr_ucontext(uctx);
	struct hihtr_dev *hr_dev = to_hr_dev(uctx->device);
	struct rdma_user_mmap_entry *rdma_entry;

	resp->qp_tab_size = hr_dev->caps.num_qps;
	resp->qp_mtt_buf_pg_sz = hr_dev->caps.mtt_buf_pg_sz + HIHTR_HW_PAGE_SHIFT;
	resp->srq_tab_size = hr_dev->caps.num_srqs;
	resp->cqe_size = hr_dev->caps.cqe_sz;
	resp->cqe_mtt_buf_pg_sz = hr_dev->caps.cqe_buf_pg_sz + HIHTR_HW_PAGE_SHIFT;

	if (context->dca_ctx.dca_mmap_entry) {
		resp->dca_qps = context->dca_ctx.max_qps;
		resp->dca_mmap_size = PAGE_SIZE * context->dca_ctx.status_npage;
		rdma_entry = &context->dca_ctx.dca_mmap_entry->rdma_entry;
		resp->dca_mmap_key = rdma_user_mmap_get_offset(rdma_entry);
	}

	if (context->reset_mmap_entry) {
		rdma_entry = &context->reset_mmap_entry->rdma_entry;
		resp->reset_mmap_key = rdma_user_mmap_get_offset(rdma_entry);
	}

	if (context->db_mmap_entry) {
		rdma_entry = &context->db_mmap_entry->rdma_entry;
		resp->db_mmap_key = rdma_user_mmap_get_offset(rdma_entry);
	}

	if (context->stats_mmap_entry) {
		rdma_entry = &context->stats_mmap_entry->rdma_entry;
		resp->stats_mmap_key = rdma_user_mmap_get_offset(rdma_entry);
	}
}

static u32 get_udca_max_qps(struct hihtr_dev *hr_dev,
			    struct hihtr_ib_alloc_ucontext *ucmd)
{
	u32 qp_num;

	if (ucmd->comp & HIHTR_ALLOC_UCTX_COMP_DCA_MAX_QPS) {
		qp_num = ucmd->dca_max_qps;
		if (!qp_num)
			qp_num = hr_dev->caps.num_qps;
	} else {
		qp_num = 0;
	}

	return qp_num;
}

static int hihtr_set_order(struct hihtr_dev *hr_dev)
{
#if defined(HIHTR_RO_ENABLE)
    struct hihtr_v3_priv *priv = (struct hihtr_v3_priv *)(hr_dev->priv);
	payload_so_type so_type = RO;
    if (roce_func_is_ppf(priv->lld_dev->hwdev)) {
        return hihtr_cmq_set_so_cfg(hr_dev, so_type);
    }
#endif

    return 0;
}

static void hihtr_alloc_dwqe_resource(struct hihtr_dev *hr_dev, struct hihtr_ucontext *context)
{
	bool is_alloc_dwqe = false;
	unsigned long flags;
	unsigned long *en_flags = context ? &(context->en_flags) : &(hr_dev->en_flags);
	pid_t c_pid = context ? context->pid : hr_dev->pid;

	spin_lock_irqsave(&(hr_dev->dwqe_resource_lock), flags);
	if (hr_dev->caps.free_dwqe_resource > 0) {
		hr_dev->caps.free_dwqe_resource--;
		(*en_flags) |= HIHTR_CAP_FLAG_DIRECT_WQE;
		is_alloc_dwqe = true;
	} else {
		(*en_flags) &= ~HIHTR_CAP_FLAG_DIRECT_WQE;
	}
	spin_unlock_irqrestore(&(hr_dev->dwqe_resource_lock), flags);

	if (is_alloc_dwqe == true) {
		hihtr_add_dwqe_pid(hr_dev, c_pid);
	}
	return;
}

static void hihtr_release_dwqe_resource(struct hihtr_dev *hr_dev, struct hihtr_ucontext *context)
{
	unsigned long flags;
	unsigned long *en_flags = context ? &(context->en_flags) : &(hr_dev->en_flags);
	pid_t c_pid = context ? context->pid : hr_dev->pid;

	if (!((*en_flags) & HIHTR_CAP_FLAG_DIRECT_WQE)) {
		return;
	}

	spin_lock_irqsave(&(hr_dev->dwqe_resource_lock), flags);
	if (hr_dev->caps.free_dwqe_resource < hr_dev->caps.max_dwqe_resource) {
		hr_dev->caps.free_dwqe_resource++;
	}
	(*en_flags) &= ~HIHTR_CAP_FLAG_DIRECT_WQE;
	spin_unlock_irqrestore(&(hr_dev->dwqe_resource_lock), flags);
	hihtr_remove_dwqe_pid(hr_dev, c_pid);
	return;
}

#if defined(ROCE_ALLOC_UCONTEXT_INT)
int hihtr_alloc_ucontext(struct ib_ucontext *uctx,
				   struct ib_udata *udata)
{
	struct hihtr_ucontext *context = to_hr_ucontext(uctx);
	struct hihtr_dev *hr_dev = to_hr_dev(uctx->device);
	struct hihtr_ib_alloc_ucontext_resp resp = {};
	struct hihtr_ib_alloc_ucontext ucmd = {};
	int ret;

	if (!hr_dev->active)
		return -EAGAIN;

	context->pid = current->pid;
	INIT_LIST_HEAD(&context->list);

	ret = ib_copy_from_udata(&ucmd, udata,
				 min(udata->inlen, sizeof(ucmd)));
	if (ret)
		return ret;

	context->en_flags = 0;
	context->config = ucmd.config & HIHTR_EXSGE_FLAGS;
	resp.config |= HIHTR_RSP_EXSGE_FLAGS;
	resp.max_inline_data = hr_dev->caps.max_sq_inline;

	// 同一qp不支持同时开启rq inline和cqe inline
	if (hr_dev->inline_type == RQ_INLINE) {
		context->config |= ucmd.config & HIHTR_RQ_INLINE_FLAGS;
		resp.config |= HIHTR_RSP_RQ_INLINE_FLAGS;
	}

	if (hr_dev->inline_type == CQE_INLINE) {
		context->config |= ucmd.config & HIHTR_CQE_INLINE_FLAGS;
		resp.config |= HIHTR_RSP_CQE_INLINE_FLAGS;
	}

	if (hr_dev->caps.flags & HIHTR_CAP_FLAG_DCA_MODE) {
		context->config |= ucmd.config & HIHTR_UCTX_CONFIG_DCA;
		resp.config |= HIHTR_UCTX_RSP_DCA_FLAGS;
	}

	if (hr_dev->caps.flags & HIHTR_CAP_FLAG_RESIZE_CQ) {
		context->config |= HIHTR_RESIZE_CQ_FLAGS;
		resp.config |= HIHTR_RSP_RESIZE_CQ_FLAGS;
		resp.resize_cq_hw_cq_depth = hr_dev->caps.resize_cq_hw_cq_depth;
	}

	ret = hihtr_uar_alloc(hr_dev, &context->uar);
	if (ret)
		goto error_fail_uar_alloc;

	ret = hihtr_alloc_uar_entry(uctx);
	if (ret)
		goto error_fail_uar_entry;

	if ((hr_dev->caps.flags & HIHTR_CAP_FLAG_CQ_RECORD_DB) ||
	    (hr_dev->caps.flags & HIHTR_CAP_FLAG_QP_RECORD_DB)) {
		INIT_LIST_HEAD(&context->page_list);
		mutex_init(&context->page_mutex);
	}

	hihtr_register_udca(hr_dev, get_udca_max_qps(hr_dev, &ucmd),
			       context);

	ret = hihtr_alloc_reset_entry(uctx);
	if (ret)
		goto error_fail_reset_entry;

	ret = hihtr_alloc_db_entry(uctx);
	if (ret)
		goto error_fail_alloc_db;

	ret = hihtr_alloc_stats_entry(uctx);
	if (ret)
		goto error_fail_alloc_stats;

	ucontext_set_resp(uctx, &resp);
	ret = ib_copy_to_udata(udata, &resp, min(udata->outlen, sizeof(resp)));
	if (ret)
		goto error_fail_copy_to_udata;

	spin_lock(&hr_dev->uctx_list_lock);
	list_add(&context->list, &hr_dev->uctx_list);
	spin_unlock(&hr_dev->uctx_list_lock);

	hihtr_register_uctx_debugfs(hr_dev, context);
	hihtr_alloc_dwqe_resource(hr_dev, context);
	return 0;

error_fail_copy_to_udata:
	hihtr_unregister_udca(hr_dev, context);
	hihtr_dealloc_stats_entry(context);
error_fail_alloc_stats:
	hihtr_dealloc_db_entry(context);
error_fail_alloc_db:
	hihtr_dealloc_reset_entry(context);
error_fail_reset_entry:
	hihtr_dealloc_uar_entry(context);
error_fail_uar_entry:
	ida_free(&hr_dev->uar_ida.ida, (int)context->uar.logic_idx);
error_fail_uar_alloc:
	return ret;
}

void hihtr_dealloc_ucontext(struct ib_ucontext *ibcontext)
{
	struct hihtr_ucontext *context = to_hr_ucontext(ibcontext);
	struct hihtr_dev *hr_dev = to_hr_dev(ibcontext->device);

	spin_lock(&hr_dev->uctx_list_lock);
	list_del(&context->list);
	spin_unlock(&hr_dev->uctx_list_lock);

	hihtr_unregister_uctx_debugfs(hr_dev, context);

	hihtr_unregister_udca(hr_dev, context);

	hihtr_dealloc_stats_entry(context);
	hihtr_dealloc_db_entry(context);
	hihtr_dealloc_uar_entry(context);
	hihtr_dealloc_reset_entry(context);
	hihtr_release_dwqe_resource(hr_dev, context);
	ida_free(&hr_dev->uar_ida.ida, (int)context->uar.logic_idx);
}
#else
struct ib_ucontext *hihtr_alloc_ucontext(struct ib_device *ibdev, struct ib_udata *udata)
{
	struct hihtr_ucontext *context = NULL;
	struct hihtr_dev *hr_dev = to_hr_dev(ibdev);
	struct hihtr_ib_alloc_ucontext_resp resp = {};
	struct hihtr_ib_alloc_ucontext ucmd = {};
	struct ib_ucontext *uctx = NULL;
	int ret;

	if (!hr_dev->active) {
		dev_err(hr_dev->dev, "Roce alloc ucontext fail: hr_dev->active is false\n");
		return (struct ib_ucontext *)ERR_PTR(-EAGAIN);
	}

	context = kzalloc(sizeof(struct hihtr_ucontext), GFP_KERNEL);
	if (!context) {
		dev_err(hr_dev->dev, "Roce alloc ucontext fail: kzalloc fail\n");
		return (struct ib_ucontext *)ERR_PTR(-ENOMEM);
	}

	uctx = &context->ibucontext;
	uctx->device = ibdev;

	context->pid = current->pid;
	INIT_LIST_HEAD(&context->list);
	ret = ib_copy_from_udata(&ucmd, udata,
				 min(udata->inlen, sizeof(ucmd)));
	if (ret)
		goto error_fail_out;

	context->config = ucmd.config & HIHTR_EXSGE_FLAGS;
	resp.config |= HIHTR_RSP_EXSGE_FLAGS;
	resp.max_inline_data = hr_dev->caps.max_sq_inline;

	// 同一qp不支持同时开启rq inline和cqe inline
	if (hr_dev->inline_type == RQ_INLINE) {
		context->config |= ucmd.config & HIHTR_RQ_INLINE_FLAGS;
		resp.config |= HIHTR_RSP_RQ_INLINE_FLAGS;
	}

	if (hr_dev->inline_type == CQE_INLINE) {
		context->config |= ucmd.config & HIHTR_CQE_INLINE_FLAGS;
		resp.config |= HIHTR_RSP_CQE_INLINE_FLAGS;
	}

	if (hr_dev->caps.flags & HIHTR_CAP_FLAG_DCA_MODE) {
		context->config |= ucmd.config & HIHTR_UCTX_CONFIG_DCA;
		resp.config |= HIHTR_UCTX_RSP_DCA_FLAGS;
	}

	if (hr_dev->caps.flags & HIHTR_CAP_FLAG_RESIZE_CQ) {
		context->config |= HIHTR_RESIZE_CQ_FLAGS;
		resp.config |= HIHTR_RSP_RESIZE_CQ_FLAGS;
		resp.resize_cq_hw_cq_depth = hr_dev->caps.resize_cq_hw_cq_depth;
	}

	ret = hihtr_uar_alloc(hr_dev, &context->uar);
	if (ret)
		goto error_fail_out;

	ret = hihtr_alloc_uar_entry(uctx);
	if (ret)
		goto error_fail_uar_entry;

	if (((hr_dev->caps.flags & HIHTR_CAP_FLAG_CQ_RECORD_DB) ||
     	(hr_dev->caps.flags & HIHTR_CAP_FLAG_QP_RECORD_DB))) {
		INIT_LIST_HEAD(&context->page_list);
		mutex_init(&context->page_mutex);
	}

	hihtr_register_udca(hr_dev, get_udca_max_qps(hr_dev, &ucmd),
			       context);
	ret = hihtr_alloc_reset_entry(uctx);
	if (ret)
		goto error_fail_reset_entry;

	ret = hihtr_alloc_db_entry(uctx);
	if (ret)
		goto error_fail_alloc_db;

	ret = hihtr_alloc_stats_entry(uctx);
	if (ret)
		goto error_fail_alloc_stats;

	ucontext_set_resp(uctx, &resp);
	ret = ib_copy_to_udata(udata, &resp, min(udata->outlen, sizeof(resp)));
	if (ret)
		goto error_fail_copy_to_udata;

	spin_lock(&hr_dev->uctx_list_lock);
	list_add(&context->list, &hr_dev->uctx_list);
	spin_unlock(&hr_dev->uctx_list_lock);

	hihtr_register_uctx_debugfs(hr_dev, context);
	hihtr_alloc_dwqe_resource(hr_dev, context);
	return uctx;

error_fail_copy_to_udata:
	hihtr_unregister_udca(hr_dev, context);
	hihtr_dealloc_stats_entry(context);
error_fail_alloc_stats:
	hihtr_dealloc_db_entry(context);
error_fail_alloc_db:
	hihtr_dealloc_reset_entry(context);
error_fail_reset_entry:
	hihtr_dealloc_uar_entry(context);
error_fail_uar_entry:
	ida_free(&hr_dev->uar_ida.ida, (int)context->uar.logic_idx);
error_fail_out:
	kfree(context);
	return (struct ib_ucontext *)ERR_PTR((long)ret);
}

int hihtr_dealloc_ucontext(struct ib_ucontext *ibcontext)
{
	struct hihtr_ucontext *context = NULL;
	struct hihtr_dev *hr_dev = NULL;

	if (ibcontext == NULL) {
        pr_err("roce dealloc_ucontext Ibcontext is null\n");
        return -EINVAL;
    }

	context = to_hr_ucontext(ibcontext);
	hr_dev = to_hr_dev(ibcontext->device);

	spin_lock(&hr_dev->uctx_list_lock);
	list_del(&context->list);
	spin_unlock(&hr_dev->uctx_list_lock);

	hihtr_unregister_uctx_debugfs(hr_dev, context);

	hihtr_unregister_udca(hr_dev, context);

	hihtr_dealloc_stats_entry(context);
	hihtr_dealloc_db_entry(context);
	hihtr_dealloc_uar_entry(context);
	hihtr_dealloc_reset_entry(context);
	hihtr_release_dwqe_resource(hr_dev, context);
	ida_free(&hr_dev->uar_ida.ida, (int)context->uar.logic_idx);
	kfree(context);

	return 0;
}
#endif

static int mmap_dca(struct ib_ucontext *context, struct vm_area_struct *vma)
{
	struct hihtr_ucontext *uctx = to_hr_ucontext(context);
	struct hihtr_dca_ctx *ctx = &uctx->dca_ctx;
	struct page **pages;
	unsigned long num;
	int ret;

	if ((vma->vm_end - vma->vm_start != (ctx->status_npage * PAGE_SIZE) ||
	     !(vma->vm_flags & VM_SHARED)))
		return -EINVAL;

	if (!(vma->vm_flags & VM_WRITE) || (vma->vm_flags & VM_EXEC))
		return -EPERM;

	if (!ctx->buf_status)
		return -EOPNOTSUPP;

	pages = kcalloc(ctx->status_npage, sizeof(struct page *), GFP_KERNEL);
	if (!pages)
		return -ENOMEM;

	for (num = 0; num < ctx->status_npage; num++)
		pages[num] = virt_to_page(ctx->buf_status + num * PAGE_SIZE);

	ret = vm_insert_pages(vma, vma->vm_start, pages, &num);
	kfree(pages);
	return ret;
}

int hihtr_mmap(struct ib_ucontext *uctx, struct vm_area_struct *vma)
{
	struct hihtr_dev *hr_dev = to_hr_dev(uctx->device);
	struct rdma_user_mmap_entry *rdma_entry;
	struct hihtr_user_mmap_entry *entry;
	phys_addr_t pfn;
	pgprot_t prot;
	int ret;

	rdma_entry = rdma_user_mmap_entry_get_pgoff(uctx, vma->vm_pgoff);
	if (!rdma_entry)
 		return -EINVAL;

	entry = to_hihtr_mmap(rdma_entry);
	pfn = entry->address >> PAGE_SHIFT;

	switch (entry->mmap_type) {
	case HIHTR_MMAP_TYPE_DB:
	case HIHTR_MMAP_TYPE_DWQE:
		prot = pgprot_device(vma->vm_page_prot);
		break;
	case HIHTR_MMAP_TYPE_DCA:
		ret = mmap_dca(uctx, vma);
		goto out;
	case HIHTR_MMAP_TYPE_RESET:
		if (vma->vm_flags & (VM_WRITE | VM_EXEC)) {
			ret = -EINVAL;
			goto out;
		}

		ret = remap_pfn_range(vma, vma->vm_start,
				      page_to_pfn(hr_dev->reset_page),
				      PAGE_SIZE, vma->vm_page_prot);
		goto out;
	case HIHTR_MMAP_TYPE_STATS:
		if (vma->vm_flags & VM_EXEC) {
			ret = -EINVAL;
			goto out;
		}

		ret = remap_pfn_range(vma, vma->vm_start,
				      page_to_pfn(hr_dev->user_count_page),
				      PAGE_SIZE, vma->vm_page_prot);
		goto out;
	default:
		ret = -EINVAL;
		goto out;
	}

#if defined(IB_VERBS_NEED_RDMA_USER_MMAP_IO)
	ret = remap_pfn_range(vma, vma->vm_start, pfn, rdma_entry->npages * PAGE_SIZE, prot);
#elif defined(RDMA_USER_MMAP_IO_HAVE_ENTRY)
	ret = rdma_user_mmap_io(uctx, vma, pfn, rdma_entry->npages * PAGE_SIZE,
				prot, rdma_entry);
#else
	ret = rdma_user_mmap_io(uctx, vma, pfn, rdma_entry->npages * PAGE_SIZE, prot);
#endif

out:
	rdma_user_mmap_entry_put(rdma_entry);

	return ret;
}

void hihtr_free_mmap(struct rdma_user_mmap_entry *rdma_entry)
{
	struct hihtr_user_mmap_entry *entry = to_hihtr_mmap(rdma_entry);

	kfree(entry);
}

#if defined(HAVE_ALL_ROCE_FUNCS_U32_PORT)
STATIC int hihtr_port_immutable(struct ib_device *ib_dev, u32 port_num, struct ib_port_immutable *immutable)
#else
STATIC int hihtr_port_immutable(struct ib_device *ib_dev, u8 port_num, struct ib_port_immutable *immutable)
#endif
{
	struct ib_port_attr attr;
	int ret;

	ret = ib_query_port(ib_dev, port_num, &attr);
	if (ret)
		return ret;

	immutable->pkey_tbl_len = attr.pkey_tbl_len;
	immutable->gid_tbl_len = attr.gid_tbl_len;
	immutable->max_mad_size = IB_MGMT_MAD_SIZE;
	immutable->core_cap_flags = RDMA_CORE_PORT_IBA_ROCE_UDP_ENCAP;

	return 0;
}

static void hihtr_get_fw_ver(struct ib_device *device, char *str)
{
	u64 fw_ver = to_hr_dev(device)->caps.fw_ver;
	unsigned int major, minor, sub_minor;
	major = upper_32_bits(fw_ver);
	minor = high_16_bits(lower_32_bits(fw_ver));
	sub_minor = low_16_bits(fw_ver);

	snprintf(str, IB_FW_VERSION_NAME_MAX, "%u.%u.%04u", major, minor,
		 sub_minor);
}

#if defined(IB_VERBS_HAVE_RDMA_STAT_DESC)
#define HIHTR_HW_CNT(ename, cname) \
	[HIHTR_HW_##ename##_CNT].name = cname
#else
#define HIHTR_HW_CNT(ename, cname) \
	[HIHTR_HW_##ename##_CNT] = cname
#endif

#if defined(IB_VERBS_HAVE_RDMA_STAT_DESC)
static const struct rdma_stat_desc hihtr_port_stats_descs[] = {
#else
static const char *const hihtr_port_stats_descs[] = {
#endif
	HIHTR_HW_CNT(RX_RC_PKT, "rx_rc_pkt"),
	HIHTR_HW_CNT(RX_UD_PKT, "rx_ud_pkt"),
	HIHTR_HW_CNT(RX_XRC_PKT, "rx_xrc_pkt"),
	HIHTR_HW_CNT(RX_PKT, "rx_pkt"),
	HIHTR_HW_CNT(RX_ERR_PKT, "rx_err_pkt"),
	HIHTR_HW_CNT(RX_CNP_PKT, "rx_cnp_pkt"),
	HIHTR_HW_CNT(RX_BUF, "rx_buffer"),
	HIHTR_HW_CNT(CQ_CQE, "cq_cqe"),
	HIHTR_HW_CNT(CQ_NOTIFY, "cq_notify"),
	HIHTR_HW_CNT(TX_RC_PKT, "tx_rc_pkt"),
	HIHTR_HW_CNT(TX_UD_PKT, "tx_ud_pkt"),
	HIHTR_HW_CNT(TX_XRC_PKT, "tx_xrc_pkt"),
	HIHTR_HW_CNT(TX_PKT, "tx_pkt"),
	HIHTR_HW_CNT(TX_ERR_PKT, "tx_err_pkt"),
	HIHTR_HW_CNT(TX_CNP_PKT, "tx_cnp_pkt"),
	HIHTR_HW_CNT(TRP_GET_IRRL_ERR_PKT, "trp_get_irrl_err_pkt"),
	HIHTR_HW_CNT(TRP_GET_MPT_ERR_PKT, "trp_get_mpt_err_pkt"),
	HIHTR_HW_CNT(ECN_DB, "ecn_doorbell"),
};

#if defined(HAVE_ALL_ROCE_FUNCS_U32_PORT)
__attribute__((unused)) static struct rdma_hw_stats *hihtr_alloc_hw_port_stats(struct ib_device *device, u32 port_num)
#else
static struct rdma_hw_stats *hihtr_alloc_hw_port_stats(struct ib_device *device, u8 port_num)
#endif
{
	struct hihtr_dev *hr_dev = to_hr_dev(device);

	if (port_num > hr_dev->caps.num_ports) {
		ibdev_err(device, "invalid port num.\n");
		return NULL;
	}

	return rdma_alloc_hw_stats_struct(hihtr_port_stats_descs,
					  ARRAY_SIZE(hihtr_port_stats_descs),
					  RDMA_HW_STATS_DEFAULT_LIFESPAN);
}

#if defined(HAVE_ALL_ROCE_FUNCS_U32_PORT)
__attribute__((unused)) static int hihtr_get_hw_stats(struct ib_device *device, struct rdma_hw_stats *stats, u32 port, int index)
#else
static int hihtr_get_hw_stats(struct ib_device *device, struct rdma_hw_stats *stats, u8 port, int index)
#endif
{
#ifdef ROCE_TODO
	struct hihtr_dev *hr_dev = to_hr_dev(device);
	int num_counters = HIHTR_HW_CNT_TOTAL;
	int ret;

	if (port == 0)
		return 0;

	if (port > hr_dev->caps.num_ports)
		return -EINVAL;

	ret = hr_dev->hw->query_hw_counter(hr_dev, stats->value, port,
					   &num_counters);
	if (ret) {
		ibdev_err(device, "failed to query hw counter, ret = %d.\n",
			  ret);
		return ret;
	}

	return num_counters;
#else
	return 0; // TODO 规避crash问题, 后续整改删除
#endif
}

static void hihtr_unregister_device(struct hihtr_dev *hr_dev)
{
	struct hihtr_ib_iboe *iboe = &hr_dev->iboe;

	hr_dev->active = false;
	unregister_netdevice_notifier(&iboe->nb);
	ib_unregister_device(&hr_dev->ib_dev);
}

const struct uapi_definition hihtr_uapi_defs[] = {
	UAPI_DEF_CHAIN(hihtr_dca_uapi_defs),
	{}
};

#if defined(ROCE_HAVE_QUERY_UCONTEXT)
static int hihtr_query_ucontext(struct ib_ucontext *ibcontext,
				   struct uverbs_attr_bundle *attrs)
{
	struct hihtr_ib_alloc_ucontext_resp resp = {};
	int attr_len;
	int ret;

	ucontext_set_resp(ibcontext, &resp);

	attr_len = uverbs_attr_get_len(attrs, 1U << UVERBS_ID_NS_SHIFT);
	if (attr_len < 0 || attr_len < sizeof(resp))
		return attr_len < 0 ? attr_len : -EINVAL;

	ret = uverbs_copy_to_struct_or_zero(attrs, 1U << UVERBS_ID_NS_SHIFT,
					    &resp, sizeof(resp));
	return ret;
}
#endif

#if !defined(RDMA_HAVE_IB_DEVICE_OPS)
static const struct ib_device hihtr_dev_ops = {
#else
static const struct ib_device_ops hihtr_dev_ops = {
#endif

#ifdef ROCE_IB_DEVICE_OPS_HAVE_MODULE
	.owner = THIS_MODULE,
	.uverbs_abi_ver = 1,
	.driver_id = RDMA_DRIVER_HIHTR,
#endif

#if defined(ROCE_HAVE_UVERBS_NO_DRIVER_ID_BINDING)
	.uverbs_no_driver_id_binding = 1,
#endif

	.get_dev_fw_str = hihtr_get_fw_ver,
	.add_gid = hihtr_add_gid,
	.query_gid = hihtr_query_gid,
	.alloc_pd = hihtr_alloc_pd,
	.alloc_ucontext = hihtr_alloc_ucontext,
	.create_ah = hihtr_create_ah,
// 6.6 内核ofed注册api的方式发生了改变，导致create ah相关的api最终没有注册
#if defined(ROCE_IB_DEVICE_OPS_HAVE_CREATE_USER_AH)
	.create_user_ah = hihtr_create_ah,
#endif
	.create_cq = hihtr_create_cq,
	.create_qp = hihtr_create_qp,
	.dealloc_pd = hihtr_dealloc_pd,
	.dealloc_ucontext = hihtr_dealloc_ucontext,
	.del_gid = hihtr_del_gid,
	.dereg_mr = hihtr_dereg_mr,
	.destroy_ah = hihtr_destroy_ah,
	.destroy_cq = hihtr_destroy_cq,
	// disassociate_ucontext设置为NULL，uverbs调用hiroce时（用户态使用对hiroce驱动）会有引用计数
	.disassociate_ucontext = NULL,
	.get_dma_mr = hihtr_get_dma_mr,
	.get_link_layer = hihtr_get_link_layer,
	.get_netdev = hihtr_get_netdev,
	.get_port_immutable = hihtr_port_immutable,
	.mmap = hihtr_mmap,
#if defined(ROCE_HAVE_MMAP_FREE)
	.mmap_free = hihtr_free_mmap,
#endif
	.modify_device = hihtr_modify_device,
	.modify_qp = hihtr_modify_qp,
	.query_ah = hihtr_query_ah,
	.query_device = hihtr_query_device,
	.query_pkey = hihtr_query_pkey,
	.query_port = hihtr_query_port,
#if defined(ROCE_HAVE_QUERY_UCONTEXT)
	.query_ucontext = hihtr_query_ucontext,
#endif
	.reg_user_mr = hihtr_reg_user_mr,
	.attach_mcast = hihtr_v3_attach_mcast,
	.detach_mcast = hihtr_v3_detach_mcast,

#if defined(ROCE_HAVE_IB_AH_OBJ)
	INIT_RDMA_OBJ_SIZE(ib_ah, hihtr_ah, ibah),
#endif
#if defined(ROCE_HAVE_IB_CQ_OBJ)
	INIT_RDMA_OBJ_SIZE(ib_cq, hihtr_cq, ib_cq),
#endif
#if defined(ROCE_HAVE_IB_PD_OBJ)
	INIT_RDMA_OBJ_SIZE(ib_pd, hihtr_pd, ibpd),
#endif
#if defined(ROCE_HAVE_UCONTEXT_OBJ)
	INIT_RDMA_OBJ_SIZE(ib_ucontext, hihtr_ucontext, ibucontext),
#endif
#if defined(ROCE_HAVE_IB_QP_OBJ)
	// 初始化size_ib_qp; 否则OFED框架申请size为0的内存，返回NULL指针，导致os crash
	INIT_RDMA_OBJ_SIZE(ib_qp, hihtr_qp, ibqp),
#endif
};

#if !defined(RDMA_HAVE_IB_DEVICE_OPS)
static const struct ib_device hihtr_resize_cq_ops = {
#else
static const struct ib_device_ops hihtr_resize_cq_ops = {
#endif
	.resize_cq = hihtr_resize_cq,
};

#if defined(HAVE_ALL_ROCE_FUNCS_U32_PORT)
// TODO: 内核6.6及以上暂时阉割hw stats统计功能，规避bug
static const struct ib_device_ops hihtr_dev_hw_stats_ops = {
    .alloc_hw_port_stats = NULL,
	.get_hw_stats = NULL,
};
#elif !defined(RDMA_HAVE_IB_DEVICE_OPS)
static const struct ib_device hihtr_dev_hw_stats_ops = {
	.get_hw_stats = hihtr_get_hw_stats,
	.alloc_hw_stats = hihtr_alloc_hw_port_stats,
};
#else
static const struct ib_device_ops hihtr_dev_hw_stats_ops = {
	.alloc_hw_stats = hihtr_alloc_hw_port_stats,
	.get_hw_stats = hihtr_get_hw_stats,
};
#endif

#if !defined(RDMA_HAVE_IB_DEVICE_OPS)
static const struct ib_device hihtr_dev_mr_ops = {
#else
static const struct ib_device_ops hihtr_dev_mr_ops = {
#endif
	.rereg_user_mr = hihtr_rereg_user_mr,
};

#if !defined(RDMA_HAVE_IB_DEVICE_OPS)
static const struct ib_device hihtr_dev_mw_ops = {
#else
static const struct ib_device_ops hihtr_dev_mw_ops = {
#endif
	.alloc_mw = hihtr_alloc_mw,
	.dealloc_mw = hihtr_dealloc_mw,

#if defined(ROCE_HAVE_IB_MW_OBJ)
	INIT_RDMA_OBJ_SIZE(ib_mw, hihtr_mw, ibmw),
#endif
};

#if !defined(RDMA_HAVE_IB_DEVICE_OPS)
static const struct ib_device hihtr_dev_frmr_ops = {
#else
static const struct ib_device_ops hihtr_dev_frmr_ops = {
#endif
	.alloc_mr = hihtr_alloc_mr,
	.map_mr_sg = hihtr_map_mr_sg,
};

#if !defined(RDMA_HAVE_IB_DEVICE_OPS)
static const struct ib_device hihtr_dev_srq_ops = {
#else
static const struct ib_device_ops hihtr_dev_srq_ops = {
#endif
	.create_srq = hihtr_create_srq,
	.destroy_srq = hihtr_destroy_srq,

#if defined(ROCE_HAVE_IB_SRQ_OBJ)
	INIT_RDMA_OBJ_SIZE(ib_srq, hihtr_srq, ibsrq),
#endif
};

#if !defined(RDMA_HAVE_IB_DEVICE_OPS)
static const struct ib_device hihtr_dev_xrcd_ops = {
#else
static const struct ib_device_ops hihtr_dev_xrcd_ops = {
#endif
	.alloc_xrcd = hihtr_alloc_xrcd,
	.dealloc_xrcd = hihtr_dealloc_xrcd,

#if defined(ROCE_HAVE_IB_XRCD_OBJ)
	INIT_RDMA_OBJ_SIZE(ib_xrcd, hihtr_xrcd, ibxrcd),
#endif
};

#if defined(ROCE_HAVE_FILL_RES_MR_ENTRY)
static const struct ib_device_ops hihtr_dev_restrack_ops = {
	// TODO: 待适配1872
	.fill_res_cq_entry = hihtr_fill_res_cq_entry,
#if defined(ROCE_HAVE_FILL_RES_CQ_ENTRY_RAW)
	.fill_res_cq_entry_raw = hihtr_fill_res_cq_entry_raw,
#endif
#if defined(ROCE_HAVE_FILL_RES_QP_ENTRY)
	.fill_res_qp_entry = hihtr_fill_res_qp_entry,
	.fill_res_qp_entry_raw = hihtr_fill_res_qp_entry_raw,
#endif
	.fill_res_mr_entry = hihtr_fill_res_mr_entry,
#if defined(ROCE_HAVE_FILL_RES_MR_ENTRY_RAW)
	.fill_res_mr_entry_raw = hihtr_fill_res_mr_entry_raw,
#endif
#if defined(ROCE_HAVE_FILL_RES_SRQ_ENTRY)
	.fill_res_srq_entry = hihtr_fill_res_srq_entry,
	.fill_res_srq_entry_raw = hihtr_fill_res_srq_entry_raw,
#endif
};
#endif

int hihtr_register_device(struct hihtr_dev *hr_dev, char *uld_dev_name)
{
	int ret;
	struct hihtr_ib_iboe *iboe = NULL;
	struct ib_device *ib_dev = NULL;
	struct device *dev = hr_dev->dev;
#if defined(IB_VERBS_HAVE_IB_DEVICE_SET_NETDEV)
	unsigned int i;
#endif

	iboe = &hr_dev->iboe;
	spin_lock_init(&iboe->lock);
	mutex_init(&hr_dev->qp_cnt.cur_qps_mutex);

	ib_dev = &hr_dev->ib_dev;

	ib_dev->node_type = RDMA_NODE_IB_CA;
	ib_dev->dev.parent = dev;

#if defined(ROCE_IB_DEVICE_HAVE_MODULE)
	ib_dev->owner = THIS_MODULE;
	ib_dev->uverbs_abi_ver = 1;
	ib_dev->driver_id = RDMA_DRIVER_HIHTR;
#endif

	ib_dev->phys_port_cnt = hr_dev->caps.num_ports;
	ib_dev->local_dma_lkey = hr_dev->caps.reserved_lkey;
	ib_dev->num_comp_vectors = hr_dev->caps.num_comp_vectors;
	ib_dev->uverbs_cmd_mask =
		(1ULL << IB_USER_VERBS_CMD_GET_CONTEXT) |
		(1ULL << IB_USER_VERBS_CMD_QUERY_DEVICE) |
		(1ULL << IB_USER_VERBS_CMD_QUERY_PORT) |
		(1ULL << IB_USER_VERBS_CMD_ALLOC_PD) |
		(1ULL << IB_USER_VERBS_CMD_DEALLOC_PD) |
		(1ULL << IB_USER_VERBS_CMD_REG_MR) |
		(1ULL << IB_USER_VERBS_CMD_DEREG_MR) |
		(1ULL << IB_USER_VERBS_CMD_CREATE_COMP_CHANNEL) |
		(1ULL << IB_USER_VERBS_CMD_CREATE_CQ) |
		(1ULL << IB_USER_VERBS_CMD_DESTROY_CQ) |
		(1ULL << IB_USER_VERBS_CMD_CREATE_QP) |
		(1ULL << IB_USER_VERBS_CMD_MODIFY_QP) |
		(1ULL << IB_USER_VERBS_CMD_QUERY_QP) |
		(1ULL << IB_USER_VERBS_CMD_DESTROY_QP) |
		(1ULL << IB_USER_VERBS_CMD_CREATE_AH) |
		(1ULL << IB_USER_VERBS_CMD_DESTROY_AH) |
		(1ULL << IB_USER_VERBS_CMD_ATTACH_MCAST) |
		(1ULL << IB_USER_VERBS_CMD_DETACH_MCAST);

#if defined(IB_DEV_HAVE_UVERBS_EX_CMD_MASK)
	ib_dev->uverbs_ex_cmd_mask |=
		(1ULL << IB_USER_VERBS_EX_CMD_QUERY_DEVICE) |
		(1ULL << IB_USER_VERBS_EX_CMD_MODIFY_CQ) |
		(1ULL << IB_USER_VERBS_EX_CMD_CREATE_CQ) |
		(1ULL << IB_USER_VERBS_EX_CMD_MODIFY_QP) |
		(1ULL << IB_USER_VERBS_EX_CMD_CREATE_QP);
#endif
	if (hr_dev->caps.flags & HIHTR_CAP_FLAG_REREG_MR) {
		ib_dev->uverbs_cmd_mask |= (1ULL << IB_USER_VERBS_CMD_REREG_MR);
		ib_set_device_ops(ib_dev, &hihtr_dev_mr_ops);
	}

	/* MW */
	if (hr_dev->caps.flags & HIHTR_CAP_FLAG_MW) {
		ib_dev->uverbs_cmd_mask |=
					(1ULL << IB_USER_VERBS_CMD_ALLOC_MW) |
					(1ULL << IB_USER_VERBS_CMD_DEALLOC_MW);
		ib_set_device_ops(ib_dev, &hihtr_dev_mw_ops);
	}

	/* FRMR */
	if (hr_dev->caps.flags & HIHTR_CAP_FLAG_FRMR)
		ib_set_device_ops(ib_dev, &hihtr_dev_frmr_ops);

	/* SRQ */
	if (hr_dev->caps.flags & HIHTR_CAP_FLAG_SRQ) {
		ib_dev->uverbs_cmd_mask |=
				(1ULL << IB_USER_VERBS_CMD_CREATE_SRQ) |
				(1ULL << IB_USER_VERBS_CMD_MODIFY_SRQ) |
				(1ULL << IB_USER_VERBS_CMD_QUERY_SRQ) |
				(1ULL << IB_USER_VERBS_CMD_DESTROY_SRQ) |
				(1ULL << IB_USER_VERBS_CMD_POST_SRQ_RECV);
		ib_set_device_ops(ib_dev, &hihtr_dev_srq_ops);
		ib_set_device_ops(ib_dev, hr_dev->hw->hihtr_dev_srq_ops);
	}

	if (hr_dev->caps.flags & HIHTR_CAP_FLAG_XRC) {
		ib_dev->uverbs_cmd_mask |=
				(1ULL << IB_USER_VERBS_CMD_OPEN_XRCD) |
				(1ULL << IB_USER_VERBS_CMD_CLOSE_XRCD) |
				(1ULL << IB_USER_VERBS_CMD_CREATE_XSRQ) |
				(1ULL << IB_USER_VERBS_CMD_OPEN_QP);
		ib_set_device_ops(ib_dev, &hihtr_dev_xrcd_ops);
	}

	/* resize cq */
	if (hr_dev->caps.flags & HIHTR_CAP_FLAG_RESIZE_CQ) {
		ib_dev->uverbs_cmd_mask |= (1ULL << IB_USER_VERBS_CMD_RESIZE_CQ);
		ib_set_device_ops(ib_dev, &hihtr_resize_cq_ops);
	}

	if (HIHTR_GET_PCI_REVISION(hr_dev) >= PCI_REVISION_ID_HIP09 &&
        !hr_dev->is_vf) {
	    ib_set_device_ops(ib_dev, &hihtr_dev_hw_stats_ops);
	}

	ib_set_device_ops(ib_dev, hr_dev->hw->hihtr_dev_ops);
	ib_set_device_ops(ib_dev, &hihtr_dev_ops);
#if defined(ROCE_HAVE_FILL_RES_MR_ENTRY)
	ib_set_device_ops(ib_dev, &hihtr_dev_restrack_ops);
#endif
	if (IS_ENABLED(CONFIG_INFINIBAND_USER_ACCESS))
#ifdef IB_DEVICE_HAVE_DRIVER_DEF
		ib_dev->driver_def = hihtr_uapi_defs;
#endif

#if defined(IB_VERBS_HAVE_IB_DEVICE_SET_NETDEV)
	for (i = 0; i < hr_dev->caps.num_ports; i++) {
		if (!hr_dev->iboe.netdevs[i])
			continue;
		ret = ib_device_set_netdev(ib_dev, hr_dev->iboe.netdevs[i],
					   i + 1);
		if (ret) {
			return ret;
		}
		dev_info(hr_dev->dev, "ib_device_set_netdev:ret = %d", ret);
	}
#endif
	dma_set_max_seg_size(dev, SZ_2G);

#if defined(IB_REG_DEV_HAVE_PORT_CALLBACK) && !defined(IB_REG_DEV_HAVE_NAME)
	strcpy(ib_dev->name, "hihtr_%d");
	ret = ib_register_device(ib_dev, NULL);
#elif defined(IB_REG_DEV_HAVE_PORT_CALLBACK)
	ret = ib_register_device(ib_dev, "hihtr_%d", NULL);
#elif defined(IB_REG_DEV_HAVE_DEVICE)
	ret = ib_register_device(ib_dev, "hihtr_%d", dev);
#else
	ret = ib_register_device(ib_dev, "hihtr_%d");
#endif
	if (ret) {
		dev_err(dev, "ib_register_device failed!\n");
		return ret;
	}

    memcpy(uld_dev_name, hr_dev->ib_dev.name, HIHTR_ULD_DEV_NAME_LEN);

	ret = hihtr_setup_mtu_mac(hr_dev);
	if (ret) {
		dev_err(dev, "setup_mtu_mac failed!\n");
		goto error_failed_setup_mtu_mac;
	}

	iboe->nb.notifier_call = hihtr_netdev_event;
	ret = register_netdevice_notifier(&iboe->nb);
	if (ret) {
		dev_err(dev, "register_netdevice_notifier failed!\n");
		goto error_failed_setup_mtu_mac;
	}

	hr_dev->active = true;
	return ret;

error_failed_setup_mtu_mac:
	ib_unregister_device(ib_dev);

	return ret;
}

static int hihtr_init_hem(struct hihtr_dev *hr_dev)
{
	struct device *dev = hr_dev->dev;
	int ret;

	ret = hihtr_init_hem_table(hr_dev, &hr_dev->mr_table.mtpt_table,
				      HEM_TYPE_MTPT, hr_dev->caps.mtpt_entry_sz,
				      hr_dev->caps.num_mtpts, 1);
	if (ret) {
		dev_err(dev, "Failed to init MTPT context memory, aborting.\n");
		return ret;
	}

	ret = hihtr_init_hem_table(hr_dev, &hr_dev->qp_table.qp_table,
				      HEM_TYPE_QPC, hr_dev->caps.qpc_sz,
				      hr_dev->caps.num_qps, 1);
	if (ret) {
		dev_err(dev, "Failed to init QP context memory, aborting.\n");
		goto err_unmap_dmpt;
	}

	ret = hihtr_init_hem_table(hr_dev, &hr_dev->qp_table.irrl_table,
				      HEM_TYPE_IRRL,
				      hr_dev->caps.irrl_entry_sz *
				      hr_dev->caps.max_qp_init_rdma,
				      hr_dev->caps.num_qps, 1);
	if (ret) {
		dev_err(dev, "Failed to init irrl_table memory, aborting.\n");
		goto err_unmap_qp;
	}

	if (hr_dev->caps.trrl_entry_sz) {
		ret = hihtr_init_hem_table(hr_dev,
					      &hr_dev->qp_table.trrl_table,
					      HEM_TYPE_TRRL,
					      hr_dev->caps.trrl_entry_sz *
					      hr_dev->caps.max_qp_dest_rdma,
					      hr_dev->caps.num_qps, 1);
		if (ret) {
			dev_err(dev,
				"Failed to init trrl_table memory, aborting.\n");
			goto err_unmap_irrl;
		}
	}

	ret = hihtr_init_hem_table(hr_dev, &hr_dev->cq_table.table,
				      HEM_TYPE_CQC, hr_dev->caps.cqc_entry_sz,
				      hr_dev->caps.num_cqs, 1);
	if (ret) {
		dev_err(dev, "Failed to init CQ context memory, aborting.\n");
		goto err_unmap_trrl;
	}

	if (hr_dev->caps.flags & HIHTR_CAP_FLAG_SRQ) {
		ret = hihtr_init_hem_table(hr_dev, &hr_dev->srq_table.table,
					      HEM_TYPE_SRQC,
					      hr_dev->caps.srqc_entry_sz,
					      hr_dev->caps.num_srqs, 1);
		if (ret) {
			dev_err(dev,
				"Failed to init SRQ context memory, aborting.\n");
			goto err_unmap_cq;
		}
	}

	ret = hihtr_init_hem_table(hr_dev,
						&hr_dev->qp_table.sccc_table,
						HEM_TYPE_SCCC,
						hr_dev->caps.sccc_sz,
						hr_dev->caps.num_qps, 1);
	if (ret) {
		dev_err(dev, "Failed to init SCC context memory, aborting.\n");
		goto err_unmap_srq;
	}

	if (hr_dev->caps.qpc_timer_entry_sz) {
		ret = hihtr_init_hem_table(hr_dev, &hr_dev->qpc_timer_table,
					      HEM_TYPE_QPC_TIMER,
					      hr_dev->caps.qpc_timer_entry_sz,
					      hr_dev->caps.qpc_timer_bt_num, 1);
		if (ret) {
			dev_err(dev,
				"Failed to init QPC timer memory, aborting.\n");
			goto err_unmap_ctx;
		}
	}

	if (hr_dev->caps.cqc_timer_entry_sz) {
		ret = hihtr_init_hem_table(hr_dev, &hr_dev->cqc_timer_table,
					      HEM_TYPE_CQC_TIMER,
					      hr_dev->caps.cqc_timer_entry_sz,
					      hr_dev->caps.cqc_timer_bt_num, 1);
		if (ret) {
			dev_err(dev,
				"Failed to init CQC timer memory, aborting.\n");
			goto err_unmap_qpc_timer;
		}
	}

	if (hr_dev->caps.gmv_entry_sz) {
		ret = hihtr_init_hem_table(hr_dev, &hr_dev->gmv_table,
					      HEM_TYPE_GMV,
					      hr_dev->caps.gmv_entry_sz,
					      hr_dev->caps.gmv_entry_num, 1);
		if (ret) {
			dev_err(dev,
				"failed to init gmv table memory, ret = %d\n",
				ret);
			goto err_unmap_cqc_timer;
		}
	}

	return 0;

err_unmap_cqc_timer:
	if (hr_dev->caps.cqc_timer_entry_sz)
		hihtr_cleanup_hem_table(hr_dev, &hr_dev->cqc_timer_table);

err_unmap_qpc_timer:
	if (hr_dev->caps.qpc_timer_entry_sz)
		hihtr_cleanup_hem_table(hr_dev, &hr_dev->qpc_timer_table);

err_unmap_ctx:
	hihtr_cleanup_hem_table(hr_dev, &hr_dev->qp_table.sccc_table);
err_unmap_srq:
	if (hr_dev->caps.flags & HIHTR_CAP_FLAG_SRQ)
		hihtr_cleanup_hem_table(hr_dev, &hr_dev->srq_table.table);

err_unmap_cq:
	hihtr_cleanup_hem_table(hr_dev, &hr_dev->cq_table.table);

err_unmap_trrl:
	if (hr_dev->caps.trrl_entry_sz)
		hihtr_cleanup_hem_table(hr_dev,
					   &hr_dev->qp_table.trrl_table);

err_unmap_irrl:
	hihtr_cleanup_hem_table(hr_dev, &hr_dev->qp_table.irrl_table);

err_unmap_qp:
	hihtr_cleanup_hem_table(hr_dev, &hr_dev->qp_table.qp_table);

err_unmap_dmpt:
	hihtr_cleanup_hem_table(hr_dev, &hr_dev->mr_table.mtpt_table);

	return ret;
}

static void hihtr_cleanup_bitmap(struct hihtr_dev *hr_dev)
{
	if (hr_dev->caps.flags & HIHTR_CAP_FLAG_XRC)
		ida_destroy(&hr_dev->xrcd_ida.ida);

	if (hr_dev->caps.flags & HIHTR_CAP_FLAG_SRQ)
		ida_destroy(&hr_dev->srq_table.srq_ida.ida);
	hihtr_cleanup_qp_table(hr_dev);
	hihtr_cleanup_cq_table(hr_dev);
	ida_destroy(&hr_dev->mr_table.mtpt_ida.ida);
	ida_destroy(&hr_dev->pd_ida.ida);
	ida_destroy(&hr_dev->uar_ida.ida);
}

static int hihtr_init_err_count(struct hihtr_dev *hr_dev)
{
    hr_dev->user_count_page = alloc_page(GFP_KERNEL | __GFP_ZERO);
    if (!hr_dev->user_count_page) {
        return -ENOMEM;
    }

    hr_dev->user_count_buf = vmap(&hr_dev->user_count_page, 1, VM_MAP, PAGE_KERNEL);
    if (!hr_dev->user_count_buf) {
        put_page(hr_dev->user_count_page);
        hr_dev->user_count_page = NULL;
        return -ENOMEM;
    }

	hr_dev->kernel_count_buf = kzalloc(sizeof(hihtr_history_error_count), GFP_KERNEL);
	if (!hr_dev->kernel_count_buf) {
        vunmap(hr_dev->user_count_buf);
        hr_dev->user_count_buf = NULL;
        put_page(hr_dev->user_count_page);
        hr_dev->user_count_page = NULL;
    	return -ENOMEM;
	}
	return 0;
}

static void hihtr_free_err_count(struct hihtr_dev *hr_dev)
{
    if (hr_dev->user_count_buf) {
        vunmap(hr_dev->user_count_buf);
        hr_dev->user_count_buf = NULL;
    }

    if (hr_dev->user_count_page) {
        put_page(hr_dev->user_count_page);
        hr_dev->user_count_page = NULL;
    }
	if (hr_dev->kernel_count_buf) {
		kfree(hr_dev->kernel_count_buf);
		hr_dev->kernel_count_buf = NULL;
	}
}

static void hihtr_teardown_hca(struct hihtr_dev *hr_dev)
{
	if (hr_dev->caps.flags & HIHTR_CAP_FLAG_DCA_MODE)
		hihtr_cleanup_dca(hr_dev);

	hihtr_cleanup_bitmap(hr_dev);
	hihtr_free_err_count(hr_dev);
}

/**
 * hihtr_setup_hca - setup host channel adapter
 * @hr_dev: pointer to hihtr roce device
 * Return : int
 */
static int hihtr_setup_hca(struct hihtr_dev *hr_dev)
{
	struct device *dev = hr_dev->dev;
	int ret;

	spin_lock_init(&hr_dev->sm_lock);

	INIT_LIST_HEAD(&hr_dev->qp_list);
	spin_lock_init(&hr_dev->qp_list_lock);
	INIT_LIST_HEAD(&hr_dev->dip_list);
	spin_lock_init(&hr_dev->dip_list_lock);

	INIT_LIST_HEAD(&hr_dev->uctx_list);
	spin_lock_init(&hr_dev->uctx_list_lock);

	if (((hr_dev->caps.flags & HIHTR_CAP_FLAG_CQ_RECORD_DB) ||
     	(hr_dev->caps.flags & HIHTR_CAP_FLAG_QP_RECORD_DB))) {
		INIT_LIST_HEAD(&hr_dev->pgdir_list);
		mutex_init(&hr_dev->pgdir_mutex);
	}

	hihtr_init_uar_table(hr_dev);

	ret = hihtr_uar_alloc(hr_dev, &hr_dev->priv_uar);
	if (ret) {
		dev_err(dev, "Failed to allocate priv_uar.\n");
		goto err_uar_table_free;
	}

	ret = hihtr_init_qp_table(hr_dev);
	if (ret) {
		dev_err(dev, "Failed to init qp_table.\n");
		goto err_uar_table_free;
	}

	ret = hihtr_init_err_count(hr_dev);
	if (ret) {
		dev_err(dev, "Failed to init err_count.\n");
		goto err_uar_table_free;
	}

	hihtr_init_pd_table(hr_dev);

	if (hr_dev->caps.flags & HIHTR_CAP_FLAG_XRC)
		hihtr_init_xrcd_table(hr_dev);

	hihtr_init_mr_table(hr_dev);

	hihtr_init_cq_table(hr_dev);

	if (hr_dev->caps.flags & HIHTR_CAP_FLAG_SRQ) {
		hihtr_init_srq_table(hr_dev);
	}

	if (hr_dev->caps.flags & HIHTR_CAP_FLAG_DCA_MODE)
		hihtr_init_dca(hr_dev);

	return 0;

err_uar_table_free:
	ida_destroy(&hr_dev->uar_ida.ida);
	return ret;
}

static void check_and_get_armed_cq(struct list_head *cq_list, struct ib_cq *cq)
{
	struct hihtr_cq *hr_cq = to_hr_cq(cq);
	unsigned long flags;

	spin_lock_irqsave(&hr_cq->lock, flags);
	if (cq->comp_handler) {
		if (!hr_cq->is_armed) {
			hr_cq->is_armed = 1;
			list_add_tail(&hr_cq->node, cq_list);
		}
	}
	spin_unlock_irqrestore(&hr_cq->lock, flags);
}

void hihtr_handle_device_err(struct hihtr_dev *hr_dev)
{
	struct hihtr_qp *hr_qp, *hr_qp_next;
	struct hihtr_cq *hr_cq;
	struct list_head cq_list;
	unsigned long flags_qp;
	unsigned long flags;

	INIT_LIST_HEAD(&cq_list);

	spin_lock_irqsave(&hr_dev->qp_list_lock, flags);
	list_for_each_entry_safe(hr_qp, hr_qp_next, &hr_dev->qp_list, node) {
		spin_lock_irqsave(&hr_qp->sq.lock, flags_qp);
		if (hr_qp->sq.tail != hr_qp->sq.head)
			check_and_get_armed_cq(&cq_list, hr_qp->ibqp.send_cq);
		spin_unlock_irqrestore(&hr_qp->sq.lock, flags_qp);

		spin_lock_irqsave(&hr_qp->rq.lock, flags_qp);
		if ((!hr_qp->ibqp.srq) && (hr_qp->rq.tail != hr_qp->rq.head))
			check_and_get_armed_cq(&cq_list, hr_qp->ibqp.recv_cq);
		spin_unlock_irqrestore(&hr_qp->rq.lock, flags_qp);
	}

	list_for_each_entry(hr_cq, &cq_list, node)
		hihtr_cq_completion(hr_dev, hr_cq->cqn);

	spin_unlock_irqrestore(&hr_dev->qp_list_lock, flags);
}

static int hihtr_alloc_dfx_cnt(struct hihtr_dev *hr_dev)
{
	hr_dev->dfx_cnt = kvcalloc(HIHTR_DFX_CNT_TOTAL, sizeof(atomic64_t),
				   GFP_KERNEL);
	if (!hr_dev->dfx_cnt)
		return -ENOMEM;

	return 0;
}

static void hihtr_dealloc_dfx_cnt(struct hihtr_dev *hr_dev)
{
	kvfree(hr_dev->dfx_cnt);
}

int hihtr_init(struct hihtr_dev *hr_dev, char *uld_dev_name)
{
	struct device *dev = hr_dev->dev;
	int ret;

	hr_dev->is_reset = false;

	ret = hihtr_alloc_dfx_cnt(hr_dev);
	if (ret) {
		dev_err(dev, "Alloc dfx_cnt failed!\n");
		return ret;
	}

	ret = hr_dev->hw->hw_profile(hr_dev);
	if (ret) {
		dev_err(dev, "Get RoCE engine profile failed!\n");
		goto error_failed_alloc_dfx_cnt;
	}

	ret = hihtr_cmd_init(hr_dev);
	if (ret) {
		dev_err(dev, "cmd init failed!\n");
		goto error_failed_alloc_dfx_cnt;
	}

	/* EQ depends on poll mode, event mode depends on EQ */
	ret = hr_dev->hw->init_eq(hr_dev);
	if (ret) {
		dev_err(dev, "eq init failed!\n");
		goto error_failed_eq_table;
	}

	ret = hihtr_init_hem(hr_dev);
	if (ret) {
		dev_err(dev, "init HEM(Hardware Entry Memory) failed!\n");
		goto error_failed_init_hem;
	}

	ret = hihtr_setup_hca(hr_dev);
	if (ret) {
		dev_err(dev, "setup hca failed!\n");
		goto error_failed_setup_hca;
	}

	if (hr_dev->hw->hw_init) {
		ret = hr_dev->hw->hw_init(hr_dev);
		if (ret) {
			dev_err(dev, "hw_init failed!\n");
			goto error_failed_engine_init;
		}
	}

	ret = hihtr_register_device(hr_dev, uld_dev_name);
	if (ret) {
		dev_err(dev, "ERR hihtr_hw_v3_dev_init");
		goto error_failed_register_device;
	}

	hihtr_register_debugfs(hr_dev);

	(void)hihtr_register_sysfs(hr_dev);

	hihtr_init_mcast_gmv_table(hr_dev);

	/* config RoCE bypass via MPU */
	ret = hihtr_bypass_func_cnt_change(hr_dev, HIHTR_FUNC_CNT_ADD);
	if (ret) {
		goto error_failed_register_device;
	}
	hr_dev->pid = current->pid;
	hihtr_alloc_dwqe_resource(hr_dev, NULL);

	ret = hihtr_set_order(hr_dev);
	if (ret) {
		dev_err(dev, "hihtr_set_order failed!\n");
	}

	return 0;

error_failed_register_device:
	if (hr_dev->hw->hw_exit)
		hr_dev->hw->hw_exit(hr_dev);

error_failed_engine_init:
	hihtr_teardown_hca(hr_dev);

error_failed_setup_hca:
	hihtr_cleanup_hem(hr_dev);

error_failed_init_hem:
	hr_dev->hw->cleanup_eq(hr_dev);

error_failed_eq_table:
	hihtr_cmd_cleanup(hr_dev);

error_failed_alloc_dfx_cnt:
	hihtr_dealloc_dfx_cnt(hr_dev);
	return ret;
}

void hihtr_exit(struct hihtr_dev *hr_dev)
{
	int ret;

	hihtr_release_dwqe_resource(hr_dev, NULL);
	/* config RoCE bypass via MPU */
	ret = hihtr_bypass_func_cnt_change(hr_dev, HIHTR_FUNC_CNT_REDUCE);
	if (ret) {
		dev_err(hr_dev->dev, "hihtr config bypass failed!\n");
	}

	hihtr_unregister_sysfs(hr_dev);
	hihtr_unregister_device(hr_dev);

	hihtr_unregister_debugfs(hr_dev);

	if (hr_dev->hw->hw_exit)
		hr_dev->hw->hw_exit(hr_dev);

	hihtr_teardown_hca(hr_dev);
	hihtr_cleanup_hem(hr_dev);

	hr_dev->hw->cleanup_eq(hr_dev);
	hihtr_cmd_cleanup(hr_dev);

	hihtr_dealloc_dfx_cnt(hr_dev);

	hihtr_destroy_mcast_gmv_table(hr_dev);
}

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
MODULE_DESCRIPTION("Intelligent RoCE driver for Huawei NIC");
MODULE_VERSION(HIHTR_DRV_VERSION);