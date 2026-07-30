/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 * Description: RDMA wqe structure format.
 * Create: 2021-12-30
 */


#ifndef ROCE_WQE_FORMAT_H
#define ROCE_WQE_FORMAT_H

#include "roce_wqe_base_format.h"
#include "roce_npu_cmd_mr_defs.h" // For DIF reg mr WQE

#define ROCE_TX_SEND 0
#define ROCE_TX_SEND_INVALIDATE 1
#define ROCE_TX_SEND_IMMEDIATE 2
#define ROCE_TX_OPTYPE_RSVD3 3
#define ROCE_TX_WRITE 4
#define ROCE_TX_WRITE_IMMEDIATE 5
#define ROCE_TX_OPTYPE_RSVD6 6
#define ROCE_TX_OPTYPE_RSVD7 7
#define ROCE_TX_READ 8
#define ROCE_TX_OPTYPE_RSVD9 9
#define ROCE_TX_OPTYPE_RSVD10 10
#define ROCE_TX_EXT_ATOMIC_COMPARE_SWAP 11
#define ROCE_TX_ATOMIC_COMPARE_SWAP 12
#define ROCE_TX_ATOMIC_FETCH_ADD 13
#define ROCE_TX_ATOMIC_MASKED_COMPARE_SWAP 14
#define ROCE_TX_ATOMIC_MASKED_FETCH_ADD 15
#define ROCE_FAST_REG_PMR 16
#define ROCE_LOCAL_INVALIDATE 17
#define ROCE_BIND_MW_TYPE1_TYPE2 18
#define ROCE_REG_SIG_MR 19
#define ROCE_LOCAL_EXT 20
#define ROCE_TX_OPTYPE_RSVD15 21
#define ROCE_RESIZE_TYPE 22
#define ROCE_DAVID_FLUSH 23 // david 排空opcode
#define ROCE_TX_OPTYPE_RSVD18 24
#define ROCE_TX_OPTYPE_RSVD19 25
#define ROCE_TX_OPTYPE_RSVD1A 26
#define ROCE_TX_OPTYPE_RSVD1B 27
#define ROCE_TX_OPTYPE_RSVD1C 28
#define ROCE_TX_OPTYPE_RSVD1D 29
#define ROCE_ERR_TYPE 30
#define ROCE_TX_OPTYPE_RSVD1F 31

/* Type1/2 MW Bind WQE */
typedef struct roce_wqe_bind_tsk_seg {
    roce_wqe_tsk_com_seg_u common;

    /* DW0~2 */
    u32 rsvd0[3];

    /* DW3 */
    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 rae : 1;
            u32 rwe : 1;
            u32 rre : 1;
            u32 type : 1;
            u32 rsvd : 28;
#else
            u32 rsvd : 28;
            u32 type : 1; /* MW type: 0-Type1 Window 1-Type2B Window */
            u32 rre : 1;  /* Remote read enable */
            u32 rwe : 1;  /* Remote write enable */
            u32 rae : 1;  /* Indicates whether remote Atomic is enabled. */
#endif
        } bs;
        u32 value;
    } dw3;

    /* DW4 */
    u32 new_rkey; /* The MW corresponds to the MPT key and is indexed to the MPT through the New_Rkey. For the type1 MW,
                   * This parameter is valid only when the value of New_Rkey is the same as the value of mem_key in the
                   * MPT. For type 2 MW, it is valid only when New_Rkey is equal to mem_key+1 in MPT.
                   * If this parameter is valid, the corresponding field in the MPT is replaced with the entire
                   * section. */

    /* DW5 */
    u32 lkey; /* Indicates the mem_key of the MR bound to the MW. */

    /* DW6 */
    u32 rsvd1;

    /* DW7~8 */
    union {
        u64 va; /* Indicates the start virtual IP address of the MW. */
        struct {
            u32 va_h32;
            u32 va_l32;
        } bs;
    } dw7;

    /* DW9~10 */
    union {
        u64 len; /* Indicates the length of the data corresponding to the MW. */
        struct {
            u32 len_h32;
            u32 len_l32;
        } bs;
    } dw9;
} roce_wqe_bind_tsk_seg_s;

/* Fast Register PMR WQE */
typedef struct roce_wqe_frmr_tsk_seg {
    roce_wqe_tsk_com_seg_u common;

    /* DW0~1 */
    u32 rsvd[2];

    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 fbo : 22;
            u32 rsvd : 10;
#else
            u32 rsvd : 10;
            u32 fbo : 22; /* This parameter is valid when ZERO_BASE is set to 1. */
#endif
        } bs;
        u32 value;
    } dw2;

    /* DW3 */
    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 rae : 1;
            u32 rwe : 1;
            u32 rre : 1;
            u32 lwe : 1;
            u32 lre : 1;
            u32 be : 1;
            u32 zbva : 1;
            u32 block : 1;
            u32 rsvd : 2;
            u32 page_size : 6;
            u32 pa_num : 16;
#else
            u32 pa_num : 16;   /* Number of registered PAs, which is calculated by the length/page_size. */
            u32 page_size : 6; /* Memory Region page size */
            u32 rsvd : 2;
            u32 block : 1;
            u32 zbva : 1; /* ZERO_BASED Permission */
            u32 be : 1;   /* Indicates whether the binding operation is enabled. */
            u32 lre : 1;  /* Local read enable */
            u32 lwe : 1;  /* Local write enable */
            u32 rre : 1;  /* Remote read enable */
            u32 rwe : 1;  /* Remote write enable */
            u32 rae : 1;  /* Indicates whether remote Atomic is enabled. */
#endif
        } bs;
        u32 value;
    } dw3;

    /* DW4 */
    u32 m_key; /* Key of the Memory Region. */

    /* DW5~6 */
    union {
        u64 va; /* Start virtual address of Memory Region. This parameter is valid only when ZBVA is not set to 1. */
        struct {
            u32 va_h32;
            u32 va_l32;
        } bs;
    } dw5;

    /* DW7~8 */
    union {
        u64 len; /* Length of Memory Region */
        struct {
            u32 len_h32;
            u32 len_l32;
        } bs;
    } dw7;

    /* DW9~10 */
    union {
        u64 pbl_addr; /* Physical address for storing the cache of the PA table. */
        struct {
            u32 pbl_addr_h32;
            u32 pbl_addr_l32;
        } bs;
    } dw9;
} roce_wqe_frmr_tsk_seg_s;

/* Local Invalidate WQE */
typedef struct roce_wqe_local_inv_tsk_seg {
    roce_wqe_tsk_com_seg_u common;

    /* DW0 */
    u32 rsvd;

    /* DW1 */
    u32 inv_key; /* Mem_Key for invalidate */

    /* DW2 */
    u32 rsvd1;
} roce_wqe_local_inv_tsk_seg_s;

typedef struct tag_roce_dif_wqe_rdma_tsk_seg {
    roce_wqe_tsk_com_seg_u common;

    /* DW1 */
    u32 data_len;

    /* DW2 */
    u32 imm_data;

    /* DW3 */
    u32 rsvd1;

    /* DW4~5 */
    union {
        u64 va;
        struct {
            u32 va_h32;
            u32 va_l32;
        } dw4;
    };

    /* DW6 */
    u32 rkey;
    roce_dif_user_data_s dif_data;
    /* DW7 */
    // u32 sig_key;
} roce_dif_wqe_rdma_tsk_seg_s;

typedef struct tag_roce_dif_wqe_snd_tsk_seg {
    roce_wqe_tsk_com_seg_u common;

    /* DW0 */
    u32 data_len; /* Length of the data sent by the SQ WQE */

    /* DW1 */
    u32 immdata_invkey; /* This parameter is valid for the immediate data operation or SEND invalidate. */

    roce_dif_user_data_s dif_data;
} roce_dif_wqe_snd_tsk_seg_s;

/* REG SIG MR Local WQE */
typedef struct roce_wqe_reg_sig_mr_seg {
    roce_wqe_tsk_com_seg_u common;

    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 rre : 1;         /* Remote read enable */
            u32 rwe : 1;         /* Remote write enable */
            u32 lwe : 1;         /* Local write enable */
            u32 sgl_mode : 1;    /* 0: Single SGL, 1: Dual SGL */
            u32 sector_size : 1; /* 0 : 512B, 1: 4096B */
            u32 rsvd1 : 3;
            u32 block_size : 6; /* Block size, which is the same as the MPT data definition. */
            u32 rsvd2 : 18;
#else
            u32 rsvd2 : 18;
            u32 block_size : 6; /* Block size, which is the same as the MPT data definition. */
            u32 rsvd1 : 3;
            u32 sector_size : 1; /* 0 : 512B, 1: 4096B */
            u32 sgl_mode : 1;    /* 0: Single SGL, 1: Dual SGL */
            u32 lwe : 1;         /* Local write enable */
            u32 rwe : 1;         /* Remote write enable */
            u32 rre : 1;         /* Remote read enable */
#endif
        } bs;
        u32 value;
    } dw0;

    u32 sig_mr_mkey;

    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 rsvd : 10;
            u32 mtt_offset : 22;
#else
            u32 mtt_offset : 22;
            u32 rsvd : 10;
#endif
        } bs;
        u32 value;
    } dw2;
    roce_dif_user_data_s dif_data;
    u32 data_mr_key;
    u32 metadata_mr_key;
    u32 rsvd1;
} roce_wqe_reg_sig_mr_seg_s;

/* * SRQ Data Format start */
typedef struct roce_wqe_srq_data_seg {
    /* DW0~1 */
    union {
        u64 addr;

        struct {
            u32 addr_h32;
            u32 addr_l32;
        } bs;
    };

    /* DW2 */
    union {
        struct {
            u32 rsv : 1;  /* Reserved field */
            u32 len : 31; /* Data length. The value can be [0 or 2G-1]. */
        } bs;
        u32 length;
    } dw2;

    /* DW3 */
    union {
        struct {
            u32 last : 1; /* Last flag. The 0-also has the next SGE. 1: The current SGE is the last one. */
            u32 ext : 1; /* Extended flag. The value 0-is normal and does not need to be extended. 1: Extended mode. The
                            address of the current SGE points to SGL. The key and len are invalid. For RoCE, the value
                            is fixed to 0. */
            u32 key : 30; /* Local_key. The least significant eight bits are keys, and the most significant 22 bits are
                             indexes. The most significant two bits are reserved and only 20 bits are used. */
        } bs;
        u32 lkey;
    } dw3;
} roce_wqe_srq_data_seg_s;
/* * SRQ Data Format end */

typedef struct roce_wqe_ulp_com_tsk_seg {
    roce_wqe_tsk_com_seg_u common;

    /* DW1 */
    u32 data_len; /* Length of the data sent by the SQ WQE */

    /* DW2 */
    u32 immdata_invkey;

    /* DW3 */
    roce_wqe_tsk_misc_seg_u dw3;

    /* DW4~5 */
    union {
        u64 va;
        struct {
            u32 va_h32;
            u32 va_l32;
#ifndef PLATFORM_MODE_SNP_MACC    /* 宏指令不支持匿名结构体 */
        } dw4;
    };
#else
        } macc_bs;
    } dw4;
#endif /* PLATFORM_MODE_SNP_MACC */

    /* DW6 */
    u32 rkey;

    /* DW7 */
    u32 ulp_value;
} roce_wqe_ulp_com_tsk_seg_s;

typedef union tag_roce_wqe_task_seg {
    sq_wqe_task_remote_common_s remote_common;
    roce_wqe_send_tsk_seg_s send;
    roce_wqe_rdma_tsk_seg_s rdma;
    roce_wqe_atomic_tsk_seg_s atomic;
    roce_wqe_ext_atomic_tsk_seg_s ext_atomic;
    roce_wqe_mask_atomic_tsk_seg_s masked_atomic;
    roce_wqe_ud_tsk_seg_s ud;
    roce_wqe_drc_tsk_seg_s drc_rdma;

    roce_wqe_bind_tsk_seg_s bind_mw;
    roce_wqe_frmr_tsk_seg_s frmr;
    roce_wqe_local_inv_tsk_seg_s local_inv;
    roce_wqe_ulp_com_tsk_seg_s ulp;
#ifdef ROCE_DIF_EN
    roce_wqe_reg_sig_mr_seg_s reg_sig_mr;
    roce_dif_wqe_rdma_tsk_seg_s dif_rdma;
#endif
} roce_wqe_task_seg_u;

#endif // ROCE_WQE_FORMAT_H