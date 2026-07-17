#ifndef NIC_PUB_CMD_H
#define NIC_PUB_CMD_H

#include "hinic5_mt.h"

/* 队列信息相关 */

/**
 * @brief struct hinic5_tx_hw_page
 * @details 用于存储硬件页面信息的结构体
 */
struct hinic5_tx_hw_page {
    u64 phy_addr;       /**< 物理地址 */
    u64 *map_addr;      /**< 映射地址 */
};

enum hinic5_show_set {
    HINIC5_SHOW_SSET_IO_STATS = 1,
};

#define HINIC5_SHOW_ITEM_LEN 32
/**
 * @brief struct hinic5_show_item
 * @details 用于显示项的结构体
 */
struct hinic5_show_item {
    char name[HINIC5_SHOW_ITEM_LEN];    /**< 项的名称 */
    u8 hexadecimal;                     /**< 值的显示方式，0表示十进制，1表示十六进制 */
    u8 rsvd[7];                         /**< 保留字段 */
    u64 value;                          /**< 项的值 */
};

/**
 * @brief struct wqe_info
 * @details 用于存储工作队列的相关信息
 */
struct wqe_info {
    int q_id;               /**< 队列ID */
    void *slq_handle;       /**< 队列句柄 */
    unsigned int wqe_id;    /**< 工作队列元素ID */
};

/**
 * @brief struct nic_sq_info
 * @details 用于存储网络接口的发送队列信息的结构体
 */
struct nic_sq_info {
    u16 q_id;           /**< 队列ID */
    u16 pi;             /**< 生产者索引 */
    u16 ci;             /**< 消费者索引 */
    u16 fi;             /**< 硬件消费者索引 */
    u32 q_depth;        /**< 队列深度 */
    u16 pi_reverse;     /**< 生产者索引的反向 */
    u16 wqebb_size;     /**< 工作队列元素的大小 */
    u8 priority;        /**< 优先级 */
    u64 ci_wqe_page_addr;   /**< sq wq首page地址 */
    u64 cla_addr;       /**< wq block地址 */
    void *slq_handle;   /**< 发送队列句柄 */
    struct hinic5_tx_hw_page direct_wqe;    /**< 直接工作队列元素 */
    struct hinic5_tx_hw_page doorbell;      /**< 门铃 */
    u32 page_idx;       /**< 页面索引 */
    u32 glb_sq_id;      /**< 全局发送队列ID */
};

/**
 * @brief struct nic_rq_info
 * @details 用于存储网络接口的接收队列信息的结构体
 */
struct nic_rq_info {
    u16 q_id;       /**< 队列ID */
    u16 delta;      /**< 差值 */
    u16 hw_ci;
    u16 ci;         /**< 消费者索引 */
    u16 sw_pi;      /**< 软件生产者索引 */
    u16 wqebb_size; /**< 工作队列元素的大小 */
    u16 q_depth;    /**< 队列深度 */
    u16 buf_len;    /**< 缓冲区长度 */

    void *slq_handle;       /**< 接收队列句柄 */
    u64 ci_wqe_page_addr;   /**< 消费者索引工作队列元素的页面地址 */
    u64 ci_cla_tbl_addr;    /**< 消费者索引缓存行对齐表地址 */

    u8 coalesc_timer_cfg;   /**< 中断超时时间，单位5us */
    u8 pending_limt;        /**< 中断聚合个数，单位8pkt */
    u16 msix_idx;           /**< MSI-X索引 */
    u32 msix_vector;        /**< MSI-X向量 */
};

/* QOS相关 */

#define MT_DCB_OPCODE_WR   BIT(0)  /* 1 - write, 0 - read */

/**
 * @brief struct hinic5_mt_dcb_state
 * @details 用于存储多任务数据中心桥(DCB)状态信息的结构体
 */
struct hinic5_mt_dcb_state {
    struct mt_msg_head head;    /**< 消息头 */

    u16 op_code; /**< 操作码 0 - get dcb state, 1 - set dcb state */
    u8 state;    /**< 状态 0 - disable,       1 - enable dcb  */
    u8 rsvd;     /**< 保留字段 */
};

#define CMD_QOS_DEV_TRUST     BIT(0)
#define CMD_QOS_DEV_DFT_COS   BIT(1)
#define CMD_QOS_DEV_PCP2COS   BIT(2)
#define CMD_QOS_DEV_DSCP2COS  BIT(3)

/**
 * @brief struct hinic5_mt_qos_dev_cfg
 * @details 用于配置QoS设备的结构体
 */
struct hinic5_mt_qos_dev_cfg {
    struct mt_msg_head head;    /**< 消息头部 */

    u8 op_code;       /**< 0：get 1: set */
    u8 rsvd0;
    u16 cfg_bitmap;   /**< bit0 - trust, bit1 - dft_cos, bit2 - pcp2cos, bit3 - dscp2cos */

    u8 trust;         /**< 0 - pcp, 1 - dscp */
    u8 dft_cos;
    u16 rsvd1;
    u8 pcp2cos[8];    /**< 必须8个一起配置 */
    u8 dscp2cos[64];  /**< 配置dscp2cos时，若cos值设置为0xFF，驱动则忽略此dscp优先级的配置，允许一次性配置多个dscp跟cos的映射关系 */
    u32 rsvd2[4];
};

/**
 * @brief struct hinic5_mt_qos_cos_cfg
 * @details 用于配置HINIC5多队列服务质量（QoS）类别设置的结构体
 */
struct hinic5_mt_qos_cos_cfg {
    struct mt_msg_head head;    /**< 消息头部，包含了消息的类型和长度等信息 */

    u8 port_id;                 /**< 端口ID，用于标识消息所属的端口 */
    u8 func_cos_bitmap;         /**< 功能类别位图，用于标识各个功能类别的使能状态 */
    u8 port_cos_bitmap;         /**< 端口类别位图，用于标识各个端口类别的使能状态 */
    u8 func_max_cos_num;        /**< 功能类别的最大数量，用于限制功能类别的数量 */
    u32 rsvd2[4];
};

enum nic_driver_cmd_type {
    NIC_TOOL_CMD_START = 0x120, /**< 平台新增命令字从0x120开始，旧命令字统一定义在@driver_cmd_type */

    /* MACsec 工具命令集 */
    MACSEC_TOOL_OP_LIST = 0x120, /**< 获取驱动内存中所有 macsec 配置信息 */
    MACSEC_TOOL_OP_DUMP,         /**< 获取芯片侧所有 macsec 配置信息 */
    MACSEC_TOOL_OP_MIB,          /**< 获取芯片侧 SC MIB 信息或者 PORT MIB 信息 */
    MACSEC_TOOL_OP_ADD,          /**< 新增 SC 或 SA 配置 */
    MACSEC_TOOL_OP_DEL,          /**< 删除 SC 或 SA 配置 */
    MACSEC_TOOL_OP_SET,          /**< 修改 SC 配置 */
    MACSEC_TOOL_OP_FLUSH,        /**< 清除某个设备管理的 macsec 配置 */
    MACSEC_TOOL_OP_MAX = 0x12F,

    NIC_CMD_EXTEND_RSV_START = 0x200,
    /* NIC工具保留命令字，产品使用该范围内的命令字 */
    NIC_CMD_EXTEND_RSV_END = 0x2FF,
};

#endif /* NIC_PUB_CMD_H */