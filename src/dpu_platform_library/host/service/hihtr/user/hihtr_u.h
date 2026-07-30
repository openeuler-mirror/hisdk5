/*
 * Copyright (c) 2016-2017 Hisilicon Limited.
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

#ifndef _HIHTR_U_H
#define _HIHTR_U_H

#include <stddef.h>
#include <endian.h>
#include <util/compiler.h>

#include <infiniband/driver.h>
#include <util/udma_barrier.h>
#include <util/util.h>
#include <infiniband/verbs.h>
#include <ccan/array_size.h>
#include "roce_ucompat_gen.h"
#if defined(UTIL_BITMAP_HAVE_BITMAP_ALLOC1)
#include <util/bitmap.h>
#else
#include <ccan/bitmap.h>
#endif
#include <ccan/container_of.h>
#include <linux/if_ether.h>
#include "hihtr_u_abi.h"
#include "hihtr_shared.h"

#define HIHTR_HW_VER2		0x100
#define HIHTR_HW_VER3		0x130

#define PFX				"hns: "

typedef _Atomic(uint64_t) atomic_bitmap_t;

/* The minimum page size is 4K for hardware */
#define HIHTR_HW_PAGE_SHIFT 12
#define HIHTR_HW_PAGE_SIZE (1 << HIHTR_HW_PAGE_SHIFT)

#define HIHTR_MAX_RC_INL_INN_SZ	32
#define HIHTR_MAX_UD_INL_INN_SZ	8
#define HIHTR_MIN_CQE_NUM		0x40
#define HIHTR_V2_MIN_WQE_NUM		0x40
#define HIHTR_MIN_SRQ_WQE_NUM	1

#define HIHTR_CQE_SIZE 0x20
#define HIHTR_V3_CQE_SIZE 0x40

#define HIHTR_SQWQE_SHIFT		6
#define HIHTR_SGE_IN_WQE		2
#define HIHTR_SGE_SIZE		16
#define HIHTR_SGE_SHIFT		4

#define HIHTR_GID_SIZE		16

#define INVALID_LKEY 0x0
#define INVALID_SGE_LENGTH 0x80000000

#define HIHTR_DWQE_PAGE_SIZE 4096

#define HIHTR_ADDRESS_MASK 0xFFFFFFFF
#define HIHTR_ADDRESS_SHIFT 32

#define RDMA_DRIVER_HIHTR 7386

#define RAW_FW_MINOR_VER_SHIFT 16
#define RAW_FW_MAJOR_VER_SHIFT 32
#define RAW_FW_SUBMINOR_VER_MASK  0xFFFF
#define DCA_BUF_SYNC_RATIO 2
#define DCA_MEM_REG_ATTR_NUM 4
#define DCA_MEM_SHRINK_ATTR_NUM 4
#define DCA_MEM_QUERY_ATTR_NUM 5
#define DCA_MEM_DETACH_ATTR_NUM 4
#define DCA_MEM_ATTACH_ATTR_NUM 6

#define NEON_VEC_BYTES    16
#define NEON_VEC_0        0
#define NEON_VEC_1        1
#define NEON_VEC_2        2
#define NEON_VEC_3        3

#define roce_get_field(origin, mask, shift) \
	(((le32toh(origin)) & (mask)) >> (shift))

#define roce_get_bit(origin, shift) \
	roce_get_field((origin), (1ul << (shift)), (shift))

#define roce_set_field(origin, mask, shift, val) \
	do { \
		(origin) &= ~htole32(mask); \
		(origin) |= htole32(((unsigned int)(val) << (shift)) & (mask)); \
	} while (0)

#define roce_set_bit(origin, shift, val) \
	roce_set_field((origin), (1ul << (shift)), (shift), (val))

#define FIELD_LOC(field_type, field_h, field_l)                                \
	field_type, field_h,                                                   \
		field_l + BUILD_ASSERT_OR_ZERO(((field_h) / 32) ==             \
					       ((field_l) / 32))

#define _hr_reg_enable(ptr, field_type, field_h, field_l)                      \
	({                                                                     \
		const field_type *_ptr = ptr;                                  \
		BUILD_ASSERT((field_h) == (field_l));                          \
		*((__le32 *)_ptr + (field_h) / 32) |=                          \
			htole32(BIT((field_l) % 32));                          \
	})

#define hr_reg_enable(ptr, field) _hr_reg_enable(ptr, field)

#define _hr_reg_clear(ptr, field_type, field_h, field_l)                       \
	({                                                                     \
		const field_type *_ptr = ptr;                                  \
		BUILD_ASSERT((field_h) >= (field_l));                          \
		*((__le32 *)_ptr + (field_h) / 32) &=                          \
			~htole32(GENMASK((field_h) % 32, (field_l) % 32));     \
	})

#define hr_reg_clear(ptr, field) _hr_reg_clear(ptr, field)

#define _hr_reg_write_bool(ptr, field_type, field_h, field_l, val)             \
	({                                                                     \
		(val) ? _hr_reg_enable(ptr, field_type, field_h, field_l) :    \
			      _hr_reg_clear(ptr, field_type, field_h, field_l);    \
	})

#define hr_reg_write_bool(ptr, field, val) _hr_reg_write_bool(ptr, field, val)

#define _hr_reg_write(ptr, field_type, field_h, field_l, val)                  \
	({                                                                     \
		const uint32_t _val = val;                                     \
		_hr_reg_clear(ptr, field_type, field_h, field_l);              \
		*((__le32 *)ptr + (field_h) / 32) |= htole32(FIELD_PREP(       \
			GENMASK((field_h) % 32, (field_l) % 32), _val));       \
	})

#define hr_reg_write(ptr, field, val) _hr_reg_write(ptr, field, val)

#define _hr_reg_read(ptr, field_type, field_h, field_l)                        \
	({                                                                     \
		const field_type *_ptr = ptr;                                  \
		BUILD_ASSERT((field_h) >= (field_l));                          \
		FIELD_GET(GENMASK((field_h) % 32, (field_l) % 32),             \
			  le32toh(*((__le32 *)_ptr + (field_h) / 32)));        \
	})

#define hr_reg_read(ptr, field) _hr_reg_read(ptr, field)

#define HIHTR_QP_TABLE_BITS 8
#define HIHTR_QP_TABLE_SIZE BIT(HIHTR_QP_TABLE_BITS)

#define HIHTR_SRQ_TABLE_BITS 8
#define HIHTR_SRQ_TABLE_SIZE BIT(HIHTR_SRQ_TABLE_BITS)

struct hihtr_device {
	struct verbs_device		ibv_dev;
	int				page_size;
	const struct hihtr_u_hw	*u_hw;
	int				hw_version;
};

struct hihtr_buf {
	void				*buf;
	unsigned int			length;
};

#define BIT_CNT_PER_BYTE       8
#define BIT_CNT_PER_LONG       (BIT_CNT_PER_BYTE * sizeof(unsigned long))

/* the sw doorbell type; */
enum hihtr_db_type {
	HIHTR_QP_TYPE_DB,
	HIHTR_CQ_TYPE_DB,
	HIHTR_SRQ_TYPE_DB,
	HIHTR_DB_TYPE_NUM
};

struct hihtr_db_page {
	struct hihtr_db_page	*prev, *next;
	struct hihtr_buf	buf;
	unsigned int		num_db;
	unsigned int		use_cnt;
#if defined(UTIL_BITMAP_HAVE_BITMAP_ALLOC1)
	unsigned long		*bitmap;
#else
	bitmap *bitmap;
#endif
};

#define HIHTR_DCA_MAX_MEM_SIZE ~0UL
#define HIHTR_DCA_DEFAULT_UNIT_PAGES 16

struct hihtr_dca_ctx {
	struct list_head mem_list;
	pthread_spinlock_t lock;
	int mem_cnt;
	unsigned int unit_size;
	uint64_t max_size;
	uint64_t min_size;
	uint64_t curr_size;

#define HIHTR_DCA_BITS_PER_STATUS 1
	unsigned int max_qps;
	unsigned int status_size;
	atomic_bitmap_t *buf_status;
	atomic_bitmap_t *sync_status;
};

struct hihtr_context {
	struct verbs_context		ibv_ctx;
	void				*uar;
	pthread_spinlock_t		uar_lock;
	void 				*db_page;
	pthread_spinlock_t		dwqe_lock;

	struct {
		struct hihtr_qp	**table;
		int			refcnt;
	} qp_table[HIHTR_QP_TABLE_SIZE];
	pthread_mutex_t			qp_table_mutex;
	uint32_t			qp_table_shift;
	uint32_t			qp_table_mask;

	struct {
		struct hihtr_srq	**table;
		int			refcnt;
	} srq_table[HIHTR_SRQ_TABLE_SIZE];
	pthread_mutex_t			srq_table_mutex;
	uint32_t			srq_table_shift;
	uint32_t			srq_table_mask;

	struct hihtr_db_page		*db_list[HIHTR_DB_TYPE_NUM];
	pthread_mutex_t			db_list_mutex;

	unsigned int			max_sq_wr;
	unsigned int			max_rq_wr;
	unsigned int			max_sge;
	unsigned int			max_srq_wr;
	unsigned int			max_srq_sge;
	uint32_t                config;
	uint32_t                max_inline_data;
	uint32_t                qp_mtt_buf_pg_sz;
	uint32_t                cqe_mtt_buf_pg_sz;
	int				max_cqe;
	unsigned int			cqe_size;
	struct hihtr_dca_ctx		dca_ctx;
	uint8_t				link_type;
	uint32_t				resize_cq_hw_cq_depth;
	hihtr_history_cqe_error_count *u_error_cnt;
};

struct hihtr_pd {
	struct ibv_pd			ibv_pd;
	unsigned int			pdn;
};

struct hihtr_cq {
	struct verbs_cq			verbs_cq;
	struct hihtr_buf		buf;
	pthread_spinlock_t		lock;
	unsigned int			cqn;
	unsigned int			cq_depth;
	unsigned int 			resize_cq_en;
	unsigned int			resize_cq_hw_cq_depth;
	unsigned int			cons_index;
	unsigned int			*db;
	unsigned int			*arm_db;
	int				arm_sn;
	unsigned long			flags;
	unsigned int			cqe_size;
	struct hihtr_v2_cqe		*cqe;
};

struct hihtr_idx_que {
	struct hihtr_buf		buf;
	unsigned int			entry_shift;
	unsigned long			*bitmap;
	int				bitmap_cnt;
	unsigned int			head;
	unsigned int			tail;
};

struct hihtr_srq {
	struct verbs_srq		verbs_srq;
	struct hihtr_idx_que		idx_que;
	struct hihtr_buf		wqe_buf;
	pthread_spinlock_t		lock;
	unsigned long			*wrid;
	unsigned int			srqn;
	unsigned int			wqe_cnt;
	unsigned int			max_gs;
	unsigned int			rsv_sge;
	unsigned int			wqe_shift;
	unsigned int			*db;
	unsigned int 			cap_flags;
	unsigned short			counter;
	unsigned long			*signal;
};

struct hihtr_wq {
	unsigned long			*wrid;
	pthread_spinlock_t		lock;
	unsigned int			wqe_cnt;
	int				max_post;
	unsigned int			head;
	unsigned int			tail;
	unsigned int			max_gs;
	uint32_t                ext_sge_cnt;
	unsigned int			rsv_sge;
	unsigned int			wqe_shift;
	unsigned int			shift; /* wq size is 2^shift */
	int				offset;
	void				*db_reg;
	unsigned long 	*signal;
};

/* record the result of sge process */
struct hihtr_sge_info {
	unsigned int valid_num; /* sge length is not 0 */
	unsigned int start_idx; /* start position of extend sge */
	unsigned int total_len; /* total length of valid sges */
};

struct hihtr_sge_ex {
	int				offset;
	unsigned int			sge_cnt;
	unsigned int			sge_shift;
};

struct hihtr_rinl_sge {
	void				*addr;
	unsigned int			len;
};

struct hihtr_rinl_wqe {
	struct hihtr_rinl_sge	*sg_list;
	unsigned int			sge_cnt;
};

struct hihtr_rinl_buf {
	struct hihtr_rinl_wqe	*wqe_list;
	unsigned int			wqe_cnt;
};

struct hihtr_dca_buf {
	void **bufs;
	unsigned int max_cnt;
	unsigned int shift;
	unsigned int dcan;
};

struct hihtr_qp {
	struct verbs_qp			verbs_qp;
	struct hihtr_buf		buf;
	struct hihtr_dca_buf		dca_wqe;
	int				max_inline_data;
	int				buf_size;
	unsigned int			sq_signal_bits;
	struct hihtr_wq		sq;
	struct hihtr_wq		rq;
	unsigned int			*rdb;
	unsigned int			*sdb;
	struct hihtr_sge_ex		ex_sge;
	unsigned int			next_sge;
	int				port_num;
	uint8_t				sl;
	uint8_t				tc_mode;
	uint8_t				priority;
	unsigned int			qkey;
	enum ibv_mtu			path_mtu;

	struct hihtr_rinl_buf	rq_rinl_buf;
	unsigned long			flags;
	int				refcnt; /* specially used for XRC */

	/* specific fields for the new post send APIs */
	int				err;
	void				*cur_wqe;
	unsigned int			rb_sq_head; /* roll back sq head */
	struct hihtr_sge_info	sge_info;
};

struct hihtr_av {
	uint8_t port;
	uint8_t gid_index;
	uint8_t hop_limit;
	uint32_t flowlabel;
	uint16_t udp_sport;
	uint8_t sl;
	uint8_t tclass;
	uint8_t dgid[HIHTR_GID_SIZE];
	uint8_t mac[ETH_ALEN];
};

struct hihtr_ah {
	struct ibv_ah ibv_ah;
	struct hihtr_av av;
};

struct hihtr_u_hw {
	uint32_t hw_version;
	struct verbs_context_ops hw_ops;
};

struct hihtr_cmd_flag {
	uint64_t comp_mask;
	uint32_t create_flags;
	uint32_t congest_type_flags;
	uint8_t protocol_type;
};

struct hihtr_dca_attach_attr {
	uint32_t sq_offset;
	uint32_t sge_offset;
	uint32_t rq_offset;
	bool force;
};

struct hihtr_dca_detach_attr {
	uint32_t sq_index;
};

#define HIHTR_BIT_MASK(nr) (1UL << ((nr) % 64))
#define HIHTR_BIT_WORD(nr) ((nr) / 64)

static inline bool atomic_test_bit(atomic_bitmap_t *p, uint32_t nr)
{
	p += HIHTR_BIT_WORD(nr);
	return !!(atomic_load(p) & HIHTR_BIT_MASK(nr));
}

static inline bool test_and_set_bit_lock(atomic_bitmap_t *p, uint32_t nr)
{
	uint64_t mask = HIHTR_BIT_MASK(nr);

	p += HIHTR_BIT_WORD(nr);
	if (atomic_load(p) & mask)
		return true;

	return (atomic_fetch_or(p, mask) & mask) != 0;
}

static inline void clear_bit_unlock(atomic_bitmap_t *p, uint32_t nr)
{
	p += HIHTR_BIT_WORD(nr);
	atomic_fetch_and(p, ~HIHTR_BIT_MASK(nr));
}

#define hr_hw_page_align(x) align(x, HIHTR_HW_PAGE_SIZE)
#define hr_hw_page_count(x) (hr_hw_page_align(x) / HIHTR_HW_PAGE_SIZE)

/*
 * The entries's buffer should be aligned to a multiple of the hardware's
 * minimum page size.
 */
#define hr_hw_page_align(x) align(x, HIHTR_HW_PAGE_SIZE)

static inline unsigned int to_hr_hem_entries_size(int count, int buf_shift)
{
	return hr_hw_page_align(count << buf_shift);
}

static inline unsigned int hr_ilog32(unsigned int count)
{
	return ilog32(count - 1);
}

static inline uint32_t to_hr_qp_table_index(uint32_t qpn,
					    struct hihtr_context *ctx)
{
	return (qpn >> ctx->qp_table_shift) & (HIHTR_QP_TABLE_SIZE - 1);
}

static inline uint32_t to_hr_srq_table_index(uint32_t srqn,
					     struct hihtr_context *ctx)
{
	return (srqn >> ctx->srq_table_shift) & (HIHTR_SRQ_TABLE_SIZE - 1);
}

static inline struct hihtr_device *to_hr_dev(struct ibv_device *ibv_dev)
{
	return container_of(ibv_dev, struct hihtr_device, ibv_dev.device);
}

static inline struct hihtr_context *to_hr_ctx(struct ibv_context *ibv_ctx)
{
	return container_of(ibv_ctx, struct hihtr_context, ibv_ctx.context);
}

static inline struct hihtr_pd *to_hr_pd(struct ibv_pd *ibv_pd)
{
	return container_of(ibv_pd, struct hihtr_pd, ibv_pd);
}

static inline struct hihtr_cq *to_hr_cq(struct ibv_cq *ibv_cq)
{
	return container_of(ibv_cq, struct hihtr_cq, verbs_cq.cq);
}

static inline struct hihtr_srq *to_hr_srq(struct ibv_srq *ibv_srq)
{
	return container_of(ibv_srq, struct hihtr_srq, verbs_srq.srq);
}

static inline struct hihtr_qp *to_hr_qp(struct ibv_qp *ibv_qp)
{
	return container_of(ibv_qp, struct hihtr_qp, verbs_qp.qp);
}

static inline struct hihtr_ah *to_hr_ah(struct ibv_ah *ibv_ah)
{
	return container_of(ibv_ah, struct hihtr_ah, ibv_ah);
}

#if defined(VERBS_OPS_HAVE_QUERY_DEVICE)
int hihtr_u_query_device(struct ibv_context *context, struct ibv_device_attr *attr);
#endif

int hihtr_u_query_device_ex(struct ibv_context *context,
			    const struct ibv_query_device_ex_input *input,
			    struct ibv_device_attr_ex *attr, size_t attr_size);
int hihtr_u_query_port(struct ibv_context *context, uint8_t port,
			  struct ibv_port_attr *attr);

struct ibv_pd *hihtr_u_alloc_pd(struct ibv_context *context);
int hihtr_u_free_pd(struct ibv_pd *pd);

#if defined(REG_MR_HAVE_HCA_VA)
struct ibv_mr *hihtr_u_reg_mr(struct ibv_pd *pd, void *addr, size_t length,
				 uint64_t hca_va, int access);
#else
struct ibv_mr *hihtr_u_reg_mr(struct ibv_pd *pd, void *addr, size_t length, int access);
#endif

int hihtr_u_rereg_mr(struct verbs_mr *vmr, int flags, struct ibv_pd *pd,
			void *addr, size_t length, int access);
int hihtr_u_dereg_mr(struct verbs_mr *vmr);

struct ibv_mw *hihtr_u_alloc_mw(struct ibv_pd *pd, enum ibv_mw_type type);
int hihtr_u_dealloc_mw(struct ibv_mw *mw);
int hihtr_u_bind_mw(struct ibv_qp *qp, struct ibv_mw *mw,
		       struct ibv_mw_bind *mw_bind);

struct ibv_cq *hihtr_u_create_cq(struct ibv_context *context, int cqe,
				    struct ibv_comp_channel *channel,
				    int comp_vector);
struct ibv_cq_ex *hihtr_u_create_cq_ex(struct ibv_context *context,
					  struct ibv_cq_init_attr_ex *cq_attr);

int hihtr_u_modify_cq(struct ibv_cq *cq, struct ibv_modify_cq_attr *attr);
int hihtr_u_destroy_cq(struct ibv_cq *cq);
int hihtr_u_resize_cq(struct ibv_cq *cq, int cqe);
void hihtr_u_cq_event(struct ibv_cq *cq);

struct ibv_srq *hihtr_u_create_srq(struct ibv_pd *pd,
				      struct ibv_srq_init_attr *srq_init_attr);
struct ibv_srq *hihtr_u_create_srq_ex(struct ibv_context *context,
					 struct ibv_srq_init_attr_ex *attr);
int hihtr_u_get_srq_num(struct ibv_srq *ibv_srq, uint32_t *srq_num);
int hihtr_u_modify_srq(struct ibv_srq *srq, struct ibv_srq_attr *srq_attr,
			  int srq_attr_mask);
int hihtr_u_query_srq(struct ibv_srq *srq, struct ibv_srq_attr *srq_attr);
struct hihtr_srq *hihtr_find_srq(struct hihtr_context *ctx,
				       uint32_t srqn);
int hihtr_u_destroy_srq(struct ibv_srq *ibv_srq);

struct ibv_qp *hihtr_u_create_qp(struct ibv_pd *pd,
				    struct ibv_qp_init_attr *attr);
struct ibv_qp *
hihtr_u_create_qp_ex(struct ibv_context *context,
			struct ibv_qp_init_attr_ex *qp_init_attr_ex);

struct ibv_qp *hihtr_u_open_qp(struct ibv_context *context,
				  struct ibv_qp_open_attr *attr);

int hihtr_u_query_qp(struct ibv_qp *ibqp, struct ibv_qp_attr *attr,
			int attr_mask, struct ibv_qp_init_attr *init_attr);

struct ibv_ah *hihtr_u_create_ah(struct ibv_pd *pd,
				    struct ibv_ah_attr *attr);
int hihtr_u_destroy_ah(struct ibv_ah *ah);

struct ibv_xrcd *
hihtr_u_open_xrcd(struct ibv_context *context,
		     struct ibv_xrcd_init_attr *xrcd_init_attr);
int hihtr_u_close_xrcd(struct ibv_xrcd *ibv_xrcd);

int hihtr_alloc_buf(struct hihtr_buf *buf, unsigned int size,
		       int page_size);
void hihtr_free_buf(struct hihtr_buf *buf);

void hihtr_free_qp_buf(struct hihtr_qp *qp, struct hihtr_context *ctx);

void hihtr_init_qp_indices(struct hihtr_qp *qp);

extern const struct hihtr_u_hw hihtr_u_hw_v2;

bool is_hihtr_dev(struct ibv_device *device);

int hihtr_attach_dca_mem(struct hihtr_context *ctx, uint32_t handle,
			    struct hihtr_dca_attach_attr *attr,
			    uint32_t size, struct hihtr_dca_buf *buf);
void hihtr_detach_dca_mem(struct hihtr_context *ctx, uint32_t handle,
			     struct hihtr_dca_detach_attr *attr);

void hihtr_shrink_dca_mem(struct hihtr_context *ctx);

void hihtr_cleanup_dca_mem(struct hihtr_context *ctx);

bool hihtr_dca_start_post(struct hihtr_dca_ctx *ctx, uint32_t dcan);

void hihtr_dca_stop_post(struct hihtr_dca_ctx *ctx, uint32_t dcan);

int hihtr_u_attach_mcast(struct ibv_qp *qp, const union ibv_gid *gid, uint16_t lid);
int hihtr_u_detach_mcast(struct ibv_qp *qp, const union ibv_gid *gid, uint16_t lid);

#endif /* _HIHTR_U_H */
