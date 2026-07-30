/*
 * Copyright (c) 2025 Hisilicon Limited. All rights reserved.
 */

#ifndef _HIHTR_COMPAT_H
#define _HIHTR_COMPAT_H

#include <linux/version.h>
#include <linux/kref.h>
#include <linux/mm_types.h>
#include <linux/radix-tree.h>
#include <linux/netdevice.h>
#include <linux/log2.h>
#include <rdma/ib_umem.h>
#include <rdma/ib_verbs.h>
#include <rdma/uverbs_ioctl.h>
#include "roce_kcompat_gen.h"

#if defined(RANDOM_NEED_GET_RANDOM_U32)
#define get_random_u32() ((u32)get_random_int())
#endif

#if defined(IB_VERBS_NEED_VALID_PORT_MIN)
#define IB_ROCE_UDP_ENCAP_VALID_PORT_MIN (0xC000)
#define IB_GRH_FLOWLABEL_MASK (0x000FFFFF)
#endif

#if defined(IB_VERBS_NEED_VALID_PORT_MAX)
#define IB_ROCE_UDP_ENCAP_VALID_PORT_MAX (0xFFFF)
#endif

#if !defined(ATTRIBUTES_HAVE_FALLTHROUGH)
#define fallthrough __attribute__((__fallthrough__))
#endif

#if defined(IB_VERBS_NEED_IBDEV_LOG)
#define ibdev_err(ibdev, format, args...)	dev_err(&(ibdev)->dev, format, ##args)
#define ibdev_warn(ibdev, format, args...)	dev_warn(&(ibdev)->dev, format, ##args)
#define ibdev_info(ibdev, format, args...)	dev_info(&(ibdev)->dev, format, ##args)
#endif

#if defined(IB_VERBS_NEED_IBDEV_RATELIMITED_LOG)
#define ibdev_err_ratelimited(ibdev, format, args...) dev_err_ratelimited(&(ibdev)->dev, format, ##args)
#endif

#if defined(IDR_NEED_IDA_ALLOC_RANGE)
#define ida_alloc_range(ida, min, max, GFP_KERNEL) ida_simple_get(ida, min, (max) + 1, GFP_KERNEL)
#define ida_free(ida, id) ida_simple_remove(ida, id)

inline int ida_alloc_max(struct ida *ida, unsigned int max, gfp_t gfp);
#endif

#if defined(IB_CHECK_MR_ACCESS_HAVE_2PARAM)
#define hihtr_check_mr_access(ib_dev, flags) ib_check_mr_access(ib_dev, (unsigned int)(flags))
#else
#define hihtr_check_mr_access(ib_dev, flags) ib_check_mr_access(flags)
#endif

#if defined(LOG2_NEED_BITS_PER)
#define bits_per(n)				\
(						\
	__builtin_constant_p(n) ? (		\
		((n) == 0 || (n) == 1)		\
			? 1 : ilog2(n) + 1	\
	) :					\
	__bits_per(n)				\
)

static inline __attribute__((const)) int __bits_per(unsigned long n)
{
	if (n < 2)
		return 1;
	if (is_power_of_2(n))
		return order_base_2(n) + 1;
	return order_base_2(n);
}
#endif

#if defined(IB_VERBS_NEED_IB_PORT_PHYS_STATE)
enum ib_port_phys_state {
	IB_PORT_PHYS_STATE_SLEEP = 1,
	IB_PORT_PHYS_STATE_POLLING = 2,
	IB_PORT_PHYS_STATE_DISABLED = 3,
	IB_PORT_PHYS_STATE_PORT_CONFIGURATION_TRAINING = 4,
	IB_PORT_PHYS_STATE_LINK_UP = 5,
	IB_PORT_PHYS_STATE_LINK_ERROR_RECOVERY = 6,
	IB_PORT_PHYS_STATE_PHY_TEST = 7,
};
#endif

#if defined(IOCTL_VERBS_NEED_IB_UVERBS_GID_TYPE)
enum ib_uverbs_gid_type {
	IB_UVERBS_GID_TYPE_IB,
	IB_UVERBS_GID_TYPE_ROCE_V1,
	IB_UVERBS_GID_TYPE_ROCE_V2,
};
#endif

#if defined(IB_VERBS_NEED_IB_BLOCK_ITER)
struct ib_block_iter {
	/* internal states */
	struct scatterlist *__sg; /* sg holding the current aligned block */
	dma_addr_t __dma_addr; /* unaligned DMA address of this block */
	unsigned int __sg_nents; /* number of SG entries */
	unsigned int __sg_advance; /* number of bytes to advance in sg in next step */
	unsigned int __pg_bit; /* alignment of current block */
};

#define rdma_for_each_block(sglist, biter, nents, pgsz)		\
	for (__rdma_block_iter_start(biter, sglist, nents,	\
				     pgsz);			\
	     __rdma_block_iter_next(biter);)

void __rdma_block_iter_start(struct ib_block_iter *biter, struct scatterlist *sglist, unsigned int nents,
			     			 unsigned long pgsz);
bool __rdma_block_iter_next(struct ib_block_iter *biter);
static inline dma_addr_t rdma_block_iter_dma_address(struct ib_block_iter *biter)
{
	return biter->__dma_addr & ~(BIT_ULL(biter->__pg_bit) - 1);
}
#endif

#if !defined(HAVE_RDMA_UMEM_FOR_EACH_DMA_BLOCK)
#define rdma_umem_for_each_dma_block(umem, biter, pgsz)                        \
	for (__rdma_umem_block_iter_start(biter, umem, pgsz);                  \
	     __rdma_block_iter_next(biter);)

static inline void __rdma_umem_block_iter_start(struct ib_block_iter *biter,
						struct ib_umem *umem,
						unsigned long pgsz)
{
	__rdma_block_iter_start(biter, umem->sg_head.sgl, umem->nmap, pgsz);
}
#endif

#if defined(BITOPS_NEED_BITS_TO_BYTES)
#define BITS_TO_BYTES(nr)	DIV_ROUND_UP(nr, BITS_PER_TYPE(char))
#endif

#if defined(RDMA_HAVE_XARRAY)
#if defined(IB_UCONTEXT_HAVE_MMAP_XA)
/* ib_ucontext has mmap_xa, use it directly */
#define HIHTR_GET_MMAP_XA(uctx) (&(uctx)->mmap_xa)
#else
/* ib_ucontext does NOT have mmap_xa, use hihtr_ucontext's */
#define HIHTR_GET_MMAP_XA(uctx) (&to_hr_ucontext(uctx)->mmap_xa)
#endif
#endif /* RDMA_HAVE_XARRAY */

#if defined(IB_VERBS_NEED_RDMA_USER_MMAP_ENTRY)
struct rdma_user_mmap_entry {
	struct ib_ucontext *ucontext;
	struct kref ref;
	size_t npages;
	unsigned long start_pgoff;
	bool driver_removed;
};

void rdma_user_mmap_entry_remove(struct rdma_user_mmap_entry *entry);
struct rdma_user_mmap_entry *
rdma_user_mmap_entry_get_pgoff(struct ib_ucontext *ucontext, unsigned long pgoff);
void rdma_user_mmap_entry_put(struct rdma_user_mmap_entry *entry);
void rdma_user_mmap_entry_free(struct kref *kref);

static inline u64 rdma_user_mmap_get_offset(const struct rdma_user_mmap_entry *entry)
{
	return (u64)entry->start_pgoff << PAGE_SHIFT;
}
#endif

#if defined(IB_VERBS_NEED_RDMA_USER_MMAP_ENTRY_INSERT_RANGE)
int rdma_user_mmap_entry_insert_range(struct ib_ucontext *ucontext, struct rdma_user_mmap_entry *entry,
				      				  size_t length, u32 min_pgoff, u32 max_pgoff);
#endif

#if defined(IB_VERBS_NEED_RDMA_USER_MMAP_ENTRY_INSERT_EXACT)
static inline int rdma_user_mmap_entry_insert_exact(struct ib_ucontext *ucontext, struct rdma_user_mmap_entry *entry,
				  							 size_t length, u32 pgoff)
{
	return rdma_user_mmap_entry_insert_range(ucontext, entry, length, pgoff, pgoff);
}
#endif

#if defined(IB_VERBS_NEED_RDMA_FLOW_LABEL_TO_UDP_SPORT)
static inline u16 rdma_flow_label_to_udp_sport(u32 fl)
{
	u32 fl_low = fl & 0x03fff, fl_high = fl & 0xFC000;

	fl_low ^= fl_high >> 14;
	return (u16)(fl_low | IB_ROCE_UDP_ENCAP_VALID_PORT_MIN);
}

static inline u32 rdma_calc_flow_label(u32 lqpn, u32 rqpn)
{
	u64 v = (u64)lqpn * rqpn;

	v ^= v >> 20;
	v ^= v >> 40;

	return (u32)(v & IB_GRH_FLOWLABEL_MASK);
}
#endif

#if defined(IB_VERBS_NEED_RDMA_GET_UDP_SPORT)
static inline u16 rdma_get_udp_sport(u32 fl, u32 lqpn, u32 rqpn)
{
	if (!fl)
		fl = rdma_calc_flow_label(lqpn, rqpn);

	return rdma_flow_label_to_udp_sport(fl);
}
#endif

#if defined(LIST_NEED_LIST_DEL_INIT_CAREFUL)
static inline void list_del_init_careful(struct list_head *entry)
{
	__list_del_entry(entry);
	entry->prev = entry;
	smp_store_release(&entry->next, entry);
}
#endif

#if !defined(HAVE_IB_UMEM_NUM_DMA_BLOCKS)
static inline size_t ib_umem_num_dma_blocks(struct ib_umem *umem, unsigned long pgsz)
{
	return (size_t)((ALIGN(umem->address + umem->length, pgsz) -
			 ALIGN_DOWN(umem->address, pgsz))) / pgsz;
}
#endif

#if defined(BITMAP_NEED_BITMAP_ZALLOC)
unsigned long *bitmap_alloc(unsigned int nbits, gfp_t flags);
unsigned long *bitmap_zalloc(unsigned int nbits, gfp_t flags);
void bitmap_free(unsigned long *bitmap);
#endif

#if !defined(MM_HAVE_VM_INSERT_PAGES)
int vm_insert_pages(struct vm_area_struct *vma, unsigned long addr,
			struct page **pages, unsigned long *num);
#endif

#if defined(SYSFS_NEED_SYSFS_EMIT)
int sysfs_emit(char *buf, const char *fmt, ...);
#endif

#if defined(IB_UMEM_NEED_IB_UMEM_FIND_BEST_PGSZ)
unsigned long ib_umem_find_best_pgsz(struct ib_umem *umem, unsigned long pgsz_bitmap, unsigned long virt);
#endif

#if defined(IB_CACHE_NEED_RDMA_READ_GID_L2_FIELDS)
int hihtr_read_gid_l2_fields(const struct ib_gid_attr *attr, u16 *vlan_id, u8 *smac);
#define rdma_read_gid_l2_fields(attr, vlan_id, smac) hihtr_read_gid_l2_fields(attr, vlan_id, smac)
#endif

#if !defined(RDMA_HAVE_IB_DEVICE_OPS)
void ib_set_device_ops(struct ib_device *dev, const struct ib_device *ops);
#endif

/* ib_umem_get with different signatures */
static inline struct ib_umem *hihtr_ib_umem_get(struct ib_ucontext *ibucontext,
						 unsigned long addr, size_t size,
						 int access, struct ib_udata *udata)
{
#if defined(IB_UMEM_GET_HAVE_IB_DEVICE) && defined(HAVE_MMU_INTERVAL_NOTIFIER)
	return ib_umem_get(ibucontext->device, addr, size, access);
#elif defined(IB_UMEM_GET_HAVE_IB_UCONTEXT)
	return ib_umem_get(ibucontext, addr, size, access, 0);
#elif defined(IB_UMEM_GET_HAVE_DMASYNC)
	return ib_umem_get(udata, addr, size, access, 0);
#else
	return ib_umem_get(udata, addr, size, access);
#endif
}

/* Get DMA address from umem sg table */
static inline dma_addr_t hihtr_umem_get_dma(struct ib_umem *umem, unsigned long offset)
{
#if defined(HAVE_IB_UMEM_SG_APPEND)
	return sg_dma_address(umem->sgt_append.sgt.sgl) + offset;
#else
	return sg_dma_address(umem->sg_head.sgl) + offset;
#endif
}

/* Get virtual address from umem sg table */
static inline void *hihtr_umem_get_virt(struct ib_umem *umem, unsigned long offset)
{
#if defined(HAVE_IB_UMEM_SG_APPEND)
	return sg_virt(umem->sgt_append.sgt.sgl) + offset;
#else
	return sg_virt(umem->sg_head.sgl) + offset;
#endif
}

/* Get sg table sgl for rdma_for_each_block */
static inline struct scatterlist *hihtr_umem_get_sgl(struct ib_umem *umem, unsigned int *nents)
{
#if defined(HAVE_IB_UMEM_SG_APPEND)
	if (nents)
		*nents = umem->sgt_append.sgt.nents;
	return umem->sgt_append.sgt.sgl;
#else
	if (nents)
		*nents = umem->sg_head.nents;
	return umem->sg_head.sgl;
#endif
}

/* Get ib_ucontext from udata or uobject */
static inline struct ib_ucontext *hihtr_mtr_get_ibucontext(struct ib_udata *udata,
							   struct ib_uobject *uobject)
{
#if defined(IB_UMEM_GET_HAVE_IB_UCONTEXT)
#if !defined(UVERBS_ATTR_BUNDLE_HAVE_DRIVER_UDATA)
	if (uobject)
		return uobject->context;
	return NULL;
#else
	struct uverbs_attr_bundle *attr = NULL;
	struct ib_ucontext *ibucontext = NULL;

	attr = udata ? container_of(udata, struct uverbs_attr_bundle, driver_udata) : NULL;
	ibucontext = udata ? ib_uverbs_get_ucontext(attr) : NULL;
	return ibucontext;
#endif
#else
	return NULL;
#endif
}

/* ib_umem_get with different signatures for MTR */
static inline struct ib_umem *hihtr_mtr_ib_umem_get(struct ib_device *ibdev,
						    struct ib_ucontext *ibucontext,
						    unsigned long user_addr,
						    size_t total_size,
						    int user_access,
						    struct ib_udata *udata)
{
#if defined(IB_UMEM_GET_HAVE_IB_DEVICE) && defined(HAVE_MMU_INTERVAL_NOTIFIER)
	return ib_umem_get(ibdev, user_addr, total_size, user_access);
#elif defined(IB_UMEM_GET_HAVE_IB_UCONTEXT)
	return ib_umem_get(ibucontext, user_addr, total_size, user_access, 0);
#elif defined(IB_UMEM_GET_HAVE_DMASYNC)
	return ib_umem_get(udata, user_addr, total_size, user_access, 0);
#else
	return ib_umem_get(udata, user_addr, total_size, user_access);
#endif
}

#if defined(UVERBS_IOCTL_NEED_UAPI_DEFINITION)
struct uapi_definition {
	u8 kind;
	u8 scope;
	union {
		struct {
			u16 object_id;
		} object_start;
		struct {
			u16 command_num;
			u8 is_ex:1;
			u8 has_udata:1;
			u8 has_resp:1;
			u8 req_size;
			u8 resp_size;
		} write;
	};

	union {
		bool (*func_is_supported)(struct ib_device *device);
		int (*func_write)(struct uverbs_attr_bundle *attrs);
		const struct uapi_definition *chain;
		const struct uverbs_object_def *chain_obj_tree;
		size_t needs_fn_offset;
	};
};
#endif

#if defined(UVERBS_IOCTL_NEED_UAPI_DEFINITION_SCOPE)
enum uapi_definition_scope {
	UAPI_SCOPE_OBJECT = 1,
	UAPI_SCOPE_METHOD = 2,
};
#endif

#if defined(UVERBS_IOCTL_NEED_UAPI_DEF_IS_OBJ_SUPPORTED)
#define UAPI_DEF_IS_OBJ_SUPPORTED(_func)                                       \
	{                                                                      \
		.kind = UAPI_DEF_IS_SUPPORTED_FUNC,                            \
		.scope = UAPI_SCOPE_OBJECT, .func_is_supported = _func,        \
	}
#endif

#if defined(UVERBS_IOCTL_NEED_UAPI_DEF_CHAIN_OBJ_TREE_NAMED)
#define UAPI_DEF_CHAIN_OBJ_TREE(_object_enum, _object_ptr, ...)                \
	{                                                                      \
		.kind = UAPI_DEF_CHAIN_OBJ_TREE,                               \
		.object_start = { .object_id = _object_enum },                 \
		.chain_obj_tree = _object_ptr,                                 \
	},								       \
		##__VA_ARGS__
#define UAPI_DEF_CHAIN_OBJ_TREE_NAMED(_object_enum, ...)                       \
	UAPI_DEF_CHAIN_OBJ_TREE(_object_enum, &UVERBS_OBJECT(_object_enum),    \
				##__VA_ARGS__)
#endif

#if !defined(UVERBS_ATTR_BUNDLE_HAVE_DRIVER_UDATA)
enum uapi_radix_data_new {
	UVERBS_API_ATTR_KEY_BITS_new = 6,
	UVERBS_API_ATTR_BKEY_LEN_new = (1 << UVERBS_API_ATTR_KEY_BITS_new) - 1,
};

#endif

#ifdef UVERBS_IOCTL_NEED_UAPI_DEF_CHAIN
enum uapi_definition_kind {
	UAPI_DEF_END = 0,
	UAPI_DEF_OBJECT_START,
	UAPI_DEF_WRITE,
	UAPI_DEF_CHAIN_OBJ_TREE,
	UAPI_DEF_CHAIN,
	UAPI_DEF_IS_SUPPORTED_FUNC,
	UAPI_DEF_IS_SUPPORTED_DEV_FN,
};
#define UAPI_DEF_CHAIN(_def_var)                                               \
{                                                                      \
	.kind = UAPI_DEF_CHAIN, .chain = _def_var,                     \
}
#endif

#endif