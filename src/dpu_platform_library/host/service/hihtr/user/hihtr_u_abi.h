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

#ifndef _HIHTR_U_ABI_H
#define _HIHTR_U_ABI_H

#include <linux/version.h>
#include <infiniband/kern-abi.h>
#include "hihtr-abi.h"
#include <kernel-abi/hihtr-abi.h>
#include "hihtr_ucompat.h"
#include "hnsdv.h"

// TODO hns-abi.h 待删除
struct hihtr_ib_alloc_ucontext2 {
	__u32 config;
	__u32 comp;
	__u32 dca_max_qps;
	__u32 reserved;
};

// TODO hns-abi.h 待删除
#define _STRUCT_hihtr_ib_alloc_ucontext2 struct { \
__u32 config; \
__u32 comp;	\
__u32 dca_max_qps;	\
__u32 reserved;	\
}

// TODO hns-abi.h 待删除
struct hihtr_ib_alloc_ucontext_ext_resp {
	__u32	qp_tab_size;
	__u32	cqe_size;
	__u32   srq_tab_size;
	__u32   cqe_mtt_buf_pg_sz;
	__u32	config;
	__u32	max_inline_data;
	__u8	mac_type;
	__u32   qp_mtt_buf_pg_sz;
	__u8	rsv1[3];
	__u32	dca_qps;
	__u32	dca_mmap_size;
	__aligned_u64 db_mmap_key;
	__aligned_u64 dca_mmap_key;
	__aligned_u64 reset_mmap_key;
	__aligned_u64 stats_mmap_key;
	__u32	resize_cq_hw_cq_depth;
};

#define _STRUCT_hihtr_ib_alloc_ucontext_ext_resp struct { \
	__u32	qp_tab_size; \
	__u32	cqe_size; \
	__u32   srq_tab_size; \
	__u32   cqe_mtt_buf_pg_sz; \
	__u32	config; \
	__u32	max_inline_data; \
	__u8	mac_type; \
	__u32   qp_mtt_buf_pg_sz; \
	__u8	rsv1[3]; \
	__u32	dca_qps; \
	__u32	dca_mmap_size; \
	__aligned_u64 db_mmap_key; \
	__aligned_u64 dca_mmap_key; \
	__aligned_u64 reset_mmap_key; \
	__aligned_u64 stats_mmap_key; \
	__u32	resize_cq_hw_cq_depth; \
}

DECLARE_DRV_CMD(hihtr_alloc_pd, IB_USER_VERBS_CMD_ALLOC_PD,
		empty, hihtr_ib_alloc_pd_resp);

DECLARE_DRV_CMD(hihtr_create_cq, IB_USER_VERBS_CMD_CREATE_CQ,
		hihtr_ib_create_cq, hihtr_ib_create_cq_resp);

DECLARE_DRV_CMD(hihtr_create_cq_ex, IB_USER_VERBS_EX_CMD_CREATE_CQ,
		hihtr_ib_create_cq, hihtr_ib_create_cq_resp);

DECLARE_DRV_CMD(hihtr_alloc_ucontext, IB_USER_VERBS_CMD_GET_CONTEXT,
		hihtr_ib_alloc_ucontext2, hihtr_ib_alloc_ucontext_ext_resp);

DECLARE_DRV_CMD(hihtr_create_qp, IB_USER_VERBS_CMD_CREATE_QP,
		hihtr_ib_create_qp2, hihtr_ib_create_qp_resp);

DECLARE_DRV_CMD(hihtr_create_qp_ex, IB_USER_VERBS_EX_CMD_CREATE_QP,
		hihtr_ib_create_qp2, hihtr_ib_create_qp_resp);

DECLARE_DRV_CMD(hihtr_create_srq, IB_USER_VERBS_CMD_CREATE_SRQ,
		hihtr_ib_create_srq, hihtr_ib_create_srq_resp);

DECLARE_DRV_CMD(hihtr_create_srq_ex, IB_USER_VERBS_CMD_CREATE_XSRQ,
		hihtr_ib_create_srq, hihtr_ib_create_srq_resp);

DECLARE_DRV_CMD(hihtr_create_ah, IB_USER_VERBS_CMD_CREATE_AH,
		empty, hihtr_ib_create_ah_resp);

DECLARE_DRV_CMD(hihtr_modify_qp_ex, IB_USER_VERBS_EX_CMD_MODIFY_QP,
		empty, hihtr_ib_modify_qp_resp);

#endif /* _HIHTR_U_ABI_H */
