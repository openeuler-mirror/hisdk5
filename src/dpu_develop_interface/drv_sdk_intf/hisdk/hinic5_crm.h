/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef HINIC5_CRM_H
#define HINIC5_CRM_H

#include <asm-generic/int-ll64.h>
#include <linux/types.h>
#include <linux/time.h>
#include <linux/spinlock.h>

#include "mag_mpu_cmd_defs.h"

#define HINIC5_DRV_VERSION GLOBAL_VERSION_STR   /**< 用于存储驱动版本信息 */
#define HINIC5_DRV_DESC "Intelligent Network Interface Card Driver"     /**< 用于表示驱动的描述信息 */

#define ARRAY_LEN(arr) ((int)((int)sizeof(arr) / (int)sizeof((arr)[0])))    /**< 计算数组长度的宏 */

#define HINIC5_MGMT_VERSION_MAX_LEN 32      /**< 定义HINIC5管理版本最大长度 */

#define HINIC5_FW_VERSION_NAME 16       /**< 定义固件版本名称的长度 */

#define HISDK5_DCB_UP_MAX          0x8 /**< 定义网络接口控制数据包(DCB)的最大用户优先级(UP) */
/**
 * @brief struct hinic5_fw_version
 * @details 定义了一个结构体，用于存储固件的版本信息
 */
struct hinic5_fw_version {
    u8 mgmt_ver[HINIC5_FW_VERSION_NAME];
    u8 microcode_ver[HINIC5_FW_VERSION_NAME];
    u8 boot_ver[HINIC5_FW_VERSION_NAME];
};
#define HINIC5_MGMT_CMD_UNSUPPORTED 0xFF    /**< 表示管理命令不支持的情况 */

/**
 * @brief enum hinic5_service_type
 * @details 定义了服务类型枚举,
 *          show each drivers only such as nic_service_cap,
 *          toe_service_cap structure, but not show service_cap
 */
enum hinic5_service_type {
    SERVICE_T_NIC       = 0,
    SERVICE_T_OVS       = 1,
    SERVICE_T_ROCE      = 2,
    SERVICE_T_TOE       = 3,
    SERVICE_T_IOE       = 4,
    SERVICE_T_FC        = 5,
    SERVICE_T_VBS       = 6,
    SERVICE_T_IPSEC     = 7,
    SERVICE_T_VIRTIO    = 8,
    SERVICE_T_MIGRATE   = 9,
    SERVICE_T_PPA       = 10,
    SERVICE_T_CUSTOM    = 11,
    SERVICE_T_VROCE     = 12,
    SERVICE_T_UB        = 13,
    SERVICE_T_JBOF      = 14,
    SERVICE_T_MACSEC    = 15,
    SERVICE_T_DMMU      = 16,
    SERVICE_T_CFM       = 17,
    SERVICE_T_BIFUR     = 18,
    SERVICE_T_HIHTR     = 19,
    SERVICE_T_MAX       = 20,

    /* Only used for interruption resource management,
     * mark the request module
     */
    SERVICE_T_INTF      = (1 << 15),
    SERVICE_T_CQM       = (1 << 16),
};

/**
 * @brief enum hinic5_ppf_flr_type
 * @details 定义hinic5_ppf_flr_type枚举类型
 */
enum hinic5_ppf_flr_type {
    STATELESS_FLR_TYPE,     /**< 无状态FLR类型 */
    STATEFUL_FLR_TYPE,      /**< 有状态FLR类型 */
};

/**
 * @brief struct nic_service_cap
 * @details 网络接口服务能力结构体
 */
struct nic_service_cap {
    u16 max_sqs;    /**< 最大发送队列数量 */
    u16 max_rqs;    /**< 最大接收队列数量 */
    u16 default_num_queues;     /**< 默认队列数量 */
};

/**
 * @brief struct ppa_service_cap
 * @details 定义了ppa服务的能力结构体
 */
struct ppa_service_cap {
    u16 qpc_fake_vf_start;      /**< 起始的虚拟函数号 */
    u16 qpc_fake_vf_num;        /**<可用的虚拟函数数量  */
    u32 qpc_fake_vf_ctx_num;    /**< 可用的虚拟函数上下文数量 */
    u32 pctx_sz;                /**< 上下文大小512B */
    u32 bloomfilter_length;     /**< 布隆过滤器长度 */
    u8 bloomfilter_en;          /**< 布隆过滤器使能标志 */
    u8 rsvd;
    u16 rsvd1;
};

/**
 * @brief struct vbs_service_cap
 * @details 描述VBS服务的能力的结构体
 */
struct vbs_service_cap {
	u16 vbs_max_volq;                       /**< 最大的卷积队列数量 */
    u16 vbs_main_pf_enable : 1;             /**< 主PF是否启用 */
    u16 vbs_vsock_pf_enable : 1;            /**< vsock PF是否启用 */
    u16 vbs_fushion_queue_pf_enable : 1;    /**< 融合队列PF是否启用 */
    u16 vbs_host_dma_data_cos : 3;          /**< 主机DMA数据的优先级 */
    u16 vbs_vmio_cpy_data_cos : 3;          /**< VMIO复制数据的优先级 */
    u16 vbs_volq_cos : 3;                   /**< 卷积队列的优先级 */
    u16 rsvd1 : 4;
    u32 vbs_child_ctx_num;
    u32 vbs_hash_bucket_num;
};

/**
 * @brief struct migr_service_cap
 * @details 迁移服务能力结构体
 */
struct migr_service_cap {
	u8 master_host_id;      /**< 主机ID */
	u8 rsvd[3];
};

/**
 * @brief struct ub_dev_cap_sdk_res
 * @details sdk中申请资源相关信息
 */
struct ub_dev_cap_sdk_res {
    u32 max_jfc;        /**< 最大Job Function Controller数量 */
    u32 max_jfr;        /**< 最大Job Function Record数量 */
    u32 max_tp;         /**< 最大Transmission Port数量 */
    u32 max_tpg;        /**< 最大Transmission Port Group数量 */
    u32 max_jetty;      /**< 最大Jetty数量 */
    u32 max_jetty_grp;  /**< 最大Jetty Group数量 */
    u32 max_mpts;       /**< 最大Multi-Path Termination Point数量 */
    u32 max_vtp;        /**< 最大Virtual Transmission Port数量 */
    u32 max_gid;        /**< 最大GID数量 */
    u32 max_utp;        /**< 最大Unicast Transmission Port数量 */
    u32 max_jfrc;       /**< 最大Job Function Record Cache数量 */

    u32 srqc_entry_sz;  /**< Shared Receive Queue Controller条目大小 */
    u32 mpt_entry_sz;   /**< Multi-Path Termination Point条目大小 */
    u32 cqc_entry_sz;   /**< Completion Queue Controller条目大小 */
    u32 qpc_entry_sz;   /**< Queue Pair Context条目大小 */

    u32 dmtt_cl_start;  /**< Data Move To Target开始地址 */
    u32 dmtt_cl_end;    /**< Data Move To Target结束地址 */
    u32 dmtt_cl_sz;     /**< Data Move To Target大小 */

    u32 cmtt_cl_start;  /**< Control Move To Target开始地址 */
    u32 cmtt_cl_end;    /**< Control Move To Target结束地址 */
    u32 cmtt_cl_sz;     /**< Control Move To Target大小 */

    u32 wqe_cl_start;   /**< Work Request Element开始地址 */
    u32 wqe_cl_end;     /**< Work Request Element结束地址 */
    u32 wqe_cl_sz;      /**< Work Request Element大小 */
};

/**
 * @brief struct ub_net_dev_cap
 * @details 网络设备能力结构体
 */
struct ub_net_dev_cap {
    u32 is_tpf;             /**< 是否支持透明转发 */
    u32 vf_cnt;             /**< 虚拟函数数量 */
    u32 port_cnt;           /**< 端口数量 */
    u32 max_mtu;            /**< 最大传输单元大小 */
    u32 comp_vector_cnt;    /**< 中断向量数量 */
};

/**
 * @brief struct ub_service_cap
 * @details 定义了一个服务能力结构体
 */
struct ub_service_cap {
    struct ub_dev_cap_sdk_res sdk_res;      /**< 设备能力SDK响应 */
    struct ub_net_dev_cap net_dev_cap;      /**< 网络设备能力 */
};

/**
 * @brief struct jbof_service_cap
 * @details 定义了一个结构体，用于描述JBOF服务的能力信息
 */
struct jbof_service_cap {
    u32 max_parent_qpc_num;     /**< 最大父QPC数量 */
    u32 max_child_qpc_num;      /**< 最大子QPC数量 */
    u32 parent_qpc_size;        /**< 父QPC大小 */
    u32 child_qpc_size;         /**< 子QPC大小 */
    u32 hash_bucket_num;        /**< 哈希桶数量 */
};

/**
 * @brief struct dmmu_service_cap
 * @details 定义了一个结构体，用于描述dmmu服务的能力信息
 */
struct dmmu_service_cap {
    u32 pasid_min;
    u32 pasid_max;
    u32 cl_start;
    u32 cl_end;
};

/**
 * @brief CFM(Common Function Module) service capability
 */
struct cfm_service_cap {
    /* CCP - Congestion Control Platform */
    u32 ccp_max_child_ctx;
    u16 ccp_child_ctx_sz;
    u16 rsvd1;

    u64 rsvd[0xF];
};

/**
 * @brief struct dev_toe_svc_cap
 * @details PF/VF ToE service resource structure
 */
struct dev_toe_svc_cap {
    /* PF resources */
    u32 max_pctxs; /**< Parent Context: max specifications 1M */
    u32 max_cctxt;
    u32 max_cqs;
    u16 max_srqs;
    u32 srq_id_start;
    u32 max_mpts;
};

/**
 * @brief struct toe_service_cap
 * @details ToE services
 */
struct toe_service_cap {
    struct dev_toe_svc_cap dev_toe_cap;

    bool alloc_flag;
    u32 pctx_sz; /**< 1KB */
    u32 scqc_sz; /**< 64B */
};

/**
 * @brief struct dev_fc_svc_cap
 * @details PF FC service resource structure defined
 */
struct dev_fc_svc_cap {
    /* PF Parent QPC */
    u32 max_parent_qpc_num; /**< max number is 2048 */

    /* PF Child QPC */
    u32 max_child_qpc_num; /**< max number is 2048 */
    u32 child_qpc_id_start;

    /* PF SCQ */
    u32 scq_num; /**< 16 */

    /* PF supports SRQ */
    u32 srq_num; /**< Number of SRQ is 2 */

    u8 vp_id_start;
    u8 vp_id_end;
};

/**
 * @brief struct fc_service_cap
 * @details FC services
 */
struct fc_service_cap {
    struct dev_fc_svc_cap dev_fc_cap;

    /* Parent QPC */
    u32 parent_qpc_size; /**< 256B */

    /* Child QPC */
    u32 child_qpc_size; /**< 256B */

    /* SQ */
    u32 sqe_size; /**< 128B(in linked list mode) */

    /* SCQ */
    u32 scqc_size; /**< Size of the Context 32B */
    u32 scqe_size; /**< 64B */

    /* SRQ */
    u32 srqc_size; /**< Size of SRQ Context (64B) */
    u32 srqe_size; /**< 32B */
};

struct dev_roce_svc_own_cap {
    u32 max_qps;
    u32 max_cqs;
    u32 max_srqs;
    u32 max_mpts;
    u32 max_drc_qps;

    u32 reserved_qps;           /**< roce_rsvd_qp */
    u32 reserved_qps_back;      /**< roce_rsvd_qp_back */
    u32 reserved_cqs;           /**< roce_rsvd_cq */
    u32 reserved_cqs_back;      /**< roce_rsvd_cq_back */
    u32 reserved_srqs;          /**< roce_rsvd_srq */
    u32 reserved_srqs_back;     /**< roce_rsvd_srq_back */
    u32 max_pd;                 /**< roce_max_pd */
    u32 max_xrcd;               /**< roce_max_xrcd */
    u32 max_gid;                /**< roce_max_gid */

    u32 cmtt_cl_start;
    u32 cmtt_cl_end;
    u32 cmtt_cl_sz;

    u32 dmtt_cl_start;
    u32 dmtt_cl_end;
    u32 dmtt_cl_sz;

    u32 wqe_cl_start;
    u32 wqe_cl_end;
    u32 wqe_cl_sz;

    u32 qpc_entry_sz;
    u32 max_wqes;
    u32 max_rq_sg;
    u32 max_sq_inline_data_sz;
    u32 max_rq_desc_sz;

    u32 rdmarc_entry_sz;
    u32 max_qp_init_rdma;
    u32 max_qp_dest_rdma;

    u32 max_srq_wqes;
    u32 max_srq_sge;
    u32 srqc_entry_sz;

    u32 max_msg_sz; /**< Message size 2GB */
    u32 max_child_ctx_num;
};

/**
 * @brief struct dev_rdma_svc_cap
 * @details RDMA service capability structure
 */
struct dev_rdma_svc_cap {
    struct dev_roce_svc_own_cap roce_own_cap;   /**< ROCE service unique parameter structure */
};

/**
 * @brief enum
 * @details Defines the RDMA service capability flag
 */
enum {
    RDMA_BMME_FLAG_LOCAL_INV = (1 << 0),
    RDMA_BMME_FLAG_REMOTE_INV = (1 << 1),
    RDMA_BMME_FLAG_FAST_REG_WR = (1 << 2),
    RDMA_BMME_FLAG_RESERVED_LKEY = (1 << 3),
    RDMA_BMME_FLAG_TYPE_2_WIN = (1 << 4),
    RDMA_BMME_FLAG_WIN_TYPE_2B = (1 << 5),

    RDMA_DEV_CAP_FLAG_XRC = (1 << 6),
    RDMA_DEV_CAP_FLAG_MEM_WINDOW = (1 << 7),
    RDMA_DEV_CAP_FLAG_ATOMIC = (1 << 8),
    RDMA_DEV_CAP_FLAG_APM = (1 << 9),
};

/**
 * @brief struct rdma_service_cap
 * @details RDMA services
 */
struct rdma_service_cap {
    struct dev_rdma_svc_cap dev_rdma_cap;

    u8 log_mtt; /**< 1. the number of MTT PA must be integer power of 2
                 * 2. represented by logarithm. Each MTT table can
                 * contain 1, 2, 4, 8, and 16 PA)
                 */

    u32 num_mtts; /* Number of MTT table (4M),
                   * is actually MTT seg number
                   */
    u32 log_mtt_seg;
    u32 mtt_entry_sz; /**< MTT table size 8B, including 1 PA(64bits) */
    u32 mpt_entry_sz; /**< MPT table size (64B) */

    u32 dmtt_cl_start;
    u32 dmtt_cl_end;
    u32 dmtt_cl_sz;

    u8 log_rdmarc; /**< 1. the number of RDMArc PA must be integer power of 2
                    * 2. represented by logarithm. Each MTT table can
                    * contain 1, 2, 4, 8, and 16 PA)
                    */

    u32 reserved_qps;   /**< Number of reserved QP */
    u32 max_sq_sg;      /**< Maximum SGE number of SQ (8) */
    u32 max_sq_desc_sz; /**< WQE maximum size of SQ(1024B), inline maximum
                         * size if 960B(944B aligned to the 960B),
                         * 960B=>wqebb alignment=>1024B
                         */
    u32 wqebb_size;     /**< Currently, the supports 64B and 128B,
                         * defined as 64Bytes
                         */

    u32 max_cqes;     /**< Size of the depth of the CQ (64K-1) */
    u32 reserved_cqs; /**< Number of reserved CQ */
    u32 cqc_entry_sz; /**< Size of the CQC (64B/128B) */
    u32 cqe_size;     /**< Size of CQE (32B) */

    u32 reserved_mrws; /**< Number of reserved MR/MR Window */

    u32 max_fmr_maps; /**< max MAP of FMR,
                       * (1 << (32-ilog2(num_mpt)))-1;
                       */

    u32 log_rdmarc_seg; /**< table number of each RDMArc seg(3) */

    /* Timeout time. Formula:Tr=4.096us*2(local_ca_ack_delay), [Tr,4Tr] */
    u32 local_ca_ack_delay;
    u32 num_ports; /**< Physical port number */

    u32 db_page_size;    /**< Size of the DB (4KB) */
    u32 direct_wqe_size; /**< Size of the DWQE (256B) */

    u32 num_pds;        /**< Maximum number of PD (128K) */
    u32 reserved_pds;   /**< Number of reserved PD */
    u32 max_xrcds;      /**< Maximum number of xrcd (64K) */
    u32 reserved_xrcds; /**< Number of reserved xrcd */

    u32 max_gid_per_port; /**< gid number (16) of each port */
    u32 gid_entry_sz;     /**< RoCE v2 GID table is 32B,
                           * compatible RoCE v1 expansion
                           */

    u32 reserved_lkey;    /**< local_dma_lkey */
    u32 num_comp_vectors; /**< Number of complete vector (32) */
    u32 page_size_cap;    /**< Supports 4K,8K,64K,256K,1M and 4M page_size */

    u32 flags;        /**< RDMA some identity */
    u32 max_frpl_len; /**< Maximum number of pages frmr registration */
    u32 max_pkeys;    /**< Number of supported pkey group */
};

/**
 * @brief struct dev_ovs_svc_cap
 * @details PF OVS service resource structure defined
 */
struct dev_ovs_svc_cap {
    u32 max_pctxs; /**< Parent Context: max specifications 1M */
    u32 fake_vf_max_pctx;
    u16 fake_vf_num;
    u16 fake_vf_start_id;
    u8 dynamic_qp_en;
};

/**
 * @brief struct ovs_service_cap
 * @details OVS services
 */
struct ovs_service_cap {
    struct dev_ovs_svc_cap dev_ovs_cap;

    u32 pctx_sz; /**< 512B */
};

/**
 * @brief struct dev_ipsec_svc_cap
 * @details PF IPsec service resource structure defined
 */
struct dev_ipsec_svc_cap {
    u32 max_sactxs; /**< max IPsec SA context num */
    u16 max_cqs;    /**< max IPsec SCQC num */
    u16 rsvd0;
    u32 max_spctxs; /**< max IPsec SP context num */
    u32 sa_hash_bucket_num;
    u32 sp_hash_bucket_num;
};

/**
 * @brief struct ipsec_service_cap
 * @details IPsec services
 */
struct ipsec_service_cap {
    struct dev_ipsec_svc_cap dev_ipsec_cap;
    u32 sactx_sz; /**< 512B */
};

struct hisdk5_dcb_state {
	u8 dcb_on;  /**< dcb on or off */
	u8 default_cos;  /**< Default COS值, 合法范围0~7 */
	u8 trust;   /**< trust状态, 0-PCP模式, 1-DSCP模式 */
	u8 rsvd1;   /**< reserved */
	u8 pcp2cos[HISDK5_DCB_UP_MAX];  /**< PCP到COS的映射 */
	u8 dscp2cos[64];    /**< DSCP到COS的映射值 */
	u32 rsvd2[7];   /**< reserved */
};

enum hisdk5_dcb_state_op {
    HISDK5_DCB_STATE_GET,
    HISDK5_DCB_STATE_SET,
};

/**
 * @brief struct irq_info
 * @details Defines the IRQ information structure
 */
struct irq_info {
    u16 msix_entry_idx; /**< IRQ corresponding index number */
    u32 irq_id;         /**< the IRQ number from OS */
};

/**
 * @brief struct interrupt_info
 * @details 中断信息结构体
 */
struct interrupt_info {
    u32 lli_set;   /**< low latency interrupt enable */
    u32 interrupt_coalesc_set; /**< interrupt coalesce enable */
    u16 msix_index;   /**< msix index */
    u8 lli_credit_limit;  /**< low latency interrupt credit value */
    u8 lli_timer_cfg;    /**< low latency interrupt timer value */
    u8 pending_limt;
    u8 coalesc_timer_cfg;  /**< interrupt coalesce timer value*/
    u8 resend_timer_cfg;  /**< interrupt resend timer value*/
};

/**
 * @brief enum hinic5_msix_state
 * @details 定义中断消息传递（MSI-X）的状态
 */
enum hinic5_msix_state {
    HINIC5_MSIX_ENABLE,     /**< 启用MSI-X */
    HINIC5_MSIX_DISABLE,    /**< 禁用MSI-X */
};

/**
 * @brief enum hinic5_msix_auto_mask
 * @details 定义中断处理的枚举类型
 */
enum hinic5_msix_auto_mask {
    HINIC5_CLR_MSIX_AUTO_MASK,  /**< 清除MSIX自动掩码 */
    HINIC5_SET_MSIX_AUTO_MASK,  /**< 设置MSIX自动掩码 */
};

/**
 * @brief enum func_type
 * @details 表示函数的类型
 */
enum func_type {
    TYPE_PF,
    TYPE_VF,
    TYPE_PPF,
    TYPE_UNKNOWN,
};

/**
 * @brief struct hinic5_init_para
 * @details 用于初始化hinic5的参数结构体
 */
struct hinic5_init_para {
    /* Record hinic_pcidev or NDIS_Adapter pointer address */
    void *adapter_hdl;      /**< 记录hinic_pcidev或NDIS_Adapter指针地址 */
#ifdef __UEFI__
    /**
     * Record pcidev or Handler pointer address
     * for example: ioremap interface input parameter
     */
    void *busdev_hdl;       /**< 记录pcidev或ub dev指针地址 */
#endif
    /**
     * Record pcidev->dev or Handler pointer address which used to
     * dma address application or dev_err print the parameter
     */
    void *dev_hdl;          /**< 记录pcidev->dev或Handler指针地址，用于DMA地址应用或dev_err打印参数 */

    void *fers2_reg_base;   /**< FERS2 register base address */
    /* Configure virtual address, PF is bar1, VF is bar0/1 */
    void *cfg_reg_base;     /**< 配置虚拟地址，PF是bar1，VF是bar0/1 */
    /* interrupt configuration register address,  PF is bar2, VF is bar2/3 */
    void *intr_reg_base;    /**< 中断配置寄存器地址，PF是bar2，VF是bar2/3 */
    /* for PF bar3 virtual address, if function is VF should set to NULL */
    void *mgmt_reg_base;    /**< 对于PF的bar3虚拟地址，如果函数是VF，应设置为NULL */

    u64 db_dwqe_len;        /**< 用于记录doorbell和direct wqe的长度 */
    u64 db_base_phy;        /**< doorbell的基础物理地址 */
    /* the doorbell address, bar4/5 higher 4M space */
    void *db_base;          /**< doorbell地址，bar4/5高4M空间 */
    /* direct wqe 4M, follow the doorbell address space */
    void *dwqe_mapping;     /**< 直接wqe 4M，跟随doorbell地址空间 */
    void **hwdev;           /**< 硬件设备指针 */
    void *chip_node;        /**< 芯片节点指针 */
    /* if use polling mode, set it true */
    bool poll;              /**< 如果使用轮询模式，设置为true */

    u16 probe_fault_level;  /**< 探测故障级别 */
};

#define HINIC5_DB_DWQE_SIZE 0x100000000     /**< Maximum support size of BAR45, DB & DWQE are both half */

#define HINIC5_DB_PAGE_SIZE 0x00001000ULL   /**< db page size: 4K */
#define HINIC5_DWQE_OFFSET 0x00000800ULL    /**< dwqe page size: 4K */

/**
 * @brief 定义一个宏，用于计算最大的数据库区域数量
 * @param HINIC5_DB_MAX_AREAS 表示最大的数据库区域数量
 * @param HINIC5_DB_DWQE_SIZE 表示每个数据库区域的大小
 * @param HINIC5_DB_PAGE_SIZE 表示每个数据库页的大小
 */
#define HINIC5_DB_MAX_AREAS (HINIC5_DB_DWQE_SIZE / HINIC5_DB_PAGE_SIZE)

#ifndef IFNAMSIZ
#define IFNAMSIZ 16
#endif
#define MAX_FUNCTION_NUM 4096

#define HINIC5_SYNFW_TIME_PERIOD (60 * 60 * 1000)   /**< 表示同步固件的时间周期 */

#define FAULT_SHOW_STR_LEN 16

/**
 * @brief enum hinic5_fault_source_type
 * @details 错误源类型枚举
 */
enum hinic5_fault_source_type {
    /* same as FAULT_TYPE_CHIP */
    HINIC5_FAULT_SRC_HW_MGMT_CHIP = 0,          /**< 硬件管理芯片错误 */
    /* same as FAULT_TYPE_UCODE */
    HINIC5_FAULT_SRC_HW_MGMT_UCODE,             /**< 硬件管理微码错误 */
    /* same as FAULT_TYPE_MEM_RD_TIMEOUT */
    HINIC5_FAULT_SRC_HW_MGMT_MEM_RD_TIMEOUT,    /**< 硬件管理内存读取超时错误 */
    /* same as FAULT_TYPE_MEM_WR_TIMEOUT */
    HINIC5_FAULT_SRC_HW_MGMT_MEM_WR_TIMEOUT,    /**< 硬件管理内存写入超时错误 */
    /* same as FAULT_TYPE_REG_RD_TIMEOUT */
    HINIC5_FAULT_SRC_HW_MGMT_REG_RD_TIMEOUT,    /**< 硬件管理寄存器读取超时错误 */
    /* same as FAULT_TYPE_REG_WR_TIMEOUT */
    HINIC5_FAULT_SRC_HW_MGMT_REG_WR_TIMEOUT,    /**< 硬件管理寄存器写入超时错误 */
    HINIC5_FAULT_SRC_SW_MGMT_UCODE,             /**< 软件管理微码错误 */
    HINIC5_FAULT_SRC_MGMT_WATCHDOG,             /**< 管理看门狗错误 */
    HINIC5_FAULT_SRC_MGMT_RESET = 8,            /**< 管理重置错误 */
    HINIC5_FAULT_SRC_HW_PHY_FAULT,              /**< 硬件PHY错误 */
    HINIC5_FAULT_SRC_TX_PAUSE_EXCP,             /**< 发送暂停异常错误 */
    HINIC5_FAULT_SRC_PCIE_LINK_DOWN = 20,       /**< PCIE链路断开错误 */
    HINIC5_FAULT_SRC_HOST_HEARTBEAT_LOST = 21,  /**< 主机心跳丢失错误 */
    HINIC5_FAULT_SRC_TX_TIMEOUT,                /**< 发送超时错误 */
    HINIC5_FAULT_SRC_TYPE_MAX,                  /**< 错误源类型最大值 */
};

/**
 * @brief union hinic5_fault_hw_mgmt
 * @details 用于处理硬件管理中的错误信息
 */
union hinic5_fault_hw_mgmt {
    u32 val[4];
    /* valid only type == FAULT_TYPE_CHIP */
    struct {
        u8 node_id;
        u8 err_level; /**< enum hinic_fault_err_level */
        u16 err_type;
        u32 err_csr_addr;
        u32 err_csr_value;
        /* func_id valid only if err_level == FAULT_LEVEL_SERIOUS_FLR */
        u8 rsvd1;
        u8 host_id;
        u16 func_id;
    } chip;

    /* valid only if type == FAULT_TYPE_UCODE */
    struct {
        u8 cause_id;
        u8 core_id;
        u8 c_id;
        u8 rsvd3;
        u32 epc;
        u32 rsvd4;
        u32 rsvd5;
    } ucode;

    /* valid only if type == FAULT_TYPE_MEM_RD_TIMEOUT ||
     * FAULT_TYPE_MEM_WR_TIMEOUT
     */
    struct {
        u32 err_csr_ctrl;
        u32 err_csr_data;
        u32 ctrl_tab;
        u32 mem_index;
    } mem_timeout;

    /* valid only if type == FAULT_TYPE_REG_RD_TIMEOUT ||
     * FAULT_TYPE_REG_WR_TIMEOUT
     */
    struct {
        u32 err_csr;
        u32 rsvd6;
        u32 rsvd7;
        u32 rsvd8;
    } reg_timeout;

    struct {
        /* 0: read; 1: write */
        u8 op_type;
        u8 port_id;
        u8 dev_ad;
        u8 rsvd9;
        u32 csr_addr;
        u32 op_data;
        u32 rsvd10;
    } phy_fault;
};

/* defined by chip */
/**
 * @brief struct hinic5_fault_event
 * @details 用于表示HINIC5的故障事件
 */
struct hinic5_fault_event {
    /* enum hinic_fault_type */
    u8 type;            /**< 故障类型 */
    u8 fault_level;     /**< SDK写入故障级别，用于ULDOVENT */
    u8 rsvd0[2];
    union hinic5_fault_hw_mgmt event;       /**< 故障硬件管理事件 */
};

/**
 * @brief struct hinic5_cmd_fault_event
 * @details 定义了一个用于存储故障事件的结构体
 */
struct hinic5_cmd_fault_event {
    u8 status;      /**< 状态字段 */
    u8 version;     /**< 版本字段 */
    u8 rsvd0[6];
    struct hinic5_fault_event event;    /**< 故障事件结构体 */
};

/**
 * @brief struct hinic5_sriov_state_info
 * @details 定义一个结构体，用于存储SR-IOV的状态信息
 */
struct hinic5_sriov_state_info {
    u8 enable;      /**< 表示SR-IOV是否启用 */
    u16 num_vfs;    /**< 表示虚拟函数的数量 */
    u32 vf_id;      /**< 待操作的func id */
};

/**
 * @brief enum hinic5_comm_event_type
 * @details 通信事件类型枚举
 */
enum hinic5_comm_event_type {
    EVENT_COMM_PCIE_LINK_DOWN,      /**< PCIE链路断开事件 */
    EVENT_COMM_HEART_LOST,          /**< 心跳丢失事件 */
    EVENT_COMM_FAULT,               /**< 设备故障事件 */
    EVENT_COMM_SRIOV_STATE_CHANGE,  /**< SR-IOV状态变化事件 */
    EVENT_COMM_CARD_REMOVE,         /**< 设备移除事件 */
    EVENT_COMM_MGMT_WATCHDOG,       /**< 管理监视器看门狗事件 */
};

/**
 * @brief enum hinic5_event_service_type
 * @details 定义了事件服务类型的枚举
 */
enum hinic5_event_service_type {
    EVENT_SRV_COMM = 0,     /**< 通用事件服务类型 */
#define SERVICE_EVENT_BASE (EVENT_SRV_COMM + 1)     /**< 定义服务事件基础值 */
    EVENT_SRV_NIC = SERVICE_EVENT_BASE + SERVICE_T_NIC,         /**< 网络接口卡事件服务类型 */
    EVENT_SRV_MIGRATE = SERVICE_EVENT_BASE + SERVICE_T_MIGRATE, /**< 迁移事件服务类型 */
};

/**
 * @brief 定义一个宏，用于生成服务事件类型的编码
 * @param svc 服务类型，取值范围为0-65535
 * @param type 事件类型，取值范围为0-65535
 *
 * @details 这个宏将服务类型和事件类型的值合并为一个32位的无符号整数，
 *          其中服务类型占用高16位，事件类型占用低16位。
 *
 * @return 返回生成的事件类型编码
 */
#define HINIC5_SRV_EVENT_TYPE(svc, type) ((((u32)(svc)) << 16) | (type))
/**
 * @brief struct hinic5_event_info
 * @details 定义了hinic5事件信息的结构体
 */
struct hinic5_event_info {
    u16 service;    /**< 枚举类型hinic5_event_service_type */
    u16 type;       /**< 事件类型 */
    u8 event_data[104];     /**< 事件数据，最大长度104字节 */
};

/**
 * @brief hinic5_set_msix_auto_mask - set msix auto mask function
 * @param hwdev: device pointer to hwdev
 * @param msix_idx: msix id
 * @param flag: msix auto_mask flag, 1-enable, 2-clear
 */
void hinic5_set_msix_auto_mask_state(void *hwdev, u16 msix_idx,
				     enum hinic5_msix_auto_mask flag);

/**
 * @brief hinic5_set_msix_state - set msix state
 * @param hwdev: device pointer to hwdev
 * @param msix_idx: msix id
 * @param flag: msix state flag, 0-enable, 1-disable
 */
void hinic5_set_msix_state(void *hwdev, u16 msix_idx,
			   enum hinic5_msix_state flag);

/**
 * @brief hinic5_misx_intr_clear_resend_bit - clear msix resend bit
 * @param hwdev: device pointer to hwdev
 * @param msix_idx: msix id
 * @param clear_resend_en: 1-clear
 */
void hinic5_misx_intr_clear_resend_bit(void *hwdev, u16 msix_idx,
				       u8 clear_resend_en);

/**
 * @brief hinic5_set_interrupt_cfg_direct - set interrupt cfg
 * @param hwdev: device pointer to hwdev
 * @param interrupt_para: interrupt info
 * @param channel: channel id
 *
 * @return 是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_set_interrupt_cfg_direct(void *hwdev,
				    struct interrupt_info *info,
				    u16 channel);
/**
 * @brief Set interrupt cfg
 * @param udkdev: device pointer to udkdev
 * @param interrupt_info: Interrupt info
 * @param channel: command message channel id is defined in enum hinic5_channel_id
 *
 * @return 是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_set_interrupt_cfg(void *dev, struct interrupt_info info,
			     u16 channel);

/**
 * @brief hinic5_get_interrupt_cfg - get interrupt cfg
 * @param dev: device pointer to hwdev
 * @param info: interrupt info
 * @param channel: channel id
 *
 * @return 是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_get_interrupt_cfg(void *dev, struct interrupt_info *info,
			     u16 channel);

/**
 * @brief hinic5_alloc_irqs - alloc irq
 * @param hwdev: device pointer to hwdev
 * @param type: service type
 * @param num: alloc number
 * @param irq_info_array: alloc irq info
 * @param act_num: alloc actual number
 *
 * @return 是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_alloc_irqs(void *hwdev, enum hinic5_service_type type, u16 num,
		      struct irq_info *irq_info_array, u16 *act_num);

/**
 * @brief hinic5_free_irq - free irq
 * @param hwdev: device pointer to hwdev
 * @param type: service type
 * @param irq_id: irq id
 */
void hinic5_free_irq(void *hwdev, enum hinic5_service_type type, u32 irq_id);

/**
 * @brief hinic5_alloc_ceqs - alloc ceqs
 * @param hwdev: device pointer to hwdev
 * @param type: service type
 * @param num: alloc ceq number
 * @param ceq_id_array: alloc ceq_id_array
 * @param act_num: alloc actual number
 *
 * @return 是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_alloc_ceqs(void *hwdev, enum hinic5_service_type type, int num,
		      int *ceq_id_array, int *act_num);

/**
 * @brief hinic5_free_irq - free ceq
 * @param hwdev: device pointer to hwdev
 * @param type: service type
 * @param irq_id: ceq id
 */
void hinic5_free_ceq(void *hwdev, enum hinic5_service_type type, int ceq_id);

/**
 * @brief hinic5_ppf_idx - get ppf id
 * @param hwdev: device pointer to hwdev
 *
 * @return ppf id
 */
u8 hinic5_ppf_idx(void *hwdev);

#ifndef __UEFI__
/**
 * @brief hinic5_write_ts_data - write time to hw rtc
 * @param hwdev: device pointer to hwdev
 * @param ts: time to write
 */
void hinic5_write_ts_data(void *hwdev, const struct timespec64 *ts);

/**
 * @brief hinic5_ts_up_en - enable time stamp update
 * @param hwdev: device pointer to hwdev
 * @param flags: update flags
 *
 * @return 是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_ts_up_en(void *hwdev, u32 flags);

/**
 * @brief hinic5_read_ts_data - read hw rtc time
 * @param hwdev: device pointer to hwdev
 * @param ts: time read from hw
 */
void hinic5_read_ts_data(void *hwdev, struct timespec64 *ts);

/**
 * @brief hinic5_set_ptp_inc - set inc val per cycle
 * @param hwdev: device pointer to hwdev
 * @param inc_val: inc val
 */
void hinic5_set_ptp_inc(void *hwdev, u32 inc_val);

/**
 * @brief hinic5_ptp_ts_update - hw rtc time update
 * @param hwdev: device pointer to hwdev
 * @param delta_ns: delta time in ns
 */
void hinic5_ptp_ts_update(void *hwdev, s32 delta_ns);

/**
 * @brief hinic5_get_non_ptp_chip_time - get the chip time
 * @param hwdev: device pointer to hwdev
 * @param chip_time: get chip_time pointer
 *
 * @return get success or fail
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_get_non_ptp_chip_time(void *dev, u64 *chip_time);

/**
 * @brief hinic5_get_non_ptp_time_diff - get the diff of
 * sys time and chip time
 * @param hwdev: device pointer to hwdev
 * @param time_diff: get time_diff pointer
 *
 * @return get success or fail
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_get_non_ptp_time_diff(void *dev, s64 *time_diff);
#endif

/**
 * @brief hinic5_get_chip_present_flag - get chip present flag
 * @param hwdev: device pointer to hwdev
 *
 * @details This state is maintained by driver.
 * The chip will be absent when
 *   - link down
 *   - PCI shutdown
 *   - PCI reset done
 *
 * Re-probe the function to redetect the absent chip.
 *
 * @return 1 - if chip present, 0 - if chip absent
 */
int hinic5_get_chip_present_flag(const void *hwdev);

/**
 * @brief hinic5_get_heartbeat_status - get heartbeat status
 * @param hwdev: device pointer to hwdev
 *
 * @return heartbeat status
 *  @retval 0          normal
 *  @retval 1          heart lost
 *  @retval 0xFFFFFFFF link down
 */
u32 hinic5_get_heartbeat_status(void *hwdev);

/**
 * @brief hinic5_support_nic - function support nic
 * @param hwdev: device pointer to hwdev
 * @param cap: nic service capbility
 *
 * @return 设备是否支持网络接口卡服务
 *      @retval true: function support nic
 *      @retval false: function not support nic
 */
bool hinic5_support_nic(void *hwdev, struct nic_service_cap *cap);

/**
 * @brief hinic5_support_ipsec - function support ipsec
 * @param hwdev: device pointer to hwdev
 * @param cap: ipsec service capbility
 *
 * @return 硬件设备是否支持IPsec服务
 *      @retval true: function support ipsec
 *      @retval false: function not support ipsec
 */
bool hinic5_support_ipsec(void *hwdev, struct ipsec_service_cap *cap);

/**
 * @brief hinic5_support_macsec - function support macsec
 * @param hwdev: device pointer to hwdev
 *
 * @return 硬件设备是否支持MACsec服务
 *      @retval true: function support macsec
 *      @retval false: function not support macsec
 */
bool hinic5_support_macsec(void *hwdev);

/**
 * @brief hinic5_support_roce - function support roce
 * @param hwdev: device pointer to hwdev
 * @param cap: roce service capbility
 *
 * @return 设备是否支持RoCE
 *      @retval true: function support roce
 *      @retval false: function not support roce
 */
bool hinic5_support_roce(void *hwdev, struct rdma_service_cap *cap);

/**
 * @brief hinic5_support_fc - function support fc
 * @param hwdev: device pointer to hwdev
 * @param cap: fc service capbility
 *
 * @return 设备是否支持fc
 *      @retval true: function support fc
 *      @retval false: function not support fc
 */
bool hinic5_support_fc(void *hwdev, struct fc_service_cap *cap);

/**
 * @brief hinic5_support_rdma - function support rdma
 * @param hwdev: device pointer to hwdev
 * @param cap: rdma service capbility
 *
 * @return 设备是否支持rdma
 *      @retval true: function support rdma
 *      @retval false: function not support rdma
 */
bool hinic5_support_rdma(void *hwdev, struct rdma_service_cap *cap);

/**
 * @brief hinic5_is_rdma_en - is rdma enable
 * @param hwdev: device pointer to hwdev
 * @param cap: rdma service capbility
 *
 * @return RDMA服务是否启用
 *      @retval true: rdma is enabled
 *      @retval false: rdma is disabled
 */
bool hinic5_is_rdma_en(void *hwdev, struct rdma_service_cap *cap);

/**
 * @brief hinic5_support_ovs - function support ovs
 * @param hwdev: device pointer to hwdev
 * @param cap: ovs service capbility
 *
 * @return 设备是否支持ovs
 *      @retval true: function support ovs
 *      @retval false: function not support ovs
 */
bool hinic5_support_ovs(void *hwdev, struct ovs_service_cap *cap);

/**
 * @brief hinic5_support_vbs - function support vbs
 * @param hwdev: device pointer to hwdev
 * @param cap: vbs service capbility
 *
 * @return 设备是否支持vbs
 *      @retval true: function support vbs
 *      @retval false: function not support vbs
 */
bool hinic5_support_vbs(void *hwdev, struct vbs_service_cap *cap);

/**
 * @brief hinic5_support_toe - sync time to hardware
 * @param hwdev: device pointer to hwdev
 * @param cap: toe service capbility
 *
 * @return 是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
bool hinic5_support_toe(void *hwdev, struct toe_service_cap *cap);

/**
 * @brief hinic5_support_ppa - function support ppa
 * @param hwdev: device pointer to hwdev
 * @param cap: ppa service capbility
 *
 * @return 是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
bool hinic5_support_ppa(void *hwdev, struct ppa_service_cap *cap);

/**
 * @brief hinic5_support_migr - function support migrate
 * @param hwdev: device pointer to hwdev
 * @param cap: migrate service capbility
 *
 * @return 是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
bool hinic5_support_migr(void *hwdev, struct migr_service_cap *cap);

/**
 * @brief hinic5_support_ub - function support ub
 * @param hwdev: device pointer to hwdev
 * @param cap: ub service capbility
 *
 * @return 是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
bool hinic5_support_ub(void *hwdev, struct ub_service_cap *cap);

/**
 * @brief hinic5_support_jbof - function support jbof
 * @param hwdev: device pointer to hwdev
 * @param cap: jbof service capbility
 *
 * @return 是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
bool hinic5_support_jbof(void *hwdev, struct jbof_service_cap *cap);

/**
 * @brief hinic5_support_vroce - function support vroce
 * @param hwdev: device pointer to hwdev
 * @param cap: roce service capbility
 *
 * @return 函数是否支持vroce
 *      @retval true: function support roce
 *      @retval false: function not support roce
 */
bool hinic5_support_vroce(void *hwdev, struct rdma_service_cap *cap);

/**
 * @brief hinic5_support_dmmu - function support dmmu
 * @param hwdev: device pointer to hwdev
 * @param cap: dmmu service capbility
 * @retval true: function support dmmu
 * @retval false: function not support dmmu
 */
bool hinic5_support_dmmu(void *hwdev, struct dmmu_service_cap *cap);

/**
 * @brief hinic5_support_biufr - function support biufr
 * @param hwdev: device pointer to hwdev
 * @retval true: function support biufr
 * @retval false: function not support biufr
 */
bool hinic5_support_bifur(void *hwdev);

/**
 * @brief hinic5_support_hihtr - function support hihtr
 * @param hwdev: device pointer to hwdev
 * @retval true: function support biufr
 * @retval false: function not support hihtr
 */
bool hinic5_support_hihtr(void *hwdev);

/**
 * @brief hinic5_sync_time - sync time to hardware
 * @param hwdev: device pointer to hwdev
 * @param time: time to sync
 *
 * @return 是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_sync_time(void *hwdev, u64 time);

/**
 * @brief hinic5_disable_mgmt_msg_report - disable mgmt report msg
 * @param hwdev: device pointer to hwdev
 */
void hinic5_disable_mgmt_msg_report(void *hwdev);

/**
 * @brief hinic5_func_for_mgmt - get function service type
 * @param hwdev: device pointer to hwdev
 *
 * @return 函数是否支持管理功能
 *      @retval true: function for mgmt
 *      @retval false: function is not for mgmt
 */
bool hinic5_func_for_mgmt(void *hwdev);

/**
 * @brief hinic5_set_pcie_order_cfg - set pcie order cfg
 * @param handle: device pointer to hwdev
 */
void hinic5_set_pcie_order_cfg(void *handle);

/**
 * @brief hinic5_init_hwdev - call to init hwdev
 * @param para: device pointer to para
 *
 * @return 是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_init_hwdev(struct hinic5_init_para *para);

/**
 * @brief hinic5_free_hwdev - free hwdev
 * @param hwdev: device pointer to hwdev
 */
void hinic5_free_hwdev(void *hwdev);

/**
 * @brief hinic5_detect_hw_present - detect hardware present
 * @param hwdev: device pointer to hwdev
 */
void hinic5_detect_hw_present(void *hwdev);

/**
 * @brief hinic5_record_pcie_error - record pcie error
 * @param hwdev: device pointer to hwdev
 */
void hinic5_record_pcie_error(void *hwdev);

/**
 * @brief hinic5_shutdown_hwdev - shutdown hwdev
 * @param hwdev: device pointer to hwdev
 */
void hinic5_shutdown_hwdev(void *hwdev);

/**
 * @brief hinic5_set_ppf_flr_type - set ppf flr type
 * @param hwdev: device pointer to hwdev
 * @param ppf_flr_type: ppf flr type
 *
 * @return 是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_set_ppf_flr_type(void *hwdev, enum hinic5_ppf_flr_type flr_type);

/* *
 * @brief hinic5_set_ppf_tbl_hotreplace_flag - set os hotreplace flag in ppf function table
 * @param hwdev: device pointer to hwdev
 * @param flag : os hotreplace flag : 0-not in os hotreplace 1-in os hotreplace
 *
 * @return 是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_set_ppf_tbl_hotreplace_flag(void *hwdev, u8 flag);

/**
 * @brief hinic5_get_mgmt_version - get management cpu version
 * @param hwdev: device pointer to hwdev
 * @param mgmt_ver: output management version
 * @param channel: channel id
 *
 * @return 是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_get_mgmt_version(void *hwdev, u8 *mgmt_ver, u8 version_size,
			    u16 channel);

/**
 * @brief hinic5_get_fw_version - get firmware version
 * @param hwdev: device pointer to hwdev
 * @param fw_ver: firmware version
 * @param channel: channel id
 *
 * @return 是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_get_fw_version(void *hwdev, struct hinic5_fw_version *fw_ver,
			  u16 channel);

/**
 * @brief hinic5_global_func_id - get global function id
 * @param hwdev: device pointer to hwdev
 *
 * @return global function id
 */
u16 hinic5_global_func_id(void *hwdev);

/**
 * @brief hinic_ppf_idx - get ppf function id
 * @param hwdev: device pointer to hwdev
 *
 * @return ppf function id
 */
u8 hinic5_ppf_idx(void *hwdev);

/**
 * @brief hinic5_vector_to_eqn - vector to eq id
 * @param hwdev: device pointer to hwdev
 * @param type: service type
 * @param vector: vertor
 *
 * @return eq id
 */
int hinic5_vector_to_eqn(void *hwdev, enum hinic5_service_type type,
			 int vector);

/**
 * @brief hinic5_glb_pf_vf_offset - get vf offset id of pf
 * @param hwdev: device pointer to hwdev
 *
 * @return vf offset id
 */
u16 hinic5_glb_pf_vf_offset(void *hwdev);

/**
 * @brief hinic5_pf_id_of_vf - get pf id of vf
 * @param hwdev: device pointer to hwdev
 *
 * @return pf id
 */
u8 hinic5_pf_id_of_vf(void *hwdev);

/**
 * @brief hinic5_func_type - get function type
 * @param hwdev: device pointer to hwdev
 *
 * @return function type
 */
enum func_type hinic5_func_type(void *hwdev);

/**
 * @brief hinic5_get_stateful_enable - get stateful status
 * @param hwdev: device pointer to hwdev
 *
 * @return stateful enabel status
 */
bool hinic5_get_stateful_enable(void *hwdev);

/**
 * @brief hinic5_host_oq_id_mask - get oq id
 * @param hwdev: device pointer to hwdev
 *
 * @return oq id
 */
u8 hinic5_host_oq_id_mask(void *hwdev);

/**
 * @brief hinic5_host_id - get host id
 * @param hwdev: device pointer to hwdev
 *
 * @return host id
 */
u8 hinic5_host_id(void *hwdev);

/**
 * @brief hinic5_in_spu - if in spu
 * @param hwdev: device pointer to hwdev
 *
 * @return if in spu
 */
bool hinic5_in_spu(void *hwdev);

/**
 * @brief hinic5_func_max_qnum - get host total function number
 * @param hwdev: device pointer to hwdev
 *
 * @return 计算或获取到的功能总数
 *      @retval non-zero: host total function number
 *      @retval zero: failure
 */
u16 hinic5_host_total_func(void *hwdev);

/**
 * @brief hinic5_func_max_qnum - get max nic queue number
 * @param hwdev: device pointer to hwdev
 *
 * @return 网络设备的最大队列数量
 *      @retval non-zero: max nic queue number
 *      @retval zero: failure
 */
u16 hinic5_func_max_nic_qnum(void *hwdev);

/**
 * @brief 获取函数cos掩码模式
 * @param hwdev 硬件设备指针
 *
 * @return u8 返回cos掩码模式
 */
u8 hinic5_func_cos_mask_mode(void *hwdev);

/**
 * @brief 获取函数默认的cos值
 * @param hwdev 硬件设备指针
 *
 * @return 返回cos值
 */
u8 hinic5_func_dev_default_cos(void *hwdev);

/**
 * @brief hinic5_func_max_qnum - get max queue number
 * @param hwdev: device pointer to hwdev
 *
 * @return 获取设备的最大队列数
 *      @retval non-zero: max queue number
 *      @retval zero: failure
 */
u16 hinic5_func_max_qnum(void *hwdev);

/**
 * @brief hinic5_er_id - get ep id
 * @param hwdev: device pointer to hwdev
 *
 * @return ep id
 */
u8 hinic5_ep_id(void *hwdev); /* Obtain service_cap.ep_id */

/**
 * @brief hinic5_er_id - get er id
 * @param hwdev: device pointer to hwdev
 *
 * @return er id
 */
u8 hinic5_er_id(void *hwdev); /* Obtain service_cap.er_id */

/**
 * @brief hinic5_physical_port_id - get physical port id
 * @param hwdev: device pointer to hwdev
 *
 * @return physical port id
 */
u8 hinic5_physical_port_id(void *hwdev); /* Obtain service_cap.port_id */

/**
 * @brief hinic5_func_max_vf - get vf number
 * @param hwdev: device pointer to hwdev
 *
 * @return 返回最大虚拟功能数量
 *      @retval non-zero: vf number
 *      @retval zero: failure
 */
u16 hinic5_func_max_vf(void *hwdev); /* Obtain service_cap.max_vf */

/*
 * @brief hinic5_max_pf_num - get global max pf number
 */
u8 hinic5_max_pf_num(void *hwdev);

/**
 * @brief hinic5_host_pf_num - get current host pf number
 * @param hwdev: device pointer to hwdev
 *
 * @return 返回获取到的PF数量
 *      @retval non-zero: pf number
 *      @retval zero: failure
 */
u32 hinic5_host_pf_num(void *hwdev); /* Obtain service_cap.pf_num */

/**
 * @brief hinic5_host_pf_id_start - get current host pf id start
 * @param hwdev: device pointer to hwdev
 *
 * @return 获取设备的PF数量
 *      @retval non-zero: pf id start
 *      @retval zero: failure
 */
u32 hinic5_host_pf_id_start(void *hwdev); /* Obtain service_cap.pf_num */

/**
 * @brief hinic5_pcie_itf_id - get pcie port id
 * @param hwdev: device pointer to hwdev
 *
 * @return pcie port id
 */
u8 hinic5_pcie_itf_id(void *hwdev);

/**
 * @brief hinic5_vf_in_pf - get vf offset in pf
 * @param hwdev: device pointer to hwdev
 *
 * @return vf offset in pf
 */
u8 hinic5_vf_in_pf(void *hwdev);

/**
 * @brief hinic5_cos_valid_bitmap - get cos valid bitmap
 * @param hwdev: device pointer to hwdev
 *
 * @return 是否成功
 *      @retval non-zero: valid cos bit map
 *      @retval zero: failure
 */
int hinic5_cos_valid_bitmap(void *hwdev, u8 *func_dft_cos, u8 *port_cos_bitmap);

/**
 * @brief hinic5_stateful_init - init stateful resource
 * @param hwdev: device pointer to hwdev
 *
 * @return 是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_stateful_init(void *hwdev);

/**
 * @brief hinic5_stateful_deinit - deinit stateful resource
 * @param hwdev: device pointer to hwdev
 */
void hinic5_stateful_deinit(void *hwdev);

/**
 * @brief hinic5_free_stateful - sdk remove free stateful resource
 * @param hwdev: device pointer to hwdev
 */
void hinic5_free_stateful(void *hwdev);

/**
 * @brief hinic5_need_init_stateful_default - get need init stateful default
 * @param hwdev: device pointer to hwdev
 */
bool hinic5_need_init_stateful_default(void *hwdev);

/**
 * @brief hinic5_get_card_present_state - get card present state
 * @param hwdev: device pointer to hwdev
 * @param card_present_state: return card present state
 *
 * @return 是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_get_card_present_state(void *hwdev, bool *card_present_state);

/**
 * @brief hinic5_func_rx_tx_flush - function flush
 * @param hwdev: device pointer to hwdev
 * @param channel: channel id
 * @param flr_timeout_ms: flr超时时间，以ms为单位
 *
 * @attention 当flr_timeout_ms为0时，使用flr超时默认值HINIC5_FLR_TIMEOUT(40s)
 *
 * @return 是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_func_rx_tx_flush(void *hwdev, u16 channel, bool wait_io, u32 flr_timeout_ms);

/**
 * @brief hinic5_flush_mgmt_workq - when remove function should flush work queue
 * @param hwdev: device pointer to hwdev
 */
void hinic5_flush_mgmt_workq(void *hwdev);

/**
 * @brief get toe ceq num
 * @param udkdev: device pointer to udkdev
 *
 * @return ceq number
 */
u8 hinic5_ceq_num(void *hwdev);

/**
 * @brief hinic5_intr_num get interrupt num
 * @param udkdev: device pointer to udkdev
 *
 * @return interrupt number
 */
u16 hinic5_intr_num(void *hwdev);

/**
 * @brief hinic5_flexq_en get flexq en
 * @param udkdev: device pointer to udkdev
 *
 * @return flexq enable: 1:enable, 0: disable
 */
u8 hinic5_flexq_en(void *hwdev);

/**
 * @brief hinic5_fault_event_report - report fault event
 * @param hwdev: device pointer to hwdev
 * @param src: fault event source, reference to enum hinic5_fault_source_type
 * @param level: fault level, reference to enum hinic5_fault_err_level
 */
void hinic5_fault_event_report(void *hwdev, u16 src, u16 level);

/**
 * @brief hinic5_probe_success - notify device probe successfull
 * @param hwdev: device pointer to hwdev
 */
void hinic5_probe_success(void *hwdev);

/**
 * @brief hinic5_set_func_svc_used_state - set function service used state
 * @param hwdev: device pointer to hwdev
 * @param svc_type: service type
 * @param state: function used state
 * @param channel: channel id
 *
 * @return 是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_set_func_svc_used_state(void *hwdev, u16 svc_type, u8 state,
				   u16 channel);

/**
 * @brief hinic5_get_self_test_result - get self test result
 * @param hwdev: device pointer to hwdev
 *
 * @return self test result
 */
u32 hinic5_get_self_test_result(void *hwdev);

/**
 * @brief set_slave_host_enable - set slave host enable
 * @param hwdev: device pointer to hwdev
 * @param host_id: set host id
 * @param slave_en-zero: slave is enable
 *
 * @return 是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
void set_slave_host_enable(void *hwdev, u8 host_id, bool enable);

/**
 * @brief hinic5_get_slave_bitmap - get slave host bitmap
 * @param hwdev: device pointer to hwdev
 * @param slave_host_bitmap-zero: slave host bitmap
 *
 * @return 是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_get_slave_bitmap(void *hwdev, u8 *slave_host_bitmap);

/**
 * @brief hinic5_get_slave_host_enable - get slave host enable
 * @param hwdev: device pointer to hwdev
 * @param host_id: get host id
 * @param slave_en-zero: slave is enable
 *
 * @return 是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_get_slave_host_enable(void *hwdev, u8 host_id, u8 *slave_en);

/**
 * @brief hinic5_set_host_migrate_enable - set migrate host enable
 * @param hwdev: device pointer to hwdev
 * @param host_id: get host id
 * @param slave_en-zero: migrate is enable
 *
 * @return 是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_set_host_migrate_enable(void *hwdev, u8 host_id, bool enable);

/**
 * @brief hinic5_get_host_migrate_enable - get migrate host enable
 * @param hwdev: device pointer to hwdev
 * @param host_id: get host id
 * @param slave_en-zero: migrte enable ptr
 *
 * @return 是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_get_host_migrate_enable(void *hwdev, u8 host_id, u8 *migrate_en);

/**
 * @brief 判断是否是从主机
 * @param hwdev 硬件设备指针
 *
 * @return 是否是从主机
 *      @retval true表示是
 *      @retval false表示不是
 */
bool hinic5_is_slave_host(void *hwdev);

/**
 * @brief 判断是否是主主机
 * @param hwdev 硬件设备指针
 *
 * @return 是否是主主机
 *      @retval true表示是
 *      @retval false表示不是
 */
bool hinic5_is_master_host(void *hwdev);

/**
 * @brief 判断是否是多核心设备
 * @param hwdev 设备句柄
 *
 * @return 是不是多核心设备
 *      @retval true 是
 *      @retval false 不是
 */
bool hinic5_is_multi_bm(void *hwdev);

/**
 * @brief 设置插件服务位图
 * @param hwdev 硬件设备
 * @param srv_type 服务类型
 * @param func_id 功能ID
 * @param attach_en 是否启用
 *
 * @return 是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hisdk5_set_plug_srv_bitmap(void *hwdev, u8 srv_type, u16 func_id, u8 attach_en);

/**
 * @brief 获取插件服务位图
 * @param hwdev 硬件设备
 * @param srv_type 服务类型
 * @param func_id 功能ID
 * @param attach_en 是否启用插件服务
 *
 * @return 是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hisdk5_get_plug_srv_bitmap(void *hwdev, u8 srv_type, u16 func_id, u8 *attach_en);

/**
 * @brief 获取设备能力
 * @param hwdev 设备句柄
 *
 * @return 是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_get_dev_cap(void *hwdev);

/**
 * @brief 获取是否是HTN设备
 * @param hwdev 设备句柄
 *
 * @return 是不是HTN设备
 * 		@retval true: 是
 * 		@retval false: 不是
 */
bool hinic5_support_htn(void *hwdev);

/**
 * @brief 获取当前 VF 是否独立运行
 * @param hwdev 设备句柄
 *
 * @details VF 依赖 PF 运行时，其管理面消息需要发送给 PF 而不是 Mgmt
 * @note 该配置对于 PF 无意义
 *
 * @return VF 是否独立运行
 */
bool hinic5_is_vf_isolation(void *hwdev);

/**
 * @brief  设置心跳检测周期和link_down检查次数
 * @param  hwdev：设备句柄
 * @param heartbeat_period：心跳检测周期
 * @param linkdown_threshold：link_down检查次数
 *
 * @details hwdev为空或心跳检测周期和断链检测次数均为0时，返回-EINVAL；
 *          当心跳检测周期不为0时，更新心跳检测周期；
 *          当断链检测次数不为0时，更新断链检测次数；
 *
 * @attention 支持仅更新检测周期/检测次数,不更新的参数直接置0即可；
 *
 * @return 是否成功
 *      @retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_set_heartbeat_period_and_linkdown_cnt(void *hwdev, u32 heartbeat_period, u32 linkdown_threshold);

/**
 * @brief  设置/获取dcb state
 * @param  hwdev[in]：设备句柄
 * @param op[in]：操作类型，HISDK5_DCB_STATE_GET(获取)/HISDK5_DCB_STATE_SET(设置)
 * @param dcb_state[in]：存储业务驱动要写入到sdk的dcb state
 *                 [out]：存储从sdk中获取的dcb state
 *
 * @details op == HISDK5_DCB_STATE_GET，从sdk中获取scb state保存到业务传入的dcb state指针对应的内存中，返回给上层业务；
 *          op == HISDK5_DCB_STATE_SET，将业务传入的dcb state保存到sdk中；
 *
 * @attention 在struct hisdk5_dcb_state中，当dcb_on == 0(dcb off)时，仅default_cos有效；
 *                                         当dcb_on != 0(dcb on)时，参数均有效，可根据trust查看当前优先级模式，
 *                                                                  根据pcp2cos/dscp2cos查看对应的映射关系；
 *
 * @return 是否成功
 *      @retval 0: 成功
 * 		@retval 非0: 失败
 */
int hinic5_dcb_state_op(void *hwdev, enum hisdk5_dcb_state_op op, struct hisdk5_dcb_state *dcb_state);

/**
 * @brief 获取设备端口速率
 *
 * @param hwdev: device pointer to hwdev
 * @param port_info: port info
 * @param channel: channel id
 * @return: 命令执行结果.
 *    @retval 0 成功
 *    @retval 非0 失败
 */
int hinic5_get_port_info(void *hwdev, struct mag_port_info *port_info, u16 channel);

/**
 * @brief 获取设备端口速率
 *
 * @param hwdev device pointer to hwdev
 * @param speed 输出端口速率信息
 * @param channel channel id， mailbox发送使用的channel id
 * @details 通过查询设备对应端口，发送mailbox消息给MPU获取端口速率
 * @attention: 函数内部涉及发送mailbox消息会休眠，禁止中断上下文等不允许休眠的流程中调用
 * @return: 设备端口速率获取返回成功或者失败
 *     @retval 0 成功
 *     @retval 非0 失败
 */
int hinic5_get_speed(void *hwdev, enum mag_cmd_port_speed *speed, u16 channel);

#endif
