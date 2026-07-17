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

#ifndef _HIHTR_U_HW_V2_H
#define _HIHTR_U_HW_V2_H

enum {
	CQE_FOR_SQ,
	CQE_FOR_RQ,
};

#define HIHTR_V2_CQ_DB_REQ_SOL		1
#define HIHTR_V2_CQ_DB_REQ_NEXT		0

#define HIHTR_COS_MIN 4

/* V2 REG DEFINITION */
#define ROCEE_VF_DB_CFG0_OFFSET			0x0230

#define HIHTR_IDX_QUE_ENTRY_SZ		4

enum {
	HIHTR_WQE_OP_SEND = 0x0,
	HIHTR_WQE_OP_SEND_WITH_INV = 0x1,
	HIHTR_WQE_OP_SEND_WITH_IMM = 0x2,
	HIHTR_WQE_OP_RDMA_WRITE = 0x3,
	HIHTR_WQE_OP_RDMA_WRITE_WITH_IMM = 0x4,
	HIHTR_WQE_OP_RDMA_READ = 0x5,
	HIHTR_WQE_OP_ATOMIC_COM_AND_SWAP = 0x6,
	HIHTR_WQE_OP_ATOMIC_FETCH_AND_ADD = 0x7,
	HIHTR_WQE_OP_ATOMIC_MASK_COMP_AND_SWAP = 0x8,
	HIHTR_WQE_OP_ATOMIC_MASK_FETCH_AND_ADD = 0x9,
	HIHTR_WQE_OP_FAST_REG_PMR = 0xa,
	HIHTR_WQE_OP_LOCAL_INV = 0xb,
	HIHTR_WQE_OP_BIND_MW_TYPE = 0xc,
	HIHTR_WQE_OP_MASK = 0x1f
};

enum {
	/* rq operations */
	HIHTR_RECV_OP_RDMA_WRITE_IMM = 0x0,
	HIHTR_RECV_OP_SEND = 0x1,
	HIHTR_RECV_OP_SEND_WITH_IMM = 0x2,
	HIHTR_RECV_OP_SEND_WITH_INV = 0x3,
};

enum {
	HIHTR_SQ_OP_SEND = 0x0,
	HIHTR_SQ_OP_SEND_WITH_INV = 0x1,
	HIHTR_SQ_OP_SEND_WITH_IMM = 0x2,
	HIHTR_SQ_OP_RDMA_WRITE = 0x3,
	HIHTR_SQ_OP_RDMA_WRITE_WITH_IMM = 0x4,
	HIHTR_SQ_OP_RDMA_READ = 0x5,
	HIHTR_SQ_OP_ATOMIC_COMP_AND_SWAP = 0x6,
	HIHTR_SQ_OP_ATOMIC_FETCH_AND_ADD = 0x7,
	HIHTR_SQ_OP_ATOMIC_MASK_COMP_AND_SWAP = 0x8,
	HIHTR_SQ_OP_ATOMIC_MASK_FETCH_AND_ADD = 0x9,
	HIHTR_SQ_OP_FAST_REG_PMR = 0xa,
	HIHTR_SQ_OP_LOCAL_INV = 0xb,
	HIHTR_SQ_OP_BIND_MW = 0xc, // 1872 not support
};

enum {
	V2_CQ_OK			=  0,
	V2_CQ_EMPTY			= -1,
	V2_CQ_POLL_ERR			= -2,
	V2_CQ_SILENT_DROP   = -3,
	V2_CQ_DUPLICATE     = -4,
};

#define SGE_LEN_ERR 0x8 // sub type err
enum {
	HIHTR_V2_CQE_SUCCESS				= 0x00,
	HIHTR_V2_CQE_LOCAL_LENGTH_ERR		= 0x01,
	HIHTR_V2_CQE_LOCAL_QP_OP_ERR			= 0x02,
	HIHTR_V2_CQE_LOCAL_PROT_ERR			= 0x04,
	HIHTR_V2_CQE_WR_FLUSH_ERR			= 0x05,
	HIHTR_V2_CQE_MEM_MANAGERENT_OP_ERR		= 0x06,
	HIHTR_V2_CQE_BAD_RESP_ERR			= 0x10,
	HIHTR_V2_CQE_LOCAL_ACCESS_ERR		= 0x11,
	HIHTR_V2_CQE_REMOTE_INVAL_REQ_ERR		= 0x12,
	HIHTR_V2_CQE_REMOTE_ACCESS_ERR		= 0x13,
	HIHTR_V2_CQE_REMOTE_OP_ERR			= 0x14,
	HIHTR_V2_CQE_TRANSPORT_RETRY_EXC_ERR		= 0x15,
	HIHTR_V2_CQE_RNR_RETRY_EXC_ERR		= 0x16,
	HIHTR_V2_CQE_REMOTE_ABORTED_ERR		= 0x22,
	HIHTR_V2_CQE_GENERAL_ERR			= 0x23,
	HIHTR_V2_CQE_XRC_VIOLATION_ERR		= 0x24,
};

enum {
	HIHTR_V3_SQ_DB,
	HIHTR_V3_RQ_DB,
	HIHTR_V3_SRQ_DB,
	HIHTR_V3_CQ_DB,
	HIHTR_V3_CQ_DB_NOTIFY
};

// 1872 doorbell srv字段定义
enum {
	HIHTR_V3_DB_SQ_SRV = 0x2,
	HIHTR_V3_DB_CQ_SRV = 0x3,
	HIHTR_V3_DB_CQ_NOTIFY_SRV = 0x4,
	HIHTR_V3_DB_EQ_SRV = 0x10
};

// 1872 doorbell C N字段定义
enum {
	HIHTR_V3_DB_SQ_CN = 0x0,
	HIHTR_V3_DB_CQ_CN = 0x3,
};

enum hihtr_wr_buf_type {
	WR_BUF_TYPE_POST_SEND,
	WR_BUF_TYPE_SEND_WR_OPS,
};

struct hihtr_db {
	__le32	byte_4;
	__le32	parameter;
};

#define V3_DB_SQ_GET_PI_LOW(pi) (uint32_t)((pi) & GENMASK(7, 0))       // 取pi低8位
#define V3_DB_SQ_GET_PI_HIGH(pi) (uint32_t)((pi >> 8) & GENMASK(7, 0)) // 取pi高8位

#define DB_FIELD_LOC(h, l) FIELD_LOC(struct hihtr_db, h, l)

// SQ / CQ / CQ_DB_NTF doorBell 有效域段公共定义 byte 0 - byte3
#define DB_Q_NUM  DB_FIELD_LOC(19, 0)
#define DB_CN     DB_FIELD_LOC(23, 22)
#define DB_COS    DB_FIELD_LOC(26, 24)
#define DB_SRV    DB_FIELD_LOC(31, 27)
#define DB_DIRECT    DB_FIELD_LOC(31, 30)

#define DB_TAG DB_FIELD_LOC(23, 0)
#define DB_CMD DB_FIELD_LOC(27, 24)
#define DB_FLAG DB_FIELD_LOC(31, 31)
#define DB_PI DB_FIELD_LOC(47, 32)

// SQ doorBell 特有有效域段定义
#define DB_SQ_PI      DB_FIELD_LOC(39, 32)

#define CPUID_FUNC_EXTENDED_FEATURES 7
#define CPUID_FLAG_AVX512F_BIT       16
#define CPUID_FLAG_AVX512BW_BIT      30
#define UDWQE_INLINE_DATA_23_16_START 16
#define UDWQE_INLINE_DATA_47_24_SHIFT 8
#define UDWQE_INLINE_DATA_47_24_START 24
#define UDWQE_INLINE_DATA_63_48_WIDTH 16
#define ADDR_HIGH32_SHIFT 32

#define DWQE_MAX_INLINE_DATA_SIZE 32

struct __attribute__((aligned(64))) hihtr_dwqe {
	uint64_t common_hdr;
    uint64_t dwqe_hdr; // take effect in direct wqe, harware will generate doorbell by this part!
    uint64_t wqe_data[6];
};
#define DIRECT_WQE_FIELD_LOC(h, l) FIELD_LOC(struct hihtr_dwqe, h, l)

#define DIRECT_WQE_Q_NUM      DIRECT_WQE_FIELD_LOC(83, 64)
#define DIRECT_WQE_CN         DIRECT_WQE_FIELD_LOC(87, 86)
#define DIRECT_WQE_COS        DIRECT_WQE_FIELD_LOC(90, 88)
#define DIRECT_WQE_DDB        DIRECT_WQE_FIELD_LOC(95, 94)
#define DIRECT_WQE_SQ_PI      DIRECT_WQE_FIELD_LOC(111, 96)
#define DIRECT_WQE_SGE_NUM    DIRECT_WQE_FIELD_LOC(127, 120)

#define DIRECT_WQE_SHIFT          (2048)
#define DIRECT_WQE_DB_DDB          2

// CQ doorBell 特有有效域段定义
#define DB_CQ_CI         DB_FIELD_LOC(55, 32)
#define DB_CQ_NOTIFY     DB_FIELD_LOC(61, 61)
#define DB_CQ_CMD_SN     DB_FIELD_LOC(63, 62)

// EQ doorBell 特有有效域段定义
#define DB_EQ_EQN DB_FIELD_LOC(7, 0)
#define DB_EQ_CMD DB_FIELD_LOC(9, 8)
#define DB_EQ_CI  DB_FIELD_LOC(55, 32)

#define RECORD_DB_CI_MASK GENMASK(23, 0)

struct hihtr_v2_cqe {
	__le32	byte_4;
	union {
		__le32	rkey;
		__le32	immtdata;
	};
	__le32	byte_12;
	__le32	byte_16;
	__le32	byte_cnt;
	__le32	smac;
	__le32	byte_28;
	__le32	byte_32;
	__le32	payload[8];
};

#define PKTYPE_V1			HIHTR_PKTYPE_ROCE_V1
#define PKTYPE_V2_IPV6			HIHTR_PKTYPE_ROCE_V2_IPV6
#define PKTYPE_V2_IPV4			HIHTR_PKTYPE_ROCE_V2_IPV4
#define PKTYPE_INVALID			HIHTR_PKTYPE_INVALID

enum hihtr_pktype {
	HIHTR_PKTYPE_ROCE_V1,
	HIHTR_PKTYPE_ROCE_V2_IPV6,
	HIHTR_PKTYPE_ROCE_V2_IPV4,
	HIHTR_PKTYPE_INVALID,
};

#define CQE_FIELD_LOC(h, l) FIELD_LOC(struct hihtr_v2_cqe, h, l)

#define CQE_OPCODE CQE_FIELD_LOC(4, 0)
#define CQE_RQ_INLINE CQE_FIELD_LOC(5, 5)
#define CQE_S_R CQE_FIELD_LOC(6, 6)
#define CQE_OWNER CQE_FIELD_LOC(7, 7)
#define CQE_STATUS CQE_FIELD_LOC(15, 8)
#define CQE_WQE_IDX CQE_FIELD_LOC(31, 16)
#define CQE_RKEY_IMMTDATA CQE_FIELD_LOC(63, 32)
#define CQE_XRC_SRQN CQE_FIELD_LOC(87, 64)
#define CQE_CQE_INLINE CQE_FIELD_LOC(89, 88)
#define CQE_LCL_QPN CQE_FIELD_LOC(119, 96)
#define CQE_SUB_STATUS CQE_FIELD_LOC(127, 120)
#define CQE_BYTE_CNT CQE_FIELD_LOC(159, 128)
#define CQE_SMAC CQE_FIELD_LOC(207, 160)
#define CQE_PORT_TYPE CQE_FIELD_LOC(209, 208)
#define CQE_VID CQE_FIELD_LOC(221, 210)
#define CQE_VID_VLD CQE_FIELD_LOC(222, 222)
#define CQE_RSV2 CQE_FIELD_LOC(223, 223)
#define CQE_RMT_QPN CQE_FIELD_LOC(247, 224)
#define CQE_SL CQE_FIELD_LOC(250, 248)
#define CQE_PORTN CQE_FIELD_LOC(253, 251)
#define CQE_GRH CQE_FIELD_LOC(254, 254)
#define CQE_LPK CQE_FIELD_LOC(255, 255)
#define CQE_RSV3 CQE_FIELD_LOC(511, 256)

struct hihtr_rc_sq_wqe {
	__le32	byte_4;
	__le32	msg_len;
	union {
		__le32	inv_key;
		__le32	immtdata;
		__le32	new_rkey;
	};
	__le32	byte_16;
	__le32	byte_20;
	__le32	rkey;
	__le64	va;
};

typedef struct _hihtr_wqe_hdr {
    uint32_t opcode  : 5;
    uint32_t db_sl_l : 2;
    uint32_t owner   : 1;
    uint32_t cqe     : 1;
    uint32_t fence   : 1;
    uint32_t so      : 1;
    uint32_t se      : 1;
    uint32_t inline_flag : 1;
    uint32_t db_sl_h : 2;
    uint32_t wqe_idx : 16;
    uint32_t flag    : 1;
} hihtr_wqe_hdr_t;

#define RCWQE_FIELD_LOC(h, l) FIELD_LOC(struct hihtr_rc_sq_wqe, h, l)

#define RCWQE_OPCODE RCWQE_FIELD_LOC(4, 0)
#define RCWQE_DB_SL_L RCWQE_FIELD_LOC(6, 5)
#define RCWQE_SQPN_L RCWQE_FIELD_LOC(6, 5)
#define RCWQE_OWNER RCWQE_FIELD_LOC(7, 7)
#define RCWQE_CQE RCWQE_FIELD_LOC(8, 8)
#define RCWQE_FENCE RCWQE_FIELD_LOC(9, 9)
#define RCWQE_SO RCWQE_FIELD_LOC(10, 10)
#define RCWQE_SE RCWQE_FIELD_LOC(11, 11)
#define RCWQE_INLINE RCWQE_FIELD_LOC(12, 12)
#define RCWQE_DB_SL_H RCWQE_FIELD_LOC(14, 13)
#define RCWQE_WQE_IDX RCWQE_FIELD_LOC(30, 15)
#define RCWQE_SQPN_H RCWQE_FIELD_LOC(30, 13)
#define RCWQE_FLAG RCWQE_FIELD_LOC(31, 31)
#define RCWQE_MSG_LEN RCWQE_FIELD_LOC(63, 32)
#define RCWQE_INV_KEY_IMMTDATA RCWQE_FIELD_LOC(95, 64)
#define RCWQE_XRC_SRQN RCWQE_FIELD_LOC(119, 96)
#define RCWQE_SGE_NUM RCWQE_FIELD_LOC(127, 120)
#define RCWQE_MSG_START_SGE_IDX RCWQE_FIELD_LOC(151, 128)
#define RCWQE_REDUCE_CODE RCWQE_FIELD_LOC(158, 152)
#define RCWQE_INLINE_TYPE RCWQE_FIELD_LOC(159, 159)
#define RCWQE_RKEY RCWQE_FIELD_LOC(191, 160)
#define RCWQE_VA_L RCWQE_FIELD_LOC(223, 192)
#define RCWQE_VA_H RCWQE_FIELD_LOC(255, 224)
#define RCWQE_LEN0 RCWQE_FIELD_LOC(287, 256)
#define RCWQE_LKEY0 RCWQE_FIELD_LOC(319, 288)
#define RCWQE_VA0_L RCWQE_FIELD_LOC(351, 320)
#define RCWQE_VA0_H RCWQE_FIELD_LOC(383, 352)
#define RCWQE_LEN1 RCWQE_FIELD_LOC(415, 384)
#define RCWQE_LKEY1 RCWQE_FIELD_LOC(447, 416)
#define RCWQE_VA1_L RCWQE_FIELD_LOC(479, 448)
#define RCWQE_VA1_H RCWQE_FIELD_LOC(511, 480)

#define RCWQE_MW_TYPE RCWQE_FIELD_LOC(256, 256)
#define RCWQE_MW_RA_EN RCWQE_FIELD_LOC(258, 258)
#define RCWQE_MW_RR_EN RCWQE_FIELD_LOC(259, 259)
#define RCWQE_MW_RW_EN RCWQE_FIELD_LOC(260, 260)

struct hihtr_v2_wqe_data_seg {
	__le32		len;
	__le32		lkey;
	__le64		addr;
};

struct hihtr_v2_wqe_raddr_seg {
	__le32		rkey;
	__le32		len;
	__le64		raddr;
};

struct hihtr_wqe_atomic_seg {
	__le64		fetchadd_swap_data;
	__le64		cmp_data;
};

int hihtr_u_v2_post_send(struct ibv_qp *ibvqp, struct ibv_send_wr *wr,
			    struct ibv_send_wr **bad_wr);

#define STD_ATOMIC_LEN 8
#define EXT_ATOMIC_16_LEN 16
#define EXT_ATOMIC_32_LEN 32
#define EXT_ATOMIC_64_LEN 64

static inline unsigned int is_std_atomic(unsigned int len)
{
    return len == STD_ATOMIC_LEN;
}

static inline unsigned int is_ext_atomic(unsigned int len)
{
    return len == EXT_ATOMIC_16_LEN || len == EXT_ATOMIC_32_LEN || len == EXT_ATOMIC_64_LEN;
}

static inline unsigned int is_atomic(unsigned int len)
{
	return is_std_atomic(len) || is_ext_atomic(len);
}

#define ATOMIC_DATA_LEN_MAX 64
#define ATOMIC_BUF_NUM_MAX 2

struct hihtr_ud_sq_wqe {
	__le32 rsv_opcode;
	__le32 msg_len;
	__le32 immtdata;
	__le32 sge_num_pd;
	__le32 rsv_msg_start_sge_idx;
	__le32 udpspn_rsv;
	__le32 qkey;
	__le32 rsv_dqpn;
	__le32 tclass_vlan;
	__le32 lbi_flow_label;
	uint8_t dmac[ETH_ALEN];
	uint8_t sgid_index;
	uint8_t smac_index;
	uint8_t dgid[HIHTR_GID_SIZE];
};

#define UDWQE_FIELD_LOC(h, l) FIELD_LOC(struct hihtr_ud_sq_wqe, h, l)

#define UDWQE_OPCODE UDWQE_FIELD_LOC(4, 0)
#define UDWQE_DB_SL_L UDWQE_FIELD_LOC(6, 5)
#define UDWQE_OWNER UDWQE_FIELD_LOC(7, 7)
#define UDWQE_CQE UDWQE_FIELD_LOC(8, 8)
#define UDWQE_RSVD1 UDWQE_FIELD_LOC(10, 9)
#define UDWQE_SE UDWQE_FIELD_LOC(11, 11)
#define UDWQE_INLINE UDWQE_FIELD_LOC(12, 12)
#define UDWQE_DB_SL_H UDWQE_FIELD_LOC(14, 13)
#define UDWQE_WQE_IDX UDWQE_FIELD_LOC(30, 15)
#define UDWQE_FLAG UDWQE_FIELD_LOC(31, 31)
#define UDWQE_MSG_LEN UDWQE_FIELD_LOC(63, 32)
#define UDWQE_IMMTDATA UDWQE_FIELD_LOC(95, 64)
#define UDWQE_PD UDWQE_FIELD_LOC(119, 96)
#define UDWQE_SGE_NUM UDWQE_FIELD_LOC(127, 120)
#define UDWQE_MSG_START_SGE_IDX UDWQE_FIELD_LOC(151, 128)
#define UDWQE_RSVD3 UDWQE_FIELD_LOC(158, 152)
#define UDWQE_INLINE_TYPE UDWQE_FIELD_LOC(159, 159)
#define UDWQE_RSVD4 UDWQE_FIELD_LOC(175, 160)
#define UDWQE_UDPSPN UDWQE_FIELD_LOC(191, 176)
#define UDWQE_QKEY UDWQE_FIELD_LOC(223, 192)
#define UDWQE_DQPN UDWQE_FIELD_LOC(247, 224)
#define UDWQE_RSVD5 UDWQE_FIELD_LOC(255, 248)
#define UDWQE_VLAN UDWQE_FIELD_LOC(271, 256)
#define UDWQE_HOPLIMIT UDWQE_FIELD_LOC(279, 272)
#define UDWQE_TCLASS UDWQE_FIELD_LOC(287, 280)
#define UDWQE_FLOW_LABEL UDWQE_FIELD_LOC(307, 288)
#define UDWQE_SL UDWQE_FIELD_LOC(311, 308)
#define UDWQE_PORTN UDWQE_FIELD_LOC(314, 312)
#define UDWQE_RSVD6 UDWQE_FIELD_LOC(317, 315)
#define UDWQE_UD_VLAN_EN UDWQE_FIELD_LOC(318, 318)
#define UDWQE_LBI UDWQE_FIELD_LOC(319, 319)
#define UDWQE_DMAC_L UDWQE_FIELD_LOC(351, 320)
#define UDWQE_DMAC_H UDWQE_FIELD_LOC(367, 352)
#define UDWQE_GMV_IDX UDWQE_FIELD_LOC(383, 368)
#define UDWQE_DGID0 UDWQE_FIELD_LOC(415, 384)
#define UDWQE_DGID1 UDWQE_FIELD_LOC(447, 416)
#define UDWQE_DGID2 UDWQE_FIELD_LOC(479, 448)
#define UDWQE_DGID3 UDWQE_FIELD_LOC(511, 480)

#define UDWQE_INLINE_DATA_15_0 UDWQE_FIELD_LOC(63, 48)
#define UDWQE_INLINE_DATA_23_16 UDWQE_FIELD_LOC(127, 120)
#define UDWQE_INLINE_DATA_47_24 UDWQE_FIELD_LOC(151, 128)
#define UDWQE_INLINE_DATA_63_48 UDWQE_FIELD_LOC(175, 160)

#define MAX_SERVICE_LEVEL 0x7

#define RCWQE_SQPN_L_WIDTH 2

void hihtr_v2_clear_qp(struct hihtr_context *ctx, struct hihtr_qp *qp);
void hihtr_attach_cq_ex_ops(struct ibv_cq_ex *cq_ex, uint64_t wc_flags);
int hihtr_attach_qp_ex_ops(struct ibv_qp_init_attr_ex *attr,
			      struct hihtr_qp *qp);

static inline bool is_rq_inline_enabled(struct hihtr_context *context)
{
	return context->config & HIHTR_RSP_RQ_INLINE_FLAGS;
}

static inline bool is_cqe_inline_enabled(struct hihtr_context *context)
{
	return context->config & HIHTR_RSP_CQE_INLINE_FLAGS;
}

static inline bool is_inline_enabled(struct hihtr_context *context)
{
	return is_rq_inline_enabled(context) || is_cqe_inline_enabled(context);
}

#if defined(__x86_64__)
#define wc_wmb() __asm__ volatile("sfence" ::: "memory")
#define rmb() __asm__ volatile("lfence" ::: "memory")
#define wmb() wc_wmb()
#define mm_store_si128(a, b) _mm_store_si128((__m128i *)(a), *(__m128i *)(b))
#elif defined(__aarch64__)
#define wc_wmb() __asm__ volatile("dsb st" ::: "memory")
#define rmb() __asm__ volatile("dsb ld" ::: "memory")
#define wmb() wc_wmb()
#define mm_store_si128(a, b) vst1q_s32((int32_t *)(a), *(int32x4_t *)(b))
#endif
#define ROCE_WRITE_WQE_SIZE_16 16
#define ROCE_WRITE_WQE_SIZE_32 32
#define ROCE_WRITE_WQE_SIZE_48 48
#endif /* _HIHTR_U_HW_V2_H */
