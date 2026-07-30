/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 * Description: RDMA cmdq attribute qp context.
 * Create: 2021-12-30
 */

#ifndef ROCE_VERBS_QP_ATTR_H
#define ROCE_VERBS_QP_ATTR_H

#include "roce_npu_cmd_mr_defs.h"

#ifndef BIG_ENDIAN
#define BIG_ENDIAN 0x4321
#endif

#define ROCE_VERBS_SQ_WQEBB_SIZE (2)
#define ROCE_VERBS_SQ_PI_VLD (1)

#define ROCE_VERBS_QP_HASH_TYPE_BOND   (0)
#define ROCE_VERBS_QP_HASH_TYPE_UBC    (1)

enum roce5_qpc_mtucode {
    ROCE_MTU_CODE_256 = 0x0,
    ROCE_MTU_CODE_512 = 0x1,
    ROCE_MTU_CODE_1K = 0x3,
    ROCE_MTU_CODE_2K = 0x7,
    ROCE_MTU_CODE_4K = 0xf
};

#pragma pack(4)
/* qpc_attr_com info ,13*4B */
typedef struct tag_roce_verbs_qpc_attr_com {
    /* DW0 */
    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 service_type : 3;
            u32 fre : 1;
            u32 rwe : 1;
            u32 rre : 1;
            u32 rae : 1;
            u32 rkey_en : 1;
            u32 dest_qp : 24;
#else
            u32 dest_qp : 24; /* Destination QP number, which is extended to 24 bits in consideration of interconnection
                                 with commercial devices. */
            u32 rkey_en : 1;
            u32 rae : 1;
            u32 rre : 1;
            u32 rwe : 1;
            u32 fre : 1;             /* Indicates whether the local FRPMR is enabled. */
            u32 service_type : 3;    /* Transmission Type
                                      * 000:RC
                                      * 001:UC
                                      * 010:RD
                                      * 011 UD
                                      * 101:XRC
                                      * Other:Reserved
                                      */
#endif
        } bs;
        u32 value;
    } dw0;

    /* DW1 */
    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 sra_max : 3;
            u32 rra_max : 3;
            u32 rnr_retry_limit : 3;
            u32 to_retry_limit : 3;
            u32 local_qp : 20;
#else
            u32 local_qp : 20;       /* Local QP number */
            u32 to_retry_limit : 3;  /* Number of ACK retransmissions. The value 7 indicates unlimited times, and the
                                        value 0 indicates no retransmission. */
            u32 rnr_retry_limit : 3; /* The maximum number of RNR retransmissions is 7. The value 7 indicates that the
                                        maximum number of retransmissions is 7, and the value 0 indicates that the
                                        retransmission is not performed. */
            u32 rra_max : 3;         /* The maximum value of responser resource is 128. */
            u32 sra_max : 3;         /* The maximum value of initiator depth is 128. */
#endif
        } bs;
        u32 value;
    } dw1;

    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 ack_to : 5;
            u32 min_rnr_nak : 5;
            u32 cont_size : 2;
            u32 cont_en : 1;
            u32 srq_en : 1;
            u32 xrc_srq_en : 1;
            u32 vroce_en : 1;
            u32 host_oqid : 16;
#else
            u32 host_oqid : 16;
            u32 vroce_en : 1;
            u32 xrc_srq_en : 1;
            u32 srq_en : 1;
            u32 cont_en : 1;
            u32 cont_size : 2;
            u32 min_rnr_nak : 5; /* NAK code of RNR. This parameter is mandatory when INIT2RNR and
                                        RTR2RTS\SQE2RTS\SQD2SQD\SQD2RTS is optional. */
            u32 ack_to : 5;
#endif
        } bs;
        u32 value;
    } dw2;

    /* DW3 */
    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 tss_timer_num : 3;
            u32 xrc_vld : 1;
            u32 srq_container : 1;
            u32 invalid_credit : 1;
            u32 ext_md : 1;
            u32 ext_mtu : 1;
            u32 dsgl_en : 1;
            u32 dif_en : 1;
            u32 pmtu : 3;
            u32 base_mtu_n : 1;
            u32 pd : 18;
#else
            u32 pd : 18;
            u32 base_mtu_n : 1;
            u32 pmtu : 3;
            u32 dif_en : 1;
            u32 dsgl_en : 1;
            u32 ext_mtu : 1;
            u32 ext_md : 1;
            u32 invalid_credit : 1;
            u32 srq_container : 1;
            u32 xrc_vld : 1;
            u32 tss_timer_num : 3;
#endif
        } bs;

        u32 value;
    } dw3;

    /* DW4 */
    u32 q_key;

    /* DW5 */
    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 ulp_type : 8;
            u32 oor_en : 1;
            u32 capture_en : 1;
            u32 rsvd : 1;
            u32 acx_mark : 1;
            u32 mtu_code : 4;       /* see enum roce5_qpc_mtucode */
            u32 port : 2;
            u32 ep : 3;
            u32 cos : 3;
            u32 so_ro : 2;
            u32 dma_attr_idx : 6;
#else
            u32 dma_attr_idx : 6;
            u32 so_ro : 2;
            u32 cos : 3;
            u32 ep : 3;
            u32 port : 2;
            u32 mtu_code : 4;       /* see enum roce5_qpc_mtucode */
            u32 acx_mark : 1;
            u32 rsvd : 1;
            u32 capture_en : 1;
            u32 oor_en : 1;
            u32 ulp_type : 8;
#endif
        } bs;
        u32 value;
    } dw5;

    /* DW6 */
    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 sd_mpt_idx : 12;
            u32 force_local : 1;
            u32 state : 4;
            u32 rsvd : 15;
#else
            u32 rsvd : 15;
            u32 state : 4;
            u32 force_local : 1;
            u32 sd_mpt_idx : 12;
#endif
        } bs;
        u32 value;
    } dw6;

    /* DW7 */
    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 rsvd : 10;
            u32 sq_cqn_lb : 1;
            u32 rq_cqn_lb : 1;
            u32 rq_cqn : 20;
#else
            u32 rq_cqn : 20;
            u32 rq_cqn_lb : 1;
            u32 sq_cqn_lb : 1;
            u32 rsvd : 10;
#endif
        } bs;
        u32 value;
    } dw7;

    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 rsvd : 8;
            u32 next_send_psn : 24;
#else
            u32 next_send_psn : 24;
            u32 rsvd : 8;
#endif
        } bs;
        u32 value;
    } dw8;

    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 rsvd : 8;
            u32 next_rcv_psn : 24;
#else
            u32 next_rcv_psn : 24;
            u32 rsvd : 8;
#endif
        } bs;
        u32 value;
    } dw9;

    /* DW10 */
    u32 lsn;

    /* DW11 */
    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 rsvd : 5;
            u32 shadow_cq_vfid : 12;
            u32 set_mpt_indx : 1;
            u32 rsvd0 : 1;
            u32 shadow : 1;
            u32 shadow_vfid : 12;
#else
            u32 shadow_vfid : 12;
            u32 shadow : 1;
            u32 rsvd0 : 1;
            u32 set_mpt_indx : 1;
            u32 shadow_cq_vfid : 12;
            u32 rsvd : 5;
#endif
        } bs;
        u32 value;
    } dw11;

    /* DW12 */
    u32 rsvd;
} roce_verbs_qpc_attr_com_s;

typedef struct tag_roce_verbs_qpc_attr_path {
    /* DW0 */
    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 udp_src_port_h : 8;
            u32 ubc_tx_hash_value : 4; // 供ubc侧选口使用
            u32 bond_tx_hash_value : 4;
            u32 dmac_h16 : 16;
#else
            u32 dmac_h16 : 16;
            u32 bond_tx_hash_value : 4;
            u32 ubc_tx_hash_value : 4; // 供ubc侧选口使用
            u32 udp_src_port_h : 8;
#endif
        } bs;
        u32 value;
    } dw0;

    u32 dmac_l32;

    /* DW2~5 */
    u8 dgid[16];

    /* DW6 */
    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 rsvd : 4;
            u32 tclass : 8;
            u32 flow_label : 20;
#else
            u32 flow_label : 20; /* GRH flow lable */
            u32 tclass : 8;
            u32 rsvd : 4;
#endif
        } bs;
        u32 value;
    } dw6;

    /* DW7 */
    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 sl : 3;
            u32 loop : 1;
            u32 udp_src_port : 8;
            u32 rsvd : 4;
            u32 base_sgid_n : 1;
            u32 sgid_index : 7;
            u32 hoplmt : 8;
#else
            u32 hoplmt : 8;
            u32 sgid_index : 7;
            u32 base_sgid_n : 1;
            u32 rsvd : 4;
            u32 udp_src_port : 8;
            u32 loop : 1;
            u32 sl : 3;
#endif
        } bs;
        u32 value;
    } dw7;
} roce_verbs_qpc_attr_path_s;

typedef struct tag_roce_verbs_qpc_attr_chip {
    /* DW0~1 */
    union {
        u64 sq_rq_l0mtt_gpa; /* hi[63:32],lo[31:03],sq_rq_gpa_sign[02:00] */
        struct {
            u32 sq_rq_l0mtt_gpa_hi;
            u32 sq_rq_l0mtt_gpa_lo;
        } bs;
    } dw0;

    /* DW2~3 */
    union {
        u64 sq_rq_pi_record_gpa_at_hop_num; /* hi[63:32],lo[31:02],sq_rq_at_hop_num[01:00] */
        struct {
            u32 sq_rq_pi_record_gpa_hi;
            u32 sq_rq_pi_record_gpa_lo_at_hop_num; /* at_hop_num: bit[01:00] */
        } bs;
    } dw2;

    /* DW4 */
    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 qp_page_size : 4;
            u32 sq_rq_mtt_page_size : 4;
            u32 rsvd1 : 3;
            u32 qp_signature : 5;
            u32 rsvd0 : 1;
            u32 dsgl : 1;
            u32 rrw_mtt_prefetch_maxlen : 2;
            u32 rc_size : 4;
            u32 rc_max_size : 3;
            u32 rq_base_ci : 5;
#else
            u32 rq_base_ci : 5;
            u32 rc_max_size : 3;
            u32 rc_size : 4;
            u32 rrw_mtt_prefetch_maxlen : 2;
            u32 dsgl : 1;
            u32 rsvd0 : 1;
            u32 qp_signature : 5;
            u32 rsvd1 : 3;
            u32 sq_rq_mtt_page_size : 4;
            u32 qp_page_size : 4;
#endif
        } bs;
        u32 value;
    } dw4;

    /* DW5 */
    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 sq_wqe_prefetch_maxnum : 3;
            u32 sq_wqe_prefetch_minnum : 3;
            u32 sq_wqe_cache_thd_sel : 2;
            u32 sq_wqecnt_lth : 4;
            u32 sq_wqecnt_rctl_en : 1;
            u32 sq_wqecnt_rctl : 1;
            u32 sq_prefetch_one_wqe : 1;
            u32 sq_prewqe_mode : 1;
            u32 sqa_wqe_prefetch_maxnum : 3;
            u32 sqa_wqe_prefetch_minnum : 3;
            u32 sqa_wqe_cache_thd_sel : 2;
            u32 sq_wqe_check_en : 1;
            u32 sq_pi_on_chip : 1;
            u32 sq_inline_en : 1;
            u32 sq_size : 5;
#else
            u32 sq_size : 5;
            u32 sq_inline_en : 1;
            u32 sq_pi_on_chip : 1;
            u32 sq_wqe_check_en : 1;
            u32 sqa_wqe_cache_thd_sel : 2;
            u32 sqa_wqe_prefetch_minnum : 3;
            u32 sqa_wqe_prefetch_maxnum : 3;
            u32 sq_prewqe_mode : 1;
            u32 sq_prefetch_one_wqe : 1;
            u32 sq_wqecnt_rctl : 1;
            u32 sq_wqecnt_rctl_en : 1;
            u32 sq_wqecnt_lth : 4;
            u32 sq_wqe_cache_thd_sel : 2;
            u32 sq_wqe_prefetch_minnum : 3;
            u32 sq_wqe_prefetch_maxnum : 3;
#endif
        } bs;
        u32 value;
    } dw5;

    /* DW6 */
    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 sq_wqe_prefetch_mode : 1;
            u32 sq_mtt_prefetch_maxlen : 3;
            u32 sqa_mtt_prefetch_maxlen : 3;
            u32 rsvd : 25;
#else
            u32 rsvd : 25;
            u32 sqa_mtt_prefetch_maxlen : 3;
            u32 sq_mtt_prefetch_maxlen : 3;
            u32 sq_wqe_prefetch_mode : 1;
#endif
        } bs;
        u32 value;
    } dw6;

    /* DW7 */
    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 rq_wqe_prefetch_maxnum : 3;
            u32 rq_wqe_prefetch_minnum : 3;
            u32 rq_wqe_cache_thd_sel : 2;
            u32 rq_wqecnt_lth : 4;
            u32 rq_wqecnt_rctl_en : 1;
            u32 rq_wqecnt_rctl : 1;
            u32 srqn : 18;
#else
            u32 srqn : 18;
            u32 rq_wqecnt_rctl : 1;
            u32 rq_wqecnt_rctl_en : 1;
            u32 rq_wqecnt_lth : 4;
            u32 rq_wqe_cache_thd_sel : 2;
            u32 rq_wqe_prefetch_minnum : 3;
            u32 rq_wqe_prefetch_maxnum : 3;
#endif
        } bs;
        u32 value;
    } dw7;

    /* DW8 */
    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 srq_wqe_rthd_sel : 2;
            u32 srq_rqecnt_th : 4;
            u32 rq_pi_on_chip : 1;
            u32 rq_inline_en : 1;
            u32 rq_wqebb_size : 3;
            u32 rq_size : 5;
            u32 xrcd : 16;
#else
            u32 xrcd : 16;
            u32 rq_size : 5;
            u32 rq_wqebb_size : 3;
            u32 rq_inline_en : 1;
            u32 rq_pi_on_chip : 1;
            u32 srq_rqecnt_th : 4;
            u32 srq_wqe_rthd_sel : 2;
#endif
        } bs;
        u32 value;
    } dw8;

    /* DW9 */
    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 container_en : 1;
            u32 container_sz : 2;
            u32 srq_warth_flag : 1;
            u32 srq_mtt_prefetch_maxlen1 : 2;
            u32 rq_mtt_prefetch_maxwqe : 3;
            u32 rq_mtt_prefetch_maxlen0 : 2;
            u32 rq_mtt_prefetch_maxlen1 : 2;
            u32 rsvd : 19;
#else
            u32 rsvd : 19;
            u32 rq_mtt_prefetch_maxlen1 : 2;
            u32 rq_mtt_prefetch_maxlen0 : 2;
            u32 rq_mtt_prefetch_maxwqe : 3;
            u32 srq_mtt_prefetch_maxlen1 : 2;
            u32 srq_warth_flag : 1;
            u32 container_sz : 2;
            u32 container_en : 1;
#endif
        } bs;
        u32 value;
    } dw9;

    /* DW10 */
    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 rc_entry_prefetch_maxnum : 3;
            u32 rc_mtt_prefetch_maxlen : 2;
            u32 rsvd : 1;
            u32 rc_entry_size : 2;
            u32 rc_page_gpa_h : 24;
#else
            u32 rc_page_gpa_h : 24; /* bit[63:40] Indicates the start GPA of RDMARC table. The driver needs to allocate
                                     * continuous physical address for the RDMARC table.Configured by Driver */
            u32 rc_entry_size : 2;
            u32 rsvd : 1;
            u32 rc_mtt_prefetch_maxlen : 2;
            u32 rc_entry_prefetch_maxnum : 3; /* Maximum number of prefetch Entries for RDMARC table.000: prefetch
                                               * number equals to zero; Others: prefetch number equals to
                                               * (2^(rc_entry_prefetch_maxnum-1)). Configured by Driver */
#endif
        } bs;
        u32 value;
    } dw10;

    /* DW11 */
    u32 rc_page_gpa_l; /* bit[39:8] */

    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 rsvd : 1;
            u32 srq_pd : 18;
            u32 srq_wqebb_size : 3;
            u32 srq_page_size : 4;
            u32 srq_size : 5;
            u32 srq_rkey_en : 1;
#else
            u32 srq_rkey_en : 1;
            u32 srq_size : 5;
            u32 srq_page_size : 4;
            u32 srq_wqebb_size : 3;
            u32 srq_pd : 18;
            u32 rsvd : 1;
#endif
        } bs;
        u32 value;
    } dw12;

    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 rsvd : 8;
            u32 srq_cqn : 20;
            u32 srq_state : 4;
#else
            u32 srq_state : 4;
            u32 srq_cqn : 20;
            u32 rsvd : 8;
#endif
        } bs;
        u32 value;
    } dw13;

    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 rsvd : 15;
            u32 qp_rkey_en : 1;
            u32 srq_xrcd : 16;
#else
            u32 srq_xrcd : 16;
            u32 qp_rkey_en : 1;
            u32 rsvd : 15;
#endif
        } bs;
        u32 value;
    } dw14;

    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 rsvd : 4;
            u32 rq_page_size : 4;
            u32 rq_pd : 18;
            u32 rq_rkey_en : 1;
            u32 rq_size : 5;
#else
            u32 rq_size : 5;
            u32 rq_rkey_en : 1;
            u32 rq_pd : 18;
            u32 rq_page_size : 4;
            u32 rsvd : 4;
#endif
        } bs;
        u32 value;
    } dw15;

    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 rsvd : 6;
            u32 sq_wqebb_size : 3;
            u32 sq_pd : 18;
            u32 sq_page_size : 4;
            u32 sq_rkey_en : 1;
#else
            u32 sq_rkey_en : 1;
            u32 sq_page_size : 4;
            u32 sq_pd : 18;
            u32 sq_wqebb_size : 3;
            u32 rsvd : 6;
#endif
        } bs;
        u32 value;
    } dw16;

    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 rsvd : 12;
            u32 sqa_cqn : 20;
#else
            u32 sqa_cqn : 20;
            u32 rsvd : 12;
#endif
        } bs;
        u32 value;
    } dw17;

    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 rsvd : 14;
            u32 ud_pd : 18;
#else
            u32 ud_pd : 18;
            u32 rsvd : 14;
#endif
        } bs;
        u32 value;
    } dw18;

    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 rsvd : 14;
            u32 qp_pd : 18;
#else
            u32 qp_pd : 18;
            u32 rsvd : 14;
#endif
        } bs;
        u32 value;
    } dw19;
} roce_verbs_qpc_attr_chip_s;

typedef struct roce_verbs_qpc_attr_vbs {
    u32 sqpc_ci_record_addr_h;
    u32 sqpc_ci_record_addr_l;
} roce_verbs_qpc_attr_vbs_s;

typedef struct roce_verbs_qpc_attr_jbof {
    u32 offload_en;
    u32 io_qpn;
    u32 sq_type;
    u32 sqpc_ci_record_addr_h;
    u32 sqpc_ci_record_addr_l;
} roce_verbs_qpc_attr_jbof_s;

typedef struct roce_verbs_qpc_attr_vroce {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
    u32 rsvd : 31;
    u32 vroce_en : 1;
#else
    u32 vroce_en : 1;
    u32 rsvd : 31;
#endif
    u32 ulp_type;
} roce_verbs_qpc_attr_vroce_s;

typedef struct roce_verbs_qpc_attr_dfx {
    /* DW0 */
    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 sq_pi : 16;
            u32 sq_ci : 16;
#else
            u32 sq_ci : 16;
            u32 sq_pi : 16;
#endif
        } bs;
        u32 value;
    } dw0;

    /* DW1 */
    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 sq_load_pi : 1;
            u32 rq_load_pi : 1;
            u32 rsvd : 6;
            u32 rc_prefetch_ci : 8;
            u32 sq_wqe_prefetch_ci : 16;
#else
            u32 sq_wqe_prefetch_ci : 16;
            u32 rc_prefetch_ci : 8;
            u32 rsvd : 6;
            u32 rq_load_pi : 1;
            u32 sq_load_pi : 1;
#endif
        } bs;
        u32 value;
    } dw1;

    /* DW2 */
    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 sqa_ci : 16;
            u32 sqa_wqe_prefetch_ci : 16;
#else
            u32 sqa_wqe_prefetch_ci : 16;
            u32 sqa_ci : 16;
#endif
        } bs;
        u32 value;
    } dw2;

    /* DW3 */
    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 sq_mtt_prefetch_wqe_ci : 16;
            u32 rq_mtt_prefetch_wqe_ci : 16;
#else
            u32 rq_mtt_prefetch_wqe_ci : 16;
            u32 sq_mtt_prefetch_wqe_ci : 16;
#endif
        } bs;
        u32 value;
    } dw3;

    /* DW4 */
    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 rq_pi : 16;
            u32 rq_ci : 16;
#else
            u32 rq_ci : 16;
            u32 rq_pi : 16;
#endif
        } bs;
        u32 value;
    } dw4;

    /* DW5 */
    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 rq_wqe_prefetch_ci : 16;
            u32 rc_pi : 8;
            u32 rc_ci : 8;
#else
            u32 rc_ci : 8;
            u32 rc_pi : 8;
            u32 rq_wqe_prefetch_ci : 16;
#endif
        } bs;
        u32 value;
    } dw5;

    /* DW6 */
    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 sw_wnd : 11;
            u32 sw_send_ce_sn : 11;
            u32 rsvd : 10;
#else
            u32 rsvd : 10;
            u32 sw_send_ce_sn : 11;
            u32 sw_wnd : 11;
#endif
        } bs;
        u32 value;
    } dw6;

    /* DW7 */
    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 rr_wnd : 11;
            u32 rr_send_ce_sn : 11;
            u32 rsvd : 10;
#else
            u32 rsvd : 10;
            u32 rr_send_ce_sn : 11;
            u32 rr_wnd : 11;
#endif
        } bs;
        u32 value;
    } dw7;

    /* DW8 */
    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 wnd_ctrl_rtt : 16;
            u32 rsvd : 16;
#else
            u32 rsvd : 16;
            u32 wnd_ctrl_rtt : 16;
#endif
        } bs;
        u32 value;
    } dw8;

    /* DW9-15 */
    u32 rsvd[7];
} roce_verbs_qpc_attr_dfx_s;

typedef struct roce_verbs_qpc_attr_ssu {
    /* DW0 */
    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 global_qpn : 20;
            u32 rsvd : 12;
#else
            u32 rsvd : 12;
            u32 global_qpn : 20;
#endif
        } bs;
        u32 value;
    } dw0;

    /* DW1-3 */
    u32 rsvd[3];
} roce_verbs_qpc_attr_ssu_s;

typedef struct roce_verbs_qpc_attr_ext {
    /* DW0~1 */
    roce_verbs_qpc_attr_vbs_s vbs_info;
    /* DW2~6 */
    roce_verbs_qpc_attr_jbof_s jbof_info;
    /* DW7~8 */
    roce_verbs_qpc_attr_vroce_s vroce_info;
    /* DW9~24 */
    roce_verbs_qpc_attr_dfx_s dfx_info;
    /* DW25~28 */
    roce_verbs_qpc_attr_ssu_s ssu_info;
} roce_verbs_qpc_attr_ext_s;

/* QPC Struct */
typedef struct tag_roce_verbs_qp_attr {
    /* com seg, DW0 ~ DW12 */
    roce_verbs_qpc_attr_com_s com_info;

    /* path seg, DW0 ~ DW7 */
    roce_verbs_qpc_attr_path_s path_info;

    /* chip seg, DW0 ~ DW19 */
    roce_verbs_qpc_attr_chip_s chip_seg;

    /* ext seg, DW0 ~ DW28 */
    roce_verbs_qpc_attr_ext_s ext_seg;
} roce_verbs_qp_attr_s;

#define ROCE_VERBS_QP_ATTR_SIZE 280

typedef struct tag_roce_verbs_qp_hw2sw_info {
    /* DW0~1 */
    u32 sq_buf_len; /* Buffer length of the SQ queue */
    u32 rq_buf_len; /* Buffer length of the RQ queue */

    /* DW2~6 */
    roce_verbs_mtt_cacheout_info_s cmtt_cache;

    /* DW7~8 */
    union {
        u64 wb_gpa; /* Address written back by the ucode after processing */

        struct {
            u32 syn_gpa_hi32; /* Upper 32 bits of the start address of mr or mw */
            u32 syn_gpa_lo32; /* Lower 32 bits of the start address of mr or mw */
        } gpa_dw;
    };
    union {
        struct {
            u32 rsvd : 16;
            u32 host_oqid : 16;
        } bs;

        u32 value;
    } dw9;
    roce_verbs_wqe_cacheout_info_s wqe_cache;
} roce_verbs_qp_hw2sw_info_s;

typedef struct tag_roce_verbs_query_qp_info {
    roce_verbs_qp_attr_s qp_attr;
} roce_verbs_query_qp_info_s;

typedef struct tag_roce_verbs_modify_ctx_info {
    u32 ctx_type;
    u32 offset;
    u32 value;
    u32 mask;
} roce_verbs_modify_ctx_info_s;

typedef struct tag_roce_verbs_xq_mtt_info {
    u32 mtt_flags;    /* Indicates whether to kick out cache. by queue (0) or VF(1). */
    u32 mtt_num; /* Number of cmtt, which needs to be assigned by the driver when the is kicked out by queue. */
    u32 mtt_cache_line_start; /* The driver needs to read the driver from the configuration file. */
    u32 mtt_cache_line_end;   /* The driver needs to read the driver from the configuration file. */
    u32 mtt_cache_line_size;  /* 0:256B,1:512B */
} roce_verbs_xq_mtt_info_s;

typedef struct tag_roce_verbs_udp_src_port_info {
    u32 udp_src_port;
    u32 rsvd[3];
} roce_verbs_udp_src_port_info_s;

typedef struct tag_roce_verbs_hash_info {
    u32 type;
    u32 hash_value;
    u32 rsvd[2];
} roce_verbs_qp_hash_info_s;

typedef struct tag_roce_verbs_dfx_info {
    u32 udp_src_port;
    u32 rx_port;
    u32 rsvd[14];
} roce_verbs_dfx_info_s;

#define ROCE_VERBS_MODIFY_RSP_MAGIC 0x5aa57bb7
typedef struct tag_roce_verbs_cmd_qp_modify_rsp {
    u32 magic;
    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 ssn : 24;
            u32 rsvd : 7;
            u32 is_ssn_vld : 1;
#else
            u32 is_ssn_vld : 1;
            u32 rsvd : 7;
            u32 ssn : 24;
#endif
        } bs;
        u32 value;
    } dw1;
    u32 rsvd[6];
} roce_verbs_cmd_qp_modify_rsp_s;

#pragma pack()

#endif /* ROCE_VERBS_QP_ATTR_H */

