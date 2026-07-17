/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef _187X_CMDQ_PRIVATE_H_
#define _187X_CMDQ_PRIVATE_H_

#include "ossl_knl.h"
#include "hinic5_nic_cmdq.h"

struct hinic5_qp_ctxt_header {
	u32 rsvd[2];
	u16 num_queues;
	u16 queue_type;
	u16 start_qid;
	u16 dest_func_id;
};

struct hinic5_clean_queue_ctxt {
	struct hinic5_qp_ctxt_header cmdq_hdr;
};

struct hinic5_qp_ctxt_block {
	struct hinic5_qp_ctxt_header	cmdq_hdr;
	union {
		struct hinic5_sq_ctxt	sq_ctxt[HINIC5_Q_CTXT_MAX];
		struct hinic5_rq_ctxt	rq_ctxt[HINIC5_Q_CTXT_MAX];
	};
};

struct hinic5_rss_cmd_header {
	u32 rsv[3];
	u16 rsv1;
	u16 dest_func_id;
};

/* NIC HTN CMD */
enum hinic5_htn_cmd {
	HINIC5_HTN_CMD_SQ_RQ_CONTEXT_MULTI_ST = 0x20,
	HINIC5_HTN_CMD_SQ_RQ_CONTEXT_MULTI_LD,
	HINIC5_HTN_CMD_TSO_LRO_SPACE_CLEAN,
	HINIC5_HTN_CMD_SVLAN_MODIFY,
	HINIC5_HTN_CMD_SET_RSS_INDIR_TABLE,
	HINIC5_HTN_CMD_GET_RSS_INDIR_TABLE,
};

struct hinic5_vlan_ctx {
	u32 rsv[2];
	u16 vlan_tag;
	u8 vlan_sel;
	u8 vlan_mode;
	u16 start_qid;
	u16 dest_func_id;
};

struct hinic5_car_cmd_header {
	u32 rsv[2];
	u32 op_num; /* 配置的car_id个数，一次最少1个，最多32个 */
	u16 rsv1;
	u16 index; /* 配置的起始index，16B为单位，index必须32B对齐 */
};

struct hinic5_car_cmd_payload {
	u32 context[4]; /* 硬件写，驱动读 */
	u32 profile[4]; /* 驱动写，硬件读 */
};

#endif
