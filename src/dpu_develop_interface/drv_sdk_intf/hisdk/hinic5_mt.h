/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef HINIC5_MT_H
#define HINIC5_MT_H

#include <linux/types.h>

#ifdef __HIFC__     /**< 如果定义了__HIFC__宏 */
#define HINIC5_DRV_NAME "hifc5"     /**< 定义驱动名为 hifc5 */
#define HINIC5_CHIP_NAME "hifc"     /**< 定义芯片名为 hifc */
#else               /**< 如果没有定义__HIFC__宏 */
#define HINIC5_DRV_NAME "hisdk5"    /**< 定义驱动名为 hisdk5 */
#define HINIC5_CHIP_NAME "hinic"    /**< 定义芯片名为 hinic */
#endif
/* Interrupt at most records, interrupt will be recorded in the FFM */

#define NICTOOL_CMD_TYPE (0x18)

/**
 * @brief struct api_cmd_rd
 * @details 用于接收API命令的结构体
 */
struct api_cmd_rd {
    u32 pf_id;      /**< pf id */
    u8 dest;        /**< node id */
    u8 *cmd;        /**< 指向API命令的指针 */
    u16 size;       /**< 表示命令的大小 */
    void *ack;      /**< 指向命令的确认信息的指针 */
    u16 ack_size;   /**< 表示确认信息的大小 */
};

/**
 * @brief struct api_cmd_wr
 * @details 用于API命令写操作的结构体
 */
struct api_cmd_wr {
    u32 pf_id;      /**< pf id */
    u8 dest;        /**< node id */
    u8 *cmd;        /**< 指向API命令的指针 */
    u16 size;       /**< 表示命令的大小 */
};

#define PF_DEV_INFO_NUM  32

/**
 * @brief struct pf_dev_info
 * @details 存储PCI设备的信息的结构体
 */
struct pf_dev_info {
    u64 bar0_size;     /**< bar0的大小 */
    u8 bus;            /**< PCI设备的总线号 */
    u8 slot;           /**< PCI设备的插槽号 */
    u8 func;           /**< PCI设备的功能号 */
    u64 phy_addr;      /**< PCI设备的物理地址 */
};

/**
 * Indicates the maximum number of interrupts that can be recorded.
 * Subsequent interrupts are not recorded in FFM.
 */
#define FFM_RECORD_NUM_MAX 64

/**
 * @brief struct ffm_intr_info
 * @details 存储中断信息的结构体
 */
struct ffm_intr_info {
    u8 node_id;         /**< 中断源的节点ID */
    u8 err_level;       /**< 中断源的错误级别 */
    u16 err_type;       /**< 中断源的错误类型 */
    u32 err_csr_addr;   /**< 中断源的地址 */
    u32 err_csr_value;  /**< 中断源的值 */
};

/**
 * @brief struct ffm_intr_tm_info
 * @details 存储中断信息和时间信息的结构体
 */
struct ffm_intr_tm_info {
    struct ffm_intr_info intr_info; /**< 中断信息 */
    u8 times;       /**< 时间 */
    u8 sec;         /**< 秒 */
    u8 min;         /**< 分钟 */
    u8 hour;        /**< 小时 */
    u8 mday;        /**< 日期 */
    u8 mon;         /**< 月份 */
    u16 year;       /**< 年份 */
};

/**
 * @brief struct ffm_record_info
 * @details 用于存储FFM记录信息的结构体
 */
struct ffm_record_info {
    u32 ffm_num;            /**< FFM编号 */
    u32 last_err_csr_addr;  /**< 最后一次错误的CSR地址 */
    u32 last_err_csr_value; /**< 最后一次错误的CSR值 */
    struct ffm_intr_tm_info ffm[FFM_RECORD_NUM_MAX];    /**< FFM中断时间信息数组 */
};

/**
 * @brief struct dbgtool_k_glb_info
 * @details 存储调试工具的全局信息的结构体
 */
struct dbgtool_k_glb_info {
    struct semaphore dbgtool_sem;   /**< 信号量，用于同步调试工具的线程 */
    struct ffm_record_info *ffm;    /**< 存储ffm记录信息 */
};

/**
 * @brief struct msg_2_up
 * @details 存储上行消息的相关信息的结构体
 */
struct msg_2_up {
    u8 pf_id;       /**< 协议ID */
    u8 mod;         /**< 模块ID */
    u8 cmd;         /**< 命令ID */
    void *buf_in;   /**< 输入缓冲区指针 */
    u16 in_size;    /**< 输入缓冲区大小 */
    void *buf_out;  /**< 输出缓冲区指针 */
    u16 *out_size;  /**< 输出缓冲区大小指针 */
};

/**
 * @brief struct dbgtool_param
 * @details 调试工具参数结构体
 */
struct dbgtool_param {
    union {
        struct api_cmd_rd api_rd;       /**< 读取命令 */
        struct api_cmd_wr api_wr;       /**< 写入命令 */
        struct pf_dev_info *dev_info;   /**< 设备信息 */
        struct ffm_record_info *ffm_rd; /**< FFM记录信息 */
        struct msg_2_up msg2up;         /**< 上行消息 */
    } param;
    char chip_name[16];                 /**< 芯片名称 */
};

/**
 * @brief typedef enum
 * @details 表示调试工具的命令类型
 */
typedef enum {
    DBGTOOL_CMD_API_RD = 0,         /**< 读取API命令 */
    DBGTOOL_CMD_API_WR,             /**< 写入API命令 */
    DBGTOOL_CMD_FFM_RD,             /**< 读取FFM命令 */
    DBGTOOL_CMD_FFM_CLR,            /**< 清除FFM命令 */
    DBGTOOL_CMD_PF_DEV_INFO_GET,    /**< 获取PF设备信息命令 */
    DBGTOOL_CMD_MSG_2_UP,           /**< 将消息发送到上层命令 */
    DBGTOOL_CMD_FREE_MEM,           /**< 释放内存命令 */
    DBGTOOL_CMD_NUM                 /**< 命令类型数量 */
} dbgtool_cmd;

#define PF_MAX_SIZE (16)
#define BUSINFO_LEN (32)
#define HINIC_FUNC_MAX_SIZE (4096)

/**
 * @brief enum module_name
 * @details 表示不同的模块名称
 */
enum module_name {
    SEND_TO_NPU = 1,            /**< 发送到NPU模块 */
    SEND_TO_MPU,                /**< 发送到MPU模块 */
    SEND_TO_SM,                 /**< 发送到SM模块 */
    SEND_TO_HW_DRIVER,          /**< 发送到硬件驱动 */
#define SEND_TO_SRV_DRV_BASE (SEND_TO_HW_DRIVER + 1)
    SEND_TO_NIC_DRIVER = SEND_TO_SRV_DRV_BASE,  /**< 发送到网络接口控制器驱动 */
    SEND_TO_OVS_DRIVER,         /**< 发送到Open vSwitch驱动 */
    SEND_TO_ROCE_DRIVER,        /**< 发送到RDMA over Converged Ethernet驱动 */
    SEND_TO_TOE_DRIVER,         /**< 发送到TCP offload驱动 */
    SEND_TO_IOE_DRIVER,         /**< 发送到I/O加速驱动 */
    SEND_TO_FC_DRIVER,          /**< 发送到光纤通道驱动 */
    SEND_TO_VBS_DRIVER,         /**< 发送到虚拟阻塞存储驱动 */
    SEND_TO_IPSEC_DRIVER,       /**< 发送到IPsec驱动 */
    SEND_TO_VIRTIO_DRIVER,      /**< 发送到Virtio驱动 */
    SEND_TO_MIGRATE_DRIVER,     /**< 发送到迁移驱动 */
    SEND_TO_PPA_DRIVER,         /**< 发送到PPA驱动 */
    SEND_TO_CUSTOM_DRIVER = SEND_TO_SRV_DRV_BASE + 11,  /**< 发送到自定义驱动 */
    SEND_TO_VROCE_DRIVER,       /**< 发送到vRDMA over Converged Ethernet驱动 */
    SEND_TO_UB_DRIVER,          /**< 发送到UB驱动 */
    SEND_TO_JBOF_DRIVER,        /**< 发送到Jumbo Frame offload驱动 */
    SEND_TO_MACSEC_DRIVER,      /**< 发送到MACsec驱动 */
    SEND_TO_BIFUR_DRIVER = SEND_TO_MACSEC_DRIVER + 3,
    SEND_TO_HIHTR_DRIVER,        /**< 发送到Hihtr驱动 */
    SEND_TO_DRIVER_MAX = SEND_TO_SRV_DRV_BASE + 20, /* reserved */
};

/**
 * @brief enum driver_cmd_type
 * @details 定义驱动命令类型枚举
 */
enum driver_cmd_type {
    TX_INFO = 0x1,                     /**< 发送信息 */
    Q_NUM = 0x2,                       /**< 队列数量 */
    TX_WQE_INFO = 0x3,                 /**< 发送工作队列信息 */
    TX_MAPPING = 0x4,                  /**< 发送映射 */
    RX_INFO = 0x5,                     /**< 接收信息 */
    RX_WQE_INFO = 0x6,                 /**< 接收工作队列信息 */
    RX_CQE_INFO = 0x7,                 /**< 接收完成队列信息 */
    UPRINT_FUNC_EN = 0x8,              /**< 打印功能启用 */
    UPRINT_FUNC_RESET = 0x9,           /**< 打印功能重置 */
    UPRINT_SET_PATH = 0xa,             /**< 设置打印路径 */
    UPRINT_GET_STATISTICS = 0xb,       /**< 获取打印统计信息 */
    FUNC_TYPE = 0xc,                   /**< 功能类型 */
    GET_FUNC_IDX = 0xd,                /**< 获取功能索引 */
    GET_INTER_NUM = 0xe,               /**< 获取内部数量 */
    CLOSE_TX_STREAM = 0xf,             /**< 关闭发送流 */
    GET_DRV_VERSION = 0x10,            /**< 获取驱动版本 */
    CLEAR_FUNC_STASTIC = 0x11,         /**< 清除功能统计信息 */
    GET_HW_STATS = 0x12,               /**< 获取硬件统计信息 */
    CLEAR_HW_STATS = 0x13,             /**< 清除硬件统计信息 */
    GET_SELF_TEST_RES = 0x14,          /**< 获取自我测试结果 */
    GET_CHIP_FAULT_STATS = 0x15,       /**< 获取芯片故障统计信息 */
    NIC_RSVD1 = 0x16,
    NIC_RSVD2 = 0x17,
    NIC_RSVD3 = 0x18,
    GET_CHIP_ID = 0x19,                /**< 获取芯片ID */
    GET_SINGLE_CARD_INFO = 0x1a,       /**< 获取单卡信息 */
    GET_FIRMWARE_ACTIVE_STATUS = 0x1b, /**< 获取固件活动状态 */
    ROCE_DFX_FUNC = 0x1c,              /**< RoCE调试功能 */
    GET_DEVICE_ID = 0x1d,              /**< 获取设备ID */
    GET_PF_DEV_INFO = 0x1e,            /**< 获取PF设备信息 */
    CMD_FREE_MEM = 0x1f,               /**< 释放内存 */
    GET_LOOPBACK_MODE = 0x20,          /**< 获取环回模式 */
    SET_LOOPBACK_MODE = 0x21,          /**< 设置环回模式 */
    SET_LINK_MODE = 0x22,              /**< 设置链接模式 */
    SET_PF_BW_LIMIT = 0x23,            /**< 设置PF带宽限制 */
    GET_PF_BW_LIMIT = 0x24,            /**< 获取PF带宽限制 */
    ROCE_CMD = 0x25,                   /**< RoCE命令 */
    GET_POLL_WEIGHT = 0x26,            /**< 获取轮询权重 */
    SET_POLL_WEIGHT = 0x27,            /**< 设置轮询权重 */
    GET_HOMOLOGUE = 0x28,              /**< 获取对端信息 */
    SET_HOMOLOGUE = 0x29,              /**< 设置对端信息 */
    GET_SSET_COUNT = 0x2a,             /**< 获取统计信息数量 */
    GET_SSET_ITEMS = 0x2b,             /**< 获取统计信息项 */
    IS_DRV_IN_VM = 0x2c,               /**< 判断是否在虚拟机中 */
    LRO_ADPT_MGMT = 0x2d,              /**< 管理LRO适配器 */
    SET_INTER_COAL_PARAM = 0x2e,       /**< 设置中断合并参数 */
    GET_INTER_COAL_PARAM = 0x2f,       /**< 获取中断合并参数 */
    GET_CHIP_INFO = 0x30,              /**< 获取芯片信息 */
    GET_NIC_STATS_LEN = 0x31,          /**< 获取NIC统计信息长度 */
    GET_NIC_STATS_STRING = 0x32,       /**< 获取NIC统计信息字符串 */
    GET_NIC_STATS_INFO = 0x33,         /**< 获取NIC统计信息 */
    GET_PF_ID = 0x34,                  /**< 获取PF ID */
    GET_MBOX_CNT = 0x35,               /**< 获取mailbox数量 */
    NIC_RSVD5 = 0x36,
    DCB_QOS_INFO = 0x37,               /**< DCB QoS信息 */
    DCB_PFC_STATE = 0x38,              /**< DCB PFC状态 */
    DCB_ETS_STATE = 0x39,              /**< DCB ETS状态 */
    DCB_STATE = 0x3a,                  /**< DCB状态 */
    QOS_DEV = 0x3b,                    /**< QOS设备 */
    GET_QOS_COS = 0x3c,                /**< 获取QOS优先级 */
    GET_ULD_DEV_NAME = 0x3d,           /**< 获取ULD设备名称 */
    GET_TX_TIMEOUT = 0x3e,             /**< 获取发送超时 */
    SET_TX_TIMEOUT = 0x3f,             /**< 设置发送超时 */

    RSS_CFG = 0x40,                    /**< RSS配置 */
    RSS_INDIR = 0x41,                  /**< RSS间接表 */
    PORT_ID = 0x42,                    /**< 端口ID */
    BOND_DFX_OPS = 0x43,               /**< BOND DFX操作 */

    GET_FUNC_CAP = 0x50,               /**< 获取功能能力 */
    GET_XSFP_PRESENT = 0x51,           /**< 获取XSFP存在状态 */
    GET_XSFP_INFO = 0x52,              /**< 获取XSFP信息 */
    DEV_NAME_TEST = 0x53,              /**< 设备名称测试 */
    GET_XSFP_INFO_COMP_CMIS = 0x54,    /**< 获取XSFP信息（支持CMIS） */
    CMD_GET_PROFILE_ID = 0x55,
    CMD_SET_PROFILE_ID = 0x56,
    CMD_MOVE_TCAM_TABLE = 0x57,

    GET_WIN_STAT = 0x60,               /**< 获取窗口状态 */
    WIN_CSR_READ = 0x61,               /**< 读取窗口CSR */
    WIN_CSR_WRITE = 0x62,              /**< 写入窗口CSR */
    WIN_API_CMD_RD = 0x63,             /**< 读取窗口API命令 */

    ROCE_CMD_SET_LDCP_PARAM = 0x70,    /**< RoCE命令设置LDCP参数 */

    ROCE_CMD_GET_QPC_FROM_CACHE = 0x80,     /**< 从缓存获取QPC */
    ROCE_CMD_GET_QPC_FROM_HOST = 0x81,      /**< 从主机获取QPC */
    ROCE_CMD_GET_CQC_FROM_CACHE = 0x82,     /**< 从缓存获取CQC */
    ROCE_CMD_GET_CQC_FROM_HOST = 0x83,      /**< 从主机获取CQC */
    ROCE_CMD_GET_SRQC_FROM_CACHE = 0x84,    /**< 从缓存获取SRQC */
    ROCE_CMD_GET_SRQC_FROM_HOST = 0x85,     /**< 从主机获取SRQC */
    ROCE_CMD_GET_MPT_FROM_CACHE = 0x86,     /**< 从缓存获取MPT */
    ROCE_CMD_GET_MPT_FROM_HOST = 0x87,      /**< 从主机获取MPT */
    ROCE_CMD_GET_GID_FROM_CACHE = 0x88,     /**< 从缓存获取GID */
    ROCE_CMD_GET_QPC_CQC_PI_CI = 0x89,      /**< 获取QPC、CQC、PI、CI */
    ROCE_CMD_GET_QP_COUNT = 0x8a,           /**< 获取QP数量 */
    ROCE_CMD_GET_DEV_ALGO = 0x8b,           /**< 获取设备算法 */
    ROCE_CMD_GET_DEV_TYPE = 0x8c,           /**< 获取设备类型 */
    ROCE_CMD_GET_HW_COUNT = 0x8d,           /**< 获取硬件计数 */
    ROCE_CMD_GET_SPECIFICATIONS = 0x8e,     /**< 从缓存获取设备的功能规格 */

    ROCE_CMD_START_CAP_PACKET = 0x90,       /**< 开始捕获数据包 */
    ROCE_CMD_STOP_CAP_PACKET = 0x91,        /**< 停止捕获数据包 */
    ROCE_CMD_QUERY_CAP_INFO = 0x92,         /**< 查询捕获信息 */
    ROCE_CMD_ENABLE_QP_CAP_PACKET = 0x93,   /**< 启用QP捕获数据包 */
    ROCE_CMD_DISABLE_QP_CAP_PACKET = 0x94,  /**< 禁用QP捕获数据包 */
    ROCE_CMD_QUERY_QP_CAP_INFO = 0x95,      /**< 查询QP捕获信息 */
    ROCE_CMD_SET_BYPASS = 0x96,             /**< 设置bypass */
    ROCE_CMD_QUERY_BYPASS = 0x97,           /**< 查询bypass */
    ROCE_CMD_GET_AEQC_FROM_CACHE = 0x98,    /**< 从缓存获取AEQC */
    ROCE_CMD_GET_AEQC_FROM_HOST = 0x99,     /**< 从主机获取AEQC */

    ROCE_CMD_ENABLE_BW_CTRL = 0xa0,         /**< 启用带宽控制 */
    ROCE_CMD_DISABLE_BW_CTRL = 0xa1,        /**< 禁用带宽控制 */
    ROCE_CMD_CHANGE_BW_CTRL_PARAM = 0xa2,   /**< 更改带宽控制参数 */
    ROCE_CMD_QUERY_BW_CTRL_PARAM = 0xa3,    /**< 查询带宽控制参数 */
    ROCE_CMD_SET_BW_WATERLINE = 0xa4,       /**< 设置带宽水线值 */
    ROCE_CMD_GET_BW_WATERLINE = 0xa5,       /**< 获取带宽水线值 */
    ROCE_CMD_SET_VNIC_WATERLINE = 0xa6,     /**< 设置VNIC水线值 */
    ROCE_CMD_GET_VNIC_WATERLINE = 0xa7,     /**< 获取VNIC水线值 */
    ROCE_CMD_ROCE_SET = 0xa8,               /**< 设置ROCE相关配置 */
    ROCE_CMD_DFX_LATCH_QUERY = 0xa9,        /**< ROCE锁存查询 */

    ROCE_CMD_TIMEOUT_ALARM = 0xb0,          /**< 超时警告 */
    ROCE_CMD_PORT_TRAFFIC = 0Xb1,           /**< 端口流量 */
    ROCE_CMD_DFX_ATTACK = 0Xb2,             /**< ROCE主机侧防攻击 */
    ROCE_CMD_ULD_IOCTL_EXTEND = 0xb3,       /**< 用户ioctl到驱动的总入口 */

    MIG_QUERY_DFX = 0xc0,                   /**< 查询迁移DFX */

    DRV_CMD_TYPE_RSV = 0xd0,                /**< 预留,不可使用 */

    NIC_CMD_ANTI_ATTACK = 0xd6,             /**< 防攻击验证 */

    VM_COMPAT_TEST = 0xFF,                  /**< VM兼容性测试 */

    SERVICE_DRV_BASE_CMD = 0x120,           /**< 业务命令字从0x120开始，0x120前面的预留给产品后续使用 */
};

/**
 * @brief enum api_chain_cmd_type
 * @details 定义API链命令类型枚举
 */
enum api_chain_cmd_type {
    API_CSR_READ,       /**< 读取CSR（控制和状态寄存器） */
    API_CSR_WRITE       /**< 写入CSR（控制和状态寄存器） */
};

/**
 * @brief sm_cmd_type
 * @details 用于表示不同的命令类型
 */
enum sm_cmd_type {
    SM_CTR_RD16 = 1,        /**< 读取16位数据的命令 */
    SM_CTR_RD32,            /**< 读取32位数据的命令 */
    SM_CTR_RD64_PAIR,       /**< 读取64位数据对的命令 */
    SM_CTR_RD64,            /**< 读取64位数据的命令 */
    SM_CTR_RD32_CLEAR,      /**< 清除32位数据的命令 */
    SM_CTR_RD64_PAIR_CLEAR, /**< 清除64位数据对的命令 */
    SM_CTR_RD64_CLEAR       /**< 清除64位数据的命令 */
};

#define CQM_AEQ_CALLBACK_CNT_MAX 128  /* 与CQM_AEQ_BASE_T_MAX保持一致 */

/**
 * @brief struct cqm_stats
 * @details 统计CQM模块的各种操作数量
 */
struct cqm_stats {
    atomic_t cqm5_cmd_alloc_cnt;             /**< 统计CQM命令分配的次数 */
    atomic_t cqm5_cmd_free_cnt;              /**< 统计CQM命令释放的次数 */
    atomic_t cqm5_send_cmd_box_cnt;          /**< 统计CQM发送命令盒子的次数 */
    atomic_t cqm5_send_cmd_imm_cnt;          /**< 统计CQM发送命令的次数 */
    atomic_t cqm5_db_addr_alloc_cnt;         /**< 统计CQM数据库地址分配的次数 */
    atomic_t cqm5_db_addr_free_cnt;          /**< 统计CQM数据库地址释放的次数 */
    atomic_t cqm_fc_srq_create_cnt;         /**< 统计CQM创建FC SRQ的次数 */
    atomic_t cqm_srq_create_cnt;            /**< 统计CQM创建SRQ的次数 */
    atomic_t cqm_rq_create_cnt;             /**< 统计CQM创建RQ的次数 */
    atomic_t cqm_qpc_mpt_create_cnt;        /**< 统计CQM创建QPC和MPT的次数 */
    atomic_t cqm_nonrdma_queue_create_cnt;  /**< 统计CQM创建非RDMA队列的次数 */
    atomic_t cqm_rdma_queue_create_cnt;     /**< 统计CQM创建RDMA队列的次数 */
    atomic_t cqm_rdma_table_create_cnt;     /**< 统计CQM创建RDMA表的次数 */
    atomic_t cqm_qpc_mpt_delete_cnt;        /**< 统计CQM删除QPC和MPT的次数 */
    atomic_t cqm_nonrdma_queue_delete_cnt;  /**< 统计CQM删除非RDMA队列的次数 */
    atomic_t cqm_rdma_queue_delete_cnt;     /**< 统计CQM删除RDMA队列的次数 */
    atomic_t cqm_rdma_table_delete_cnt;     /**< 统计CQM删除RDMA表的次数 */
    atomic_t cqm_func_timer_clear_cnt;      /**< 统计CQM清除函数计时器的次数 */
    atomic_t cqm_func_hash_buf_clear_cnt;   /**< 统计CQM清除函数哈希缓冲区的次数 */
    atomic_t cqm_scq_callback_cnt;          /**< 统计CQM SCQ回调的次数 */
    atomic_t cqm_ecq_callback_cnt;          /**< 统计CQM ECQ回调的次数 */
    atomic_t cqm_nocq_callback_cnt;         /**< 统计CQM NOCQ回调的次数 */
    atomic_t cqm_aeq_callback_cnt[CQM_AEQ_CALLBACK_CNT_MAX];    /**< 统计CQM AEQ回调的次数 */
};

/**
 * @brief struct link_event_stats
 * @details 用于统计链路事件的数量
 */
struct link_event_stats {
    atomic_t link_down_stats;       /**< 表示链路已经断开的事件数量 */
    atomic_t link_up_stats;         /**< 表示链路已经连接的事件数量 */
};

/**
 * @brief enum hinic5_fault_err_level
 * @details 错误级别枚举类型
 */
enum hinic5_fault_err_level {
    FAULT_LEVEL_FATAL,          /**< 致命错误 */
    FAULT_LEVEL_SERIOUS_RESET,  /**< 严重错误，需要重置 */
    FAULT_LEVEL_HOST,           /**< 主机错误 */
    FAULT_LEVEL_SERIOUS_FLR,    /**< 严重错误，需要FLR（函数级复位） */
    FAULT_LEVEL_GENERAL,        /**< 一般错误 */
    FAULT_LEVEL_SUGGESTION,     /**< 建议错误 */
    FAULT_LEVEL_MAX,            /**< 错误级别最大值 */
};

/**
 * @brief enum hinic5_fault_type
 * @details 定义了可能出现的故障类型
 */
enum hinic5_fault_type {
    FAULT_TYPE_CHIP,            /**< 芯片故障 */
    FAULT_TYPE_UCODE,           /**< 微码故障 */
    FAULT_TYPE_MEM_RD_TIMEOUT,  /**< 读取内存超时故障 */
    FAULT_TYPE_MEM_WR_TIMEOUT,  /**< 写入内存超时故障 */
    FAULT_TYPE_REG_RD_TIMEOUT,  /**< 读取寄存器超时故障 */
    FAULT_TYPE_REG_WR_TIMEOUT,  /**< 写入寄存器超时故障 */
    FAULT_TYPE_PHY_FAULT,       /**< 物理故障 */
    FAULT_TYPE_TSENSOR_FAULT,   /**< 温度传感器故障 */
    FAULT_TYPE_MAX,             /**< 故障类型最大值 */
};

/**
 * @brief struct fault_event_stats
 * @details 故障事件统计结构体
 */
struct fault_event_stats {
    /* HINIC_NODE_ID_MAX: temp use the value of 1822(22) */
    atomic_t chip_fault_stats[22][FAULT_LEVEL_MAX]; /**< 各个芯片的故障等级统计 */
    atomic_t fault_type_stat[FAULT_TYPE_MAX];       /**< 各种故障类型的统计 */
    atomic_t pcie_fault_stats;                      /**< PCIE故障的统计 */
};

/**
 * @brief enum hinic5_ucode_event_type
 * @details 定义了Ucode事件类型的枚举类型
 */
enum hinic5_ucode_event_type {
    HINIC5_INTERNAL_OTHER_FATAL_ERROR = 0x0,    /**< 内部其他致命错误 */
    HINIC5_HTN_PTP_EVENT = 0x1,                 /**< HTN PTP事件 */
    HINIC5_CHANNEL_BUSY = 0x7,                  /**< 通道忙 */
    HINIC5_NIC_FATAL_ERROR_MAX = 0x8,           /**< NIC致命错误的最大值 */
};

/**
 * @brief struct hinic5_hw_stats
 * @details 硬件统计结构体
 */
struct hinic5_hw_stats {
    atomic_t heart_lost_stats;                                  /**< 心跳丢失统计 */
    struct cqm_stats cqm_stats;                                 /**< CQM统计 */
    struct link_event_stats link_event_stats;                   /**< 链路事件统计 */
    struct fault_event_stats fault_event_stats;                 /**< 故障事件统计 */
    atomic_t nic_ucode_event_stats[HINIC5_NIC_FATAL_ERROR_MAX]; /**< NIC微码事件统计 */
};

#ifndef IFNAMSIZ
#define IFNAMSIZ 16
#endif

/**
 * @brief struct pf_info
 * @details 用于存储网络接口的信息
 */
struct pf_info {
    char name[IFNAMSIZ];        /**< 网络接口的名称 */
    char bus_info[BUSINFO_LEN]; /**< 网络接口的总线信息 */
    u32 pf_type;                /**< 网络接口的类型 */
};

/**
 * @brief struct card_info
 * @details 用于存储卡片信息
 * @param pf_num
 *              in_param: 累计已获取的pf num(PF_MAX_SIZE的整数倍)
 *              out_param: 当前获取的pf_info的数量及待获取pf_info的pf num总和
 */
struct card_info {
    struct pf_info pf[PF_MAX_SIZE];     /**< 用于存储卡片的各种信息 */
    u32 pf_num;                         /**< 用于存储卡片的数量 */
};

struct func_mbox_cnt_info {
    char bus_info[BUSINFO_LEN];
    u64 send_cnt;
    u64 ack_cnt;
};

struct card_mbox_cnt_info {
    struct func_mbox_cnt_info func_info[HINIC_FUNC_MAX_SIZE];
    u32 func_num;
};

/**
 * @brief struct hinic5_nic_loop_mode
 * @details 用于循环模式的结构体
 */
struct hinic5_nic_loop_mode {
    u32 loop_mode;      /**< 循环模式的标识符 */
    u32 loop_ctrl;      /**< 循环控制的标识符 */
};

/**
 * @brief struct hinic5_pf_info
 * @details 用于存储PF的信息
 */
struct hinic5_pf_info {
    u32 isvalid;    /**< 表示PF信息是否有效的标志位 */
    u32 pf_id;      /**< PF的唯一标识符 */
};

#define HINIC5_CHIP_FAULT_SIZE (110 * 1024)
#define MAX_DRV_BUF_SIZE 4096

/**
 * @brief struct nic_cmd_chip_fault_stats
 * @details 网络接口命令芯片故障统计信息结构体
 */
struct nic_cmd_chip_fault_stats {
    u32 offset;                             /**< 偏移量 */
    u8 chip_fault_stats[MAX_DRV_BUF_SIZE];  /**< 芯片故障统计信息数组 */
};

#define NIC_TOOL_MAGIC 'x'      /**< 表示NIC工具的魔数 */

#ifdef STORAGE_PANGEA
#define CARD_MAX_SIZE (16)      /**< 定义卡片的最大大小为16 */
#else
#define CARD_MAX_SIZE (64)      /**< 卡片的最大大小为64 */
#endif

/**
 * @brief struct nic_card_id
 * @details 用于存储网卡的ID和数量
 */
struct nic_card_id {
    u32 id[CARD_MAX_SIZE];  /**< 网卡ID数组，最大长度为CARD_MAX_SIZE */
    u32 num;                /**< 网卡数量 */
};

/**
 * @brief struct func_dev_info
 * @details 用于存储函数相关的dev信息
 */
struct func_dev_info {
    u64 bar0_phy_addr;      /**< bar0的物理地址 */
    u64 bar0_size;          /**< bar0的大小 */
    u64 bar1_phy_addr;      /**< bar1的物理地址 */
    u64 bar1_size;          /**< bar1的大小 */
    u64 bar3_phy_addr;      /**< bar3的物理地址 */
    u64 bar3_size;          /**< bar3的大小 */
    u64 rsvd1[4];
};

/**
 * @brief struct hinic5_card_func_info
 * @details 用于存储卡片功能信息
 */
struct hinic5_card_func_info {
    u32 num_pf;             /**< 物理函数数量 */
    u32 rsvd0;
    u64 usr_api_phy_addr;   /**< 用户API物理地址 */
    struct func_dev_info dev_info[CARD_MAX_SIZE]; /**< 函数dev信息数组 */
};

#define MAX_VER_INFO_LEN 128    /**< 定义最大版本信息长度常量 */
/**
 * @brief struct drv_version_info
 * @details 定义驱动版本信息结构体
 */
struct drv_version_info {
    char ver[MAX_VER_INFO_LEN]; /**< 定义版本信息字符数组，长度为MAX_VER_INFO_LEN */
};

#define MT_EPERM        1       /**< 不允许操作 */
#define MT_EIO          2       /**< I/O错误 */
#define MT_EINVAL       3       /**< 无效参数 */
#define	MT_EBUSY        4       /**< 设备或资源忙 */
#define MT_EOPNOTSUPP   0xFF    /**< 不支持操作 */

/**
 * @brief struct mt_msg_head
 * @details 用于存储消息头的结构体
 */
struct mt_msg_head {
    u8 status;      /**< 状态 */
    u8 rsvd1[3];    /**< 保留字段 */
};

/**
 * @brief enum mt_api_type
 * @details 用于表示不同的API类型
 */
enum mt_api_type {
    API_TYPE_MBOX = 1,          /**< MBOX API类型 */
    API_TYPE_API_CHAIN_BYPASS,  /**< API链式调用绕过API类型 */
    API_TYPE_API_CHAIN_TO_MPU,  /**< API链式调用到MPU的API类型 */
    API_TYPE_CLP,               /**< CLP API类型 */
};

/**
 * @brief struct npu_cmd_st
 * @details 用于描述NPU命令的结构体
 */
struct npu_cmd_st {
    u32 mod : 8;            /**< 模块ID，占用32位中的8位 */
    u32 cmd : 8;            /**< 命令ID，占用32位中的8位 */
    u32 ack_type : 3;       /**< 确认类型，占用32位中的3位 */
    u32 direct_resp : 1;    /**< 直接响应标志，占用32位中的1位 */
    u32 len : 12;           /**< 长度，占用32位中的12位 */
};

/**
 * @brief struct mpu_cmd_st
 * @details 用于存储MPU命令的结构体
 */
struct mpu_cmd_st {
    u32 api_type : 8;   /**< 定义了一个32位的无符号整数，用于存储API类型，占用8位 */
    u32 mod : 8;        /**< 定义了一个32位的无符号整数，用于存储模块，占用8位 */
    u32 cmd : 16;       /**< 定义了一个32位的无符号整数，用于存储命令，占用16位 */
};

/**
 * @brief struct msg_module
 * @details 消息模块结构体，用于存储设备名、模块信息、命令格式、超时时间、函数索引、输入输出缓冲区大小、缓冲区指针、总线号、端口ID等信息
 */
struct msg_module {
	char device_name[IFNAMSIZ];     /**< 设备名，存储设备的名称 */
    u32 module;                     /**< 模块信息，存储模块的相关信息 */
    /**
     * @brief 存储消息的格式
     */
    union {
        u32 msg_formate;
        struct npu_cmd_st npu_cmd;
        struct mpu_cmd_st mpu_cmd;
    };
    u32 timeout;            /**< 超时时间，存储操作的超时时间 */
    u32 func_idx;           /**< 函数索引，存储函数的索引信息 */
    u32 buf_in_size;        /**< 输入缓冲区大小，存储输入缓冲区的大小 */
    u32 buf_out_size;       /**< 输出缓冲区大小，存储输出缓冲区的大小 */
    void *in_buf;           /**< 输入缓冲区指针，存储输入缓冲区的指针 */
    void *out_buf;          /**< 输出缓冲区指针，存储输出缓冲区的指针 */
    int bus_num;            /**< 总线编号，存储总线的编号信息 */
    u8 port_id;             /**< 端口ID，存储端口的ID信息 */
    u8 use_func_idx;        /**< 表示使用func_idx对设备下发命令, 配合func_idx使用 */
    u8 rsvd1[2];
    u32 rsvd2[4];
};

#define  MQM_FLOW_NUM	2
#define  MQM_XID_NUM	32768
#define  MQM_COS_VLD 	1
#define  MQM_COS_INVLD 	0
#define  MQM_COS_NUM    8

typedef struct hinic5_mqm_send_db_cmd_s {
    u32 service_type;
    u32 cflag;
    u32 no_fliter;
    u32 pf_id;
    u32 cos_vld[8];
    u32 db_cnt[8];
    u32 time[8];
    u32 time_max;
    u32 speed_pps[8];
    u32 length[8];
    u32 num[8];
    u32 mode;
    u32 db_dw0_rsv;
    u32 db_dw1_value;
    u32 rand;
} mqm_send_db_cmd_s;

struct hinic5_mqm_db_num_len {
    u32	db_info;
    u32	pi_hi;
};
typedef union {
    struct {
        u32 num         	: 8;
        u32 length        	: 18;
        u32 mode         	: 2;
        u32 rsvd         	: 4;
    }bs;
    u32 value;
}u_hinic5_mqm_db_num_len;

struct hinic5_mt_msg {
    const void *buf_in;
    void *buf_out;
    u32 in_size;
    u32 out_size;
};

/**
 * @brief struct hinic5_non_ptp_info
 * @details 定义一个结构体，用于存储non ptp 时间信息
 */
struct hinic5_non_ptp_info {
	char name[IFNAMSIZ];                         /**< 存储时间对应的chip设备名 */
	u64 non_ptp_time_diff_enable;                /**< 非PTP时间差使能 */
	s64 non_ptp_time_diff;                       /**< 非PTP时间差 */
	atomic_t  ref_cnt;                            /**< 使用该non ptp info的引用计数 */
};

/**
 * @brief hinic5_set_freq_reduce_ratio
 * @param dev: device pointer
 * @param ratio: non ptp chip time 降频比, 需要大于0
 *
 * @return 是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_set_freq_reduce_ratio(void *dev, u32 ratio);

/**
 * @brief hinic5_set_non_ptp_time_diff_en
 * @param dev: device pointer
 * @param enable: non ptp chip time 使能标记, 0 disable, 1 enable
 *
 * @return 是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_set_non_ptp_time_diff_en(void *dev, bool enable);

/**
 * @brief SDK驱动命令钩子函数
 *
 * @param lld_dev device pointer to hinic5_lld_dev
 * @param cmd 命令字
 * @param nt_msg 命令内容
 * @param support 是否支持该命令， 产品需要根据命令字判断是否支持
 *
 * @details 由产品重载
 *
 * @return: 命令执行结果.
 *     @retval 0 成功
 *     @retval 非0 失败
 */
int hinic5_nictool_cmd_extend_handle(void *lld_dev, u32 cmd, struct hinic5_mt_msg *mt_msg, bool *support);

#endif /* _HINIC5_MT_H_ */
