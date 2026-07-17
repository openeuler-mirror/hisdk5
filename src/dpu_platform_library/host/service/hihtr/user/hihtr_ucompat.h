/*
 * Copyright (c) 2025 Hisilicon Limited. All rights reserved.
 */
#ifndef _HIHTR_UCOMPAT_H
#define _HIHTR_UCOMPAT_H

#include <stdio.h>
#include <stdlib.h>
#include <infiniband/driver.h>
#include <infiniband/cmd_write.h>
#include "roce_ucompat_gen.h"
#if !defined(UTIL_BITMAP_HAVE_BITMAP_ALLOC1)
#include <ccan/bitmap.h>
#endif

#define NUM_5 (5)
#define NUM_4 (4)
#define NUM_3 (3)
#define NUM_2 (2)
#define FLOW_LABEL_HIGH_BIT_SHIFT (14)

#define ASSERT(f)  assert(f)

#if defined(UTIL_NEED_BIT)
#define BIT(nr)     (1UL << (nr))
#endif

#if defined(VERBS_DRIVER_NEED_VERBS_CQ)
struct verbs_cq {
	union {
		struct ibv_cq cq;
		struct ibv_cq_ex cq_ex;
	};
};
#endif

#if !defined(MAD_HAVE_ALIGN)
#define ALIGN(l, size) (((l) + ((size) - 1)) / (size) * (size))
#endif

#if !defined(ILOG_HAVE_ILOG32)
int ilog32(uint32_t _v);
int ilog64(uint64_t _v);
#endif

#if defined(UTIL_NEED_ROUNDUP_POW_OF_TWO)
static inline uint64_t roundup_pow_of_two(uint64_t n)
{
	return n == 1 ? 1 : 1ULL << ilog64(n - 1);
}
#endif

#if defined(VERBS_DRIVER_NEED_VERBS_LOG)
enum {
	VERBS_LOG_LEVEL_NONE,
	VERBS_LOG_ERR,
	VERBS_LOG_WARN,
	VERBS_LOG_INFO,
	VERBS_LOG_DEBUG,
};

void __verbs_log(struct verbs_context *ctx, uint32_t level,
		 const char *fmt, ...);

#define verbs_log(ctx, level, format, arg...)                                  \
do {                                                                           \
	int tmp = errno;                                                       \
	__verbs_log(ctx, level, "%s: %s:%d: " format,                          \
		    (ctx)->context.device->name, __func__, __LINE__, ##arg);   \
	errno = tmp;                                                           \
} while (0)

#define verbs_debug(ctx, format, arg...) \
	verbs_log(ctx, VERBS_LOG_DEBUG, format, ##arg)

#define verbs_info(ctx, format, arg...) \
	verbs_log(ctx, VERBS_LOG_INFO, format, ##arg)

#define verbs_warn(ctx, format, arg...) \
	verbs_log(ctx, VERBS_LOG_WARN, format, ##arg)

#define verbs_err(ctx, format, arg...) \
	verbs_log(ctx, VERBS_LOG_ERR, format, ##arg)

#define verbs_log_datapath(ctx, level, format, arg...) \
	verbs_log(ctx, level, format, ##arg)

#define verbs_debug_datapath(ctx, format, arg...) \
	verbs_log_datapath(ctx, VERBS_LOG_DEBUG, format, ##arg)

#define verbs_info_datapath(ctx, format, arg...) \
	verbs_log_datapath(ctx, VERBS_LOG_INFO, format, ##arg)

#define verbs_warn_datapath(ctx, format, arg...) \
	verbs_log_datapath(ctx, VERBS_LOG_WARN, format, ##arg)

#define verbs_err_datapath(ctx, format, arg...) \
	verbs_log_datapath(ctx, VERBS_LOG_ERR, format, ##arg)
#endif

#if defined(UTIL_NEED_DIV_ROUND_UP)
static inline unsigned long DIV_ROUND_UP(unsigned long n, unsigned long d)
{
	return (n + d - 1) / d;
}
#endif

#if defined(UTIL_NEED_ALIGN)
static inline unsigned long align(unsigned long val, unsigned long align)
{
	return (val + align - 1) & ~(align - 1);
}
#endif

#if defined(UTIL_NEED_FIELD_GET)
#define __bf_shf(x) (__builtin_ffsll(x) - 1)

#define FIELD_GET(_mask, _reg)                                                 \
	({                                                                     \
		(typeof(_mask))(((_reg) & (_mask)) >> __bf_shf(_mask));        \
	})

#define FIELD_PREP(_mask, _val)                                                \
	({                                                                     \
		((typeof(_mask))(_val) << __bf_shf(_mask)) & (_mask);          \
	})
#endif

#if defined(VERBS_NEED_IBV_DATA_BUF)
struct ibv_data_buf {
	void *addr;
	size_t length;
};
#endif

#if defined(UTIL_NEED_GENMASK)
#define GENMASK(h, l) \
	(((~0UL) - (1UL << (l)) + 1) & (~0UL >> (BITS_PER_LONG - 1 - (h))))
#endif

#if !defined(BITMAP_HAVE_BITMAP_FIND_FIRST_BIT)
#define BITS_PER_LONG	   (8 * sizeof(long))
#define BITS_TO_LONGS(nr)  (((nr) + BITS_PER_LONG - 1) / BITS_PER_LONG)
#define BMP_WORD_INDEX(n) (BITS_TO_LONGS((n) + 1) - 1)
#define BMP_FIRST_WORD_MASK(start) (~0UL << ((start) & (BITS_PER_LONG - 1)))

unsigned long bitmap_find_first_bit(const bitmap *bmp,
				    unsigned long start, unsigned long end);
#endif

#if !defined(HAVE_IBV_QP_INIT_ATTR_SEND_OPS_FLAGS)
#define IBV_QP_INIT_ATTR_SEND_OPS_FLAGS (1 << 6)
#endif

#if defined(NEED_IB_GRH_FLOWLABEL_MASK)
#define IB_ROCE_UDP_ENCAP_VALID_PORT_MIN (0xC000)
#define IB_ROCE_UDP_ENCAP_VALID_PORT_MAX (0xFFFF)
#define IB_GRH_FLOWLABEL_MASK (0x000FFFFF)
#endif

#if defined(UTIL_NEED_GET_RANDOM)
unsigned int get_random(void);
#endif

#if defined(NEED_IBV_FLOW_LABEL_TO_UDP_SPORT)
static inline uint16_t ibv_flow_label_to_udp_sport(uint32_t fl)
{
	uint32_t fl_low = fl & 0x03FFF, fl_high = fl & 0xFC000;

	fl_low ^= fl_high >> FLOW_LABEL_HIGH_BIT_SHIFT;
	return (uint16_t)(fl_low | IB_ROCE_UDP_ENCAP_VALID_PORT_MIN);
}
#endif

struct verbs_context *hihtr_alloc_context(struct ibv_device *ibdev, int cmd_fd, void *private_data);

#ifdef ALLOC_CONTEXT_HAVE_PRIVATE_DATA
struct verbs_context *compat_hihtr_alloc_context(struct ibv_device *ibdev,
						    int cmd_fd,
						    void *private_data);
#else
struct verbs_context *compat_hihtr_alloc_context(struct ibv_device *ibdev,
						    int cmd_fd);
#endif

int compat_ibv_cmd_modify_qp_ex(struct ibv_qp *qp,
                                struct ibv_qp_attr *attr,
                                int attr_mask,
                                struct ibv_modify_qp_ex *cmd,
								size_t cmd_core_size, size_t cmd_size,
                                struct ib_uverbs_ex_modify_qp_resp *resp,
								size_t resp_core_size, size_t resp_size);

#if !defined(HAVE_IBV_CMD_QUERY_DEV_ANY)
int compat_ibv_cmd_query_device_ex(struct ibv_context *context,
                            const struct ibv_query_device_ex_input *query_input,
                            struct ibv_device_attr_ex *device_attr, size_t attr_size,
                            uint64_t *raw_fw_version,
                            struct ibv_query_device_ex *query_cmd,
                            size_t cmd_size,
                            struct ib_uverbs_ex_query_device_resp *query_resp,
                            size_t resp_size);
#endif

int compat_ibv_cmd_create_qp_ex2(struct ibv_context *context,
                          struct verbs_qp *qp, int vqp_size,
                          struct ibv_qp_init_attr_ex *qp_init_attr,
                          struct ibv_create_qp_ex *create_cmd,
                          size_t cmd_size, size_t cmd_core_size,
                          struct ib_uverbs_ex_create_qp_resp *create_resp,
                          size_t resp_size, size_t resp_core_size);

#endif /* _HIHTR_UCOMPAT_H */