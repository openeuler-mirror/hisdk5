/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2030. All rights reserved.
 * Description   : hmm cmd define
 * Author        : /
 * Create        : 2025/11/21
 * Notes         :
 * History       : None
 */
#ifndef HMM_CMD_DEFS_H
#define HMM_CMD_DEFS_H

/**
 * @brief enum rdma_hmm_cmd
 * @details 此枚举类型定义了RDMA HMM模块中使用的各种命令代码
 */
enum rdma_hmm_cmd {
    RDMA_CMD_SW2HW_MPT = 0x70,     /**< 将软件的多路径项（MPT）转换为硬件的多路径项 */
    RDMA_CMD_HW2SW_MPT = 0x71,     /**< 将硬件的多路径项转换为软件的多路径项 */
    RDMA_CMD_MODIFY_MPT = 0x72,    /**< 修改多路径项 */
    RDMA_CMD_QUERY_MPT = 0x73,     /**< 查询多路径项 */
    RDMA_CMD_FLUSH_TPT = 0x74,     /**< 刷新传输页表项 */
    RDMA_CMD_SYNC_TPT = 0x75       /**< 同步传输页表项 */
};

#define HMM_RDMA_USER_DATA_LENGTH 6 // mr_attr userdata段大小
#define DAVID_PATH_MAX_NUM 0x4      // 单个david和dpu总线侧最大路径数

#pragma pack(4)
typedef struct tag_hmm_verbs_cmd_header {
    union {
        u32 value;

        struct {
            u32 version : 8;
            u32 sub_cmd : 8;
            u32 cmd_bitmask : 16; // CMD_TYPE_BITMASK_E
        } bs;
    } dw0;

    u32 index; // qpn/cqn/srqn/mpt_index/gid idx

    u32 opt;

    union {
        u32 value;

        struct {
            u32 cmd_type : 8;
            u32 rsvd : 7;
            u32 seg_ext : 1;
            u32 cmd_len : 16; // verbs cmd total len(include cmd_com),unit:byte
        } bs;
    } dw3;
} hmm_verbs_cmd_header_s;

typedef struct tag_hmm_verbs_mr_attr {
    /* DW0 */
    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 buf_page_size : 4; /* Page_size of the buffer */
            u32 mtt_layer_num : 3; /* Mtt level */
            u32 mtt_page_size : 4; /* Page_size of mtt */
            u32 rsvd2 : 3;
            u32 david_en : 1;
            u32 zbva : 1;      /* The value 1 indicates that ZBVA is supported, that is, iova = 0. */
            u32 access_lr : 1; /* 1: Indicates that the local read permission is supported. */
            u32 access_lw : 1; /* The value 1 indicates that the local write permission is supported. */
            u32 access_rr : 1; /* 1: Indicates that the remote read permission is supported. */
            u32 access_rw : 1; /* 1: The remote write permission is supported. */
            u32 access_ra : 1; /* The value 1 indicates that the remote Atomic permission is supported. */
            u32 bpd : 1;       /* 1: Bound to pd */
            u32 bqp : 1;       /* 1: Bound to qp */
            u32 dif_mode : 1;
            u32 rkey : 1;
            u32 pa : 1;                /* Flag bit of DMA_MR */
            u32 r_w : 1;               /* Mr or mw. The value 1 indicates MR, and the value 0 indicates MW. */
            u32 remote_invalid_en : 1; /* Indicates whether to support the remote INVALID operation. */
            u32 invalid_en : 1;        /* Indicates whether to support the INVALID operation. */
            u32 fast_reg_en : 1;       /* Indicates whether the FRMR operation is supported. */
            u32 remote_access_en : 1;  /* Indicates whether the FRMR can specify remote rights. */
            u32 access_bind : 1;       /* Whether the mr supports the binding of the mw */
#else
            u32 access_bind : 1;       /* Whether the mr supports the binding of the mw */
            u32 remote_access_en : 1;  /* Indicates whether the FRMR can specify remote rights. */
            u32 fast_reg_en : 1;       /* Indicates whether the FRMR operation is supported. */
            u32 invalid_en : 1;        /* Indicates whether to support the INVALID operation. */
            u32 remote_invalid_en : 1; /* Indicates whether to support the remote INVALID operation. */
            u32 r_w : 1;               /* Mr or mw */
            u32 pa : 1;                /* Flag bit of DMA_MR */
            u32 rkey : 1;
            u32 dif_mode : 1;
            u32 bqp : 1;       /* 1: Bound to qp */
            u32 bpd : 1;       /* 1: Bound to pd */
            u32 access_ra : 1; /* The value 1 indicates that the remote Atomic permission is supported. */
            u32 access_rw : 1; /* 1: The remote write permission is supported. */
            u32 access_rr : 1; /* 1: Indicates that the remote read permission is supported. */
            u32 access_lw : 1; /* The value 1 indicates that the local write permission is supported. */
            u32 access_lr : 1; /* 1: Indicates that the local read permission is supported. */
            u32 zbva : 1;      /* The value 1 indicates that ZBVA is supported, that is, iova = 0. */
            u32 david_en : 1;
            u32 rsvd2 : 3;
            u32 mtt_page_size : 4; /* Page_size of mtt */
            u32 mtt_layer_num : 3; /* Number of mtt levels */
            u32 buf_page_size : 4; /* Page_size of the buffer */
#endif
        } bs;
        u32 value;
    } dw0;

    /* DW1 */
    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 so_ro : 2;
            u32 dma_attr_idx : 6;
            u32 sector_size : 1;
            u32 ep : 3;
            u32 qpn : 20;
#else
            u32 qpn : 20; /* Qp bound to mw */
            u32 ep : 3;
            u32 sector_size : 1;  /* 0:512B, 1:4KB */
            u32 dma_attr_idx : 6; /* Dma attribute index */
            u32 so_ro : 2;        /* Dma order-preserving flag */
#endif
        } bs;
        u32 value;
    } dw1;

    /* DW2 */
    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 status : 4;
            u32 indirect_mr : 1;
            u32 cos : 3;
            u32 block_size : 6;
            u32 pdn : 18;
#else
            u32 pdn : 18;       /* Pd bound to mr or mw */
            u32 block_size : 6; /* 2^(page_size+12) + 8*block_size */
            u32 cos : 3;
            u32 indirect_mr : 1;
            u32 status : 4; /* Mpt status. Valid values are VALID, FREE, and INVALID. */
#endif
        } bs;
        u32 value;
    } dw2;

    /* DW3 */
    union {
        struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && ((BYTE_ORDER == BIG_ENDIAN))
            u32 mkey : 8;
            u32 sw_dif_en : 1;
            u32 page_mode : 1;
            u32 fbo : 22;
#else
            u32 fbo : 22;
            u32 page_mode : 1;
            u32 sw_dif_en : 1;
            u32 mkey : 8;     /* The index is not included. */
#endif
        } bs;
        u32 value;
    } dw3;

    /* DW4~5 */
    union {
        u64 iova; /* Start address of mr or mw */
        struct {
            u32 iova_hi; /* Upper 32 bits of the start address of mr or mw */
            u32 iova_lo; /* Lower 32 bits of the start address of mr or mw */
        } dw4;
    };

    /* DW6~7 */
    union {
        u64 length; /* Length of mr or mw */
        struct {
            u32 length_hi; /* Length of mr or mw */
            u32 length_lo; /* Length of mr or mw */
        } dw6;
    };

    /* DW8~9 */
    union {
        u64 mtt_base_addr; /* Mtt base address (pa),low 3bits(gpa_sign) */
        struct {
            u32 mtt_base_addr_hi; /* Mtt base address (pa) upper 32 bits */
            u32 mtt_base_addr_lo; /* Lower 32 bits of mtt base address (pa),low 3bits(gpa_sign) */
        } dw8;
    };

    /* DW10 */
    union {
        u32 mr_mkey; /* This parameter is valid for MW. */
        u32 mw_cnt;  /* This parameter is valid when the MR is used. */
    };

    /* DW11 */
    u32 mtt_sz;

    /* DW12~17 */
    u32 userdata[HMM_RDMA_USER_DATA_LENGTH];
} hmm_verbs_mr_attr_s;

typedef struct tag_hmm_verbs_mtt_cacheout_info {
    u32 mtt_flags;    /* Indicates whether to kick out cache. by queue (0) or VF(1). */
    u32 mtt_num; /* Number of cmtt, which needs to be assigned by the driver when the is kicked out by queue. */
    u32 mtt_cache_line_start; /* The driver needs to read the driver from the configuration file. */
    u32 mtt_cache_line_end;   /* The driver needs to read the driver from the configuration file. */
    u32 mtt_cache_line_size;  /* 0:256B,1:512B */
} hmm_verbs_mtt_cacheout_info_s;
#pragma pack()

typedef struct tag_hmm_uni_cmd_mpt_hw2sw {
    hmm_verbs_cmd_header_s com;
    hmm_verbs_mtt_cacheout_info_s dmtt_cache;
} hmm_uni_cmd_mpt_hw2sw_s;

typedef struct tag_hmm_uni_cmd_sw2hw_mpt {
    hmm_verbs_cmd_header_s com;
    hmm_verbs_mr_attr_s mr_attr;
} hmm_uni_cmd_mpt_sw2hw_s;

#define HMM_CMDQ_CREAT_MR_LOAD_PERMISSION_SPF global_spram_f11
#define HMM_CMDQ_CREAT_MR_LOAD_PERMISSION_SPF_ID 11

#define HMM_CMDQ_CREAT_MR_LOAD_TID_FE_SPF global_spram_f6
#define HMM_CMDQ_CREAT_MR_LOAD_TID_FE_SPF_ID 6

typedef struct {
    u16 rsvd0 : 1;
    u16 ubc_port_id : 3;    /* 当前port_id，用于选取oq_id, 并不要求和端口号严格对应 */
    u16 david_ue_id : 12;   /* 分配给当前David的逻辑上第一个UBC port口的fe */
} david_port_ue_s;

/**
 * @brief struct mptc_david - david fe信息
 * @details david直通场景需要mptc携带david fe等相关信息
 */
typedef struct mptc_david {
    /* DW0 ~ DW1 */
    david_port_ue_s david_port_ue[DAVID_PATH_MAX_NUM];

    struct {
        u32 david_en : 1;        /* 标识当前func是否具有david直通能力，使能后可以不走david */
        u32 at_flag : 1;         /* 用于记录使用TID0还是使用TID1 */
        u32 rsvd4 : 2;
        u32 active_port_num : 4; /* 当前有效的UBC Port平面个数 */
        u32 rsvd5 : 2;
        u32 david_id : 6;        /* 模板表中的david id，表示当前David在PoD内统一编号，用于对不同david做OQ限速 */
        u32 rsvd6 : 16;
    } dw2;
} mptc_david_s;

/**
 * @brief hmm cmdq流程创建mptc时编辑mptc内容
 * @param mpt_addr: mptc的地址
 * @details 此接口位置在初始化mptc的cmdq流程中，可重载
 * @return 0: 无错误；其他：错误
 */
u32 hmm_creat_mpt_ext_edit_mptc(u8 *mpt_addr);

/**
 * @brief 获取创建mr cmdq流程中vf id
 * @param void
 * @attention void
 * @return 返回vf id
 */
u16 hmm_cmdq_mr_vf_id_get(void);

/**
 * @brief 获取创建mr cmdq流程中mr_attr的user_data
 * @param user_data_len 出参，返回user_data_len大小
 * @attention void
 * @return 返回user_data地址指针
 */
u8 *hmm_cmdq_mr_user_data_get(u8 *user_data_len);

/**
 * @brief 在hmm创建mr cmdq流程中设置mr为pa模式
 * @param mpt_addr: mptc的地址
 * @attention 默认为非pa模式，当不需要做va到pa转换时使能pa模式
 * @return void
 */
void hmm_cmdq_mr_pa_mode_set(u8 *mpt_addr);

/**
 * @brief 在hmm创建mr cmdq流程中设置mptc中david域段
 * @param mpt_addr: mptc的地址；mptc_david: mptc中david域段内容
 * @attention void
 * @return void
 */
void hmm_cmdq_creat_mr_mptc_david_set(u8 *mpt_addr, mptc_david_s *mptc_david);

#endif /* HMM_CMD_DEFS_H */