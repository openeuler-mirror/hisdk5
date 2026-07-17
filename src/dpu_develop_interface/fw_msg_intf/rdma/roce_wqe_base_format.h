/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 * Description: RDMA wqe structure format.
 * Create: 2021-12-30
 */

#ifndef ROCE_WQE_BASE_FORMAT_H
#define ROCE_WQE_BASE_FORMAT_H
#include "base_type.h"
/* **************************************************************** */
typedef struct roce5_wqe_ctrl_seg {
    /* DW0 */
    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 owner : 1;
            u32 ctrlsl : 2;
            u32 csl : 2;
            u32 difsl : 3;
            u32 cr : 1;
            u32 df : 1;
            u32 va : 1;
            u32 tsl : 5;
            u32 cf : 1;
            u32 wf : 1;
            u32 wqe_ssn : 2;
            u32 fde : 1;
            u32 fast : 1;
            u32 drvsl : 2;
            u32 bdsl : 8;
#else
            u32 bdsl : 8;  /* Data segment length, in the unit of 1B. When inline is used, the length of inline is
                              described. The total length of the data segment must be aligned to 8B. */
            u32 drvsl : 2; /* Indicates the length of the driver section. The value is counted by 8B, and the value of
                              RoCE is 0. */
            u32 fast : 1;  /* Fast Path Enable，指示在SQ_FETCH_WQE response API时，是否返回DMA */
            u32 fde : 1;    /* fast direct wqe，指示fast dircet wqe 使能，使用psm进行加速 */
            u32 wqe_ssn : 2; /* wqe_ssn 表示驱动侧记录的wqe ssn最低2 bit，用于psm fast direct wqe去重 */
            u32 wf : 1; /* 0-Normal WQE, 1-link WQE */
            u32 cf : 1; /* Complete segment format flag. The 0-complete segment directly contains the status information.
                           1: SGL */
            u32 tsl : 5; /* Length of the task segment. The value ranges from 8 to 48. The value of RoCE ranges from 8
                            to 48. */
            u32 va : 1;  /* SGE address format flag. The 0-SGE contains the physical address and length. The 1-SGE
                            contains the virtual address, length, and key. The value of RoCE is 1. */
            u32 df : 1;  /* Data segment format flag bit. 0-describes data in SGE format. 1: Data is described in inline
                            format. */
            u32 cr : 1;  /* The CQE generates the request flag. The 0-does not generate the CQE. 1: Generate CQE */
            u32 difsl : 3;  /* DIF segment length. The value is counted by 8 bytes. The value of RoCE is 0. */
            u32 csl : 2;    /* Length of the completed segment. The value is counted by 8B. The value of RoCE is 0. */
            u32 ctrlsl : 2; /* Length of the control segment. The value is 8 bytes and the value of RoCE is 16. */
            u32 owner : 1; /* Owner bit. The value 1 indicates all hardware, and the value 0 indicates all software. The
                              meaning of the queue owner bit is reversed every time the queue owner is traversed. */
#endif
        } bs;
        u32 value;
    } dw0;

    /* DW1 */
    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 cl : 4;
            u32 signature : 8;
            u32 rsvd : 4;
            u32 mask_pi : 16;
#else
            u32 mask_pi : 16; /* Pi is the value of the queue depth mask. It is valid when direct wqe. */
            u32 rsvd : 4;
            u32 signature : 8;
            u32 cl : 4; /* The length of CQE is generated in the task segment. */
#endif
        } bs;
        u32 value;
    } dw1;

    /* DW2 */
    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 type : 2;
            u32 rsvd1 : 3;
            u32 cos : 3;
            u32 cp_flag : 1;
            u32 rsvd : 1;
            u32 ctx_size : 2;
            u32 qpn : 20;
#else
            u32 qpn : 20;
            u32 ctx_size : 2; /* RoCE QPC size, 512B */
            u32 rsvd : 1;
            u32 cp_flag : 1; /* control plane flag */
            u32 cos : 3;     /* Scheduling priority. The value source is SL. */
            u32 rsvd1 : 3;
            u32 type : 2; /* Set RoCE SQ Doorbell to 2 and RoCE Arm CQ Doorbell to 3. */
#endif
        } bs;
        u32 value;
    } dw2;

    /* DW3 */
    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 sub_type : 4;   /*  */
            u32 sgid_index : 7; /* gid index */
            u32 mtu_shift : 3;  /* 256B:0;512B:1;1024B:2;2048B:3;4096B:4 */
            u32 rsvd : 1;
            u32 xrc_vld : 1;    /* 1:XRC service type */
            u32 pi : 16;        /* host sw write the sq produce index high 8bit to this section; */
#else
            u32 pi : 16; /* host sw write the sq produce index high 8bit to this section; */
            u32 xrc_vld : 1;
            u32 rsvd : 1;
            u32 mtu_shift : 3;
            u32 sgid_index : 7;
            u32 sub_type : 4;
#endif
        } bs;
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 queue_id : 4;
            u32 rsvd : 12;
            u32 pi : 16;
#else
            u32 pi : 16;
            u32 rsvd : 12;
            u32 queue_id : 4;
#endif
        } bs1;
        u32 value;
    } dw3;
} roce_wqe_ctrl_seg_s;

typedef union roce5_wqe_tsk_com_seg {
    struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
        u32 se : 1;
        u32 f : 1;
        u32 c : 1;
        u32 op_type : 5;
        u32 so : 1;
        u32 rsvd : 3;
        u32 dif_en : 1;
        u32 rsvd0 : 1;
        u32 xrc_srqn : 18;
#else
        u32 xrc_srqn : 18; /* The XRC is valid, and the remote SRQN is specified. */
        u32 rsvd0 : 1;
        u32 dif_en : 1;
        u32 rsvd : 3;
        u32 so : 1; /* Strong order-preserving flag, valid only for Local invalidate\Type 2 Bind MW and FRPMR (consider
                       the implementation at the bottom layer) */
        u32 op_type : 5; /* Operation type of the SQ WQE.
                          * 8'h00-Send
                          * 8'h01-Send with Invalidate
                          * 8'h02-Send with Immediate Data
                          * 8'h03-rsvd
                          * 8'h04-RDMA Write
                          * 8'h05-RDMA Write with Immediate Data
                          * 8'h06-RDMA WRITE CMD64
                          * 8'h07-rsvd
                          * 8'h08-RDMA READ
                          * 8'h09-ATOMIC WRITE
                          * 8'h0a-FLUSH
                          * 8'h0b-rsvd
                          * 8'h0c-Atomic compare & swap
                          * 8'h0d-Atomic Fetch & ADD
                          * 8'h0e-Atomic Masked Compare & Swap (Extended Atomic operation)
                          * 8'h0f-Atomic Masked Fetch & Add (Extended Atomic operation)
                          * 8'h10-Fast Register PMR
                          * 8'h11-Local Invalidate
                          * 8'h12-Bind Memory Window Type1/2
                          * 8'h13-Local operation(extended for further local operation)
                          * other-Reserved */
        u32 c : 1;       /* Indicates whether the SQ generates the CQE, which is required by the microcode. */
        u32 f : 1;       /* Indicates whether the SQ requires order-preserving. */
        u32 se : 1;      /* Indicates whether the packet carries the SE flag. */
#endif
    } bs;

    u32 value;
} roce_wqe_tsk_com_seg_u;

typedef union roce_wqe_tsk_misc_seg {
    struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
        u32 pi : 16; // 完整的没有掩码队列深度的pi, fast direct wqe场景有效
        u32 cmd_len : 8;
        u32 last_ext_len : 8;
#else
        u32 last_ext_len : 8;
        u32 cmd_len : 8;
        u32 pi : 16;
#endif
    } bs;

    u32 value;
} roce_wqe_tsk_misc_seg_u;

/* *
 * Struct name:      sq_wqe_task_remote_common_s.
 * @brief :          SQ WQE common.
 * Description:
 */
typedef struct tag_sq_wqe_task_remote_common {
    roce_wqe_tsk_com_seg_u common;

    u32 data_len; /* SQ WQE DMA LEN */

    u32 immdata_invkey; /* SEND invalidate or immedate */

    roce_wqe_tsk_misc_seg_u dw3;
} sq_wqe_task_remote_common_s;

/* Send WQE/Send with imme WQE/Send with invalid(inline or not inline) */
typedef struct roce_wqe_send_tsk_seg {
    roce_wqe_tsk_com_seg_u common;

    /* DW1 */
    u32 data_len; /* Length of the data sent by the SQ WQE */

    /* DW2 */
    u32 immdata_invkey; /* This parameter is valid for the immediate data operation or SEND invalidate. */

    /* DW3 */
    roce_wqe_tsk_misc_seg_u dw3;
} roce_wqe_send_tsk_seg_s;

/* RDMA Read WQE/RDMA Write WQE/RDMA Write with imme WQE(inline or non-inline) */
typedef struct roce_wqe_rdma_tsk_seg {
    roce_wqe_tsk_com_seg_u common;

    /* DW1 */
    u32 data_len;

    /* DW2 */
    u32 imm_data;

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
    u32 ulp; // ulp可使用
} roce_wqe_rdma_tsk_seg_s;

/* Atomic WQE */
typedef struct roce_wqe_atomic_tsk_seg {
    roce_wqe_tsk_com_seg_u common;

    /* DW1 */
    u32 key;

    /* DW2~3 */
    union {
        u64 va;
        struct {
            u32 va_h32;
            u32 va_l32;
        } bs;
    } dw2;

    /* DW4~5 */
    union {
        u64 swap_add_data;
        struct {
            u32 swap_add_data_h32;
            u32 swap_add_data_l32;
        } bs;
    } dw4;

    /* DW6~7 */
    union {
        u64 cmp_data;
        struct {
            u32 cmp_data_h32;
            u32 cmp_data_l32;
        } bs;
    } dw6;
} roce_wqe_atomic_tsk_seg_s;

/* ext Atomic WQE */
#define ROCE_WQE_ATOMIC_DATA_SIZE 32
#define ROCE_WQE_ATOMIC_DATA_LEN 128
#define ROCE_WQE_ATOMIC_DATA_SIZE_2B_ALIGN (ROCE_WQE_ATOMIC_DATA_SIZE >> 1)
typedef struct roce5_wqe_ext_atomic_tsk_seg {
    roce_wqe_tsk_com_seg_u common;

    /* DW1 */
    u32 key;

    /* DW2~3 */
    union {
        u64 va;
        struct {
            u32 va_h32;
            u32 va_l32;
        } dw2;
    };
    u32 atomic_data[ROCE_WQE_ATOMIC_DATA_SIZE];
} roce_wqe_ext_atomic_tsk_seg_s;

/* Mask Atomic WQE */
typedef struct roce_wqe_mask_atomic_tsk_seg {
    roce_wqe_tsk_com_seg_u common;

    /* DW1 */
    u32 rkey;

    /* DW2~3 */
    union {
        u64 va;
        struct {
            u32 va_h32;
            u32 va_l32;
        } bs;
    } dw2;

    /* DW4~5 */
    union {
        u64 swap_add_data;
        struct {
            u32 swap_add_data_h32;
            u32 swap_add_data_l32;
        } bs;
    } dw4;

    /* DW6~7 */
    union {
        u64 cmp_data;
        struct {
            u32 cmp_data_h32;
            u32 cmp_data_l32;
        } bs;
    } dw6;

    /* DW8~9 */
    union {
        u64 swap_msk;
        struct {
            u32 swap_msk_h32;
            u32 swap_msk_l32;
        } bs;
    } dw8;

    /* DW9~10 */
    union {
        u64 cmp_msk;
        struct {
            u32 cmp_msk_h32;
            u32 cmp_msk_l32;
        } bs;
    } dw10;
} roce_wqe_mask_atomic_tsk_seg_s;

/* UD send WQE */
typedef struct roce_wqe_ud_tsk_seg {
    /* DW0 */
    roce_wqe_tsk_com_seg_u common;

    /* DW1 */
    u32 data_len;

    /* DW2 */
    u32 immdata_invkey;

    /* DW3 */
    roce_wqe_tsk_misc_seg_u dw2;

    /* DW4 */
    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 fl : 1;
            u32 wqe_cos : 3; /* DB cos or Path cos or MQM cos */
            u32 stat_rate : 4;
            u32 rsvd : 6;
            u32 pd : 18;
#else
            u32 pd : 18; /* Used to verify the PD in the QPC. */
            u32 rsvd : 6;
            u32 stat_rate : 4; /* Maximum static rate control 0: No limit on the static rate (100% port speed)
                                * 1-6: reserved
                                * 7: 2.5 Gb/s.  8: 10 Gb/s.  9: 30 Gb/s. 10: 5 Gb/s. 11: 20 Gb/s.
                                * 12: 40 Gb/s. 13: 60 Gb/s. 14: 80 Gb/s.15: 120 Gb/s. */
            u32 wqe_cos : 3;   /* DB cos or Path cos or MQM cos */
            u32 fl : 1;
#endif
        } bs;
        u32 value;
    } dw3;

    /* DW5 */
    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 tc : 8;
            u32 rsvd : 4;
            u32 port : 4;
            u32 vlan_en : 1;
            u32 sgid_idx : 7;
            u32 hop_limit : 8;
#else
            u32 hop_limit : 8;
            u32 sgid_idx : 7;
            u32 vlan_en : 1;
            u32 port : 4;
            u32 rsvd : 4;
            u32 tc : 8;
#endif
        } bs;
        u32 value;
    } dw4;

    /* DW6 */
    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 rsvd : 2;
            u32 smac_index : 10;
            u32 flow_label : 20;
#else
            u32 flow_label : 20;
            u32 smac_index : 10;
            u32 rsvd : 2;
#endif
        } bs;
        u32 value;
    } dw5;

    /* DW7~10 */
    u8 dgid[16];

    /* DW11 */
    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 rsvd : 8;
            u32 dst_qp : 24;
#else
            u32 dst_qp : 24;
            u32 rsvd : 8;
#endif
        } bs;
        u32 value;
    } dw10;

    /* DW12 */
    u32 qkey;

    /* DW13 */
    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 vlan_pri : 3; /* send pkt pri */
            u32 cfi : 1;
            u32 vlan_id : 12;
            u32 dmac_h16 : 16;
#else
            u32 dmac_h16 : 16;
            u32 vlan_id : 12;
            u32 cfi : 1;
            u32 vlan_pri : 3; /* send pkt pri */
#endif
        } bs;
        u32 value;
    } dw12;

    /* DW14 */
    u32 dmac_l32;

    /* DW15 */
    u32 rsvd;
} roce_wqe_ud_tsk_seg_s;

/* DRC send WQE */
typedef struct roce_wqe_drc_rdma_tsk_seg {
    /* DW0~2 */
    u32 rsvd[3];

    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 fl : 1;
            u32 wqe_cos : 3; /* DB cos or Path cos or MQM cos */
            u32 stat_rate : 4;
            u32 rsvd : 6;
            u32 pd : 18;
#else
            u32 pd : 18; /* Used to verify the PD in the QPC. */
            u32 rsvd : 6;
            u32 stat_rate : 4; /* Maximum static rate control 0: No limit on the static rate (100% port speed)
                                * 1-6: reserved
                                * 7: 2.5 Gb/s.  8: 10 Gb/s.  9: 30 Gb/s. 10: 5 Gb/s. 11: 20 Gb/s.
                                * 12: 40 Gb/s. 13: 60 Gb/s. 14: 80 Gb/s.15: 120 Gb/s. */
            u32 wqe_cos : 3;   /* DB cos or Path cos or MQM cos */
            u32 fl : 1;
#endif
        } bs;
        u32 value;
    } dw3;

    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 tc : 8;
            u32 rsvd : 4;
            u32 port : 4;
            u32 vlan_en : 1;
            u32 sgid_idx : 7;
            u32 hop_limit : 8;
#else
            u32 hop_limit : 8;
            u32 sgid_idx : 7;
            u32 vlan_en : 1;
            u32 port : 4;
            u32 rsvd : 4;
            u32 tc : 8;
#endif
        } bs;
        u32 value;
    } dw4;

    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 rsvd : 2;
            u32 smac_index : 10;
            u32 flow_label : 20;
#else
            u32 flow_label : 20;
            u32 smac_index : 10;
            u32 rsvd : 2;
#endif
        } bs;
        u32 value;
    } dw5;

    u8 dgid[16];

    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 rsvd : 8;
            u32 drctn : 24;
#else
            u32 drctn : 24;
            u32 rsvd : 8;
#endif
        } bs;
        u32 value;
    } dw10;

    /* DW11 */
    u32 drct_key_l;

    /* DW12 */
    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 vlan_pri : 3; /* send pkt pri */
            u32 cfi : 1;
            u32 vlan_id : 12;
            u32 dmac_h16 : 16;
#else
            u32 dmac_h16 : 16;
            u32 vlan_id : 12;
            u32 cfi : 1;
            u32 vlan_pri : 3; /* send pkt pri */
#endif
        } bs;
        u32 value;
    } dw12;

    /* DW13 */
    u32 rsvd1;

    /* DW14 */
    u32 dmac_l32;

    /* DW15 */
    u32 drct_key_h;
} roce_wqe_drc_tsk_seg_s;

typedef struct roce_wqe_data_seg {
    /* DW0~1 */
    union {
        u64 addr;
        struct {
            u32 addr_h32;
            u32 addr_l32;
        } bs;
    };

    /* DW2~3 */
    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 rsvd : 1;
            u32 len : 31;
#else
            u32 len : 31;
            u32 rsvd : 1;
#endif
        } bs;
        u32 value;
    } dw2;

    /* DW4 */
    u32 key;
} roce_wqe_data_seg_s;

#endif // ROCE_WQE_BASE_FORMAT_H
