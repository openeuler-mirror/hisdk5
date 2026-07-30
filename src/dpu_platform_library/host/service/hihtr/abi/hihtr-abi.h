/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR Linux-OpenIB) */
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

#ifndef HIHTR_ABI_USER_H
#define HIHTR_ABI_USER_H
#include <linux/version.h>
#include <linux/types.h>

#define HIHTR_CQ_RESIZE_CQ_DISABLE 0
#define HIHTR_CQ_RESIZE_CQ_ENABLE 1

enum hihtr_create_cq_create_flags {
	HIHTR_CREATE_CQ_FLAGS_POE_MODE = 1 << 0,
	HIHTR_CREATE_CQ_FLAGS_WRITE_WITH_NOTIFY = 1 << 1,
};

enum hihtr_create_cq_comp_mask {
	HIHTR_CREATE_CQ_COMP_MASK_CNT_MODE = 1 << 8,
	HIHTR_CREATE_CQ_COMP_MASK_ALWAYS_ARMED_EN = 1 << 9,
	HIHTR_CREATE_CQ_COMP_MASK_CREATE_FLAG = 1 << 10,
};

struct hihtr_ib_create_cq {
	__aligned_u64 buf_addr;
	__aligned_u64 db_addr;
	__u32 cqe_size;
	__u32 attr_flags; /* Use enum hihtr_create_cq_comp_mask */
	__aligned_u64 create_flags; /* Use enum hihtr_create_cq_create_flags */
	__u8 poe_channel;
	__u8 notify_mode;
	__u16 notify_idx;
	__u16 reserved[2];
};

enum hihtr_cq_cap_flags {
	HIHTR_CQ_FLAG_RECORD_DB = 1 << 0,
	HIHTR_CQ_FLAG_POE_EN = 1 << 2,
	HIHTR_CQ_FLAG_NOTIFY_EN = 1 << 3,
};

struct hihtr_ib_create_cq_resp {
	__aligned_u64 cqn; /* Only 32 bits used, 64 for compat */
	__aligned_u64 cap_flags;
};

struct hihtr_ib_create_srq {
	__aligned_u64 buf_addr;
	__aligned_u64 db_addr;
	__aligned_u64 que_addr;
	__u32 req_cap_flags; /* Use enum hihtr_srq_cap_flags */
	__u32 reserved;
};

enum hihtr_srq_cap_flags {
	HIHTR_SRQ_CAP_RECORD_DB = 1 << 0,
};

struct hihtr_ib_create_srq_resp {
	__u32	srqn;
	__u32	cap_flags; /* Use enum hihtr_srq_cap_flags */
};

enum hihtr_create_qp_comp_mask {
	HIHTR_CREATE_QP_MASK_CREATE_FLAGS = 1 << 0,
	HIHTR_CREATE_QP_MASK_CONGEST_TYPE = 1 << 1,
	HIHTR_CREATE_QP_MASK_PROTOCOL_TYPE = 1 << 2,
	HIHTR_CREATE_QP_MASK_ADAPTIVE_ROUTING = 1 << 3,
	HIHTR_CREATE_QP_MASK_OUT_OF_ORDER = 1 << 4,
	HIHTR_CREATE_QP_MASK_DYNAMIC_RTT = 1 << 5,
};

enum hihtr_create_qp_flags {
	HIHTR_CREATE_QP_FLAGS_STARS_MODE = 1 << 0,
};

enum hihtr_congest_type_flags {
	HIHTR_CREATE_QP_FLAGS_DCQCN = 1 << 0,
	HIHTR_CREATE_QP_FLAGS_LDCP = 1 << 1,
	HIHTR_CREATE_QP_FLAGS_HC3 = 1 << 2,
	HIHTR_CREATE_QP_FLAGS_DIP = 1 << 3,
};

struct hihtr_ib_create_qp {
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

enum hihtr_qp_cap_flags {
	HIHTR_QP_CAP_RQ_RECORD_DB = 1 << 0,
	HIHTR_QP_CAP_SQ_RECORD_DB = 1 << 1,
	HIHTR_QP_CAP_OWNER_DB = 1 << 2,
	HIHTR_QP_CAP_SVE_DIRECT_WQE = 1 << 3,
	HIHTR_QP_CAP_DYNAMIC_CTX_ATTACH = 1 << 4,
	HIHTR_QP_CAP_DIRECT_WQE = 1 << 5,
	HIHTR_QP_CAP_DYNAMIC_CTX_DETACH = 1 << 6,
	HIHTR_QP_CAP_STARS_SQ_MODE = 1 << 7,
	HIHTR_QP_CAP_WRITE_WITH_NOTIFY = 1 << 8,
	HIHTR_QP_CAP_ADAPTIVE_ROUTING = 1 << 9,
	HIHTR_QP_CAP_OUT_OF_ORDER = 1 << 10,
	HIHTR_QP_CAP_DYN_RTT = 1 << 11,
};

struct hihtr_ib_create_qp_resp {
	__aligned_u64 cap_flags; /* Use enum hihtr_qp_cap_flags */
};

#define HIHTR_IB_INVALID_ID 0XFFFF
struct hihtr_ib_hw_id {
	__u16 chip_id;
	__u16 die_id;
	__u16 func_id;
	__u16 reserved;
};

enum hihtr_query_device_comp_mask {
	HIHTR_QUERY_DEVICE_MASK_POE_CH_NUM = 1 << 31,
};

struct hihtr_ib_query_device_resp {
	__u32   comp_mask; /* Use enum hihtr_query_device_comp_mask */
	__u32   len;
	__u32   demo;
	__u8    congest_type;
	__u8	poe_ch_num;
	__u8    reserved[2];
	struct hihtr_ib_hw_id hw_id;
};

struct hihtr_ib_create_ah_resp {
	__u8    priority;
	__u8    tc_mode;
	__u8    dmac[6];
	__u16   vlan_id;
	__u8    vlan_en;
	__u8    reserved[5];
};

struct hihtr_ib_modify_qp_resp {
	__u8	tc_mode;
	__u8	priority;
	__u8	reserved[6];
	__u32	dcan;
	__u32	rsv2;
};

enum hihtr_ctx_config {
	HIHTR_EXSGE_FLAGS = 1 << 0,
	HIHTR_RQ_INLINE_FLAGS = 1 << 1,
	HIHTR_CQE_INLINE_FLAGS = 1 << 2,
	HIHTR_UCTX_CONFIG_DCA = 1 << 3,
	HIHTR_UCONTEXT_CONGEST_TYPE = 1 << 4,
	HIHTR_UCTX_DYN_QP_PGSZ = 1 << 5,
	HIHTR_RESIZE_CQ_FLAGS = 1 << 6,
	HIHTR_UCONTEXT_LINK_TYPE = 1 << 30,
	HIHTR_UCONTEXT_CONFIG_UHW = 1 << 31,
};

enum {
	HIHTR_RSP_EXSGE_FLAGS = 1 << 0,
	HIHTR_RSP_RQ_INLINE_FLAGS = 1 << 1,
	HIHTR_RSP_CQE_INLINE_FLAGS = 1 << 2,
	HIHTR_UCTX_RSP_DCA_FLAGS = HIHTR_UCTX_CONFIG_DCA,
	HIHTR_UCTX_RSP_DYN_QP_PGSZ = HIHTR_UCTX_DYN_QP_PGSZ,
	HIHTR_RSP_RESIZE_CQ_FLAGS = HIHTR_RESIZE_CQ_FLAGS,
};

struct hihtr_ib_alloc_ucontext_resp {
	__u32	qp_tab_size;
	__u32	cqe_size;
	__u32   srq_tab_size;
	__u32   cqe_mtt_buf_pg_sz;
	__u32	config; /* Use hihtr_ctx_config */
	__u32	max_inline_data;
	__u8	mac_type;
	__u32	qp_mtt_buf_pg_sz;
	__u8	rsv1[3];
	__u32	dca_qps;
	__u32	dca_mmap_size;
	__aligned_u64 db_mmap_key;
	__aligned_u64 dca_mmap_key;
	__aligned_u64 reset_mmap_key;
	__aligned_u64 stats_mmap_key;
	__u32	resize_cq_hw_cq_depth;
};

enum hihtr_uctx_comp_mask {
	HIHTR_ALLOC_UCTX_COMP_DCA_MAX_QPS = 1 << 0,
};

struct hihtr_ib_alloc_ucontext {
	__u32 config;
	__u32 comp; /* use hihtr_uctx_comp_mask */
	__u32 dca_max_qps;
	__u32 reserved;
};

struct hihtr_ib_alloc_pd_resp {
	__u32 pdn;
};

#define UVERBS_ID_NS_MASK 0xF000
#define UVERBS_ID_NS_SHIFT 12

enum hihtr_ib_objects {
	HIHTR_IB_OBJECT_DCA_MEM = (1U << UVERBS_ID_NS_SHIFT),
};

enum hihtr_ib_dca_mem_methods {
	HIHTR_IB_METHOD_DCA_MEM_REG = (1U << UVERBS_ID_NS_SHIFT),
	HIHTR_IB_METHOD_DCA_MEM_DEREG,
	HIHTR_IB_METHOD_DCA_MEM_SHRINK,
	HIHTR_IB_METHOD_DCA_MEM_ATTACH,
	HIHTR_IB_METHOD_DCA_MEM_DETACH,
	HIHTR_IB_METHOD_DCA_MEM_QUERY,
};

enum hihtr_ib_dca_mem_reg_attrs {
	HIHTR_IB_ATTR_DCA_MEM_REG_HANDLE = (1U << UVERBS_ID_NS_SHIFT),
	HIHTR_IB_ATTR_DCA_MEM_REG_FLAGS,
	HIHTR_IB_ATTR_DCA_MEM_REG_LEN,
	HIHTR_IB_ATTR_DCA_MEM_REG_ADDR,
	HIHTR_IB_ATTR_DCA_MEM_REG_KEY,
};

enum hihtr_ib_dca_mem_dereg_attrs {
	HIHTR_IB_ATTR_DCA_MEM_DEREG_HANDLE = (1U << UVERBS_ID_NS_SHIFT),
};

enum hihtr_ib_dca_mem_shrink_attrs {
	HIHTR_IB_ATTR_DCA_MEM_SHRINK_HANDLE = (1U << UVERBS_ID_NS_SHIFT),
	HIHTR_IB_ATTR_DCA_MEM_SHRINK_RESERVED_SIZE,
	HIHTR_IB_ATTR_DCA_MEM_SHRINK_OUT_FREE_KEY,
	HIHTR_IB_ATTR_DCA_MEM_SHRINK_OUT_FREE_MEMS,
};

enum hihtr_ib_dca_mem_attach_attrs {
	HIHTR_IB_ATTR_DCA_MEM_ATTACH_HANDLE = (1U << UVERBS_ID_NS_SHIFT),
	HIHTR_IB_ATTR_DCA_MEM_ATTACH_SQ_OFFSET,
	HIHTR_IB_ATTR_DCA_MEM_ATTACH_SGE_OFFSET,
	HIHTR_IB_ATTR_DCA_MEM_ATTACH_RQ_OFFSET,
	HIHTR_IB_ATTR_DCA_MEM_ATTACH_OUT_ALLOC_FLAGS,
	HIHTR_IB_ATTR_DCA_MEM_ATTACH_OUT_ALLOC_PAGES,
};

enum hihtr_ib_dca_mem_detach_attrs {
	HIHTR_IB_ATTR_DCA_MEM_DETACH_HANDLE = (1U << UVERBS_ID_NS_SHIFT),
	HIHTR_IB_ATTR_DCA_MEM_DETACH_SQ_INDEX,
};

enum hihtr_ib_dca_mem_query_attrs {
	HIHTR_IB_ATTR_DCA_MEM_QUERY_HANDLE = (1U << UVERBS_ID_NS_SHIFT),
	HIHTR_IB_ATTR_DCA_MEM_QUERY_PAGE_INDEX,
	HIHTR_IB_ATTR_DCA_MEM_QUERY_OUT_KEY,
	HIHTR_IB_ATTR_DCA_MEM_QUERY_OUT_OFFSET,
	HIHTR_IB_ATTR_DCA_MEM_QUERY_OUT_PAGE_COUNT,
};
#endif /* HIHTR_ABI_USER_H */