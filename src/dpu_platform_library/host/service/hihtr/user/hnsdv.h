/* SPDX-License-Identifier: GPL-2.0 OR BSD-2-Clause */
/*
 * Copyright (c) 2024 HiSilicon Limited.
 */
#ifndef __HNSDV_H__
#define __HNSDV_H__
#include <sys/types.h>
#include <infiniband/verbs.h>

enum hnsdv_context_attr_flags {
	HNSDV_CONTEXT_FLAGS_DCA = 1 << 0,
};

enum hnsdv_context_comp_mask {
	HNSDV_CONTEXT_MASK_DCA_PRIME_QPS = 1 << 0,
	HNSDV_CONTEXT_MASK_DCA_UNIT_SIZE = 1 << 1,
	HNSDV_CONTEXT_MASK_DCA_MAX_SIZE = 1 << 2,
	HNSDV_CONTEXT_MASK_DCA_MIN_SIZE = 1 << 3,
};

struct hnsdv_context_attr {
	uint64_t flags; /* Use enum hnsdv_context_attr_flags */
	uint64_t comp_mask; /* Use enum hnsdv_context_comp_mask */
	uint32_t dca_prime_qps;
	uint32_t dca_unit_size;
	uint64_t dca_max_size;
	uint64_t dca_min_size;
};

enum hnsdv_qp_init_attr_mask {
	HNSDV_QP_INIT_ATTR_MASK_QP_CREATE_FLAGS	= 1 << 0,
	HNSDV_QP_INIT_ATTR_MASK_QP_CONGEST_TYPE	= 1 << 1,
	HNSDV_QP_INIT_ATTR_MASK_QP_PROTOCOL_TYPE = 1 << 2,
	HNSDV_QP_INIT_ATTR_MASK_QP_ADAPTIVE_ROUTING = 1 << 3,
	HNSDV_QP_INIT_ATTR_MASK_QP_OUT_OF_ORDER_RECEIVE = 1 << 4,
	HNSDV_QP_INIT_ATTR_MASK_QP_DYNAMIC_RTT = 1 << 5,
	HNSDV_QP_INIT_ATTR_MASK_QP_RET_ATTR = 1 << 6,
};

struct hnsdv_qp_props {
	void *buf;
	void *sq;
	void *rq;
	unsigned int *sdb;
	unsigned int *rdb;
	uint8_t sl;
	int max_inline_data;
	enum ibv_mtu path_mtu;
	void *ex_sge;
	unsigned int next_sge;
	unsigned long flags;
	void *sge_info;
	void *rq_rinl_buf;
	unsigned int sq_signal_bits;
	void *db_reg;
	void *reset_state;
};

struct hnsdv_qp_init_attr {
	uint64_t comp_mask; /* Use enum hnsdv_qp_init_attr_mask */
	uint32_t create_flags; /* Use enum hnsdv_qp_create_flags */
	uint8_t congest_type; /* Use enum hnsdv_qp_congest_ctrl_type */
	uint8_t protocol_type; /* Use enum hnsdv_qp_protocol_type */
	uint8_t reserved[6];
	struct hnsdv_qp_props *hihtr_qp_props;
};

enum hnsdv_device_link_type {
	HNSDV_DEV_LINK_TYPE_ETH,
	HNSDV_DEV_LINK_TYPE_HCCS,
	HNSDV_DEV_LINK_TYPE_UB,
};

enum hnsdv_qp_protocol_type {
	HNSDV_QP_PROTOCOL_TYPE_RDMA,
	HNSDV_QP_PROTOCOL_TYPE_UB,
	HNSDV_QP_PROTOCOL_TYPE_GUARD,
};

enum hnsdv_qp_create_flags {
	HNSDV_QP_CREATE_ENABLE_DCA_MODE = 1 << 0,
	HNSDV_QP_CREATE_ENABLE_STARS_MODE = 1 << 1,
};

struct ibv_qp *hnsdv_create_qp(struct ibv_context *context,
			       struct ibv_qp_init_attr_ex *qp_attr,
			       struct hnsdv_qp_init_attr *hihtr_attr);

struct hihtr_ib_create_qp2 {
	__aligned_u64 buf_addr;
	__aligned_u64 db_addr;
	__u8 log_sq_bb_count;
	__u8 log_sq_stride;
	__u8 sq_no_prefetch;
	__u8 protocol_type;
	__u8 device_ar_en;
	__u8 udp_srcport_range;
	__u8 switch_mp_en;
	__u8 pageshift;
	__aligned_u64 sdb_addr;
	__aligned_u64 comp_mask; /* Use enum hihtr_create_qp_comp_mask */
	__aligned_u64 create_flags; /* Use enum hihtr_create_qp_flags */
	__aligned_u64 congest_type_flags;
	__u8 oor_en;
	__u16 on_flight_size;
	__u8 reorder_cap;
	__u8 reorder_cq_en;
	__u8 reorder_cq_shift;
	__u8 dyn_at;
	__u16 rtt;
	__u8 reserved2[7];
};

#define _STRUCT_hihtr_ib_create_qp2 struct { \
	__aligned_u64 buf_addr; \
	__aligned_u64 db_addr; \
	__u8 log_sq_bb_count; \
	__u8 log_sq_stride; \
	__u8 sq_no_prefetch; \
	__u8 protocol_type; \
	__u8 device_ar_en; \
	__u8 udp_srcport_range; \
	__u8 switch_mp_en; \
	__u8 pageshift; \
	__aligned_u64 sdb_addr; \
	__aligned_u64 comp_mask; \
	__aligned_u64 create_flags;  \
	__aligned_u64 congest_type_flags; \
	__u8 oor_en; \
	__u16 on_flight_size; \
	__u8 reorder_cap; \
	__u8 reorder_cq_en; \
	__u8 reorder_cq_shift; \
	__u8 dyn_at; \
	__u16 rtt; \
	__u8 reserved2[7]; \
}
#endif