/*
 * Copyright (c) 2025 Hisilicon Limited. All rights reserved.
 */

#include <linux/types.h>
#include <linux/random.h>
#include <linux/mm.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/if_vlan.h>
#include <uapi/asm-generic/errno-base.h>
#include <rdma/ib_umem.h>
#include "hihtr_device.h"
#include "hihtr_compat.h"

#ifndef ibdev_dbg
#define ibdev_dbg(ibdev, fmt, ...) \
    dev_dbg(&(ibdev)->dev, fmt, ##__VA_ARGS__)
#endif

#if !defined(MM_HAVE_VM_INSERT_PAGES)
int vm_insert_pages(struct vm_area_struct *vma, unsigned long addr,
			struct page **pages, unsigned long *num)
{
    unsigned long idx = 0, pgcount = *num;
	int err = -EINVAL;

	for (; idx < pgcount; ++idx) {
		err = vm_insert_page(vma, addr + (PAGE_SIZE * idx), pages[idx]);
		if (err)
			break;
	}
	*num = pgcount - idx;
	return err;
}
#endif

#if defined(BITMAP_NEED_BITMAP_ZALLOC)
unsigned long *bitmap_alloc(unsigned int nbits, gfp_t flags)
{
	return kmalloc_array(BITS_TO_LONGS(nbits), sizeof(unsigned long),
			     flags);
}

unsigned long *bitmap_zalloc(unsigned int nbits, gfp_t flags)
{
	return bitmap_alloc(nbits, flags | __GFP_ZERO);
}

void bitmap_free(unsigned long *bitmap)
{
	kfree(bitmap);
}
#endif

#if defined(IB_VERBS_NEED_IB_BLOCK_ITER)
void __rdma_block_iter_start(struct ib_block_iter *biter,
			     struct scatterlist *sglist, unsigned int nents,
			     unsigned long pgsz)
{
	memset(biter, 0, sizeof(struct ib_block_iter));
	biter->__sg = sglist;
	biter->__sg_nents = nents;

	/* Driver provides best block size to use */
	biter->__pg_bit = __fls(pgsz);
}

bool __rdma_block_iter_next(struct ib_block_iter *biter)
{
	unsigned int block_offset;
	unsigned int sg_delta;

	if (!biter->__sg_nents || !biter->__sg)
		return false;

	biter->__dma_addr = sg_dma_address(biter->__sg) + biter->__sg_advance;
	block_offset = biter->__dma_addr & (BIT_ULL(biter->__pg_bit) - 1);
	sg_delta = BIT_ULL(biter->__pg_bit) - block_offset;
	if (sg_dma_len(biter->__sg) - biter->__sg_advance > sg_delta) {
		biter->__sg_advance += sg_delta;
	} else {
		biter->__sg_advance = 0;
		biter->__sg = sg_next(biter->__sg);
		biter->__sg_nents--;
	}

	return true;
}
#endif

#if defined(SYSFS_NEED_SYSFS_EMIT)
int sysfs_emit(char *buf, const char *fmt, ...)
{
	va_list args;
	int len;

	if (WARN(!buf || offset_in_page(buf),
		 "invalid sysfs_emit: buf:%p\n", buf))
		return 0;

	va_start(args, fmt);
	len = vscnprintf(buf, PAGE_SIZE, fmt, args);
	va_end(args);

	return len;
}
#endif

#if defined(IDR_NEED_IDA_ALLOC_RANGE)
inline int ida_alloc_max(struct ida *ida, unsigned int max, gfp_t gfp)
{
	return ida_alloc_range(ida, 0, max, gfp);
}
#endif

#if defined(IB_CACHE_NEED_RDMA_READ_GID_L2_FIELDS)
static int get_lower_dev_vlan(struct net_device *lower_dev, void *data)
{
	u16 *vlan_id = (u16 *)data;

	if (is_vlan_dev(lower_dev))
		*vlan_id = vlan_dev_vlan_id(lower_dev);

	return 1;
}

int hihtr_read_gid_l2_fields(const struct ib_gid_attr *attr, u16 *vlan_id, u8 *smac)
{
	struct net_device *ndev;

	rcu_read_lock();
	ndev = rcu_dereference(attr->ndev);
	if (!ndev) {
		rcu_read_unlock();
		return -ENODEV;
	}
	if (smac)
		ether_addr_copy(smac, ndev->dev_addr);
	if (vlan_id) {
		*vlan_id = 0xffff;
		if (is_vlan_dev(ndev)) {
			*vlan_id = vlan_dev_vlan_id(ndev);
		} else {
			/* If the netdev is upper device and if it's lower
			 * device is vlan device, consider vlan id of the
			 * the lower vlan device for this gid entry.
			 */
			netdev_walk_all_lower_dev_rcu(attr->ndev,
					get_lower_dev_vlan, vlan_id);
		}
	}
	rcu_read_unlock();
	return 0;
}
#endif

#if defined(IB_UMEM_NEED_IB_UMEM_FIND_BEST_PGSZ)
static inline unsigned int rdma_find_pg_bit(unsigned long addr,
					    unsigned long pgsz_bitmap)
{
	unsigned long align;
	unsigned long pgsz;

	align = addr & -addr;

	pgsz = pgsz_bitmap & ~(-align << 1);
	if (!pgsz)
		return __ffs(pgsz_bitmap);

	return __fls(pgsz);
}

unsigned long ib_umem_find_best_pgsz(struct ib_umem *umem,
				     unsigned long pgsz_bitmap,
				     unsigned long virt)
{
	struct scatterlist *sg;
	unsigned int best_pg_bit;
	unsigned long va, pgoff;
	dma_addr_t mask;
	int i;

	if (WARN_ON(!(pgsz_bitmap & GENMASK(PAGE_SHIFT, 0))))
		return 0;

	va = virt;
	mask = roundup_pow_of_two(umem->length);
	pgoff = umem->address & ~PAGE_MASK;

	for_each_sg(umem->sg_head.sgl, sg, umem->nmap, i) {
		mask |= (sg_dma_address(sg) + pgoff) ^ va;
		if (i && i != (umem->nmap - 1))
			mask |= sg_dma_len(sg);
		va += sg_dma_len(sg) - pgoff;
		pgoff = 0;
	}
	best_pg_bit = rdma_find_pg_bit(mask, pgsz_bitmap);

	return BIT_ULL(best_pg_bit);
}
#endif

#if defined(IB_VERBS_NEED_RDMA_USER_MMAP_ENTRY)
void rdma_user_mmap_entry_put(struct rdma_user_mmap_entry *entry)
{
	kref_put(&entry->ref, rdma_user_mmap_entry_free);
}

#if defined(RDMA_HAVE_XARRAY)
struct rdma_user_mmap_entry *rdma_user_mmap_entry_get_pgoff(struct ib_ucontext *ucontext, unsigned long pgoff)
{
	struct xarray *mmap_xa = HIHTR_GET_MMAP_XA(ucontext);
	struct rdma_user_mmap_entry *entry;
	if (pgoff > U32_MAX)
		return NULL;

	xa_lock(mmap_xa);

	entry = xa_load(mmap_xa, pgoff);
	if (!entry || entry->start_pgoff != pgoff || entry->driver_removed ||
	    !kref_get_unless_zero(&entry->ref))
		goto err;

	xa_unlock(mmap_xa);

	ibdev_dbg(ucontext->device, "mmap: pgoff[%#lx] npages[%#zx] returned\n",
		  pgoff, entry->npages);

	return entry;

err:
	xa_unlock(mmap_xa);
	return NULL;
}

void rdma_user_mmap_entry_free(struct kref *kref)
{
	struct rdma_user_mmap_entry *rdma_entry =
		container_of(kref, struct rdma_user_mmap_entry, ref);
    struct xarray *mmap_xa = HIHTR_GET_MMAP_XA(rdma_entry->ucontext);
	unsigned long i;
    struct hihtr_user_mmap_entry *entry;

	xa_lock(mmap_xa);
	for (i = 0; i < rdma_entry->npages; i++)
		__xa_erase(mmap_xa, rdma_entry->start_pgoff + i);
	xa_unlock(mmap_xa);

	ibdev_dbg(rdma_entry->ucontext->device, "mmap: pgoff[%#lx] npages[%#zx] removed\n",
		  rdma_entry->start_pgoff, rdma_entry->npages);

    entry = to_hihtr_mmap(rdma_entry);
	kfree(entry);
}

void rdma_user_mmap_entry_remove(struct rdma_user_mmap_entry *entry)
{
	struct xarray *mmap_xa;
	if (!entry)
		return;

	mmap_xa = HIHTR_GET_MMAP_XA(entry->ucontext);
	xa_lock(mmap_xa);
	entry->driver_removed = true;
	xa_unlock(mmap_xa);
	kref_put(&entry->ref, rdma_user_mmap_entry_free);
}
#else
struct rdma_user_mmap_entry *rdma_user_mmap_entry_get_pgoff(struct ib_ucontext *ucontext, unsigned long pgoff)
{
	struct hihtr_ucontext *hr_uctx = to_hr_ucontext(ucontext);
	struct rdma_user_mmap_entry *entry;
	unsigned long flags;

	if (pgoff > U32_MAX)
		return NULL;

	spin_lock_irqsave(&hr_uctx->tree_lock, flags);

	entry = radix_tree_lookup(&hr_uctx->root, pgoff);
	if (!entry || entry->start_pgoff != pgoff || entry->driver_removed ||
	    !kref_get_unless_zero(&entry->ref))
		goto err;

	spin_unlock_irqrestore(&hr_uctx->tree_lock, flags);

	dev_dbg(&ucontext->device->dev, "mmap: pgoff[%#lx] npages[%#zx] returned\n",
		  pgoff, entry->npages);

	return entry;

err:
	spin_unlock_irqrestore(&hr_uctx->tree_lock, flags);
	return NULL;
}

void rdma_user_mmap_entry_free(struct kref *kref)
{
	struct rdma_user_mmap_entry *entry = container_of(kref, struct rdma_user_mmap_entry, ref);
	struct hihtr_ucontext *hr_uctx = to_hr_ucontext(entry->ucontext);
	unsigned long i;
	unsigned long flags;

	spin_lock_irqsave(&hr_uctx->tree_lock, flags);
	for (i = 0; i < entry->npages; i++)
		radix_tree_delete(&hr_uctx->root, entry->start_pgoff + i);
	spin_unlock_irqrestore(&hr_uctx->tree_lock, flags);

	pr_debug("mmap: pgoff[%#lx] npages[%#zx] removed\n", entry->start_pgoff, entry->npages);

	kfree(entry);
}

void rdma_user_mmap_entry_remove(struct rdma_user_mmap_entry *entry)
{
	struct hihtr_ucontext *hr_uctx = to_hr_ucontext(entry->ucontext);
	unsigned long flags;

	if (!entry)
		return;

	spin_lock_irqsave(&hr_uctx->tree_lock, flags);
	entry->driver_removed = true;
	spin_unlock_irqrestore(&hr_uctx->tree_lock, flags);
	kref_put(&entry->ref, rdma_user_mmap_entry_free);
}
#endif
#endif /* IB_VERBS_NEED_RDMA_USER_MMAP_ENTRY */

#if defined(IB_VERBS_NEED_RDMA_USER_MMAP_ENTRY_INSERT_RANGE)
#if defined(RDMA_HAVE_XARRAY)
int rdma_user_mmap_entry_insert_range(struct ib_ucontext *uctx,
				      struct rdma_user_mmap_entry *entry,
				      size_t length, u32 min_pgoff,
				      u32 max_pgoff)
{
	struct xarray *mmap_xa = HIHTR_GET_MMAP_XA(uctx);
	XA_STATE(xas, mmap_xa, min_pgoff);
	u32 i, xa_first, xa_last, npages;
	int err;

	if (!entry) {
		ibdev_err(uctx->device, "mmap insert failed: entry is NULL\n");
		return -EINVAL;
	}

	entry->ucontext = uctx;
	kref_init(&entry->ref);

	xa_lock(mmap_xa);

	npages = (u32)DIV_ROUND_UP(length, PAGE_SIZE);
	entry->npages = npages;

	while (true) {
		xas_find_marked(&xas, max_pgoff, XA_FREE_MARK);
		if (xas.xa_node == XAS_RESTART)
			goto err_unlock;

		xa_first = xas.xa_index;

		if (check_add_overflow(xa_first, npages, &xa_last))
			goto err_unlock;

		xas_next_entry(&xas, xa_last - 1);
		if (xas.xa_node == XAS_BOUNDS || xas.xa_index >= xa_last)
			break;
	}

	for (i = xa_first; i < xa_last; i++) {
		err = __xa_insert(mmap_xa, i, entry, GFP_KERNEL);
		if (err)
			goto err_undo;
	}

	entry->start_pgoff = xa_first;
	xa_unlock(mmap_xa);

	ibdev_dbg(uctx->device, "mmap: pgoff[%#lx] npages[%#x] inserted\n",
		  entry->start_pgoff, npages);

	return 0;

err_undo:
	for (; i > xa_first; i--)
		__xa_erase(mmap_xa, i - 1);

err_unlock:
	xa_unlock(mmap_xa);
	return -ENOMEM;
}
#else
int rdma_user_mmap_entry_insert_range(struct ib_ucontext *ucontext, struct rdma_user_mmap_entry *entry,
									  size_t length, u32 min_pgoff, u32 max_pgoff)
{
	struct hihtr_ucontext *hr_uctx = to_hr_ucontext(ucontext);
	u32 tree_root_first = min_pgoff, tree_root_last = 0, npages;
	int err;
	u32 i;
	unsigned long flags;
	unsigned long pos;
	struct radix_tree_iter iter;
	void **slot;
	bool all_free;

	if (!entry)
		return -EINVAL;

	kref_init(&entry->ref);
	entry->ucontext = ucontext;

	spin_lock_irqsave(&hr_uctx->tree_lock, flags);

	npages = (u32)DIV_ROUND_UP(length, PAGE_SIZE);
	entry->npages = npages;
	while (true) {
		radix_tree_for_each_slot(slot, &hr_uctx->root, &iter, tree_root_first) {
			pos = iter.index;
			if (pos > max_pgoff)
				goto err_unlock;

			if (pos >= tree_root_first) {
				break;
			}
		}

		// 计算可能的空闲范围
		tree_root_last = tree_root_first + npages;

		if (tree_root_last - 1 <= max_pgoff) {
			all_free = true;

			for (i = tree_root_first; i < tree_root_last; i++) {
				if (radix_tree_lookup(&hr_uctx->root, i))
					all_free = false;
			}

			if (all_free)
				break;
		} else {
			goto err_unlock;
		}

		if (tree_root_first > max_pgoff + 1 - npages)
			goto err_unlock;

		tree_root_first = iter.index + 1;
	}

	for (i = tree_root_first; i < tree_root_last; i++) {
		err = radix_tree_insert(&hr_uctx->root, i, entry);
		if (err)
			goto err_undo;
	}

	entry->start_pgoff = tree_root_first;
	spin_unlock_irqrestore(&hr_uctx->tree_lock, flags);

	dev_dbg(&ucontext->device->dev, "mmap: pgoff[%#lx] npages[%#x] inserted\n",
		  entry->start_pgoff, npages);

	return 0;

err_undo:
	for (; i > tree_root_first; i--)
		radix_tree_delete(&hr_uctx->root, i - 1);

err_unlock:
	spin_unlock_irqrestore(&hr_uctx->tree_lock, flags);
	return -ENOMEM;
}
#endif
#endif /* IB_VERBS_NEED_RDMA_USER_MMAP_ENTRY_INSERT_RANGE */

#if !defined(RDMA_HAVE_IB_DEVICE_OPS)
void ib_set_device_ops(struct ib_device *dev, const struct ib_device *ops)
{
#define SET_DEVICE_OP(ptr, name)                                               \
	do {                                                                   \
		if (ops->name)                                                 \
			if (!((ptr)->name))				       \
				(ptr)->name = ops->name;                       \
	} while (0)

	if (ops->driver_id != RDMA_DRIVER_UNKNOWN) {
		WARN_ON(dev->driver_id != RDMA_DRIVER_UNKNOWN &&
			dev->driver_id != ops->driver_id);
		dev->driver_id = ops->driver_id;
	}

	if (ops->owner) {
		WARN_ON(dev->owner && dev->owner != ops->owner);
		dev->owner = ops->owner;
	}

	if (ops->uverbs_abi_ver)
		dev->uverbs_abi_ver = ops->uverbs_abi_ver;

	SET_DEVICE_OP(dev, add_gid);
	SET_DEVICE_OP(dev, alloc_dm);
	SET_DEVICE_OP(dev, alloc_hw_stats);
	SET_DEVICE_OP(dev, alloc_mr);
	SET_DEVICE_OP(dev, alloc_mw);
	SET_DEVICE_OP(dev, alloc_pd);
	SET_DEVICE_OP(dev, alloc_rdma_netdev);
	SET_DEVICE_OP(dev, alloc_ucontext);
	SET_DEVICE_OP(dev, alloc_xrcd);
	SET_DEVICE_OP(dev, attach_mcast);
	SET_DEVICE_OP(dev, check_mr_status);
	SET_DEVICE_OP(dev, create_ah);
	SET_DEVICE_OP(dev, create_counters);
	SET_DEVICE_OP(dev, create_cq);
	SET_DEVICE_OP(dev, create_flow);
	SET_DEVICE_OP(dev, create_flow_action_esp);
	SET_DEVICE_OP(dev, create_qp);
	SET_DEVICE_OP(dev, create_rwq_ind_table);
	SET_DEVICE_OP(dev, create_srq);
	SET_DEVICE_OP(dev, create_wq);
	SET_DEVICE_OP(dev, dealloc_dm);
	SET_DEVICE_OP(dev, dealloc_mw);
	SET_DEVICE_OP(dev, dealloc_pd);
	SET_DEVICE_OP(dev, dealloc_ucontext);
	SET_DEVICE_OP(dev, dealloc_xrcd);
	SET_DEVICE_OP(dev, del_gid);
	SET_DEVICE_OP(dev, dereg_mr);
	SET_DEVICE_OP(dev, destroy_ah);
	SET_DEVICE_OP(dev, destroy_counters);
	SET_DEVICE_OP(dev, destroy_cq);
	SET_DEVICE_OP(dev, destroy_flow);
	SET_DEVICE_OP(dev, destroy_flow_action);
	SET_DEVICE_OP(dev, destroy_qp);
	SET_DEVICE_OP(dev, destroy_rwq_ind_table);
	SET_DEVICE_OP(dev, destroy_srq);
	SET_DEVICE_OP(dev, destroy_wq);
	SET_DEVICE_OP(dev, detach_mcast);
	SET_DEVICE_OP(dev, disassociate_ucontext);
	SET_DEVICE_OP(dev, drain_rq);
	SET_DEVICE_OP(dev, drain_sq);
	SET_DEVICE_OP(dev, get_dev_fw_str);
	SET_DEVICE_OP(dev, get_dma_mr);
	SET_DEVICE_OP(dev, get_hw_stats);
	SET_DEVICE_OP(dev, get_link_layer);
	SET_DEVICE_OP(dev, get_netdev);
	SET_DEVICE_OP(dev, get_port_immutable);
	SET_DEVICE_OP(dev, get_vector_affinity);
	SET_DEVICE_OP(dev, get_vf_config);
	SET_DEVICE_OP(dev, get_vf_stats);
	SET_DEVICE_OP(dev, map_mr_sg);
	SET_DEVICE_OP(dev, mmap);
	SET_DEVICE_OP(dev, modify_ah);
	SET_DEVICE_OP(dev, modify_cq);
	SET_DEVICE_OP(dev, modify_device);
	SET_DEVICE_OP(dev, modify_flow_action_esp);
	SET_DEVICE_OP(dev, modify_port);
	SET_DEVICE_OP(dev, modify_qp);
	SET_DEVICE_OP(dev, modify_srq);
	SET_DEVICE_OP(dev, modify_wq);
	SET_DEVICE_OP(dev, peek_cq);
	SET_DEVICE_OP(dev, poll_cq);
	SET_DEVICE_OP(dev, post_recv);
	SET_DEVICE_OP(dev, post_send);
	SET_DEVICE_OP(dev, post_srq_recv);
	SET_DEVICE_OP(dev, process_mad);
	SET_DEVICE_OP(dev, query_ah);
	SET_DEVICE_OP(dev, query_device);
	SET_DEVICE_OP(dev, query_gid);
	SET_DEVICE_OP(dev, query_pkey);
	SET_DEVICE_OP(dev, query_port);
	SET_DEVICE_OP(dev, query_qp);
	SET_DEVICE_OP(dev, query_srq);
#ifdef IB_VERBS_HAVE_RDMA_NETDEV_GET_PARAMS
	SET_DEVICE_OP(dev, rdma_netdev_get_params);
#endif
	SET_DEVICE_OP(dev, read_counters);
	SET_DEVICE_OP(dev, reg_dm_mr);
	SET_DEVICE_OP(dev, reg_user_mr);
	SET_DEVICE_OP(dev, req_ncomp_notif);
	SET_DEVICE_OP(dev, req_notify_cq);
	SET_DEVICE_OP(dev, rereg_user_mr);
	SET_DEVICE_OP(dev, resize_cq);
	SET_DEVICE_OP(dev, set_vf_guid);
	SET_DEVICE_OP(dev, set_vf_link_state);
}
#endif
