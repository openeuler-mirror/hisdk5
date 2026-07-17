/*
 * Copyright (c) 2016 Hisilicon Limited.
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

#ifndef _HIHTR_DEVICE_H
#define _HIHTR_DEVICE_H

#include <linux/list.h>
#include <linux/version.h>
#include <rdma/ib_verbs.h>
#include "hihtr-abi.h"
#include "hihtr_compat.h"
#include "shared/hihtr_shared.h"

#if !defined(RDMA_HAVE_XARRAY)
#include <linux/radix-tree.h>
#include <linux/idr.h>
#endif

#define PCI_REVISION_ID_HIP08			0x21 // 1620
#define PCI_REVISION_ID_HIP09			0x30 // 1630
#define PCI_REVISION_ID_HIP10			0x32 // 1636

// HIHTR_DRIVER 宏未定义
#ifdef HIHTR_DRIVER
#define HIHTR_GET_PCI_REVISION(hr_dev) (hr_dev->pci_dev->revision)
#else
#define HIHTR_GET_PCI_REVISION(hr_dev) (PCI_REVISION_ID_HIP10) // 1872参考1636实现, 走这个分支
#endif

#define HIHTR_MAX_MSG_LEN			0x80000000

#define HIHTR_IB_MIN_SQ_STRIDE		6

#define BA_BYTE_LEN				8

#define HIHTR_MIN_CQE_NUM			0x40
#define HIHTR_MIN_SRQ_WQE_NUM		1

#define HIHTR_MAX_IRQ_NUM			128

#define HIHTR_SGE_IN_WQE			2
#define HIHTR_SGE_SHIFT			4

#define EQ_ENABLE				1
#define EQ_DISABLE				0

#define HIHTR_CEQ				0
#define HIHTR_AEQ				1
#define HIHTR_IS_RESETTING			1

#define HIHTR_CEQE_SIZE 0x4
#define HIHTR_AEQE_SIZE 0x10

#define HIHTR_V3_EQE_SIZE 0x40

#define HIHTR_V2_CQE_SIZE 32
#define HIHTR_V3_CQE_SIZE 64

#define HIHTR_V2_QPC_SZ 256
#define HIHTR_V3_QPC_SZ 512

#define HIHTR_CEQ_MAX_INTERVAL		0xffff
#define HIHTR_EQ_MAXCNT_MASK			1
#define HIHTR_EQ_PERIOD_MASK			2

#define HIHTR_MAX_PORTS			6
#define HIHTR_GID_SIZE			16
#define HIHTR_SGE_SIZE			16
#define HIHTR_DWQE_SIZE			4096

#define HIHTR_MCAST_HASH_SIZE    256
#define HIHTR_MCAST_ENTRY_SIZE   8
#define HIHTR_MCAST_MAX_QP       10
#define HIHTR_HOP_NUM_0			0xff

#define MR_TYPE_MR				0x00
#define MR_TYPE_FRMR				0x01
#define MR_TYPE_DMA				0x03

#define HIHTR_FRMR_MAX_PA			512

#define PKEY_ID					0xffff
#define NODE_DESC_SIZE				64
#define DB_REG_OFFSET				0x1000

#define ATOMIC_WR_LEN				8

#define HIHTR_ULD_DEV_NAME_LEN 16

/* Configure to HW for PAGE_SIZE larger than 4KB */
#define PG_SHIFT_OFFSET				(PAGE_SHIFT - 12)

#define HIHTR_IDX_QUE_ENTRY_SZ		4
#define SRQ_DB_REG				0x230

/* Convert string to decimal number */
#define HIHTR_CONVERSION_NUM			10

// ROCEE RX 相关计数
#define ROCEE_RC_PKT_RX_CNT_SIZE 4
#define ROCEE_UD_PKT_RX_CNT_SIZE 4
#define ROCEE_XRC_PKT_RX_CNT_SIZE 4
#define ROCEE_PKT_RX_CNT_SIZE 4
#define ROCEE_ERR_PKT_RX_CNT_SIZE 4
#define ROCEE_CNP_PKT_RX_CNT_SIZE 4

// ROCEE TX 相关计数
#define ROCEE_RC_PKT_TX_CNT_SIZE 4
#define ROCEE_UD_PKT_TX_CNT_SIZE 4
#define ROCEE_XRC_PKT_TX_CNT_SIZE 4
#define ROCEE_PKT_TX_CNT_SIZE 4
#define ROCEE_ERR_PKT_TX_CNT_SIZE 4
#define ROCEE_CNP_PKT_TX_CNT_SIZE 4

// TXDMA 相关计数
#define ROCE_TX_IN_PKT_CNT_SIZE 4
#define ROCE_TX_OUT_PKT_CNT_SIZE 4
#define ROCE_TX_IN_AR_CNT_SIZE 4
#define AXI_DATA_ROCE_CNT_SIZE 4

// IPSU 相关计数
#define IPSUTX_RX_PKT_CNT_SIZE 9
#define IPSUTX_TX_PKT_CNT_SIZE 9
#define IPSURX_RX_PKT_CNT_SIZE 7
#define IPSURX_TX_PKT_CNT_SIZE 7

// CPB 相关计数
#define CIT_IN_CHNL_PKT_CNT_SIZE 8
#define CPT_OUT_CHNL_PKT_CNT_SIZE 4
#define CIR_IN_PORT_PKT_CNT_SIZE 6
#define CPR_OUT_CHNL_PKT_CNT_SIZE 13
#define CPR_OUT_BYP_PKT_CNT_SIZE 4
// PETX 相关计数宏
#define PETX_CPB_PKT_CNT_SIZE 5
#define PETX_MAG_PKT_CNT_SIZE 5

// PERX 相关计数宏
#define PERX_RCV_RDMA_PKT_CNT_SIZE 5
#define PERX_RDMA_PKT_CNT_SIZE 5

#define TOTAL_DP_PKT_REG_NUM 161
#define HIHTR_FAKE_DB_NUM 1
#define HIHTR_PF_DEVICE_ID 0x0229
#define HIHTR_VF_DEVICE_ID 0x022a
#define HIHTR_DRV_VERSION  GLOBAL_VERSION_STR

enum {
	SERV_TYPE_RC,
	SERV_TYPE_UC,
	SERV_TYPE_RD,
	SERV_TYPE_UD,
	SERV_TYPE_XRC = 5,
};

enum hihtr_event {
	HIHTR_EVENT_TYPE_PATH_MIG                  = 0x01,
	HIHTR_EVENT_TYPE_PATH_MIG_FAILED           = 0x02,
	HIHTR_EVENT_TYPE_COMM_EST                  = 0x03,
	HIHTR_EVENT_TYPE_SQ_DRAINED                = 0x04,
	HIHTR_EVENT_TYPE_WQ_CATAS_ERROR            = 0x05,
	HIHTR_EVENT_TYPE_INV_REQ_LOCAL_WQ_ERROR    = 0x06,
	HIHTR_EVENT_TYPE_LOCAL_WQ_ACCESS_ERROR     = 0x07,
	HIHTR_EVENT_TYPE_SRQ_LIMIT_REACH           = 0x08,
	HIHTR_EVENT_TYPE_SRQ_LAST_WQE_REACH        = 0x09,
	HIHTR_EVENT_TYPE_SRQ_CATAS_ERROR           = 0x0a,
	HIHTR_EVENT_TYPE_CQ_ACCESS_ERROR           = 0x0b,
	HIHTR_EVENT_TYPE_CQ_OVERFLOW               = 0x0c,
	HIHTR_EVENT_TYPE_CQ_ID_INVALID             = 0x0d,
	HIHTR_EVENT_TYPE_PORT_CHANGE               = 0x0f,
	/* 0x10 and 0x11 is unused in currently application case */
	HIHTR_EVENT_TYPE_DB_OVERFLOW               = 0x12,
	HIHTR_EVENT_TYPE_MB                        = 0x13,
	HIHTR_EVENT_TYPE_FLR			      = 0x15,
	HIHTR_EVENT_TYPE_XRCD_VIOLATION	      = 0x16,
	HIHTR_EVENT_TYPE_INVALID_XRCETH	      = 0x17,
};

/* Private QP creation flags to be passed in ib_qp_init_attr.create_flags.
 *
 * These flags are intended for internal use by the hihtr driver, and they
 * rely on the range reserved for that use in the ib_qp_create_flags enum.
 */
enum hihtr_qp_create_flags {
	HIHTR_QP_CREATE_DCA_EN = IB_QP_CREATE_RESERVED_START,
};

enum {
	HIHTR_CAP_FLAG_REREG_MR		= BIT(0),
	HIHTR_CAP_FLAG_ROCE_V1_V2		= BIT(1),
	/* discard this bit, reserved for compatibility */
	HIHTR_CAP_FLAG_DISCARD		= BIT(2),
	HIHTR_CAP_FLAG_CQ_RECORD_DB		= BIT(3),
	HIHTR_CAP_FLAG_QP_RECORD_DB		= BIT(4),
	HIHTR_CAP_FLAG_SRQ			= BIT(5),
	HIHTR_CAP_FLAG_XRC			= BIT(6),
	HIHTR_CAP_FLAG_MW			= BIT(7),
	HIHTR_CAP_FLAG_FRMR                  = BIT(8),
	HIHTR_CAP_FLAG_CONGEST_CTRL		= BIT(9),
	HIHTR_CAP_FLAG_ATOMIC		= BIT(10),
	HIHTR_CAP_FLAG_DIRECT_WQE		= BIT(12),
	HIHTR_CAP_FLAG_SDI_MODE		= BIT(14),
	HIHTR_CAP_FLAG_DCA_MODE		= BIT(15),
	HIHTR_CAP_FLAG_STASH			= BIT(17),
	HIHTR_CAP_FLAG_CQE_INLINE		= BIT(19),
	HIHTR_CAP_FLAG_RQ_INLINE		= BIT(20),
	HIHTR_CAP_FLAG_BOND			= BIT(21),
	HIHTR_CAP_FLAG_SRQ_RECORD_DB		= BIT(22),
	HIHTR_CAP_FLAG_RESIZE_CQ	= BIT(23),
};

#define HIHTR_DB_TYPE_COUNT			2
#define HIHTR_DB_UNIT_SIZE			4

enum {
	HIHTR_DB_PER_PAGE = PAGE_SIZE / 4
};

enum hihtr_reset_stage {
	HIHTR_STATE_NON_RST,
	HIHTR_STATE_RST_BEF_DOWN,
	HIHTR_STATE_RST_DOWN,
	HIHTR_STATE_RST_UNINIT,
	HIHTR_STATE_RST_INIT,
	HIHTR_STATE_RST_INITED,
};

enum hihtr_instance_state {
	HIHTR_STATE_NON_INIT,
	HIHTR_STATE_INIT,
	HIHTR_STATE_INITED,
	HIHTR_STATE_UNINIT,
};

enum hihtr_qp_mcast_state {
	HIHTR_QP_MCAST_FREE,
	HIHTR_QP_MCAST_OCCUPY,
};

enum {
	HIHTR_RST_DIRECT_RETURN		= 0,
};

#define HIHTR_CMD_SUCCESS			1

#define HIHTR_MAX_HOP_NUM			3

/* The minimum page size is 4K for hardware */
#define HIHTR_HW_PAGE_SHIFT			12
#define HIHTR_HW_PAGE_SIZE			(1 << HIHTR_HW_PAGE_SHIFT)

struct hihtr_uar {
	u64		pfn;
	unsigned long	index;
	unsigned long	logic_idx;
};

enum hihtr_mmap_type {
	HIHTR_MMAP_TYPE_DB = 1,
	HIHTR_MMAP_TYPE_DWQE,
	HIHTR_MMAP_TYPE_DCA,
	HIHTR_MMAP_TYPE_RESET,
	HIHTR_MMAP_TYPE_STATS,
};

struct hihtr_user_mmap_entry {
	struct rdma_user_mmap_entry rdma_entry;
	enum hihtr_mmap_type mmap_type;
	u64 address;
};

struct hihtr_dca_ctx {
	struct list_head pool; /* all DCA mems link to @pool */
	spinlock_t pool_lock; /* protect @pool */
	unsigned int free_mems; /* free mem num in pool */
	size_t free_size; /* free mem size in pool */
	size_t total_size; /* total size in pool */
	size_t max_size; /* max size the pool can expand to */
	size_t min_size; /* shrink if @free_size > @min_size */
	unsigned int unit_size; /* unit size per DCA mem */

	unsigned int max_qps;
	unsigned int status_npage;
	struct ida ida;

#define HIHTR_DCA_BITS_PER_STATUS 1
	unsigned long *buf_status;
	unsigned long *sync_status;

	bool exit_aging;
	struct list_head aging_proc_list;
	struct list_head aging_new_list;
	spinlock_t aging_lock;
	struct delayed_work aging_dwork;
	struct hihtr_user_mmap_entry *dca_mmap_entry;
};

struct hihtr_ucontext {
	struct ib_ucontext	ibucontext;
	struct list_head list; /* link all uctx to uctx_list on hr_dev */
	pid_t pid; /* process id to which the uctx belongs */
	struct hihtr_uar	uar;
	struct list_head	page_list;
	struct mutex		page_mutex;
	struct hihtr_user_mmap_entry *db_mmap_entry;
	struct hihtr_user_mmap_entry *uar_mmap_entry;
	struct hihtr_user_mmap_entry *reset_mmap_entry;
	struct hihtr_user_mmap_entry *stats_mmap_entry;
	u32					config;
	unsigned long			en_flags;
	struct hihtr_dca_ctx	dca_ctx;
	void *dca_dbgfs;
/* Only define mmap_xa in hihtr_ucontext if:
 * - Kernel has XARRAY support, AND
 * - ib_ucontext does NOT have mmap_xa member
 * If ib_ucontext has mmap_xa, we use ib_ucontext.mmap_xa directly
 */
#if defined(RDMA_HAVE_XARRAY) && !defined(IB_UCONTEXT_HAVE_MMAP_XA)
	struct xarray mmap_xa;
#elif !defined(RDMA_HAVE_XARRAY)
	struct radix_tree_root root;
    spinlock_t tree_lock;
#endif
};

struct hihtr_pd {
	struct ib_pd		ibpd;
	unsigned long		pdn;
};

struct hihtr_xrcd {
	struct ib_xrcd ibxrcd;
	u32 xrcdn;
};

struct hihtr_bitmap {
	/* Bitmap Traversal last a bit which is 1 */
	unsigned long		last;
	unsigned long		top;
	unsigned long		max;
	unsigned long		reserved_top;
	unsigned long		mask;
	spinlock_t		lock;
	unsigned long		*table;
};

struct hihtr_ida {
	struct ida ida;
	u32 min; /* Lowest ID to allocate.  */
	u32 max; /* Highest ID to allocate. */
};

/* For Hardware Entry Memory */
struct hihtr_hem_table {
	/* HEM type: 0 = qpc, 1 = mtt, 2 = cqc, 3 = srq, 4 = other */
	u32		type;
	/* HEM array elment num */
	unsigned long	num_hem;
	/* Single obj size */
	unsigned long	obj_size;
	unsigned long	table_chunk_size;
	unsigned long obj_count;
	int		lowmem;
	struct mutex	mutex;
	struct hihtr_hem **hem;
	u64		**bt_l1;
	dma_addr_t	*bt_l1_dma_addr;
	u64		**bt_l0;
	dma_addr_t	*bt_l0_dma_addr;
};

struct hihtr_buf_region {
	u32 offset; /* page offset */
	u32 count; /* page count */
	int hopnum; /* addressing hop num */
};

#define HIHTR_MAX_BT_REGION	3
#define HIHTR_MAX_BT_LEVEL	3
struct hihtr_hem_list {
	struct list_head root_bt;
	/* link all bt dma mem by hop config */
	struct list_head mid_bt[HIHTR_MAX_BT_REGION][HIHTR_MAX_BT_LEVEL];
	struct list_head btm_bt; /* link all bottom bt in @mid_bt */
	dma_addr_t root_ba; /* pointer to the root ba table */
};

struct hihtr_buf_attr {
	struct {
		size_t	size;  /* region size */
		int	hopnum; /* multi-hop addressing hop num */
	} region[HIHTR_MAX_BT_REGION];
	unsigned int region_count; /* valid region count */
	unsigned int page_shift;  /* buffer page shift */
	unsigned int user_access; /* umem access flag */
	u64 iova;
	bool mtt_only; /* only alloc buffer-required MTT memory */
	bool adaptive; /* adaptive for page_shift and hopnum */
};

struct hihtr_hem_cfg {
	dma_addr_t	root_ba; /* root BA table's address */
	bool		is_direct; /* addressing without BA table */
	unsigned int	ba_pg_shift; /* BA table page shift */
	unsigned int	buf_pg_shift; /* buffer page shift */
	unsigned int	buf_pg_count;  /* buffer page count */
	struct hihtr_buf_region region[HIHTR_MAX_BT_REGION];
	unsigned int	region_count;
};

/* memory translate region */
struct hihtr_mtr {
	struct hihtr_hem_list hem_list; /* multi-hop addressing resource */
	struct ib_umem		*umem; /* user space buffer */
	struct hihtr_buf	*kmem; /* kernel space buffer */
	struct hihtr_hem_cfg  hem_cfg; /* config for hardware addressing */
};

/* DCA config */
struct hihtr_dca_cfg {
	spinlock_t		lock;
	u16			attach_count;
	u32			buf_id;
	u32			dcan;
	void			**buf_list;
	u32			npages;
	u32			sq_idx;
	bool			aging_enable;
	struct list_head	aging_node;
};

struct hihtr_mw {
	struct ib_mw		ibmw;
	u32			pdn;
	u32			rkey;
	int			enabled; /* MW's active status */
	u32			pbl_hop_num;
	u32			pbl_ba_pg_sz;
	u32			pbl_buf_pg_sz;
};

struct hihtr_mr {
	struct ib_mr		ibmr;
	u64			iova; /* MR's virtual original addr */
	u64			size; /* Address range of MR */
	u32			key; /* Key of MR */
	u32			pd;   /* PD num of MR */
	u32			access; /* Access permission of MR */
	int			enabled; /* MR's active status */
	int			type; /* MR's register type */
	u32			pbl_hop_num; /* multi-hop number */
	struct hihtr_mtr	pbl_mtr;
	u32			npages;
	dma_addr_t		*page_list;
};

struct hihtr_mr_table {
	struct hihtr_ida mtpt_ida;
	struct hihtr_hem_table	mtpt_table;
};

struct hihtr_inv_work {
	struct work_struct	work;
	struct hihtr_dev	*hr_dev;
	u32			rkey;       /* rkey to invalidate */
	volatile bool	done;       /* work completed flag */
};

struct hihtr_wq {
	u64		*wrid;     /* Work request ID */
	u32		*inv_rkey; /* Saved invalidate rkey for LOCAL_INV→NOP mapping */
	spinlock_t	lock;
	u32		wqe_cnt;  /* WQE num */
	u32		max_gs;
	u32		rsv_sge;
	u32		offset;
	int		wqe_offset;
	u32		wqe_shift; /* WQE size */
	u32		head;
	u32		tail;
	void __iomem	*db_reg;
	u32		ext_sge_cnt;
	struct hihtr_inv_work	inv_work;   /* deferred invalidation work */
};

struct hihtr_sge {
	unsigned int	sge_cnt; /* SGE num */
	u32		offset;
	u32		sge_shift; /* SGE size */
	int		wqe_offset;
};

struct hihtr_buf_list {
	void		*buf;
	dma_addr_t	map;
};

/*
 * %HIHTR_BUF_DIRECT indicates that the all memory must be in a continuous
 * dma address range.
 *
 * %HIHTR_BUF_NOSLEEP indicates that the caller cannot sleep.
 *
 * %HIHTR_BUF_NOFAIL allocation only failed when allocated size is zero, even
 * the allocated size is smaller than the required size.
 */
enum {
	HIHTR_BUF_DIRECT = BIT(0),
	HIHTR_BUF_NOSLEEP = BIT(1),
	HIHTR_BUF_NOFAIL = BIT(2),
};

struct hihtr_buf {
	struct hihtr_buf_list	*trunk_list;
	u32				ntrunks;
	u32				npages;
	unsigned int			trunk_shift;
	unsigned int			page_shift;
};

struct hihtr_db_pgdir {
	struct list_head	list;
	DECLARE_BITMAP(order0, HIHTR_DB_PER_PAGE);
	DECLARE_BITMAP(order1, HIHTR_DB_PER_PAGE / HIHTR_DB_TYPE_COUNT);
	unsigned long		*bits[HIHTR_DB_TYPE_COUNT];
	u32			*page;
	dma_addr_t		db_dma;
};

struct hihtr_user_db_page {
	struct list_head	list;
	struct ib_umem		*umem;
	unsigned long		user_virt;
	refcount_t		refcount;
};

struct hihtr_db {
	u32		*db_record;
	union {
		struct hihtr_db_pgdir *pgdir;
		struct hihtr_user_db_page *user_page;
	} u;
	dma_addr_t	dma;
	void		*virt_addr;
	unsigned long	index;
	unsigned long	order;
};

enum hihtr_cq_dfx {
	HIHTR_CQ_SEND_CQE,
	HIHTR_CQ_RECV_CQE,
	HIHTR_CQ_DFX_TOTAL
};

struct hihtr_cq {
	struct ib_cq			ib_cq;
	struct hihtr_mtr		mtr;
	struct hihtr_db		db;
	u32				flags;
	spinlock_t			lock;
	u32				cq_depth;
	u32 			resize_cq_en;
	u32				resize_cq_hw_cq_depth;
	u32				cons_index;
	u32				*set_ci_db;
	void __iomem			*db_reg;
	int				arm_sn;
	int				cqe_size;
	unsigned long			cqn;
	u32				vector;
	refcount_t			refcount;
	struct completion		free;
	struct list_head		sq_list; /* all qps on this send cq */
	struct list_head		rq_list; /* all qps on this recv cq */
	int				is_armed; /* cq is armed */
	struct list_head		node; /* all armed cqs are on a list */
	u32 dfx_cnt[HIHTR_CQ_DFX_TOTAL];
};

struct hihtr_idx_que {
	struct hihtr_mtr		mtr;
	u32				entry_shift;
	unsigned long			*bitmap;
	u32				head;
	u32				tail;
};

struct hihtr_srq {
	struct ib_srq		ibsrq;
	unsigned long		srqn;
	u32			wqe_cnt;
	int			max_gs;
	u32			rsv_sge;
	u32			wqe_shift;
	u32			cqn;
	u32			xrcdn;
	void __iomem		*db_reg;

	refcount_t		refcount;
	struct completion	free;

	struct hihtr_mtr	buf_mtr;

	u64		       *wrid;
	struct hihtr_idx_que idx_que;
	spinlock_t		lock;
	struct mutex		mutex;
	void (*event)(struct hihtr_srq *srq, enum hihtr_event event);
	struct hihtr_db db;
	u32 		cap_flags;
};

struct hihtr_uar_table {
	struct hihtr_bitmap bitmap;
};

#if defined(RDMA_HAVE_XARRAY)
struct hihtr_dip_table {
	struct xarray dip_xa;
	u32 next_dip_idx;
};
#else
struct hihtr_dip_table {
	struct idr dip_idr;
	u32 next_dip_idx;
};
#endif

struct hihtr_qp_table {
	struct hihtr_hem_table	qp_table;
	struct hihtr_hem_table	irrl_table;
	struct hihtr_hem_table	trrl_table;
	struct hihtr_hem_table	sccc_table;
	struct mutex			scc_mutex;
	struct mutex			ida_mutex;
	struct hihtr_ida qp_ida;
	struct hihtr_dip_table dip_table;
};

struct hihtr_cq_table {
#if defined(RDMA_HAVE_XARRAY)
	struct xarray array;
#else
	struct radix_tree_root root;
	spinlock_t tree_lock;
#endif
	struct hihtr_hem_table	table;
	struct hihtr_ida cq_ida;
	struct mutex ida_mutex;
};

struct hihtr_srq_table {
	struct hihtr_ida		srq_ida;
#if defined(RDMA_HAVE_XARRAY)
	struct xarray			xa;
#else
	struct radix_tree_root root;
	spinlock_t tree_lock;
#endif
	struct hihtr_hem_table	table;
};

struct hihtr_av {
	u8 port;
	u8 gid_index;
	u8 stat_rate;
	u8 hop_limit;
	u32 flowlabel;
	u16 udp_sport;
	u8 sl;
	u8 tclass;
	u8 dgid[HIHTR_GID_SIZE];
	u8 mac[ETH_ALEN];
	u16 vlan_id;
	u8 vlan_en;
};

struct hihtr_ah {
	struct ib_ah		ibah;
	struct hihtr_av	av;
};

struct hihtr_cmd_context {
	struct completion	done;
	int			result;
	int			next;
	u64			out_param;
	u16			token;
	u16			busy;
};

enum hihtr_cmdq_state {
	HIHTR_CMDQ_STATE_NORMAL,
	HIHTR_CMDQ_STATE_FATAL_ERR,
};

struct hihtr_cmdq {
	struct dma_pool		*pool;
	struct semaphore	poll_sem;
	/*
	 * Event mode: cmd register mutex protection,
	 * ensure to not exceed max_cmds and user use limit region
	 */
	struct semaphore	event_sem;
	int			max_cmds;
	spinlock_t		context_lock;
	int			free_head;
	struct hihtr_cmd_context *context;
	/*
	 * Process whether use event mode, init default non-zero
	 * After the event queue of cmd event ready,
	 * can switch into event mode
	 * close device, switch into poll mode(non event mode)
	 */
	u8			use_events;
	enum hihtr_cmdq_state state;
};

struct hihtr_cmd_mailbox {
	void		       *buf;
	dma_addr_t		dma;
};

struct hihtr_mbox_msg {
	u64 in_param;
	u64 out_param;
	u8 cmd;
	u32 tag;
	u16 token;
	u8 event_en;
};

struct hihtr_dev;

enum {
	HIHTR_FLUSH_FLAG = 0,
};

struct hihtr_work {
	struct hihtr_dev *hr_dev;
	struct work_struct work;
	int event_type;
	int sub_type;
	u32 queue_num;
};

struct hihtr_qp {
	struct ib_qp		ibqp;
	struct hihtr_wq	rq;
	struct hihtr_db	rdb;
	struct hihtr_db	sdb;
	unsigned long		en_flags;
	u32			doorbell_qpn;
	enum ib_sig_type	sq_signal_bits;
	struct hihtr_wq	sq;

	struct hihtr_mtr	mtr;
	struct hihtr_dca_cfg	dca_cfg;

	u32			buff_size;
	struct mutex		mutex;
	u8			port;
	u8			phy_port;
	u8			sl;
	u8			resp_depth;
	u8			state;
	u32                     atomic_rd_en;
	u32			qkey;
	void			(*event)(struct hihtr_qp *qp,
					 enum hihtr_event event_type);
	unsigned long		qpn;
	u8 dip_vld; /* 0: not ipqcn or cc not enable, 1: ipqcn */
	u32 dip_idx;

	u32			xrcdn;

	refcount_t		refcount;
	struct completion	free;

	struct hihtr_sge	sge;
	u32			next_sge;
	enum ib_mtu		path_mtu;
	u32			max_inline_data;
	u8			free_mr_en;

	/* 0: flush needed, 1: unneeded */
	unsigned long		flush_flag;
	struct hihtr_work	flush_work;
	struct list_head	node; /* all qps are on a list */
	struct list_head	rq_node; /* all recv qps are on a list */
	struct list_head	sq_node; /* all send qps are on a list */
	u32			config;
	u8			tc_mode;
	u8			priority;
	u8          mcast_flag;
};

struct hihtr_ib_iboe {
	spinlock_t		lock;
	struct net_device      *netdevs[HIHTR_MAX_PORTS];
	struct notifier_block	nb;
	u8			phy_port[HIHTR_MAX_PORTS];
	enum ib_port_state	port_state[HIHTR_MAX_PORTS];
};

struct hihtr_ceqe {
	__le32	comp;
	__le32	rsv[15];
};

#define CEQE_FIELD_LOC(h, l) FIELD_LOC(struct hihtr_ceqe, h, l)

#define CEQE_CQN CEQE_FIELD_LOC(23, 0)
#define CEQE_OWNER CEQE_FIELD_LOC(31, 31)

struct hihtr_aeqe {
	__le32 asyn;
	union {
		struct {
			__le32 num;
			u32 rsv0;
			u32 rsv1;
		} queue_event;

		struct {
			__le64  out_param;
			__le16  token;
			u8	status;
			u8	rsv0;
		} __packed cmd;
	 } event;
	__le32 rsv[12];
};

#define AEQE_FIELD_LOC(h, l) FIELD_LOC(struct hihtr_aeqe, h, l)

#define AEQE_EVENT_TYPE AEQE_FIELD_LOC(7, 0)
#define AEQE_SUB_TYPE AEQE_FIELD_LOC(15, 8)
#define AEQE_OWNER AEQE_FIELD_LOC(31, 31)
#define AEQE_EVENT_QUEUE_NUM AEQE_FIELD_LOC(55, 32)

struct hihtr_eq {
	struct hihtr_dev		*hr_dev;
	void __iomem			*db_reg;

	int				type_flag; /* Aeq:1 ceq:0 */
	int				eqn;
	u32				entries;
	int				eqe_size;
	int				irq;
	u32				cons_index;
	int				over_ignore;
	int				coalesce;
	int				arm_st;
	int				hop_num;
	struct hihtr_mtr		mtr;
	u16				eq_max_cnt;
	u32				eq_period;
	int				shift;
	int				event_type;
	int				sub_type;
};

struct hihtr_eq_table {
	struct hihtr_eq	*eq;
};

enum hihtr_scc_algo {
	HIHTR_SCC_ALGO_DCQCN = 0,
	HIHTR_SCC_ALGO_LDCP,
	HIHTR_SCC_ALGO_DIP,
	HIHTR_SCC_ALGO_TOTAL,
};

struct hihtr_caps {
	u64		fw_ver;
	u8		num_ports;
	int		gid_table_len[HIHTR_MAX_PORTS];
	int		pkey_table_len[HIHTR_MAX_PORTS];
	int		local_ca_ack_delay;
	int		num_uars;
	u32		phy_num_uars;
	u32		max_sq_sg;
	u32		max_sq_inline;
	u32		max_rq_sg;
	u32		max_extend_sg;
	u32		num_qps;
	u32		num_pi_qps;
	u32		reserved_qps;
	int		num_qpc_timer;
	u32		num_srqs;
	u32		max_sq_wqes;
	u32		max_rq_wqes;
	u32		max_srq_wrs;
	u32		max_srq_sges;
	u32		max_sq_desc_sz;
	u32		max_rq_desc_sz;
	u32		max_srq_desc_sz;
	int		max_qp_init_rdma;
	int		max_qp_dest_rdma;
	u32		num_cqs;
	u32		max_cqes;
	u32		min_cqes;
	u32		min_wqes;
	u32		reserved_cqs;
	u32		reserved_srqs;
	int		num_aeq_vectors;
	int		num_comp_vectors;
	int		num_other_vectors;
	u32		num_mtpts;
	u32		num_mtt_segs;
	u32		num_srqwqe_segs;
	u32		num_idx_segs;
	int		reserved_mrws;
	int		reserved_uars;
	int		num_pds;
	int		reserved_pds;
	u32		num_xrcds;
	u32		reserved_xrcds;
	u32		mtt_entry_sz;
	u32		cqe_sz;
	u32		page_size_cap;
	u32		reserved_lkey;
	int		mtpt_entry_sz;
	int		qpc_sz;
	int		irrl_entry_sz;
	int		trrl_entry_sz;
	int		cqc_entry_sz;
	int		sccc_sz;
	int		qpc_timer_entry_sz;
	int		cqc_timer_entry_sz;
	int		srqc_entry_sz;
	int		idx_entry_sz;
	u32		pbl_ba_pg_sz;
	u32		pbl_buf_pg_sz;
	u32		pbl_hop_num;
	int		aeqe_depth;
	int		ceqe_depth;
	u32		aeqe_size;
	u32		ceqe_size;
	enum ib_mtu	max_mtu;
	u32		qpc_bt_num;
	u32		qpc_timer_bt_num;
	u32		srqc_bt_num;
	u32		cqc_bt_num;
	u32		cqc_timer_bt_num;
	u32		mpt_bt_num;
	u32		eqc_bt_num;
	u32		smac_bt_num;
	u32		sgid_bt_num;
	u32		sccc_bt_num;
	u32		gmv_bt_num;
	u32		qpc_ba_pg_sz;
	u32		qpc_buf_pg_sz;
	u32		qpc_hop_num;
	u32		srqc_ba_pg_sz;
	u32		srqc_buf_pg_sz;
	u32		srqc_hop_num;
	u32		cqc_ba_pg_sz;
	u32		cqc_buf_pg_sz;
	u32		cqc_hop_num;
	u32		mpt_ba_pg_sz;
	u32		mpt_buf_pg_sz;
	u32		mpt_hop_num;
	u32		mtt_ba_pg_sz;
	u32		mtt_buf_pg_sz;
	u32		mtt_hop_num;
	u32		wqe_sq_hop_num;
	u32		wqe_sge_hop_num;
	u32		wqe_rq_hop_num;
	u32		sccc_ba_pg_sz;
	u32		sccc_buf_pg_sz;
	u32		sccc_hop_num;
	u32		qpc_timer_ba_pg_sz;
	u32		qpc_timer_buf_pg_sz;
	u32		qpc_timer_hop_num;
	u32		cqc_timer_ba_pg_sz;
	u32		cqc_timer_buf_pg_sz;
	u32		cqc_timer_hop_num;
	u32		cqe_ba_pg_sz; /* page_size = 4K*(2^cqe_ba_pg_sz) */
	u32		cqe_buf_pg_sz;
	u32		cqe_hop_num;
	u32		srqwqe_ba_pg_sz;
	u32		srqwqe_buf_pg_sz;
	u32		srqwqe_hop_num;
	u32		idx_ba_pg_sz;
	u32		idx_buf_pg_sz;
	u32		idx_hop_num;
	u32		eqe_ba_pg_sz;
	u32		eqe_buf_pg_sz;
	u32		eqe_hop_num;
	u32		gmv_entry_num;
	u32		gmv_entry_sz;
	u32		gmv_ba_pg_sz;
	u32		gmv_buf_pg_sz;
	u32		gmv_hop_num;
	u32		sl_num;
	u32		llm_buf_pg_sz;
	u32		chunk_sz; /* chunk size in non multihop mode */
	u64		flags;
	u16		default_ceq_max_cnt;
	u16		default_ceq_period;
	u16		default_aeq_max_cnt;
	u16		default_aeq_period;
	u16		default_aeq_arm_st;
	u16		default_ceq_arm_st;
	u32     max_pi;
	u8		congest_type;
	u8		default_congest_type;
	u8		max_dwqe_resource;
	u8		free_dwqe_resource;
	u32		resize_cq_hw_cq_depth;
};

enum hihtr_device_state {
	HIHTR_DEVICE_STATE_INITED,
	HIHTR_DEVICE_STATE_RST_DOWN,
	HIHTR_DEVICE_STATE_UNINIT,
};

enum hihtr_hw_pkt_stat_index {
	HIHTR_HW_RX_RC_PKT_CNT,
	HIHTR_HW_RX_UD_PKT_CNT,
	HIHTR_HW_RX_XRC_PKT_CNT,
	HIHTR_HW_RX_PKT_CNT,
	HIHTR_HW_RX_ERR_PKT_CNT,
	HIHTR_HW_RX_CNP_PKT_CNT,
	HIHTR_HW_RX_BUF_CNT,
	HIHTR_HW_CQ_CQE_CNT,
	HIHTR_HW_CQ_NOTIFY_CNT,
	HIHTR_ROCEE_EQDB_ISSUE_CNT,
	HIHTR_ROCEE_EQDB_EXEC_CNT,
	HIHTR_ROCEE_TDP_DBSF_RX_MDB_DB_CNT,
	HIHTR_ROCEE_TDP_DBSF_RX_EXTQ_DEQ_DB_CNT,
	HIHTR_ROCEE_SRQC_MAX_CNT,
	HIHTR_QMM_SRQC_HIT_CNT,
	HIHTR_QMM_SRQC_RPL_CNT,
	HIHTR_ROCEE_MPT_MAX_CNT,
	HIHTR_GEN_AC_QP_MDB_CQE_CNT,
	HIHTR_TRP_RX_SOF_CNT,
	HIHTR_HW_TX_RC_PKT_CNT,
	HIHTR_HW_TX_UD_PKT_CNT,
	HIHTR_HW_TX_XRC_PKT_CNT,
	HIHTR_HW_TX_PKT_CNT,
	HIHTR_HW_TX_ERR_PKT_CNT,
	HIHTR_HW_TX_CNP_PKT_CNT,
	HIHTR_HW_TRP_GET_IRRL_ERR_PKT_CNT,
	HIHTR_HW_TRP_GET_MPT_ERR_PKT_CNT,
	HIHTR_HW_ECN_DB_CNT,
	HIHTR_ROCEE_DWQE_WQE_ISSUE_CNT,
	HIHTR_ROCEE_DWQE_WQE_EXEC_CNT,
	HIHTR_ROCEE_DB_ISSUE_CNT,
	HIHTR_ROCEE_DB_EXEC_CNT,
	HIHTR_MDB_DFX_CNT_0,
	HIHTR_MDB_DFX_CNT_1,
	HIHTR_MDB_DFX_CNT_3,
	HIHTR_MDB_DFX_CNT_4,
	HIHTR_ROCEE_TDP_TRP_CNT,
	HIHTR_ROCEE_TDP_MDB_CNT,
	HIHTR_ROCEE_TDP_LP_CNT,
	HIHTR_ROCEE_TDP_QMM_CNT,
	HIHTR_ROCEE_TDP_EXT_ENQ_CNT,
	HIHTR_ROCEE_TDP_DBSF_TX_MDB_DB_FLT_CNT,
	HIHTR_ROCEE_TDP_DBSF_TX_MDB_DB_DROP_CNT,
	HIHTR_ROCEE_TDP_DBSF_TX_EXTQ_ENQ_DB_CNT,
	HIHTR_ROCEE_TDP_DBSF_TX_EXTQ_ENQ_DB_FLT_CNT,
	HIHTR_ROCEE_TDP_DBSF_TX_EXTQ_DEQ_DB_CNT,
	HIHTR_ROCEE_TSP_EXTDB_RB_CNT,
	HIHTR_GEN_AC_CQ_POE_CNT,
	HIHTR_QMM_MPT_HIT_CNT,
	HIHTR_QMM_MPT_RPL_CNT,
	HIHTR_GEN_AC_QP_TSP_CQE_CNT,
	HIHTR_GEN_AC_QP_TSP_AE_CNT,
	HIHTR_ROCEE_TSP_WQE_TOTAL_SRH_CNT,
	HIHTR_ROCEE_DWQE_WQE_DROP_CNT,
	HIHTR_ROCEE_DB_TIMEOUT_DROP_CNT,
	HIHTR_CAEP_RAS_DFX,
	HIHTR_ROCEE_CAEP_IRQ_CNT,
	HIHTR_MDB_DFX_CNT_2,
	HIHTR_TSP_EXT_SGE_AXI_CNT,
	HIHTR_TRP_SGE_AXI_CNT,
	HIHTR_START_TYPE_ERR_CNT,
	HIHTR_ROCEE_MBX_ISSUE_CNT,
	HIHTR_ROCEE_MBX_EXEC_CNT,
	HIHTR_CQC_MAX_CNT_BK0,
	HIHTR_CQC_MAX_CNT_BK1,
	HIHTR_ROCEE_QPC_MAX_CNT_0,
	HIHTR_ROCEE_QPC_MAX_CNT_1,
	HIHTR_HW_CNT_TOTAL,
};

struct hihtr_data_path_pkt_cnt {
    // ROCEE RX 相关计数
    u32 rocee_rc_pkt_rx_cnt[ROCEE_RC_PKT_RX_CNT_SIZE];
    u32 rocee_ud_pkt_rx_cnt[ROCEE_UD_PKT_RX_CNT_SIZE];
    u32 rocee_xrc_pkt_rx_cnt[ROCEE_XRC_PKT_RX_CNT_SIZE];
    u32 rocee_pkt_rx_cnt[ROCEE_PKT_RX_CNT_SIZE];
    u32 rocee_err_pkt_rx_cnt[ROCEE_ERR_PKT_RX_CNT_SIZE];
    u32 rocee_cnp_pkt_rx_cnt[ROCEE_CNP_PKT_RX_CNT_SIZE];

    // ROCEE TX 相关计数
    u32 rocee_rc_pkt_tx_cnt[ROCEE_RC_PKT_TX_CNT_SIZE];
    u32 rocee_ud_pkt_tx_cnt[ROCEE_UD_PKT_TX_CNT_SIZE];
    u32 rocee_xrc_pkt_tx_cnt[ROCEE_XRC_PKT_TX_CNT_SIZE];
    u32 rocee_pkt_tx_cnt[ROCEE_PKT_TX_CNT_SIZE];
    u32 rocee_err_pkt_tx_cnt[ROCEE_ERR_PKT_TX_CNT_SIZE];
    u32 rocee_cnp_pkt_tx_cnt[ROCEE_CNP_PKT_TX_CNT_SIZE];

	// TXDMA 相关计数
    u64 roce_tx_in_pkt_cnt[ROCE_TX_IN_PKT_CNT_SIZE];
    u64 roce_tx_out_pkt_cnt[ROCE_TX_OUT_PKT_CNT_SIZE];
    u64 roce_tx_in_ar_cnt[ROCE_TX_IN_AR_CNT_SIZE];
    u64 axi_data_roce_cnt[AXI_DATA_ROCE_CNT_SIZE];

    // IPSU 相关计数
    u32 ipsutx_rx_pkt_cnt[IPSUTX_RX_PKT_CNT_SIZE];
    u32 ipsutx_tx_pkt_cnt[IPSUTX_TX_PKT_CNT_SIZE];
    u32 ipsurx_rx_pkt_cnt[IPSURX_RX_PKT_CNT_SIZE];
    u32 ipsurx_tx_pkt_cnt[IPSURX_TX_PKT_CNT_SIZE];

    // CPB 相关计数
    u32 cit_in_pkt_sop_cnt;
    u32 cit_in_pkt_eop_cnt;
    u32 cir_in_pkt_eop_cnt;
    u32 cir_in_pkt_sop_cnt;
	u32 cpr_out_pkt_sop_cnt;
    u32 cit_in_chnl_pkt_cnt[CIT_IN_CHNL_PKT_CNT_SIZE];
    u32 cir_in_port_pkt_cnt[CIR_IN_PORT_PKT_CNT_SIZE];
    u32 cpt_out_pkt_sop_cnt;
    u32 cpt_out_pkt_eop_cnt;
    u32 cpr_out_pkt_eop_cnt;
    u32 cpt_out_chnl_pkt_cnt[CPT_OUT_CHNL_PKT_CNT_SIZE];
    u32 cpr_out_chnl_pkt_cnt[CPR_OUT_CHNL_PKT_CNT_SIZE];
    u32 cpr_out_byp_pkt_cnt[CPR_OUT_BYP_PKT_CNT_SIZE];

    // PE 相关计数
    u32 petx_cpb_pkt_cnt[PETX_CPB_PKT_CNT_SIZE];
    u32 petx_mag_pkt_cnt[PETX_MAG_PKT_CNT_SIZE];
    u32 perx_rcv_rdma_pkt_cnt[PERX_RCV_RDMA_PKT_CNT_SIZE];
    u32 perx_rdma_pkt_cnt[PERX_RDMA_PKT_CNT_SIZE];
	u32 rocee_db_issue_cnt;
    u32 rocee_db_exec_cnt;
};

enum hihtr_sw_dfx_stat_index {
	HIHTR_DFX_AEQE_CNT,
	HIHTR_DFX_CEQE_CNT,
	HIHTR_DFX_CMDS_CNT,
	HIHTR_DFX_CMDS_ERR_CNT,
	HIHTR_DFX_MBX_POSTED_CNT,
	HIHTR_DFX_MBX_POLLED_CNT,
	HIHTR_DFX_MBX_EVENT_CNT,
	HIHTR_DFX_QP_CREATE_ERR_CNT,
	HIHTR_DFX_QP_MODIFY_ERR_CNT,
	HIHTR_DFX_CQ_CREATE_ERR_CNT,
	HIHTR_DFX_CQ_MODIFY_ERR_CNT,
	HIHTR_DFX_SRQ_CREATE_ERR_CNT,
	HIHTR_DFX_SRQ_MODIFY_ERR_CNT,
	HIHTR_DFX_XRCD_ALLOC_ERR_CNT,
	HIHTR_DFX_MR_REG_ERR_CNT,
	HIHTR_DFX_MR_REREG_ERR_CNT,
	HIHTR_DFX_AH_CREATE_ERR_CNT,
	HIHTR_DFX_MMAP_ERR_CNT,
	HIHTR_DFX_UCTX_ALLOC_ERR_CNT,
	HIHTR_DFX_CNT_TOTAL
};

struct hihtr_hw {
	int (*cmq_init)(struct hihtr_dev *hr_dev);
	void (*cmq_exit)(struct hihtr_dev *hr_dev);
	int (*hw_profile)(struct hihtr_dev *hr_dev);
	int (*hw_init)(struct hihtr_dev *hr_dev);
	void (*hw_exit)(struct hihtr_dev *hr_dev);
	int (*post_mbox)(struct hihtr_dev *hr_dev,
			 struct hihtr_mbox_msg *mbox_msg);
	int (*poll_mbox_done)(struct hihtr_dev *hr_dev);
	bool (*chk_mbox_avail)(struct hihtr_dev *hr_dev, bool *is_busy);
	int (*set_gid)(struct hihtr_dev *hr_dev, int gid_index,
		       const union ib_gid *gid, const struct ib_gid_attr *attr);
	int (*set_mac)(struct hihtr_dev *hr_dev, u8 phy_port,
		       const u8 *addr);
	int (*write_mtpt)(struct hihtr_dev *hr_dev, void *mb_buf,
			  struct hihtr_mr *mr);
	int (*rereg_write_mtpt)(struct hihtr_dev *hr_dev,
				struct hihtr_mr *mr, int flags,
				void *mb_buf);
	int (*frmr_write_mtpt)(struct hihtr_dev *hr_dev, void *mb_buf,
			       struct hihtr_mr *mr);
	int (*mw_write_mtpt)(void *mb_buf, struct hihtr_mw *mw);
	void (*write_cqc)(struct hihtr_dev *hr_dev,
			  struct hihtr_cq *hr_cq, void *mb_buf, u64 *mtts,
			  dma_addr_t dma_handle);
	int (*set_hem)(struct hihtr_dev *hr_dev,
		       struct hihtr_hem_table *table, int obj, u32 step_idx);
	int (*clear_hem)(struct hihtr_dev *hr_dev,
			 struct hihtr_hem_table *table, int obj,
			 u32 step_idx);
	int (*set_dca_buf)(struct hihtr_dev *hr_dev,
			   struct hihtr_qp *hr_qp);
	bool (*chk_dca_buf_inactive)(struct hihtr_dev *hr_dev,
				     struct hihtr_qp *hr_qp);
	int (*modify_qp)(struct ib_qp *ibqp, const struct ib_qp_attr *attr,
			 int attr_mask, enum ib_qp_state cur_state,
			 enum ib_qp_state new_state, struct ib_udata *udata);
	int (*qp_flow_control_init)(struct hihtr_dev *hr_dev,
			 struct hihtr_qp *hr_qp);
	void (*dereg_mr)(struct hihtr_dev *hr_dev);
	int (*init_eq)(struct hihtr_dev *hr_dev);
	void (*cleanup_eq)(struct hihtr_dev *hr_dev);
	int (*write_srqc)(struct hihtr_srq *srq, void *mb_buf);
	int (*query_cqc)(struct hihtr_dev *hr_dev, u32 cqn, void *buffer);
	int (*query_qpc)(struct hihtr_dev *hr_dev, u32 qpn, void *buffer);
	int (*query_mpt)(struct hihtr_dev *hr_dev, u32 key, void *buffer);
	int (*query_srqc)(struct hihtr_dev *hr_dev, u32 srqn, void *buffer);
	int (*query_aeqc)(struct hihtr_dev *hr_dev, void *buffer);
	int (*query_sccc)(struct hihtr_dev *hr_dev, u32 qpn, void *buffer);
	int (*get_cos)(struct hihtr_dev *hr_dev,
				const struct rdma_ah_attr *ah_attr, u8 *priority);
	int (*query_hw_counter)(struct hihtr_dev *hr_dev,
				u64 *stats, u32 port, int *hw_counters);
	int (*query_hw_dp_pkt_counter)(struct hihtr_dev *hr_dev,
				struct hihtr_data_path_pkt_cnt *dp_pkt_cnt, int *dp_pkt_counters);
#if defined(RDMA_HAVE_IB_DEVICE_OPS)
	const struct ib_device_ops *hihtr_dev_ops;
	const struct ib_device_ops *hihtr_dev_srq_ops;
#else
	const struct ib_device *hihtr_dev_ops;
	const struct ib_device *hihtr_dev_srq_ops;
#endif
	int (*set_scc)(struct hihtr_dev *hr_dev,
			u32 scc_enable, u32 algo_id, u32 template_id, u32 scc_mask);
	int (*query_scc)(struct hihtr_dev *hr_dev, void *buffer);
	int (*set_scc_temp)(struct hihtr_dev *hr_dev, void *buffer);
	int (*query_scc_temp)(struct hihtr_dev *hr_dev, void *buffer);
	int (*modify_eq)(struct hihtr_dev *hr_dev, struct hihtr_eq *eq,
			  u16 eq_count, u16 eq_period, u16 type);
	int (*modify_cnp_tx_period)(struct hihtr_dev *hr_dev, u32 tx_period);
	int (*query_cnp_tx_period)(struct hihtr_dev *hr_dev, u32 *tx_period);
};

struct hihtr_port {
	struct hihtr_dev *hr_dev;
	u8 port_num;
	struct kobject kobj;
};

enum {
	HIHTR_LINK_TYPE_ETH,
	HIHTR_LINK_TYPE_HCCS,
	HIHTR_LINK_TYPE_UB,
	HIHTR_LINK_TYPE_CNT,
};

struct hihtr_mtr_node {
	struct hihtr_mtr mtr;
	struct list_head list;
};

struct hihtr_qp_cnt {
    struct mutex cur_qps_mutex;
    u32 alloc_qp_cnt;
    u32 del_qp_cnt;
};

struct hihtr_mgid_hash_node {
    u8 gid[16];
    u32 counter;
    u32 index;
    struct hlist_node node;
};

struct pid_node {
    struct list_head node;
    pid_t pid;
};

struct hihtr_dev {
	struct ib_device	ib_dev;
	struct pci_dev		*pci_dev;
	struct device		*dev;
	void			*dbgfs; /* debugfs for this dev */

	struct list_head	uctx_list; /* list of all uctx on this dev */
	spinlock_t		uctx_list_lock; /* protect @uctx_list */

	struct hihtr_uar     priv_uar;
	const char		*irq_names[HIHTR_MAX_IRQ_NUM];
	spinlock_t		sm_lock;
	bool			active;
	bool			is_reset;
	bool			dis_db;
	unsigned long		reset_cnt;
	struct hihtr_ib_iboe iboe;
	enum hihtr_device_state state;
	struct list_head	qp_list; /* list of all qps on this dev */
	spinlock_t		qp_list_lock; /* protect qp_list */
	struct list_head	dip_list; /* list of all dest ips on this dev */
	spinlock_t		dip_list_lock; /* protect dip_list */

	struct list_head        pgdir_list;
	struct mutex            pgdir_mutex;
	int			irq[HIHTR_MAX_IRQ_NUM];
	u8 __iomem		*reg_base;
	void __iomem		*mem_base;
	struct hihtr_caps	caps;
#if defined(RDMA_HAVE_XARRAY)
	struct xarray		qp_table_xa;
#else
	struct radix_tree_root qp_table_root;
	spinlock_t	tree_lock;
#endif

	struct hihtr_dca_ctx	dca_ctx;

	unsigned char	dev_addr[HIHTR_MAX_PORTS][ETH_ALEN];
	u64			sys_image_guid;
	u32                     vendor_id;
	u32                     vendor_part_id;
	u32                     hw_rev;
	void __iomem            *priv_addr;

	struct hihtr_cmdq	cmd;
	struct hihtr_ida pd_ida;
	struct hihtr_ida xrcd_ida;
	struct hihtr_ida uar_ida;
	struct hihtr_mr_table  mr_table;
	struct hihtr_cq_table  cq_table;
	struct hihtr_srq_table srq_table;
	struct hihtr_qp_table  qp_table;
	struct hihtr_eq_table  eq_table;
	struct hihtr_hem_table  qpc_timer_table;
	struct hihtr_hem_table  cqc_timer_table;
	/* GMV is the memory area that the driver allocates for the hardware
	 * to store SGID, SMAC and VLAN information.
	 */
	struct hihtr_hem_table  gmv_table;

	int			cmd_mod;
	u8			mac_type;
	int			loop_idc;
	u32			sdb_offset;
	u32			odb_offset;
	struct page		*reset_page; /* store reset state */
	void			*reset_kaddr; /* addr of reset page */
	const struct hihtr_hw *hw;
	void			*priv;
	struct workqueue_struct *irq_workq;
	struct work_struct ecc_work;
	u32 func_num;
	u8 is_vf;
	u32 cong_algo_tmpl_id;
	u32 scc_algo_sel;
	u64 dwqe_page;
	struct hihtr_port port_data[HIHTR_MAX_PORTS];
	atomic64_t *dfx_cnt;
	struct hihtr_qp_cnt qp_cnt;
	struct hlist_head gmv_hash_table[HIHTR_MCAST_HASH_SIZE];
	spinlock_t gmv_hash_lock;
	u32 mcast_count;
	u8 inline_type;
	unsigned long			en_flags;
	spinlock_t dwqe_resource_lock;
	struct list_head dwqe_pid_list;
    spinlock_t dwqe_pid_lock;
	pid_t pid;
	struct page		*user_count_page;
	hihtr_history_cqe_error_count *user_count_buf;
	hihtr_history_error_count *kernel_count_buf;
};

static inline struct hihtr_dev *to_hr_dev(struct ib_device *ib_dev)
{
	return container_of(ib_dev, struct hihtr_dev, ib_dev);
}

static inline struct hihtr_ucontext
			*to_hr_ucontext(struct ib_ucontext *ibucontext)
{
	return container_of(ibucontext, struct hihtr_ucontext, ibucontext);
}

static inline struct hihtr_pd *to_hr_pd(struct ib_pd *ibpd)
{
	return container_of(ibpd, struct hihtr_pd, ibpd);
}

static inline struct hihtr_xrcd *to_hr_xrcd(struct ib_xrcd *ibxrcd)
{
	return container_of(ibxrcd, struct hihtr_xrcd, ibxrcd);
}

static inline struct hihtr_ah *to_hr_ah(struct ib_ah *ibah)
{
	return container_of(ibah, struct hihtr_ah, ibah);
}

static inline struct hihtr_mr *to_hr_mr(struct ib_mr *ibmr)
{
	return container_of(ibmr, struct hihtr_mr, ibmr);
}

static inline struct hihtr_mw *to_hr_mw(struct ib_mw *ibmw)
{
	return container_of(ibmw, struct hihtr_mw, ibmw);
}

static inline struct hihtr_qp *to_hr_qp(struct ib_qp *ibqp)
{
	return container_of(ibqp, struct hihtr_qp, ibqp);
}

static inline struct hihtr_cq *to_hr_cq(struct ib_cq *ib_cq)
{
	return container_of(ib_cq, struct hihtr_cq, ib_cq);
}

static inline struct hihtr_srq *to_hr_srq(struct ib_srq *ibsrq)
{
	return container_of(ibsrq, struct hihtr_srq, ibsrq);
}

static inline struct hihtr_user_mmap_entry *
to_hihtr_mmap(struct rdma_user_mmap_entry *rdma_entry)
{
	return container_of(rdma_entry, struct hihtr_user_mmap_entry, rdma_entry);
}

static inline void hihtr_write64_k(__le32 val[2], void __iomem *dest)
{
	writeq(*(u64 *)val, dest);
}

#if defined(RDMA_HAVE_XARRAY)
static inline struct hihtr_qp
	*__hihtr_qp_lookup(struct hihtr_dev *hr_dev, u32 qpn)
{
	return xa_load(&hr_dev->qp_table_xa, qpn);
}
#endif

static inline void *hihtr_buf_offset(struct hihtr_buf *buf,
					unsigned int offset)
{
	return (char *)(buf->trunk_list[offset >> buf->trunk_shift].buf) +
			(offset & ((1 << buf->trunk_shift) - 1));
}

static inline dma_addr_t hihtr_buf_dma_addr(struct hihtr_buf *buf,
					       unsigned int offset)
{
	return buf->trunk_list[offset >> buf->trunk_shift].map +
			(offset & ((1 << buf->trunk_shift) - 1));
}

static inline dma_addr_t hihtr_buf_page(struct hihtr_buf *buf, u32 idx)
{
	return hihtr_buf_dma_addr(buf, idx << buf->page_shift);
}

#define hr_hw_page_align(x)		ALIGN(x, 1 << HIHTR_HW_PAGE_SHIFT)

static inline u64 to_hr_hw_page_addr(u64 addr)
{
	return addr >> HIHTR_HW_PAGE_SHIFT;
}

static inline u32 to_hr_hw_page_shift(u32 page_shift)
{
	return page_shift - HIHTR_HW_PAGE_SHIFT;
}

static inline u32 to_hr_hem_hopnum(u32 hopnum, u32 count)
{
	if (count > 0)
		return hopnum == HIHTR_HOP_NUM_0 ? 0 : hopnum;

	return 0;
}

static inline u32 to_hr_hem_entries_size(u32 count, u32 buf_shift)
{
	return hr_hw_page_align(count << buf_shift);
}

static inline u32 to_hr_hem_entries_count(u32 count, u32 buf_shift)
{
	return hr_hw_page_align(count << buf_shift) >> buf_shift;
}

static inline u32 to_hr_hem_entries_shift(u32 count, u32 buf_shift)
{
	if (!count)
		return 0;

	return ilog2(to_hr_hem_entries_count(count, buf_shift));
}

#define DSCP_SHIFT 2
#define HIHTR_COS_MIN 4

static inline u8 get_dscp_from_tclass(const struct ib_global_route *grh)
{
	return grh->sgid_attr->gid_type == IB_GID_TYPE_ROCE_UDP_ENCAP ?
	       grh->traffic_class >> DSCP_SHIFT : grh->traffic_class;
}

static inline u8 to_rdma_port_num(u8 phy_port_num)
{
	return phy_port_num + 1;
}

static inline enum ib_port_state get_port_state(struct net_device *net_dev)
{
	return (netif_running(net_dev) && netif_carrier_ok(net_dev)) ?
		IB_PORT_ACTIVE : IB_PORT_DOWN;
}

void hihtr_init_uar_table(struct hihtr_dev *dev);
int hihtr_uar_alloc(struct hihtr_dev *dev, struct hihtr_uar *uar);

int hihtr_cmd_init(struct hihtr_dev *hr_dev);
void hihtr_cmd_cleanup(struct hihtr_dev *hr_dev);
void hihtr_cmd_event(struct hihtr_dev *hr_dev, u16 token, u8 status,
			u64 out_param);
int hihtr_cmd_use_events(struct hihtr_dev *hr_dev);
void hihtr_cmd_use_polling(struct hihtr_dev *hr_dev);

/* hihtr hw need current block and next block addr from mtt */
#define MTT_MIN_COUNT	 2
static inline dma_addr_t hihtr_get_mtr_ba(struct hihtr_mtr *mtr)
{
	return mtr->hem_cfg.root_ba;
}
int hihtr_mtr_find(struct hihtr_dev *hr_dev, struct hihtr_mtr *mtr,
		      u32 offset, u64 *mtt_buf, int mtt_max, u64 *base_addr);

int hihtr_mtr_create(struct hihtr_dev *hr_dev, struct hihtr_mtr *mtr,
			struct hihtr_buf_attr *buf_attr,
			unsigned int page_shift, struct ib_udata *udata,
			unsigned long user_addr, struct ib_uobject *uobject);
void hihtr_mtr_destroy(struct hihtr_dev *hr_dev,
			  struct hihtr_mtr *mtr);
int hihtr_mtr_map(struct hihtr_dev *hr_dev, struct hihtr_mtr *mtr,
		     dma_addr_t *pages, unsigned int page_cnt);

void hihtr_init_pd_table(struct hihtr_dev *hr_dev);
void hihtr_init_mr_table(struct hihtr_dev *hr_dev);
void hihtr_init_cq_table(struct hihtr_dev *hr_dev);
int hihtr_init_qp_table(struct hihtr_dev *hr_dev);
void hihtr_init_srq_table(struct hihtr_dev *hr_dev);
void hihtr_init_xrcd_table(struct hihtr_dev *hr_dev);

void hihtr_cleanup_eq_table(struct hihtr_dev *hr_dev);
void hihtr_cleanup_cq_table(struct hihtr_dev *hr_dev);
void hihtr_cleanup_qp_table(struct hihtr_dev *hr_dev);

#if defined(ROCE_CREATE_AH_INT) && defined(ROCE_CREATE_AH_HAVE_FLAGS)
int hihtr_create_ah(struct ib_ah *ibah, struct rdma_ah_attr *init_attr, u32 flags, struct ib_udata *udata);
#elif defined(ROCE_CREATE_AH_INT)
int hihtr_create_ah(struct ib_ah *ibah, struct rdma_ah_init_attr *init_attr, struct ib_udata *udata);
#else
#if defined(ROCE_CREATE_AH_HAVE_FLAGS) || defined(ROCE_IB_DEVICE_CREATE_AH_HAVE_FLAGS)
struct ib_ah *hihtr_create_ah(struct ib_pd *pd, struct rdma_ah_attr *ah_attr, u32 flags, struct ib_udata *udata);
#else
struct ib_ah *hihtr_create_ah(struct ib_pd *pd, struct rdma_ah_attr *ah_attr, struct ib_udata *udata);
#endif
#endif

int hihtr_query_ah(struct ib_ah *ibah, struct rdma_ah_attr *ah_attr);

#if defined(ROCE_CREATE_AH_INT)
#if defined(ROCE_DESTROY_AH_VOID)
static inline void hihtr_destroy_ah(struct ib_ah *ah, u32 flags) {}
#else
static inline int hihtr_destroy_ah(struct ib_ah *ah, u32 flags)
{
	return 0;
}
#endif
#else
#if defined(ROCE_DESTROY_AH_HAVE_FLAGS) || defined(ROCE_IB_DEVICE_DESTROY_AH_HAVE_FLAGS)
static inline int hihtr_destroy_ah(struct ib_ah *ah, u32 flags)
#else
static inline int hihtr_destroy_ah(struct ib_ah *ah)
#endif
{
	struct hihtr_ah *roce_ah = NULL;

	if (ah == NULL) {
		pr_err("hihtr_destroy_ah:ah is NULL\n");
		return -EINVAL;
	}

	roce_ah = to_hr_ah(ah);

	kfree(roce_ah);

	return 0;
}
#endif

#if defined(ROCE_ALLOC_PD_INT)
int hihtr_alloc_pd(struct ib_pd *pd, struct ib_udata *udata);
#else
struct ib_pd *hihtr_alloc_pd(struct ib_device *ibdev, struct ib_ucontext *ibcontext, struct ib_udata *udata);
#endif

#if defined(ROCE_DEALLOC_PD_VOID)
void hihtr_dealloc_pd(struct ib_pd *pd, struct ib_udata *udata);
#elif defined(ROCE_DEALLOC_PD_HAVE_UDATA)
int hihtr_dealloc_pd(struct ib_pd *pd, struct ib_udata *udata);
#else
int hihtr_dealloc_pd(struct ib_pd *ibpd);
#endif

struct ib_mr *hihtr_get_dma_mr(struct ib_pd *pd, int acc);
struct ib_mr *hihtr_reg_user_mr(struct ib_pd *pd, u64 start, u64 length,
				   u64 virt_addr, int access_flags,
				   struct ib_udata *udata);

#if defined(ROCE_CREATE_QP_INT)
int hihtr_create_qp(struct ib_qp *ib_qp, struct ib_qp_init_attr *init_attr, struct ib_udata *udata);
#else
struct ib_qp *hihtr_create_qp(struct ib_pd *ib_pd, struct ib_qp_init_attr *init_attr, struct ib_udata *udata);
#endif

#if defined(REREG_USR_MR_RETURN_IB_MR)
struct ib_mr *hihtr_rereg_user_mr(struct ib_mr *ibmr, int flags, u64 start, u64 length,
			   u64 virt_addr, int mr_access_flags, struct ib_pd *pd,
			   struct ib_udata *udata);
#else
int hihtr_rereg_user_mr(struct ib_mr *mr, int flags, u64 start, u64 length,
			   u64 virt_addr, int mr_access_flags, struct ib_pd *pd,
			   struct ib_udata *udata);
#endif

#if defined(ROCE_HAVE_FILL_RES_SRQ_ENTRY)
int hihtr_fill_res_srq_entry(struct sk_buff *msg, struct ib_srq *ib_srq);
int hihtr_fill_res_srq_entry_raw(struct sk_buff *msg, struct ib_srq *ib_srq);
#endif

#if defined(ROCE_ALLOC_MR_HAVE_UDATA)
struct ib_mr *hihtr_alloc_mr(struct ib_pd *pd, enum ib_mr_type mr_type, u32 max_num_sg, struct ib_udata *udata);
#else
struct ib_mr *hihtr_alloc_mr(struct ib_pd *pd, enum ib_mr_type mr_type, u32 max_num_sg);
#endif

int hihtr_map_mr_sg(struct ib_mr *ibmr, struct scatterlist *sg, int sg_nents,
		       unsigned int *sg_offset);

#if defined(ROCE_DEREG_MR_INT_HAVE_UDATA)
int hihtr_dereg_mr(struct ib_mr *ibmr, struct ib_udata *udata);
#else
int hihtr_dereg_mr(struct ib_mr *ibmr);
#endif

unsigned long key_to_hw_index(u32 key);

#if defined(ROCE_ALLOC_MW_INT)
int hihtr_alloc_mw(struct ib_mw *mw, struct ib_udata *udata);
#else
struct ib_mw *hihtr_alloc_mw(struct ib_pd *ibpd, enum ib_mw_type type, struct ib_udata *udata);
#endif
int hihtr_dealloc_mw(struct ib_mw *ibmw);

void hihtr_buf_free(struct hihtr_dev *hr_dev, struct hihtr_buf *buf);
struct hihtr_buf *hihtr_buf_alloc(struct hihtr_dev *hr_dev, u32 size,
					u32 page_shift, u32 flags);

int hihtr_get_kmem_bufs(struct hihtr_dev *hr_dev, dma_addr_t *bufs,
			   int buf_cnt, struct hihtr_buf *buf,
			   unsigned int page_shift);
int hihtr_get_umem_bufs(struct hihtr_dev *hr_dev, dma_addr_t *bufs,
			   int buf_cnt, struct ib_umem *umem,
			   unsigned int page_shift);

struct hihtr_user_mmap_entry *
hihtr_user_mmap_entry_insert(struct ib_ucontext *ucontext, u64 address,
				size_t length,
				enum hihtr_mmap_type mmap_type);

#if defined(ROCE_CREATE_SRQ_INT)
int hihtr_create_srq(struct ib_srq *srq, struct ib_srq_init_attr *srq_init_attr, struct ib_udata *udata);
#else
struct ib_srq *hihtr_create_srq(struct ib_pd *ibpd, struct ib_srq_init_attr *srq_init_attr, struct ib_udata *udata);
#endif
int hihtr_modify_srq(struct ib_srq *ibsrq, struct ib_srq_attr *srq_attr,
			enum ib_srq_attr_mask srq_attr_mask,
			struct ib_udata *udata);

#if defined(ROCE_DESTROY_SRQ_VOID)
void hihtr_destroy_srq(struct ib_srq *ibsrq, struct ib_udata *udata);
#elif defined(ROCE_DESTROY_SRQ_HAVE_UDATA)
int hihtr_destroy_srq(struct ib_srq *ibsrq, struct ib_udata *udata);
#else
int hihtr_destroy_srq(struct ib_srq *ibsrq);
#endif

#if defined(ROCE_ALLOC_XRCD_INT)
int hihtr_alloc_xrcd(struct ib_xrcd *ib_xrcd, struct ib_udata *udata);
#elif defined(ROCE_IB_DEVICE_ALLOC_XRCD_HAVE_IB_UCONTEXT) || defined(ROCE_ALLOC_XRCD_HAVE_IB_UCONTEXT)
struct ib_xrcd *hihtr_alloc_xrcd(struct ib_device *ibdev, struct ib_ucontext *ibcontext, struct ib_udata *udata);
#else
struct ib_xrcd *hihtr_alloc_xrcd(struct ib_device *ibdev, struct ib_udata *udata);
#endif

#if defined(ROCE_DEALLOC_XRCD_HAVE_IB_UDATA)
int hihtr_dealloc_xrcd(struct ib_xrcd *ib_xrcd, struct ib_udata *udata);
#else
int hihtr_dealloc_xrcd(struct ib_xrcd *ib_xrcd);
#endif
int hihtr_modify_qp(struct ib_qp *ibqp, struct ib_qp_attr *attr,
		       int attr_mask, struct ib_udata *udata);
void init_flush_work(struct hihtr_dev *hr_dev, struct hihtr_qp *hr_qp);
void *hihtr_get_recv_wqe(struct hihtr_qp *hr_qp, unsigned int n);
void *hihtr_get_send_wqe(struct hihtr_qp *hr_qp, unsigned int n);
void *hihtr_get_extend_sge(struct hihtr_qp *hr_qp, unsigned int n);
bool hihtr_wq_overflow(struct hihtr_wq *hr_wq, u32 nreq,
			  struct ib_cq *ib_cq);
void hihtr_lock_cqs(struct hihtr_cq *send_cq,
		       struct hihtr_cq *recv_cq);
void hihtr_unlock_cqs(struct hihtr_cq *send_cq,
			 struct hihtr_cq *recv_cq);
void hihtr_qp_remove(struct hihtr_dev *hr_dev, struct hihtr_qp *hr_qp);
void hihtr_qp_destroy(struct hihtr_dev *hr_dev, struct hihtr_qp *hr_qp,
			 struct ib_udata *udata);
__be32 send_ieth(const struct ib_send_wr *wr);
int to_hr_qp_type(int qp_type);

#if defined(ROCE_CREATE_CQ_INT_HAVE_BUNDLE)
int hihtr_create_cq(struct ib_cq *ib_cq, const struct ib_cq_init_attr *attr,
            	struct uverbs_attr_bundle *attrs);
#elif defined(ROCE_CREATE_CQ_INT)
int hihtr_create_cq(struct ib_cq *ib_cq, const struct ib_cq_init_attr *attr,
		       struct ib_udata *udata);
#else
struct ib_cq *hihtr_create_cq(struct ib_device *ibdev, const struct ib_cq_init_attr *attr,
								 struct ib_ucontext *ibcontext, struct ib_udata *udata);
#endif

#if defined(ROCE_DESTROY_CQ_VOID)
void hihtr_destroy_cq(struct ib_cq *ib_cq, struct ib_udata *udata);
#elif defined(ROCE_DESTROY_CQ_HAVE_UDATA)
int hihtr_destroy_cq(struct ib_cq *ib_cq, struct ib_udata *udata);
#else
int hihtr_destroy_cq(struct ib_cq *ib_cq);
#endif

int hihtr_db_map_user(struct hihtr_ucontext *context, unsigned long virt,
			 struct hihtr_db *db, struct ib_udata *udata);
void hihtr_db_unmap_user(struct hihtr_ucontext *context,
			    struct hihtr_db *db);
int hihtr_alloc_db(struct hihtr_dev *hr_dev, struct hihtr_db *db,
		      int order);
void hihtr_free_db(struct hihtr_dev *hr_dev, struct hihtr_db *db);

void hihtr_cq_completion(struct hihtr_dev *hr_dev, u32 cqn);
int hihtr_resize_cq(struct ib_cq *ibcq, int entries, struct ib_udata *udata);
void hihtr_cq_event(struct hihtr_dev *hr_dev, u32 cqn, int event_type);
void flush_cqe(struct hihtr_dev *dev, struct hihtr_qp *qp);
void hihtr_qp_event(struct hihtr_dev *hr_dev, u32 qpn, int event_type);
void hihtr_srq_event(struct hihtr_dev *hr_dev, u32 srqn, int event_type);
u8 hihtr_get_gid_index(struct hihtr_dev *hr_dev, u8 port, int gid_index);
void hihtr_handle_device_err(struct hihtr_dev *hr_dev);
int hihtr_fill_res_cq_entry(struct sk_buff *msg, struct ib_cq *ib_cq);
int hihtr_fill_res_cq_entry_raw(struct sk_buff *msg, struct ib_cq *ib_cq);
int hihtr_fill_res_qp_entry(struct sk_buff *msg, struct ib_qp *ib_qp);
int hihtr_fill_res_qp_entry_raw(struct sk_buff *msg, struct ib_qp *ib_qp);
int hihtr_fill_res_mr_entry(struct sk_buff *msg, struct ib_mr *ib_mr);
int hihtr_fill_res_mr_entry_raw(struct sk_buff *msg, struct ib_mr *ib_mr);

int hihtr_init(struct hihtr_dev *hr_dev, char *uld_dev_name);
void hihtr_exit(struct hihtr_dev *hr_dev);

/* Adapt for hnae5, TODO: replace by hisdk5
 */
enum hinic5_tc_map_mode {
	HINIC5_TC_MAP_MODE_PCP,
	HINIC5_TC_MAP_MODE_DSCP,
};

enum hnae5_mac_type {
	HNAE5_MAC_ETH,
	HNAE5_MAC_ROH,
};

int hihtr_register_sysfs(struct hihtr_dev *hr_dev);
int hihtr_register_device(struct hihtr_dev *hr_dev, char *uld_dev_name);
void hihtr_add_dwqe_pid(struct hihtr_dev *hr_dev, pid_t c_pid);
void hihtr_remove_dwqe_pid(struct hihtr_dev *hr_dev, pid_t c_pid);
void hihtr_unregister_sysfs(struct hihtr_dev *hr_dev);

#if defined(HIHTR_DT)
int hihtr_v3_profile(struct hihtr_dev *hr_dev);
int hihtr_v3_vf_profile(struct hihtr_dev *hr_dev);
int hihtr_mmap(struct ib_ucontext *uctx, struct vm_area_struct *vma);
int hihtr_v3_set_hem(struct hihtr_dev *hr_dev,
			       struct hihtr_hem_table *table, int obj,
			       u32 step_idx);
int hihtr_v3_clear_hem(struct hihtr_dev *hr_dev,
				 struct hihtr_hem_table *table,
				 int tag, u32 step_idx);
#endif

#endif /* _HIHTR_DEVICE_H */
