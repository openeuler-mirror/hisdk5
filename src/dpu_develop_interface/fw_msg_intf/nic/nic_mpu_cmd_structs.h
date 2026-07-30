/*
 * mailbox命令字结构体定义文件
 * 头文件通过MAILBOX基线表格中的脚本自动生成。如有增删，请先修改基线表格，然后自动生成该头文件
 * 修改流程见：https://wiki.huawei.com/domains/1574/wiki/8/WIKI202509298479069
 */

#ifndef HINIC5_NIC_CMD_STRUCTS_H
#define HINIC5_NIC_CMD_STRUCTS_H

#if defined(__LINUX__) || defined(__VMWARE__)
#include <linux/types.h>
#endif

#include "mpu_cmd_base_defs.h"

/**
 * @brief 管理消息头部结构体
 * @details 该结构体用于存储管理消息的头部信息，包括状态、版本和保留字段。
 */
struct hinic5_mgmt_msg_head {
    u8 status;     /**< 状态字段 */
    u8 version;     /**< 版本字段 */
    u8 rsvd0[6];     /**< 保留域段 */
};

/**
 * @brief struct hinic5_cmd_register_vf
 * @details nic vf register
 */
struct hinic5_cmd_register_vf {
    struct hinic5_mgmt_msg_head msg_head;     /**< 命令字消息头 */
    u8 op_register;     /**< 0 - unregister, 1 - register */
    u8 rsvd1[3];     /**< 保留域段 */
    u32 support_extra_feature;     /**< support extra feature */
    u8 rsvd2[32];     /**< 保留域段 */
};

/**
 * @brief 存储函数表的配置信息
 * @details 该结构体包含了接收队列的WQE缓冲区大小、最大传输单元（MTU）以及保留字段。
 */
struct hinic5_func_tbl_cfg {
    u16 rx_wqe_buf_size;     /**< 接收队列的WQE缓冲区大小 */
    u16 mtu;     /**< 最大传输单元（MTU） */
    u8 rx_compact_wqe_en;     /**< rx 8Byte wqe(合一cqe)使能 */
    u8 rsvd0[3];     /**< 保留域段 */
    u8 mac[6];
    u16 vlan_id;
    u32 rsvd1[6];     /**< 保留域段 */
};

/**
 * @brief 用于设置函数表的结构体
 * @details 该结构体用于设置函数表，包含了管理消息头部、函数ID、保留字段、配置位图和函数表配置等信息。
 */
struct hinic5_cmd_set_func_tbl {
    struct hinic5_mgmt_msg_head msg_head;     /**< 命令字消息头 */
    u16 func_id;     /**< Func ID */
    u16 rsvd;     /**< 保留域段 */
    u32 cfg_bitmap;     /**< 设置位图，由hinic5_func_tbl_cfg_bitmap定义 */
    struct hinic5_func_tbl_cfg tbl_cfg;     /**< 配置表 */
};

/**
 * @brief 可配置的func级的属性
 * @details nic vport state info
 */
struct hinic5_vport_state {
    struct hinic5_mgmt_msg_head msg_head;     /**< 命令字消息头 */
    u16 func_id;     /**< Func ID */
    u16 rsvd1;     /**< 保留域段 */
    u8 state;     /**< 0--disable, 1--enable */
    u8 num_qps;     /**< queue pairs number */
    u8 rx_compact_wqe_en;
    u8 rsvd2;     /**< 保留域段 */
};

/**
 * @brief 可配置的func级的属性
 * @details nic 配置rx_mode命令结构体
 */
struct hinic5_rx_mode_config {
    struct hinic5_mgmt_msg_head msg_head;     /**< 命令字消息头 */
    u16 func_id;     /**< Func ID */
    u16 rsvd1;     /**< 保留域段 */
    u32 rx_mode;     /**< rx mode */
};

/**
 * @brief 可配置的func级的属性
 * @details nic 设置 cons idx attr
 */
struct hinic5_cmd_cons_idx_attr {
    struct hinic5_mgmt_msg_head msg_head;     /**< 命令字消息头 */
    u16 func_idx;     /**< Func ID */
    u8 dma_attr_off;     /**< dma attribute offset */
    u8 pending_limit;     /**< pending limit */
    u8 coalescing_time;     /**< coalescing time */
    u8 intr_en;     /**< interrupt enable */
    u16 intr_idx;     /**< interrupt index */
    u32 l2nic_sqn;     /**< l2nic sequence number */
    u32 rsvd1;     /**< 保留域段 */
    u64 ci_addr;     /**< ci address */
};

/**
 * @brief 定义了一个用于存储端口统计信息的结构体
 * @details 该结构体包含了管理消息头部和两个字段，用于存储端口统计信息。
 */
struct hinic5_port_stats_info {
    struct hinic5_mgmt_msg_head msg_head;     /**< 命令字消息头 */
    u16 func_id;     /**< Func ID */
    u16 rsvd1;     /**< 保留域段 */
};

/**
 * @brief 定义了一个结构体，用于存储vport的统计信息
 * @details vport的发送和接收统计信息，包括单播、多播、广播包的数量和字节数，以及发送和接收被丢弃和出现错误的包数。
 */
struct hinic5_vport_stats {
    u64 tx_unicast_pkts_vport;     /**< 发送的单播包数 */
    u64 tx_unicast_bytes_vport;     /**< 发送的单播字节数 */
    u64 tx_multicast_pkts_vport;     /**< 发送的多播包数 */
    u64 tx_multicast_bytes_vport;     /**< 发送的多播字节数 */
    u64 tx_broadcast_pkts_vport;     /**< 发送的广播包数 */
    u64 tx_broadcast_bytes_vport;     /**< 发送的广播字节数 */
    u64 rx_unicast_pkts_vport;     /**< 接收的单播包数 */
    u64 rx_unicast_bytes_vport;     /**< 接收的单播字节数 */
    u64 rx_multicast_pkts_vport;     /**< 接收的多播包数 */
    u64 rx_multicast_bytes_vport;     /**< 接收的多播字节数 */
    u64 rx_broadcast_pkts_vport;     /**< 接收的广播包数 */
    u64 rx_broadcast_bytes_vport;     /**< 接收的广播字节数 */
    u64 tx_discard_vport;     /**< 发送被丢弃的包数 */
    u64 rx_discard_vport;     /**< 接收被丢弃的包数 */
    u64 tx_err_vport;     /**< 发送出现错误的包数 */
    u64 rx_err_vport;     /**< 接收出现错误的包数 */
};

/**
 * @brief 定义了一个用于存储vport统计信息的结构体
 * @details 该结构体包含了vport统计信息的相关数据，包括管理消息头部、统计信息的大小、保留字段、vport统计信息以及额外的保留字段。
 */
struct hinic5_cmd_vport_stats {
    struct hinic5_mgmt_msg_head msg_head;     /**< 命令字消息头 */
    u32 stats_size;     /**< 统计信息的大小 */
    u32 rsvd1;     /**< 保留域段 */
    struct hinic5_vport_stats stats;     /**< vport统计信息 */
    u64 rsvd2[6];     /**< 保留域段 */
};

/**
 * @brief hinic5_cmd_clear_qp_resource
 * @details nic 清除qp资源命令结构体
 */
struct hinic5_cmd_clear_qp_resource {
    struct hinic5_mgmt_msg_head msg_head;     /**< 命令字消息头 */
    u16 func_id;     /**< Func ID */
    u16 rsvd1;     /**< 保留域段 */
};

/**
 * @brief hinic5_cmd_lro_config
 * @details nic 配置lro结构体
 */
struct hinic5_cmd_lro_config {
    struct hinic5_mgmt_msg_head msg_head;     /**< 命令字消息头 */
    u16 func_id;     /**< Func ID */
    u8 opcode;     /**< operation code */
    u8 rsvd1;     /**< 保留域段 */
    u8 lro_ipv4_en;     /**< LRO for IPv4 */
    u8 lro_ipv6_en;     /**< LRO for IPv6 */
    u8 lro_max_pkt_len;     /**< unit is 1K */
    u8 resv2[13];     /**< 保留域段 */
};

/**
 * @brief hinic5_cmd_lro_timer
 * @details nic 配置lro timer
 */
struct hinic5_cmd_lro_timer {
    struct hinic5_mgmt_msg_head msg_head;     /**< 命令字消息头 */
    u8 opcode;     /**< 1: set timer value, 0: get timer value */
    u8 rsvd1;     /**< 保留域段 */
    u16 rsvd2;     /**< 保留域段 */
    u32 timer;     /**< timer value */
};

#define NIC_MAX_FEATURE_QWORD 4

/**
 * @brief hinic5_cmd_feature_nego
 * @details nic自协商特性命令结构体
 */
struct hinic5_cmd_feature_nego {
    struct hinic5_mgmt_msg_head msg_head;     /**< 命令字消息头 */
    u16 func_id;     /**< Func ID */
    u8 opcode;     /**< opeartion code 1: set, 0: get */
    u8 rsvd;     /**< 保留域段 */
    u64 s_feature[NIC_MAX_FEATURE_QWORD];     /**< feature */
};

/**
 * @brief hinic5_cmd_local_lro_state
 * @details nic 设置lro state
 */
struct hinic5_cmd_local_lro_state {
    struct hinic5_mgmt_msg_head msg_head;     /**< 命令字消息头 */
    u16 func_id;     /**< 1: set timer value, 0: get timer value */
    u8 opcode;     /**< 0: get state, 1: set state */
    u8 state;     /**< 0: disable, 1: enable */
};

/**
 * @brief hinic5_cmd_cache_out_qp_resource
 * @details nic 设置 cache_out_qp_resource
 */
struct hinic5_cmd_cache_out_qp_resource {
    struct hinic5_mgmt_msg_head msg_head;     /**< 命令字消息头 */
    u16 func_id;     /**< Func ID */
    u16 rsvd1;     /**< 保留域段 */
};

/**
 * @brief 定义了一个接收队列完成队列上下文结构体
 * @details 该结构体用于描述接收队列完成队列的上下文信息
 */
struct hinic5_rq_cqe_ctx {
    struct hinic5_mgmt_msg_head msg_head;     /**< 命令字消息头 */
    u8 cqe_type;     /**< 完成队列类型 */
    u8 rq_id;     /**< 接收队列ID */
    u8 threshold_cqe_num;     /**< 完成队列阈值 */
    u8 rsvd1;     /**< 保留域段 */
    u16 msix_entry_idx;     /**< MSI-X入口索引 */
    u16 rsvd2;     /**< 保留域段 */
    u32 ci_addr_hi;     /**< 完成队列地址的高位 */
    u32 ci_addr_lo;     /**< 完成队列地址的低位 */
    u16 timer_loop;     /**< 定时器循环次数 */
    u16 rsvd3;     /**< 保留域段 */
};

struct hinic5_rq_enable {
    struct hinic5_mgmt_msg_head msg_head;     /**< 命令字消息头 */
    u32 rq_id;
    u8 rq_enable;
    u8 rsvd1[3];     /**< 保留域段 */
};

#define ETH_ALEN 6     /**< Ethernet address length */

/**
 * @brief hinic5_port_mac_set
 * @details nic port mac设置命令结构体
 */
struct hinic5_port_mac_set {
    struct hinic5_mgmt_msg_head msg_head;     /**< 命令字消息头 */
    u16 func_id;     /**< Func ID */
    u16 vlan_id;     /**< vlan id */
    u8 vf_lag_en;     /**< vf_lag使能标志位（计算使用，保留） */
    u8 rsvd1;     /**< 保留域段 */
    u8 mac[ETH_ALEN];     /**< mac address */
};

/**
 * @brief hinic5_port_mac_update
 * @details nic port mac修改命令结构体
 */
struct hinic5_port_mac_update {
    struct hinic5_mgmt_msg_head msg_head;     /**< 命令字消息头 */
    u16 func_id;     /**< Func ID */
    u16 vlan_id;     /**< vlan id */
    u16 rsvd1;     /**< 保留域段 */
    u8 old_mac[ETH_ALEN];     /**< mac address */
    u16 rsvd2;     /**< 保留域段 */
    u8 new_mac[ETH_ALEN];     /**< mac address */
};

#define CHIP_ATTR_MAC_MAX_SIZE 192

/**
 * @brief nic_cmd_mac_info
 * @details MAC模块接口
 */
struct nic_cmd_mac_info {
    struct hinic5_mgmt_msg_head head;     /**< 命令字消息头 */
    u32 valid_bitmap;     /**< valid bitmap, 未使用 */
    u16 rsvd1;     /**< 保留域段 */
    u8 host_id[32];     /**< host id, 未使用 */
    u8 port_id[32];     /**< port id, 未使用 */
    u8 mac_addr[CHIP_ATTR_MAC_MAX_SIZE];     /**< mac addr */
};

/**
 * @brief hinic5_cmd_vlan_config
 * @details nic 配置vlan信息
 */
struct hinic5_cmd_vlan_config {
    struct hinic5_mgmt_msg_head msg_head;     /**< 命令字消息头 */
    u16 func_id;     /**< Func ID */
    u8 opcode;     /**< opration code */
    u8 rsvd1;     /**< 保留域段 */
    u16 vlan_id;     /**< vlan id */
    u8 blacklist_flag;     /**< 黑名单标志位（计算使用，保留） */
    u8 rsvd2;     /**< 保留域段 */
};

struct hinic5_cmd_vxlan_port_info {
    struct hinic5_mgmt_msg_head msg_head;   /**< 命令字消息头 */
    u16 func_id;  /**< Func ID */
    u8 opcode;  /**< opration code */
    u8 cfg_mode;  /**< 优先级标志位 */
    u16 vxlan_port;  /**< 目标port */
    u8 pkt_fmt;  /**< 0：vxlan报文  1：vxlan_gpe报文 */
    u8 rsvd2;  /**< 保留域段 */
};

/**
 * @brief hinic5_cmd_set_vlan_filter
 * @details nic set vlan filter
 */
struct hinic5_cmd_set_vlan_filter {
    struct hinic5_mgmt_msg_head msg_head;     /**< 命令字消息头 */
    u16 func_id;     /**< Func ID */
    u8 rsvd1[2];     /**< 保留域段 */
    u32 vlan_filter_ctrl;     /**< bit0:vlan filter en; bit1:broadcast_filter_en */
};

/**
 * @brief hinic5_cmd_vlan_offload
 * @details nic 设置 vlan offload
 */
struct hinic5_cmd_vlan_offload {
    struct hinic5_mgmt_msg_head msg_head;     /**< 命令字消息头 */
    u16 func_id;     /**< Func ID */
    u8 vlan_offload;     /**< vlan offload */
    u8 rsvd1[5];     /**< 保留域段 */
};

struct hinic5_smac_check_state {
    struct hinic5_mgmt_msg_head head;     /**< 命令字消息头 */
    u8 smac_check_en;     /**< 1: enable 0: disable */
    u8 op_code;     /**< 1: set 0: get */
    u8 flash_en;     /**< flash使能标志位 */
    u8 rsvd;     /**< 保留域段 */
};

/**
 * @brief hinic5_cmd_vf_vlan_config
 * @details nic 配置vf的vlan信息
 */
struct hinic5_cmd_vf_vlan_config {
    struct hinic5_mgmt_msg_head msg_head;     /**< 命令字消息头 */
    u16 func_id;     /**< Func ID */
    u8 opcode;     /**< opration code */
    u8 rsvd1;     /**< 保留域段 */
    u16 vlan_id;     /**< vlan id */
    u8 qos;     /**< qos */
    u8 rsvd2[5];     /**< 保留域段 */
};

/**
 * @brief hinic5_cmd_vf_trust_config
 * @details nic 配置vf的trust信息
 */
struct hinic5_cmd_vf_trust_config {
    struct hinic5_mgmt_msg_head msg_head;     /**< 命令字消息头 */
    u16 func_id;    /**< Func ID */
    u16 vlan_id;    /**< vlan id */
    u8 trust;   /* vf_trust: 0-disable; 1-enable */
    u8 rsvd2[67];    /**< 保留域段 */
};

/**
 * @brief hinic5_cmd_spoofchk_set
 * @details nic 配置 spoofchk
 */
struct hinic5_cmd_spoofchk_set {
    struct hinic5_mgmt_msg_head msg_head;     /**< 命令字消息头 */
    u16 func_id;     /**< Func ID */
    u8 state;     /**< state */
    u8 rsvd1;     /**< 保留域段 */
};

#define NIC_RATE_DIRECT_RX_BW 0     /**< RX带宽限速 */
#define NIC_RATE_DIRECT_TX_BW 1     /**< TX带宽限速 */
#define NIC_RATE_DIRECT_RX_PERCENT 2     /**< RX百分比限速 （设置命令1872实现存在差异） 其他部分完全一样 */
#define NIC_RATE_DIRECT_TX_PERCENT 3     /**< TX百分比限速 */
#define NIC_RATE_OP_SET 0     /**< 限速设置 */
#define NIC_RATE_OP_GET 1     /**< 限速查询 */
#define NIC_RATE_OP_FLASH 2     /**< FLASH固化 */
#define NIC_RATE_OP_UNUSE 4     /**< 用于兼容性适配，针对1823（由于不会去读该域段，因此实际会做限速配置的操作），1872/1825等后面代次则实际不做任何操作直接返回 */

/**
 * @brief hinic5_cmd_rate_cfg
 * @details nic rate 速率配置
 */
struct hinic5_cmd_rate_cfg {
    struct hinic5_mgmt_msg_head msg_head;     /**< 命令字消息头 */
    u16 func_id;     /**< Func ID */
    u8 cfg_mode;     /**< BIT[0]：设置和查询, 0-设置, 1代表查询; BIT[1]：是否支持固化到Flash  目前仅支持百分比限速的固化；BIT[2]：若为1表示什么都不操作直接返回 */
    u8 direct;     /**< 方向：  0-RX带宽限速 1-TX带宽限速 2-RX_RATE百分比 3-TX_RATE百分比 */
    u32 cir;     /**< C桶速率 Mbps 旧域段名称min_rate */
    u32 pir;     /**< P桶速率 Mbps 若为带宽限速，则代表C桶速率(Mbps)，若direct为百分比限速，则代表百分比，旧域段名称max_rate */
    u32 cbs;     /**< C桶桶深 Mbit 0则使用默认桶深 */
    u32 pbs;     /**< P桶桶深 Mbit 0则使用默认桶深 */
};

#define NIC_RATE_MODE_PERCENT 0     /**< 按照百分比限速生效. */
#define NIC_RATE_MODE_BANDWIDTH 1     /**< 按照带宽限速生效. */

#define ERR_RATE_BW_PARAM_OVERFLOW 254 /**< Bandwidth rate param overflow．*/
/**
 * @brief hinic5_cmd_rate_cfg_ret
 * @details nic rate 速率配置查询
 */
struct hinic5_cmd_rate_cfg_ret {
    struct hinic5_mgmt_msg_head msg_head;     /**< 命令字消息头 */
    u16 car_id;     /**< RX方向限速需要返回CAR_ID */
    u8 rate_mode;     /**< 当前限速生效的模式：0-百分比限速 1-带宽限速 */
    u8 rsvd;     /**< 保留域段 */
    u32 cir;     /**< C桶速率 Mbps 旧域段名称min_rate */
    u32 pir;     /**< P桶速率 Mbps 若为带宽限速，则代表C桶速率(Mbps)，若direct为百分比限速，则代表百分比，旧域段名称max_rate */
    u32 cbs;     /**< C桶桶深 Mbit 0则使用默认桶深 */
    u32 pbs;     /**< P桶桶深 Mbit 0则使用默认桶深 */
};

#define NIC_DCB_COS_MAX 0x8     /**< 定义网络接口控制数据包(NIC DCB)的最大优先级(COS) */

/**
 * @brief 定义了一个用于RSS配置的结构体
 * @details 该结构体包含了RSS配置所需的各种参数，如函数ID、RSS使能标志、优先级队列数量、优先级与流量类别的映射关系、队列对数量等。
 */
struct hinic5_cmd_rss_config {
    struct hinic5_mgmt_msg_head msg_head;     /**< 命令字消息头 */
    u16 func_id;     /**< 函数ID，用于标识当前配置的是哪个函数的RSS */
    u8 rss_en;     /**< RSS使能标志，如果为1则表示RSS功能开启，0则表示关闭 */
    u8 rq_priority_number;     /**< 优先级队列的数量，决定了RSS的具体实现方式 */
    u8 prio_tc[NIC_DCB_COS_MAX];     /**< 优先级与流量类别的映射关系，数组长度为NIC_DCB_COS_MAX */
    u16 num_qps;     /**< 队列对的数量，决定了RSS的具体实现方式 */
    u16 rsvd1;     /**< 保留域段 */
};

/**
 * @brief 定义了一个RSS（Receive Side Scaling，接收端缩放）模板管理结构体
 * @details 该结构体用于管理RSS模板，包含了管理消息头部、函数ID、命令、模板ID等信息
 */
struct hinic5_rss_template_mgmt {
    struct hinic5_mgmt_msg_head msg_head;     /**< 命令字消息头 */
    u16 func_id;     /**< 函数ID，用于标识当前消息对应的功能模块 */
    u8 cmd;     /**< 命令，用于标识需要执行的操作 */
    u8 template_id;     /**< 模板ID，用于标识当前操作对应的模板 */
    u8 rsvd1[4];     /**< 保留域段 */
};

/**
 * @brief 定义了一个RSS（Receive Side Scaling，接收端缩放）上下文表结构体
 * @details 该结构体包含了rss context表内的相关数据，包含管理消息头部、func id、上下文信息以及保留字段。
 */
struct hinic5_rss_context_table {
    struct hinic5_mgmt_msg_head msg_head;     /**< 命令字消息头 */
    u16 func_id;     /**< 函数ID，用于标识当前消息对应的功能模块 */
    u16 rsvd1;     /**< 保留域段 */
    u32 context;     /**< RSS上下文 */
};

#define NIC_RSS_KEY_SIZE 40     /**< 定义RSS ket大小为40 */

/**
 * @brief 定义了一个用于RSS哈希键的结构体
 * @details 该结构体用于存储RSS哈希键相关信息
 */
struct hinic5_cmd_rss_hash_key {
    struct hinic5_mgmt_msg_head msg_head;     /**< 命令字消息头 */
    u16 func_id;     /**< 函数ID */
    u8 opcode;     /**< 操作码 */
    u8 rsvd1;     /**< 保留域段 */
    u8 key[NIC_RSS_KEY_SIZE];     /**< RSS哈希键 */
};

/**
 * @brief 定义了一个用于RSS引擎类型的结构体
 * @details 该结构体用于描述RSS引擎类型相关的信息
 */
struct hinic5_cmd_rss_engine_type {
    struct hinic5_mgmt_msg_head msg_head;     /**< 命令字消息头 */
    u16 func_id;     /**< 函数ID */
    u8 opcode;     /**< 操作码 */
    u8 hash_engine;     /**< 哈希引擎 */
    u8 rsvd1[4];     /**< 保留域段 */
};

/**
 * @brief hinic5_ipcs_err_rss_enable_operation_s
 * @details IP checksum error packets, enable rss quadruple hash
 */
struct hinic5_ipcs_err_rss_enable_operation_s {
    struct hinic5_mgmt_msg_head head;     /**< 命令字消息头 */
    u8 en_tag;
    u8 type;     /**< 1: set  0: get */
    u8 rsvd[2];     /**< 保留域段 */
};

/**
 * @brief 网络接口命令过滤删除规则的结构体
 * @details 该结构体用于存储网络接口命令过滤删除规则的相关信息。
 */
struct nic_cmd_fdir_del_rules {
    struct hinic5_mgmt_msg_head head;     /**< 命令字消息头 */
    u16 func_id;     /**< 函数ID */
    u8 type;     /**< 类型 */
    u8 rsvusrdata;     /**< 用户数据 */
    u32 index_start;     /**< 索引开始位置 */
    u32 index_num;     /**< 索引数量 */
};

/**
 * @brief 定义了一个用于刷新TCAM规则的结构体
 * @details 该结构体包含了管理消息头部和函数ID两个成员，用于刷新TCAM规则。
 */
struct nic_cmd_flush_tcam_rules {
    struct hinic5_mgmt_msg_head head;     /**< 命令字消息头 */
    u16 func_id;     /**< 函数ID */
    u16 rsvd;     /**< 保留域段 */
};

/**
 * @brief 分配TCAM块的输入结构体
 * @details 该结构体用于分配TCAM块的输入参数，包含头部信息、函数ID、分配标志、TCAM类型、TCAM块索引和分配块数量等信息。
 */
struct nic_cmd_ctrl_tcam_block_in {
    struct hinic5_mgmt_msg_head head;     /**< 命令字消息头 */
    u16 func_id;     /**< 函数ID */
    u8 alloc_en;     /**< 0: 释放分配的tcam block, 1: 申请新的tcam block */
    u8 tcam_type;     /**< 0: 分配16 size 的tcam block, 1: 分配0 size的tcam block, 其他预留 */
    u16 tcam_block_index;     /**< index */
    u16 alloc_block_num;     /**< 驱动发给uP表示驱动希望分配的block大小。uP返回给驱动的接口，表示uP 支持的分配的tcam block大小 */
};

/**
 * @brief 分配TCAM块输出结构体
 * @details 该结构体用于存储分配TCAM块的输出信息。
 */
struct nic_cmd_ctrl_tcam_block_out {
    struct hinic5_mgmt_msg_head head;     /**< 命令字消息头 */
    u16 func_id;     /**< 函数ID */
    u8 alloc_en;     /**< 0: 释放分配的tcam block, 1: 申请新的tcam block */
    u8 tcam_type;     /**< 0: 分配16 size 的tcam block, 1: 分配0 size的tcam block, 其他预留 */
    u16 tcam_block_index;     /**< index */
    u16 mpu_alloc_block_size;     /**< 驱动发给uP表示驱动希望分配的block大小，uP返回给驱动的接口，表示uP 支持的分配的tcam block大小 */
};

/**
 * @brief nic_cmd_set_tcam_enable
 * @details enable tcam
 */
struct nic_cmd_set_tcam_enable {
    struct hinic5_mgmt_msg_head head;     /**< 命令字消息头 */
    u16 func_id;     /**< 函数ID */
    u8 tcam_enable;     /**< tcam enable */
    u8 rsvd1;     /**< 保留域段 */
    u32 rsvd2;     /**< 保留域段 */
};

/**
 * @brief 分配TCAM块的输入结构体
 * @details 该结构体用于分配TCAM块的输入参数，包含头部信息、函数ID、分配标志、TCAM类型、TCAM块索引和分配块数量等信息。
 */
struct nic_cmd_dfx_fdir_tcam_block_table {
    struct hinic5_mgmt_msg_head head;     /**< 命令字消息头 */
    u8 tcam_type;     /**< TCAM类型 */
    u8 valid;     /**< 有效标志位 */
    u16 tcam_block_index;     /**< TCAM块索引 */
    u16 use_function_id;     /**< 使用的函数ID */
    u16 rsvd;     /**< 保留域段 */
};

struct hinic5_ppa_cfg_table_id_cmd {
    struct hinic5_mgmt_msg_head msg_head;     /**< 命令字消息头 */
    u16 rsvd0;     /**< 保留域段 */
    u16 cmd;     /**< command */
    u16 table_id;     /**< table id */
    u16 rsvd1;     /**< 保留域段 */
};

struct hinic5_ppa_cfg_ppa_en_cmd {
    struct hinic5_mgmt_msg_head msg_head;     /**< 命令字消息头 */
    u16 func_id;     /**< function id */
    u8 ppa_en;     /**< ppa enable */
    u8 rsvd;     /**< 保留域段 */
};

struct hinic5_ppa_cfg_mode_cmd {
    struct hinic5_mgmt_msg_head msg_head;     /**< 命令字消息头 */
    u16 rsvd0;     /**< function id */
    u8 ppa_mode;     /**< ppa mode */
    u8 qpc_func_nums;     /**< qpc function numbers */
    u16 base_qpc_func_id;     /**< base qpc function id */
    u16 rsvd1;     /**< 保留域段 */
};

struct hinic5_ppa_flush_en_cmd {
    struct hinic5_mgmt_msg_head msg_head;     /**< 命令字消息头 */
    u16 rsvd0;     /**< 保留域段 */
    u8 flush_en;     /**< 0 flush done, 1 in flush operation */
    u8 rsvd1;     /**< 保留域段 */
};

struct nic_cmd_set_fdir_status {
    struct hinic5_mgmt_msg_head head;     /**< 命令字消息头 */
    u16 func_id;     /**< function id */
    u16 index;     /**< index */
    u8 pkt_type_en;     /**< packet type enable */
    u8 pkt_type;     /**< packet type */
    u8 qid;     /**< queue id */
    u8 flag;     /**< packet drop flag */
};

struct hinic5_ppa_fdir_query_cmd {
    struct hinic5_mgmt_msg_head msg_head;     /**< 命令字消息头 */
    u32 index;     /**< index */
    u32 rsvd;     /**< 保留域段 */
    u64 pkt_nums;     /**< packet type */
    u64 pkt_bytes;     /**< packet bytes */
};

struct hinic5_port_state {
    struct hinic5_mgmt_msg_head msg_head;     /**< 命令字消息头 */
    u16 func_id;     /**< function id */
    u16 rsvd1;     /**< 保留域段 */
    u8 state;     /**< 0--disable, 1--enable */
    u8 rsvd2[3];     /**< 保留域段 */
};

struct hinic5_cmd_pause_config {
    struct hinic5_mgmt_msg_head msg_head;     /**< 命令字消息头 */
    u8 port_id;     /**< port id */
    u8 opcode;     /**< operation code */
    u16 rsvd1;     /**< 保留域段 */
    u8 auto_neg;     /**< auto negotiation */
    u8 rx_pause;     /**< receive pause */
    u8 tx_pause;     /**< send pause */
    u8 rsvd2[5];     /**< 保留域段 */
};

struct hinic5_port_car_info {
    u32 cir;     /**< unit: kbps, range:[1,400*1000*1000], i.e. 1Kbps~400Gbps(400M*kbps) */
    u32 xir;     /**< unit: kbps, range:[1,400*1000*1000], i.e. 1Kbps~400Gbps(400M*kbps) */
    u32 cbs;     /**< unit: Byte, range:[1,320*1000*1000], i.e. 1byte~2560Mbit */
    u32 xbs;     /**< unit: Byte, range:[1,320*1000*1000], i.e. 1byte~2560Mbit */
};

/**
 * @brief hinic5_cmd_set_port_car
 * @details nic 设置car限速命令结构体(1825)
 */
struct hinic5_cmd_set_port_car {
    struct hinic5_mgmt_msg_head msg_head;     /**< 命令字消息头 */
    u8 port_id;     /**< port id */
    u8 opcode;     /**< 0--set car profile, 1--set car state, 2--get car profile */
    u8 state;     /**< 0--disable, 1--enable */
    u8 level;     /**< limit level */
    struct hinic5_port_car_info car;     /**< car info */
};

struct hinic5_cmd_set_car_option {
    u8 type;     /**< 0:port, 1:func, 2:vnic group*/
    u8 port_id;
    u16 func_id;
    u8 car_enable;
    u8 opcode;     /**< 0--set car profile, 1--get car profile */
    u8 pkt_type;
    u8 car_alg_type;
};

/**
 * @brief hinic5_cmd_set_car
 * @details nic 设置car限速命令结构体(1872)
 */
struct hinic5_cmd_set_car {
    struct hinic5_mgmt_msg_head msg_head;     /**< 命令字消息头 */
    struct hinic5_cmd_set_car_option option;
    struct hinic5_port_car_info car;     /**< car info */
};

/**
 * @brief hinic5_car_profile
 * @details nic 设置car限速命令结构体(1872)
 */
struct hinic5_car_profile {
    struct hinic5_mgmt_msg_head msg_head;     /**< 命令字消息头 */
    u16 car_id;     /**< car id */
    u8 level;     /**< limit level */
    u8 rsvd;     /**< 保留域段 */
    u32 profile[4];     /**< car profile */
};

#define NIC_DCB_TC_MAX 0x8     /**< 定义网络接口控制数据包(NIC DCB)的最大流量类别(TC) */

/**
 * @brief hinic5_cmd_ets_cfg
 * @details nic 配置ets命令结构体
 */
struct hinic5_cmd_ets_cfg {
    struct hinic5_mgmt_msg_head head;     /**< 命令字消息头 */
    u8 port_id;     /**< port id */
    u8 op_code;     /**< 1 - set, 0 - get */
    u8 cfg_bitmap;     /**< bit0 - cos_tc, bit1 - tc_bw, bit2 - cos_prio, bit3 - cos_bw, bit4 - tc_prio */
    u8 rsvd;     /**< 保留域段 */
    u8 cos_tc[NIC_DCB_COS_MAX];
    u8 tc_bw[NIC_DCB_TC_MAX];
    u8 cos_prio[NIC_DCB_COS_MAX];     /**< 0 - DWRR, 1 - STRICT */
    u8 cos_bw[NIC_DCB_COS_MAX];
    u8 tc_prio[NIC_DCB_TC_MAX];     /**< 0 - DWRR, 1 - STRICT */
};

struct hinic5_cmd_set_pfc {
    struct hinic5_mgmt_msg_head head;     /**< 命令字消息头 */
    u8 port_id;     /**< port id */
    u8 op_code;     /**< 0：get 1: set pfc_en  2: set pfc_bitmap 3: set all */
    u8 pfc_en;     /**< pfc_en 和 pfc_bitmap 必须同时设置 */
    u8 pfc_bitmap;     /**< pfc bitmap */
    u8 rsvd[4];     /**< 保留域段 */
};

/**
 * @brief hinic5_cmd_set_dcb_state
 * @details nic 设置dcb state命令结构体
 */
struct hinic5_cmd_set_dcb_state {
    struct hinic5_mgmt_msg_head head;     /**< 命令字消息头 */
    u16 func_id;     /**< port id */
    u8 op_code;     /**< 0 - get dcb state, 1 - set dcb state */
    u8 state;     /**< 0 - disable, 1 - enable dcb */
    u8 port_state;     /**< 0 - disable, 1 - enable dcb */
    u8 rsvd[7];     /**< 保留域段 */
};

struct hinic5_cmd_qos_port_cfg {
    struct hinic5_mgmt_msg_head head;     /**< 命令字消息头 */
    u8 port_id;     /**< port id */
    u8 op_code;     /**< 0 - get, 1 - set */
    u8 cfg_bitmap;     /**< bit0 - trust, bit1 - dft_cos */
    u8 rsvd0;     /**< 保留域段 */
    u8 trust;     /**< trust state */
    u8 dft_cos;     /**< dft cos */
    u8 rsvd1[18];     /**< 保留域段 */
};

struct hinic5_cmd_qos_map_cfg {
    struct hinic5_mgmt_msg_head head;     /**< 命令字消息头 */
    u8 op_code;     /**< operation code */
    u8 cfg_bitmap;     /**< bit0 - pcp2cos, bit1 - dscp2cos */
    u16 rsvd0;     /**< 保留域段 */
    u8 pcp2cos[8];     /**< 必须8个一起配置 */
    u8 dscp2cos[64];     /**< 配置dscp2cos时，若cos值设置为0xFF，MPU则忽略此dscp优先级的配置，允许一次性配置多个dscp跟cos的映射关系 */
    u32 rsvd1[4];     /**< 保留域段 */
};

/**
 * @brief hinic5_cmd_qos_extend_cfg
 * @details nic 配置qos命令扩展结构体
 */
struct hinic5_cmd_qos_extend_cfg {
    struct hinic5_mgmt_msg_head head;     /**< 命令字消息头 */
    u8 port_id;     /**< port id */
    u8 op_code;     /**< bit0: 1-set tc rate limit, 0-get tc rate limit */
    u16 rsvd0;     /**< 保留域段 */
    u16 port_speed;     /**< 端口速率, 单位Gbps */
    u16 rsvd1;     /**< 保留域段 */
    u32 rate_limit[NIC_DCB_COS_MAX];     /**< tc限速速率值, 单位Mbps */
    u32 port_cir; /**< port的cir值, 单位Mbps  */
    u32 rsvd2[115];     /**< 保留域段 */
};

/**
 * @brief hinic5_force_pkt_drop
 * @details nic force pkt drop命令结构体
 */
struct hinic5_force_pkt_drop {
    struct hinic5_mgmt_msg_head msg_head;     /**< 命令字消息头 */
    u8 port;     /**< port id */
    u8 rsvd1[3];     /**< 保留域段 */
};

/**
 * @brief nic_cmd_pause_inquiry_cfg
 * @details pfc风暴检测配置
 */
struct nic_cmd_pause_inquiry_cfg {
    struct hinic5_mgmt_msg_head head;     /**< 命令字消息头 */
    u32 valid;     /**< valid */
    u32 type;     /**< 1: set, 2: get */
    u32 rx_inquiry_pause_drop_pkts_en;     /**< rx 卸包使能 */
    u32 rx_inquiry_pause_period_ms;     /**< rx pause 检测周期 默认 200ms */
    u32 rx_inquiry_pause_times;     /**< rx pause 检测次数 默认1次 */
    u32 rx_inquiry_pause_frame_thd;     /**< rx pause 检测阈值 默认 PAUSE_FRAME_THD_10G/25G/40G/100 */
    u32 rx_inquiry_tx_total_pkts;     /**< rx pause 检测tx收包总数 */
    u32 tx_inquiry_pause_en;     /**< tx pause 检测使能 */
    u32 tx_inquiry_pause_period_ms;     /**< tx pause 检测周期 默认 200ms */
    u32 tx_inquiry_pause_times;     /**< tx pause 检测次数 默认 5次 */
    u32 tx_inquiry_pause_frame_thd;     /**< tx pause 检测阈值 */
    u32 tx_inquiry_rx_total_pkts;     /**< tx pause 检测rx收包总数 */
    u32 rsvd[4];     /**< 保留域段 */
};

struct nic_bios_cfg {
    u32 signature;     /**< 签名，用于判断FLASH的内容合法性 */
    u8 pxe_en;     /**< PXE enable: 0 - disable 1 - enable */
    u8 extend_mode;
    u8 rsvd0[2];     /**< 保留域段 */
    u8 pxe_vlan_en;     /**< PXE VLAN enable: 0 - disable 1 - enable */
    u8 pxe_vlan_pri;     /**< PXE VLAN priority: 0-7 */
    u16 pxe_vlan_id;     /**< PXE VLAN ID 1-4094 */
    u32 service_mode;     /**< 参考CHIPIF_SERVICE_MODE_x 宏 */
    u32 pf_bw;     /**< PF速率，百分比 0-100 */
    u8 speed;     /**< enum of port speed */
    u8 auto_neg;     /**< 自协商开关 0 - 字段无效 1 - 开2 - 关 */
    u8 lanes;     /**< lane num */
    u8 fec;     /**< FEC模式, 参考 enum mag_cmd_port_fec */
    u8 auto_adapt;     /**< 自适应模式配置0 - 无效配置 1 - 开启 2 - 关闭 */
    u8 func_valid;     /**< 指示func_id是否有效; 0 - 无效，other - 有效 */
    u8 func_id;     /**< 当func_valid不为0时，该成员才有意义 */
    u8 sriov_en;     /**< SRIOV-EN: 0 - 无效配置， 1 - 开启， 2 - 关闭 */
};

struct nic_cmd_bios_cfg {
    struct hinic5_mgmt_msg_head head;     /**< 命令字消息头 */
    u32 op_code;     /**< Operation Code: Bit0[0: read 1:write, BIT1-6: cfg_mask */
    struct nic_bios_cfg bios_cfg;     /**< BIOS configuration */
};

/**
 * @brief 定义了一个用于存储半卸载bond func link状态信息的结构体
 * @details 半卸载bond创建/更新/删除会给非slave的func发送link消息
 */
struct hinic5_bond_link_info {
    struct hinic5_mgmt_msg_head head;     /**< 管理消息头部信息 */
    u8 bond_en;     /**< bond 是否开启 0:关闭 1:开启 */
    u8 port_id;     /**< port id */
    u8 link_status;     /**< bond的非slave设备link状态 0:down 1:up */
    u8 rsvd[13];     /**< 保留域段 */
};

/**
 * @brief 发生PN超阈值中断时, 内部解析信息结构体
 * @details 包含阈值、SC索引、AN等信息
 */
struct macsec_pn_expired_report_info {
    u64 sci[32];     /**< 发生超阈值事件对应SC的SCI */
    u8 an[32];     /**< 发生超阈值事件对应SA的AN号 */
    u8 pn_expired_size;     /**< SA阈值 */
    u8 reserved[7];     /**< 保留域段 */
};

/**
 * @brief macsec pn阈值上报消息结构体
 * @details 发生PN超阈值中断时, 向驱动上报消息结构体定义
 */
struct macsec_pn_expired_report_cmd {
    struct hinic5_mgmt_msg_head head;     /**< 管理消息头部信息 */
    struct macsec_pn_expired_report_info info;     /**< 超阈值相关结构体信息 */
    u64 reserved;     /**< 保留域段 */
};

/**
 * @brief 定义了一个用于删除bond的结构体
 * @details 该结构体包含了删除bond所需的所有信息
 */
struct hinic5_cmd_delete_bond {
    struct hinic5_mgmt_msg_head head;     /**< 命令字消息头 */
    u32 bond_id;     /**< bond的ID */
    u32 rsvd[2];     /**< 保留域段 */
};

/**
 * @brief 定义一个结构体，用于存储bond设备的开启/关闭信息
 * @details 该结构体包含了bond设备的开启/关闭信息，包括bond设备号、开启/关闭标识和保留字段。
 */
struct hinic5_open_close_bond_info {
    u32 bond_id;     /**< bond设备号 */
    u32 open_close_flag;     /**< 开启/关闭bond标识:1为open, 0为close */
    u32 rsvd[2];     /**< 保留域段 */
};

/**
 * @brief 定义了一个结构体，用于存储MPU bond的消息接口的相关信息
 * @details 该结构体包含了管理消息头部的信息和MPU bond的开启或关闭的相关信息
 */
struct hinic5_cmd_open_close_bond {
    struct hinic5_mgmt_msg_head head;     /**< 命令字消息头 */
    struct hinic5_open_close_bond_info open_close_bond_info;     /**< 存储MPU bond的开启或关闭的相关信息 */
};

/**
 * @brief LACPDU的port相关字段
 * @details 该结构体用于描述LACPDU（链接聚合控制协议数据单元）中的port相关参数。
 */
struct lacp_port_params {
    u16 port_number;     /**< 端口号 */
    u16 port_priority;     /**< 端口优先级 */
    u16 key;     /**< 密钥 */
    u16 system_priority;     /**< 系统优先级 */
    u8 system[ETH_ALEN];     /**< 系统MAC地址 */
    u8 port_state;     /**< 端口状态 */
    u8 rsvd;     /**< 保留域段 */
};

/**
 * @brief 定义了一个结构体，用于存储LACP（链接聚合控制协议）端口的信息
 * @details 该结构体包含了多个成员，每个成员都有其特定的含义和用途。
 */
struct lacp_port_info {
    u32 selected;     /**< 表示该端口是否被选中 */
    u32 aggregator_port_id;     /**< 使用的 aggregator port ID */
    struct lacp_port_params actor;     /**< actor port参数 */
    struct lacp_port_params partner;     /**< partner port参数 */
    u64 tx_lacp_pkts;     /**< 发送的LACP数据包数量 */
    u64 rx_lacp_pkts;     /**< 接收的LACP数据包数量 */
    u64 rx_8023ad_drop;     /**< 丢弃的802.3ad数据包数量 */
    u64 tx_8023ad_drop;     /**< 发送的802.3ad数据包数量 */
    u64 unknown_pkt_drop;     /**< 丢弃的未知数据包数量 */
    u64 rx_marker_pkts;     /**< 接收的marker数据包数量 */
    u64 tx_marker_pkts;     /**< 发送的marker数据包数量 */
};

#define BOND_MAX_PORT_NUM 4     /**< bond 支持最大port数量 */
#define BOND_MAX_HOST_NUM 4     /**< bond 支持最大host数量 */

/**
 * @brief 定义了一个用于存储bond状态信息的结构体
 * @details 该结构体包含了bond状态的各种信息，如bond_id、链路状态、slave port状态、port个数等，并且还包含了每个port的lacp信息，以及每个host成功和失败上报lacp协商结果的次数等。
 */
struct hinic5_bond_status_info {
    struct hinic5_mgmt_msg_head head;     /**< 命令字消息头 */
    u32 bond_id;     /**< bond id */
    u32 bon_mmi_status;     /**< 该bond子设备的链路状态 */
    u32 active_bitmap;     /**< 该bond子设备的slave port状态 */
    u32 port_count;     /**< 该bond子设备个数 */
    struct lacp_port_info port_info[BOND_MAX_PORT_NUM];     /**< 每个port的lacp信息 */
    u64 success_report_cnt[BOND_MAX_HOST_NUM];     /**< 每个host成功上报lacp协商结果次数 */
    u64 fail_report_cnt[BOND_MAX_HOST_NUM];     /**< 每个host上报lacp协商结果失败次数 */
    u64 poll_timeout;     /**< 轮询超时时间 */
    u64 fast_periodic_timeout;     /**< 快速周期性超时时间 */
    u64 slow_periodic_timeout;     /**< 慢速周期性超时时间 */
    u64 short_timeout;     /**< 短超时时间 */
    u64 long_timeout;     /**< 长超时时间 */
    u64 aggregate_wait_timeout;     /**< 聚合等待超时时间 */
    u64 tx_period_timeout;     /**< 发送周期超时时间 */
    u64 rx_marker_timer;     /**< RX标记定时器 */
    u8 bond_mode;     /**< bond模式 */
    u8 arp_dual_en;     /**< arp双发使能标记 */
    u8 rsvd[6];     /**< 保留域段 */
};

/**
 * @brief 定义了一个用于存储bond活动报告信息的结构体
 * @details 该结构体包含了bond活动报告的相关信息，如管理消息头部信息、bond设备的ID、bond子设备的链路状态、bond子设备的slave port状态等。
 */
struct hinic5_bond_active_report_info {
    struct hinic5_mgmt_msg_head head;     /**< 命令字消息头 */
    u32 bond_id;     /**< bond的ID */
    u32 bon_mmi_status;     /**< bond子设备的链路状态 */
    u32 active_bitmap;     /**< bond子设备的slave port状态 */
    u8 rsvd[16];     /**< 保留域段 */
};

#define DFX_SM_TBL_BUF_MAX 768     /**< 定义DFX分配的SM表最多为768B */
#define MAC_TBL_RD_TYPE_MAC_INFO 0     /**< 标识获取数据类型为MAC表内容 */
#define MAC_TBL_RD_TYPE_MAC_RES_STAT 1     /**< 标识获取数据类型为MAC资源统计 */

/**
 * @brief MAC表参数
 * @details 该结构体用于存储MAC表参数，包括tbl_index、cnt和total_cnt。
 */
struct mac_table_arg {
    u32 tbl_index;     /**< MAC表索引 */
    u32 cnt;     /**< 计数 */
    u32 total_cnt;     /**< 总计数 */
    u8 mac_tbl_rd_type;     /**< 0-读取MAC内容, 1-读取MAC表资源统计 */
    u8 rsvd[3];     /**< 保留字段 */
};

/**
 * @brief VLAN电路表参数
 * @details 该结构体用于存储VLAN电路表参数，包括er_id和vlan_id。
 */
struct vlan_elb_table_arg {
    u32 er_id;     /**< er ID */
    u32 vlan_id;     /**< VLAN ID */
};

/**
 * @brief VLAN过滤参数(VLAN Filter1 Table)
 * @details 该结构体用于存储VLAN过滤参数，包括tbl_index和func_id。
 */
struct vlan_filter_arg {
    u32 tbl_index;     /**< VLAN Filter1表索引 */
    u32 func_id;     /**< 功能ID */
};

/**
 * @brief VLAN过滤参数(VLAN Filter2 Table)
 * @details 该结构体用于存储VLAN过滤参数，包括vlan_id。
 */
struct vlan_filter2_arg {
    u32 vlan_id;     /**< VLAN ID */
};

/**
 * @brief 多播电路表参数
 * @details 该结构体用于存储多播电路表参数，包括mc_id。
 */
struct mc_elb_arg {
    u32 mc_id;     /**< 多播ID */
};

/**
 * @brief 功能表参数
 * @details 该结构体用于存储功能表参数，包括func_id。
 */
struct func_tbl_arg {
    u32 func_id;     /**< 功能ID */
};

/**
 * @brief 端口表参数
 * @details 该结构体用于存储端口表参数，包括port_id。
 */
struct port_tbl_arg {
    u32 port_id;     /**< 端口ID */
};

/**
 * @brief 该结构体用于存储FDIR IO表参数，包括tbl_index、cnt和total_cnt。
 * @details 该结构体用于存储端口表参数，包括port_id。
 */
struct fdir_io_table_arg {
    u32 tbl_index;     /**< FDIR IO表索引 */
    u32 cnt;     /**< 计数 */
    u32 total_cnt;     /**< 总计数 */
};

/**
 * @brief FlexQ表参数
 * @details 该结构体用于存储FlexQ表参数，包括tbl_index、cnt、total_cnt、left_cnt_die0和left_cnt_die1。
 */
struct flexq_table_arg {
    u32 tbl_index;     /**< FlexQ表索引 */
    u32 cnt;     /**< 计数 */
    u32 total_cnt;     /**< 总计数 */
    u16 left_cnt_die0;     /**< 剩余计数DIE0 */
    u16 left_cnt_die1;     /**< 剩余计数DIE1 */
};

/**
 * @brief 定义一个联合体，用于存储各种类型的参数
 * @details 该联合体包含了多个结构体，每个结构体代表不同的参数类型。这些参数类型包括MAC表参数、VLAN电路表参数、VLAN过滤参数、多播电路表参数、功能表参数、端口表参数、FDIR IO表参数和FlexQ表参数。每个参数类型都有其特定的成员变量，如tbl_index、cnt、total_cnt等。
 */
typedef union {
    struct mac_table_arg mac_table_arg;
    struct vlan_elb_table_arg vlan_elb_table_arg;
    struct vlan_filter_arg vlan_filter_arg;
    struct vlan_filter2_arg vlan_filter2_arg;
    struct mc_elb_arg mc_elb_arg;
    struct func_tbl_arg func_tbl_arg;
    struct port_tbl_arg port_tbl_arg;
    struct fdir_io_table_arg fdir_io_table_arg;
    struct flexq_table_arg flexq_table_arg;
    u32 args[4];     /**< 参数数组 */
} sm_tbl_args;

/**
 * @brief 定义了一个网络接口控制器命令的数据分析表结构体
 * @details 该结构体用于存储网络接口控制器命令的数据分析表相关信息
 */
struct nic_cmd_dfx_sm_table {
    struct hinic5_mgmt_msg_head msg_head;     /**< 命令字消息头 */
    u32 tbl_type;     /**< 表类型，用于标识表的用途 */
    sm_tbl_args args;     /**< 表参数，用于传递表的相关参数 */
    u8 tbl_buf[DFX_SM_TBL_BUF_MAX];     /**< 表缓冲区，用于存储表的数据 */
};

#define MAC_TBL_UC_CNT_FUNC_RD_NUM 256     /**< 单次命令最多读取256个Function的单播统计 */

/**
 * @brief 定义了一个MAC表项共享数据资源的结构体（用于转换nic_cmd_dfx_sm_table结构体的tbl_buf数据）
 * @details 该结构体用于存储单播、组播、共享资源，单播独占资源的使用情况
 */
typedef struct nic_cmd_dfx_mac_res_stats_info {
    u16 uc_mac_cnt;                                     /**< 单播表资源使用统计 */
    u16 mc_mac_cnt;                                     /**< 组播表资源使用统计 */
    u16 share_mac_res_cur_cnt;                          /**< 当前共享资源池使用统计 */
    u16 share_mac_res_total;                            /**< 共享资源池总大小 */
    u16 func_uc_mac_cnt[MAC_TBL_UC_CNT_FUNC_RD_NUM];    /**< Func粒度单播表资源使用统计 */
    u8 rsvd[248];                                       /**< 保留字段 */
} nic_cmd_dfx_mac_res_stats_info_s;

/**
 * @brief mpu_lt_info
 * @details nic liner tabel info
 */
struct mpu_lt_info {
    u8 node;     /**< node id */
    u8 inst;     /**< instance id */
    u8 entry_size;     /**< entry size */
    u8 sml_table_id;     /**< sml table id */
    u32 lt_index;     /**< liner tabel index */
    u32 offset;     /**< offset */
    u32 len;     /**< length */
};

struct nic_mpu_lt_opera {
    struct hinic5_mgmt_msg_head msg_head;     /**< 命令字消息头 */
    struct mpu_lt_info net_lt_cmd;
    u8 data[100];     /**< data */
};

struct hinic5_veb_set {
    struct hinic5_mgmt_msg_head msg_head;   /**< 命令字消息头 */

    u16 opcode;      /**< 操作类型: 0=查询, 1=配置 */
    u16 set_status;  /**< 配置请求值: 0=off, 1=on */
    u16 cur_status;  /**< 当前状态: 0=off, 1=on, 2=error */
    u16 rsvd0;       /**< 保留字段 */
    u32 rsvd[30];    /**< 保留字段 */
};

/**
 * @brief nic_cmd_capture_info
 * @details ucode capture cfg info
 */
struct nic_cmd_capture_info {
    struct hinic5_mgmt_msg_head msg_head;     /**< 命令字消息头 */
    u32 op_type;     /**< operation type */
    u32 func_port;     /**< function port */
    u32 is_en_trx;     /**< 也作为tx_rx */
    u32 offset_cos;     /**< 也作为cos */
    u32 data_vlan;     /**< 也作为vlan */
};

struct nic_cmd_vhd_config {
    struct hinic5_mgmt_msg_head head;     /**< 命令字消息头 */
    u16 func_id;     /**< function id */
    u8 vhd_type;     /**< vhd type */
    u8 virtio_small_enable;     /**< 0: mergeable mode, 1: small mode */
};

#define TC_ACL_KEY_BYTE 44     /**< 320 bits + 2 byte pad align to 4 byte */

/**
 * @brief struct hinic5_tc_action_info
 * @details info about tc action
 */
struct hinic5_tc_action_info {
    u16 action_flag;     /**< action flag */
    u16 output;     /**< output */
    u8 flow_queue;     /**< flow queue */
    u8 vxlan_tbl_index;     /**< vxlan table index */
    u16 vlan_tag;     /**< vlan tag */
    u32 flow_mark;     /**< flow mark */
    u16 vlan_sel;     /**< type of vlan frame */
    u16 count_id;     /**< count id */
};

/**
 * @brief struct hinic5_tc_cfg_info
 * @details info about add/del tc flower rule
 */
struct hinic5_tc_cfg_info {
    struct hinic5_mgmt_msg_head head;     /**< 命令字消息头 */
    u8 key_tcam_mem[TC_ACL_KEY_BYTE];     /**< tcam key mem */
    u8 mask_tcam_mem[TC_ACL_KEY_BYTE];     /**< tcam mask mem */
    struct hinic5_tc_action_info action;     /**< action info */
    u16 opcode;     /**< 0:del, 1:add */
    u16 index;     /**< index id */
    u8 group_vld;     /**< tcam group vld */
    u8 group_id;     /**< tcam group id */
    u16 rsvd;     /**< 保留域段 */
};

#define ACL_LCAM_BITMAP_LEN 32     /**< 320 bits + 2 byte pad align to 4 byte */

/**
 * @brief struct hinic5_tc_flush_info
 * @details info about add/del tc flower rule
 */
struct hinic5_tc_flush_info {
    struct hinic5_mgmt_msg_head head;     /**< 命令字消息头 */
    u64 active_bitmap[ACL_LCAM_BITMAP_LEN];     /**< active bitmap */
};

/**
 * @brief struct hinic5_tc_vxlan_hdr_info
 * @details info about tc vxlan header
 */
struct hinic5_tc_vxlan_hdr_info {
    u8 dmac[6];     /**< destination mac */
    u8 smac[6];     /**< source mac */
    u16 vlan;     /**< vlan tag */
    u8 tos;     /**< type of service */
    u8 rsvd0;     /**< 保留域段 */
    u8 sip[4];     /**< source ip */
    u8 dip[4];     /**< destination ip */
    u16 sport;     /**< source port */
    u16 rsvd1;     /**< 保留域段 */
    u32 vni;     /**< bit[0:23]: vni, bit[24:31]: rsvd */
};

/**
 * @brief struct hinic5_tc_vxlan_tbl_cfg_info
 * @details info about add/get/del tunnel encap vxlan table entry
 */
struct hinic5_tc_vxlan_tbl_cfg_info {
    struct hinic5_mgmt_msg_head head;     /**< 命令字消息头 */
    struct hinic5_tc_vxlan_hdr_info vxlan_hdr;     /**< vxlan header */
    u16 index;     /**< index id */
    u16 opcode;     /**< 0:del, 1:add, 2:get */
    u32 rsvd;     /**< 保留域段 */
};

/**
 * @brief struct hinic5_tc_move_info
 * @details pfe tcam rule 删除命令结构体
 */
struct hinic5_tc_move_info {
    struct hinic5_mgmt_msg_head head;     /**< 命令字消息头 */
    u32 old_index;     /**< old index */
    u32 new_index;     /**< new index */
    u32 len;     /**< length */
};

/**
 * @brief struct hinic5_tc_pfe_cfg_profile_info
 * @details pfe profile config from register
 */
struct hinic5_tc_pfe_cfg_profile_info {
    struct hinic5_mgmt_msg_head head;     /**< 命令字消息头 */
    u32 reg_value;     /**< register value */
    u32 opcode;     /**< 0: select profile 1: 3-2 shift 2: 3-1-2 shift */
    u32 rsvd;     /**< 保留域段 */
};

#define PFE_ACL_AGING_BLOCK_NUM 128

/**
 * @brief struct hinic5_tc_aging_info
 * @details info about pfe tc aging table
 */
struct hinic5_tc_aging_info {
    struct hinic5_mgmt_msg_head head;     /**< 命令字消息头 */
    u16 addr;     /**< address */
    u16 opcode;     /**< 0:get, 1:set, 2:status */
    u32 entry_h[PFE_ACL_AGING_BLOCK_NUM];     /**< high 32 entry */
    u32 entry_l[PFE_ACL_AGING_BLOCK_NUM];     /**< lower 32 entry */
    u16 status;     /**< pfe aging table enable status, 0:disable; 1:enable */
    u16 rsvd;     /**< 保留域段 */
};

#define HTN_CNT_SIZE 8

/**
 * @brief struct hinic5_tc_pfe_cnt_info
 * @details pfe count info
 */
struct hinic5_tc_pfe_cnt_info {
    struct hinic5_mgmt_msg_head head;     /**< 命令字消息头 */
    u32 htn_cnt[HTN_CNT_SIZE];     /**< HTN count */
    u16 mode;     /**< 0:tx, 1:rx */
    u16 opcode;     /**< 0:get, 1:reset */
    u32 count_id;     /**< count id */
};

/* VF 可用的vport计数资源 */
#define MAX_NIC_COUNT_ID 24

/**
 * @brief struct hinic5_nic_vport_cnt_info
 * @details nic vport count info
 */
struct hinic5_nic_vport_cnt_info {
    struct hinic5_mgmt_msg_head head;     /**< 命令字消息头 */
    struct hinic5_vport_stats stats;     /**< vport统计 */
    u32 cnt_res[MAX_NIC_COUNT_ID];     /**< 分配vport计数资源的func id */
    u32 cur_cnt;     /**< 当前被占用的资源数 */
    u32 func_id;     /**< 配置或查询的func id */
    u32 index;     /**< 配置或查询的func id对应的芯片index */
    u32 opcode;     /**< 参考 nic_vport_cnt_op_e */
    u32 rsv1;     /**< 保留域段 */
    u64 rsv2[192];     /**< 保留域段 */
};

/**
 * @brief struct hinic5_tc_pfe_cfg_reg_info
 * @details pfe config from register
 */
struct hinic5_tc_pfe_cfg_reg_info {
    struct hinic5_mgmt_msg_head head;     /**< 命令字消息头 */
    u32 reg_value;     /**< register value */
    u32 reg_value2;     /**< register value */
    u32 rsvd;     /**< 保留域段 */
};

#define TC_XY_KEY_SIZE 11

/**
 * @brief struct hinic5_tc_tcam_info
 * @details info about tcam info
 */
struct hinic5_tc_tcam_info {
    struct hinic5_mgmt_msg_head head;     /**< 命令字消息头 */
    u32 key_x[TC_XY_KEY_SIZE];     /**< tcam key_x info */
    u32 key_y[TC_XY_KEY_SIZE];     /**< tcam key_y info */
    struct hinic5_tc_action_info action;     /**< action info */
    u16 opcode;     /**< 0:del, 1:add, 2:group, 3:get */
    u16 index;     /**< index id */
    u8 group_cnt;     /**< group cnt */
    u8 rsvd[3];     /**< 保留域段 */
};

#define PFE_VTEP_TBL_IP_SIZE 4
#define PFE_VTEP_TBL_IP_NUM 8

struct pfe_vtep_ip {
    u32 is_ipv6;     /**< 0: ipv4, 1: ipv6 */
    u32 ip_addr[PFE_VTEP_TBL_IP_SIZE];     /**< ip_addr[0]: IPv6 DIP[127:96] or IPv4 DIP[31:0]
                                            ip_addr[1]: IPv6 DIP[95:64] or IPv4 need set to 0
                                            ip_addr[2]: IPv6 DIP[63:32] or IPv4 need set to 0
											ip_addr[3]: IPv6 DIP[31:0] or IPv4 need set to 0 */
};

/**
 * @brief struct hinic5_tc_pfe_vtep_ip_cmd
 * @details pfe vtep表配置
 */
struct hinic5_tc_pfe_vtep_ip_cmd {
    struct hinic5_mgmt_msg_head head;     /**< 命令字消息头 */
    struct pfe_vtep_ip dip[PFE_VTEP_TBL_IP_NUM];     /**< destination ip */
    u16 num;     /**< num of ip in dip table */
    u16 opcode;     /**< 0: del, 1: add, 2: query */
    u32 rsvd;     /**< 保留域段 */
};

#define DEFAULT_ACTION_REG_NUM 9     /**< 默认ACTION(TX2个，RX1个，每个ACTION3个寄存器)涉及的寄存器个数 */

/**
 * @brief struct hinic5_tc_default_action_info
 * @details info about pfe default action
 */
struct hinic5_tc_default_action_info {
    struct hinic5_mgmt_msg_head head;     /**< 命令字消息头 */
    u8 opcode;     /**< action type: 0:drop; 1:upcall; 2:show; 3:output to port */
    u8 addr;     /**< default action reg addr: 0:tx default action1; 1:tx default action2; 2:rx default action */
    u16 index;     /**< func index */
    u32 action[DEFAULT_ACTION_REG_NUM];     /**< default action info */
};

/**
 * @brief struct hinic5_tc_pfe_tcam_freq_info
 * @details pfe tcam_freq info
 */
struct hinic5_tc_pfe_tcam_freq_info {
    struct hinic5_mgmt_msg_head head;     /**< 命令字消息头 */
    u32 tcam_sel;     /**< TCAM_CLK_SEL */
    u8 mode;     /**< 1:500MHz, 2:250MHz, 3:125MHz */
    u8 opcode;     /**< 0:get, 1:set */
    u16 rsvd;     /**< 保留域段 */
};

/**
 * @brief struct hinic5_tc_tcam_clock_gating_cfg_info
 * @details info about tcam clock gating
 */
struct hinic5_tc_tcam_clock_gating_cfg_info {
    struct hinic5_mgmt_msg_head head;     /**< 命令字消息头 */
    u8 opcode;     /**< 0: get, 1: set */
    u8 status;     /**< 0: 时钟关断, 1: 时钟打开 */
    u16 rsvd;     /**< 保留域段 */
};

#define MAX_CEQ_PER_FUNC 0x20

/**
 * @brief 定义了一个用于设置NIC中断控制的结构体
 * @details 该结构体包含了设置NIC中断控制所需的各种信息
 */
struct mig_nic_set_ceq_ctrl {
    struct hinic5_mgmt_msg_head head;     /**< 命令字消息头 */
    u16 func_id;     /**< func id */
    u8 ceq_num;     /**< 中断控制的数量 */
    u8 rsvd;     /**< 保留域段 */
    u32 ceq_ctrl0[MAX_CEQ_PER_FUNC];     /**< 中断控制器0的数组，每个元素对应一个中断控制器 */
    u32 ceq_ctrl1[MAX_CEQ_PER_FUNC];     /**< 中断控制器1的数组，每个元素对应一个中断控制器 */
};

#define MAX_INTR_NUM 0x80     /**< 最大中断数量128 */

/**
 * @brief 定义了一个用于迁移NIC中断信息的结构体
 * @details 该结构体包含了管理消息头部、函数ID、中断号、MSI-X控制寄存器操作以及MSI-X控制寄存器0和4的值。
 */
struct mig_nic_msix_info_rw {
    struct hinic5_mgmt_msg_head head;     /**< 命令字消息头 */
    u16 func_id;     /**< func id */
    u8 intr_num;     /**< 中断号 */
    u8 msi_ctl_csr_op;     /**< MSI-X控制寄存器操作，用于标识对MSI-X控制寄存器的操作类型 */
    u32 msix_ctrl0[MAX_INTR_NUM];     /**< MSI-X控制寄存器0的值，每个中断对应一个值 */
    u32 msix_ctrl4[MAX_INTR_NUM];     /**< MSI-X控制寄存器1的值，每个中断对应一个值 */
};

#define MIG_FUNC_TBL_SIZE 0x50     /**< TBL size = (64 + 16(rsvd)) */
#define MIG_VAT_TBL_SIZE 0x10     /**< vat size: 0x10 */

/**
 * @brief 定义了一个结构体，用于存储网络接口卡的功能和虚拟地址表信息
 * @details 该结构体包含了网络接口卡的管理消息头部、功能ID、操作码、功能使用标志、功能表和虚拟地址表等信息。
 */
struct mig_nic_func_vat_tbl {
    struct hinic5_mgmt_msg_head head;     /**< 命令字消息头 */
    u16 func_id;     /**< func id */
    u8 opcode;     /**< 操作码 */
    u8 func_used;     /**< function使用标志 */
    u8 func_tbl[MIG_FUNC_TBL_SIZE];     /**< func表 */
    u8 vat_tbl[MIG_VAT_TBL_SIZE];     /**< 虚拟地址表 */
    u8 rsvd[4];     /**< 保留域段 */
};

#define MIG_FAST_MSG_VF_PAGE_NUM 4

/**
 * @brief 定义了一个结构体，用于存储网络接口的虚拟功能信息
 * @details 结构体中包含了网络接口的虚拟功能信息，如发送队列数量、接收队列数量、命令队列数量等。
 */
struct mig_nic_vf_info {
    u8 sq_num;     /**< 发送队列数量 */
    u8 rq_num;     /**< 接收队列数量 */
    u8 cmdq_num;     /**< 命令队列数量 */
    u8 cmdq_depth;     /**< 命令队列深度 */
    u32 rq_depth;     /**< 接收队列深度 */
    u32 sq_depth;     /**< 发送队列深度 */
    u32 sq_ci_base_addr_h;     /**< 发送队列完成指针基地址的高位 */
    u32 sq_ci_base_addr_l;     /**< 发送队列完成指针基地址的低位 */
    u16 bat_size;     /**< 批处理大小 */
    u8 valid;     /**< 有效标志 */
    u8 fast_msg_en;
    u8 fast_msg_page_num;
    u8 rsvd[3];
    u64 fast_msg_page_addr[MIG_FAST_MSG_VF_PAGE_NUM];
};

/**
 * @brief 定义了一个结构体，用于存储网卡功能配置信息
 * @details 该结构体包含了网卡管理消息头部信息、功能ID、保留字段和虚拟功能信息。
 */
struct mig_nic_func_cfg {
    struct hinic5_mgmt_msg_head head;     /**< 命令字消息头 */
    u16 func_id;     /**< func id */
    u16 rsvd;     /**< 保留域段 */
    struct mig_nic_vf_info vf_info;     /**< VF信息 */
};

/**
 * @brief 此结构体用于检查消息盒是否为空
 * @details 此结构体主要用于检查网络接口卡的消息盒是否为空，以便于进行后续的操作。
 */
struct mig_nic_chk_mbx_empty {
    struct hinic5_mgmt_msg_head head;     /**< 命令字消息头 */
    u16 func_id;     /**< func id */
    u8 bme;     /**< 表示消息盒是否为空的标志 */
    u8 is_func_used;     /**< 表示func是否被使用的标志 */
    u8 cmdq_num;     /**< get cmdq num for stop cmdq */
    u8 rsvd[3];     /**< 保留域段 */
};

/**
 * @brief 定义了一个结构体，用于表示迁移网络接口虚拟端口的状态
 * @details 该结构体包含了网络接口虚拟端口的状态信息，如管理消息头部、功能ID、状态等。
 */
struct mig_nic_vport_state {
    struct hinic5_mgmt_msg_head msg_head;     /**< 命令字消息头 */
    u16 func_id;     /**< func id */
    u16 rsvd1;     /**< 保留域段 */
    u8 state;     /**< 0--disable, 1--enable */
    u8 rsvd2[3];     /**< 保留域段 */
};

#define SQ_CI_ATTR_SIZE 0x14     /**< SQ CI地址长度 */
#define SQ_CI_INDIR_TBL_SIZE 0x20     /**< SQ CI间接表大小 */
#define MAX_CI_TBL_NUM 0x20     /**< mailbox max 2kb, one sq ci(attr + tbl) 52B, 32 *52 = 1664B < 2kb */

/**
 * @brief 定义一个结构体，用于存储单个NIC的发送队列控制信息
 * @details 该结构体包含两个成员，sq_ci_tbl和sq_ci_attr，分别用于存储发送队列控制表和发送队列控制属性。
 */
struct mig_nic_single_sq_ci {
    u8 sq_ci_tbl[SQ_CI_INDIR_TBL_SIZE];     /**< sq ci表 */
    u8 sq_ci_attr[SQ_CI_ATTR_SIZE];     /**< sq ci地址表 */
};

/**
 * @brief 网络接口结构体，用于存储网络接口的信息
 * @details 该结构体包含了网络接口的基本信息和服务队列的信息
 */
struct mig_nic_sq_ci {
    struct hinic5_mgmt_msg_head msg_head;     /**< 命令字消息头 */
    u16 func_id;     /**< 功能ID，用于标识网络接口的功能 */
    u8 opcode;     /**< 操作码，用于标识网络接口的操作类型 */
    u8 curr_sq_id;     /**< 当前的服务队列ID，用于标识当前正在使用的服务队列 */
    u8 round_queue_num;     /**< 循环队列数量，用于标识网络接口的循环队列数 */
    u8 rsvd[3];     /**< 保留域段 */
    struct mig_nic_single_sq_ci sq_ci[MAX_CI_TBL_NUM];     /**< 单个sq的ci信息 */
};

#define RSS_INDIR_TBL_SIZE 0x200     /**< rss间接表的大小 */
#define RSS_KEY_SIZE 0x28     /**< rss key的大小 */

/**
 * @brief 定义了一个用于网络接口的接收方缩放（RSS）的结构体
 * @details 该结构体包含了RSS相关的各种参数，用于配置网络接口的RSS功能。
 */
struct mig_nic_rss_tbl {
    u8 rss_enable;     /**< 启用或禁用RSS的标志 */
    u8 rss_hash_engine;     /**< RSS哈希引擎的类型 */
    u16 rsvd;     /**< 保留域段 */
    u32 rss_ctx;     /**< RSS上下文 */
    u8 rss_indri_tbl[RSS_INDIR_TBL_SIZE];     /**< RSS间接表 */
    u8 rss_key[RSS_KEY_SIZE];     /**< RSS密钥 */
};

/**
 * @brief 定义了一个用于网络接口配置的结构体，包含了管理消息头、功能ID、操作码、预留字段和RSS表的信息
 * @details 该结构体用于网络接口的配置，包含了管理消息头、功能ID、操作码、预留字段和RSS表的信息，可以用于实现网络接口的各种配置操作。
 */
struct mig_nic_cfg_rss_tbl {
    struct mgmt_msg_head msg_head;     /**< 命令字消息头 */
    u16 func_id;     /**< func id */
    u8 opcode;     /**< 操作码 */
    u8 rsvd;     /**< 保留域段 */
    struct mig_nic_rss_tbl rss_tbl;     /**< rss表 */
};

#define MAX_CMDQ_NUM 0x4     /**< 最大cmdq数量 */
#define ENHANCED_CMDQ_CTX_SIZE 0x30     /**< 定义增强型cmdq context size为48 */

/**
 * @brief 定义了一个结构体，用于存储网卡临时配置命令队列的上下文信息
 * @details 该结构体包含了管理消息头部、函数ID、命令队列数量、保留字段和命令队列上下文等信息
 */
struct mig_nic_tmp_cfg_cmdq_ctx {
    struct mgmt_msg_head msg_head;     /**< 命令字消息头 */
    u16 func_id;     /**< func id */
    u8 cmdq_num;     /**< cmdq数量 */
    u8 rsvd;     /**< 保留域段 */
    u8 cmdq_ctx[MAX_CMDQ_NUM * ENHANCED_CMDQ_CTX_SIZE];     /**< cmdq上下文 */
};

/**
 * @brief 网卡迁移队列停止信息结构体
 * @details 该结构体用于描述网卡迁移队列停止的相关信息。
 */
struct nic_mig_sq_stop {
    struct hinic5_mgmt_msg_head msg_head;     /**< 命令字消息头 */
    u16 func_id;     /**< func id */
    u8 is_stop;     /**< 是否停止 */
    u8 sq_num;     /**< sq队列号 */
    u8 rsvd[4];     /**< 保留域段 */
};

struct mig_nic_fast_msg_addr {
    struct mgmt_msg_head head;     /**< 命令字消息头 */
    u16 func_id;     /**< func id */
    u8 opcode;     /**< 操作码 */
    u8 page_num;     /**< page num */
    u32 rsvd;     /**< 保留域段 */
    u64 page_addr[MIG_FAST_MSG_VF_PAGE_NUM];     /**< page addr */
};

/**
 * @brief struct hinic5_cmd_set_pcie_flr_mgmt
 * @details nic 设置pcie flr命令结构体
 */
struct hinic5_cmd_set_pcie_flr_mgmt {
    struct hinic5_mgmt_msg_head msg_head;     /**< 命令字消息头 */
    u16 func_id;     /**< func id */
    u16 rsvd1;     /**< 保留域段 */
};

struct hinic5_cmd_lro_cfg {
    struct hinic5_mgmt_msg_head msg_head;     /**< 命令字消息头 */
    u16 func_id;     /**< func id */
    u8 data;     /**< data to be set or get */
    u8 data_type;     /**< data type: 参考 lro_cfg_operate_type 枚举定义 */
    u8 opcode;     /**< 0: get, 1: set */
    u8 rsvd1[3];     /**< 保留域段 */
};

#define NIC_DCB_UP_MAX 0x8     /**< 定义网络接口控制数据包(NIC DCB)的最大用户优先级(UP) */

/**
 * @brief hinic5_dcb_state
 * @details nic dcb state命令结构体
 */
struct hinic5_dcb_state {
    u8 dcb_on;     /**< dcb on or off */
    u8 default_cos;     /**< default cos */
    u8 trust;     /**< trust state */
    u8 rsvd1;     /**< 保留域段 */
    u8 pcp2cos[NIC_DCB_UP_MAX];     /**< pcp to cos */
    u8 dscp2cos[64];     /**< dscp to cos */
    u32 rsvd2[7];     /**< 保留域段 */
};

/**
 * @brief hinic5_cmd_vf_dcb_state
 * @details nic vf dcb state命令结构体
 */
struct hinic5_cmd_vf_dcb_state {
    struct hinic5_mgmt_msg_head msg_head;     /**< 命令字消息头 */
    struct hinic5_dcb_state state;     /**< dcb state */
};

struct hinic5_cmd_port_info {
    struct hinic5_mgmt_msg_head msg_head;     /**< 命令字消息头 */
    u8 port_id;
    u8 rsvd1[3];     /**< 保留域段 */
    u8 port_type;
    u8 autoneg_cap;
    u8 autoneg_state;
    u8 duplex;
    u8 speed;
    u8 fec;
    u16 rsvd2;     /**< 保留域段 */
    u32 rsvd3[4];     /**< 保留域段 */
};

/**
 * @brief nic_cmd_bond_active_report_info
 * @details lacp协商结果更新之后向主机侧发送异步消息通知结构体
 */
struct nic_cmd_bond_active_report_info {
    struct mgmt_msg_head head;     /**< 命令字消息头 */
    u32 bond_id;
    u32 bon_mmi_status;     /**< 该bond子设备的链路状态 */
    u32 active_bitmap;     /**< 该bond子设备的slave port状态 */
    u8 rsvd[16];     /**< 保留域段 */
};

/**
 * @brief nic_cmd_tx_pause_notice
 * @details pfc/pause风暴tx异常上报
 */
struct nic_cmd_tx_pause_notice {
    struct hinic5_mgmt_msg_head head;     /**< 命令字消息头 */
    u32 tx_pause_except;     /**< 1: 异常，0: 正常 */
    u32 except_level;     /**< 异常等级 */
    u32 rsvd;     /**< 保留域段 */
};

#pragma pack(4)
typedef struct fault_msg_s {
    struct mgmt_msg_head msg_head;     /**< 命令字消息头 */
    u8 num;     /**< 发送的自定义报文数量 */
    u8 rsvd[3];     /**< 保留域段 */
    u16 real_size;
    u8 mode;     /**< 0: read, 1: write */
    u8 type;     /**< 0:不立即发送通知, 1: 立即发送通知 */
    u8 data[0];     /**< 大小由real_size指定 */
} fault_msg_t;
#pragma pack()

#define TCAM_FLOW_KEY_SIZE 44     /**< 定义tcam key的大小为44B */

typedef struct {
    u32 qid : 10;     /**< 如果flag==1，则fdir_qid表示group id；如果flag==0则fdir_qid表示qid */
    u32 flag : 1;
    u32 rsvd : 21;     /**< 保留域段 */
} qid_htn_s;

typedef union {
    qid_htn_s qid_htn;
    u32 qid;
} qid_u;

/**
 * @brief 定义一个结构体，用于存储TCAM查找的结果
 * @details 包含Flow Director的复合目标信息（qid/group + flag）和保留字段。
 */
struct tcam_result {
    qid_u fdir_info;
    u32 rsvd;     /**< 保留域段 */
};

/**
 * @brief 定义一个结构体，用于存储TCAM流键的x和y值
 * @details 结构体用于存储TCAM流键的x和y值，TCAM流键是一种用于流表查找的数据结构，其中x和y值是流键的两个组成部分。
 */
struct tcam_key_x_y {
    u8 x[TCAM_FLOW_KEY_SIZE];     /**< x值，大小为TCAM_FLOW_KEY_SIZE */
    u8 y[TCAM_FLOW_KEY_SIZE];     /**< y值，大小为TCAM_FLOW_KEY_SIZE */
};

/**
 * @brief 用于存储网络接口控制器（NIC）的TCAM配置规则
 * @details 包含了TCAM配置规则的索引、数据和键，用于在NIC中存储和管理TCAM配置规则。
 */
struct nic_tcam_cfg_rule {
    u32 index;     /**< 规则的索引 */
    struct tcam_result data;     /**< 规则的数据 */
    struct tcam_key_x_y key;     /**< 规则的键 */
};

/**
 * @brief TCAM配置规则
 * @details 包含了TCAM规则的各种配置信息，如规则的键值、掩码、动作等。
 */
struct nic_cmd_fdir_add_rule {
    struct hinic5_mgmt_msg_head head;     /**< 命令字消息头 */
    u16 func_id;     /**< Func ID */
    u8 type;     /**< 规则类型 */
    u8 usrdata;     /**< 用户数据 */
    struct nic_tcam_cfg_rule rule;     /**< 配置表 */
};

/**
 * @brief 网络接口命令过滤器获取规则的结构体
 * @details 该结构体用于存储网络接口命令过滤器获取规则的相关信息。
 */
struct nic_cmd_fdir_get_rule {
    struct hinic5_mgmt_msg_head head;     /**< 命令字消息头 */
    u32 index;     /**< 索引字段 */
    u8 valid;     /**< 有效性字段 */
    u8 type;     /**< 类型字段 */
    u16 rsvd;     /**< 保留域段 */
    struct tcam_key_x_y key;     /**< TCAM键字段 */
    struct tcam_result data;     /**< TCAM结果字段 */
    u64 packet_count;     /**< 包计数字段 */
    u64 byte_count;     /**< 字节计数字段 */
};

#define NIC_TCAM_BLOCK_LARGE_NUM 256     /**< 定义TCAM BLOCK最大值为256 */
#define NIC_TCAM_BLOCK_LARGE_SIZE 16     /**< 定义一个block最大size为16 */
#define TCAM_RULE_FDIR_TYPE 0     /**< 定义TCAM规则的类型，FDIR类型对应值为0 */
#define TCAM_RULE_PPA_TYPE 1     /**< 定义TCAM规则的类型，PPA类型对应值为1 */

/**
 * @brief 获取TCAM块规则的结构体
 * @details 该结构体用于获取TCAM块规则，包括TCAM块类型、TCAM表类型、TCAM块索引、有效性数组、TCAM键数组和TCAM结果数组。
 */
struct nic_cmd_fdir_get_block_rules {
    struct hinic5_mgmt_msg_head head;     /**< 命令字消息头 */
    u8 tcam_block_type;     /**< TCAM块类型，目前仅有 NIC_TCAM_BLOCK_TYPE_LARGE */
    u8 tcam_table_type;     /**< TCAM规则类型，TCAM_RULE_PPA_TYPE 或 TCAM_RULE_FDIR_TYPE */
    u16 tcam_block_index;     /**< TCAM块索引 */
    u8 valid[NIC_TCAM_BLOCK_LARGE_SIZE];     /**< 有效性数组 */
    struct tcam_key_x_y key[NIC_TCAM_BLOCK_LARGE_SIZE];     /**< TCAM键数组 */
    struct tcam_result data[NIC_TCAM_BLOCK_LARGE_SIZE];     /**< TCAM结果数组 */
};

struct hinic5_rate_xir_xbs {
    u32 cir;     /**< unit: kbps */
    u32 pir;     /**< unit: kbps */
    u32 cbs;     /**< unit: Byte */
    u32 pbs;     /**< unit: Byte */
};

/**
 * @brief nic tx MQM速率配置
 * @details 用于设置和获取func的MQM限速参数以及设置fun映射表。
 */
struct hinic5_cmd_tx_limit_rate {
    struct hinic5_mgmt_msg_head msg_head;     /**< 命令字消息头 */
    u16 func_id;     /**< func ID */
    u8 op_code;     /**< 操作类型： 0 - 默认； 1 - get； 2 - set； 3 - map */
    u8 vnicgrp_flag;     /**< vnicgrp操作标识：0 - vnic；1 - vnic group */
    u32 vnicgrp_id;     /**< vnic group ID */
    u32 vnic_id;     /**< vnic ID */
    u8 limit_type;     /**< 限速类型：0 - pps; 1-bps */
    u8 rsvd1[3];     /**< 保留域段 */
    struct hinic5_rate_xir_xbs rate_para;     /**< rate_cfg */
    u32 rsvd2[64];     /**< 保留域段 */
};

#define HINIC5_TX_SET_PROMISC_SKIP 0     /**< 设置混杂接收未知单播报文 */
#define HINIC5_TX_GET_PROMISC_SKIP 1     /**< 读取混杂接收未知单播报文 */

/**
 * @brief 设置混杂是否接收未知单播报文
 * @details 该结构体用于开关是否上送单播未知报文到开混杂PF，包括
 */
struct hinic5_tx_promisc_cfg {
    struct hinic5_mgmt_msg_head head;     /**< 命令字消息头 */
    u8 port_id;     /**< 网络侧端口id */
    u8 promisc_skip_en;     /**< 0：关闭上送 1：打开上送 */
    u8 opcode;     /**< 0：设置 1：读取 */
    u8 rsvd;     /**< 保留域段 */
};

#define HINIC5_ARP_PKT_MAX_LEN 512

/**
 * @brief 向MPU传递ARP相关报文内容结构体
 * @details 该结构体用于向MPU传递需要代发的ARP或ND报文
 */
struct hinic5_arp_pkt_info {
    struct hinic5_mgmt_msg_head head;     /**< 命令字消息头 */
    u16 func_id;     /**< func ID */
    u16 pkt_length;     /**< 报文长度 */
    u16 origin_queue_id;     /**< 驱动发送报文的队列id */
    u8  rsvd[2];     /**< 保留域段 */
    u32 rsvd1[4];     /**< 保留域段 */
    u8 pkt_buf[HINIC5_ARP_PKT_MAX_LEN];     /**< 报文缓冲区 */
};

/**
 * @brief 向MPU传递Bond配置信息结构体
 * @details 该结构体用于向MPU传递卸载Bond配置信息
 */
struct hinic5_cmd_cfg_bond {
    struct hinic5_mgmt_msg_head head;     /**< 命令字消息头 */
    u16 func_id;
    u8 opcode;     /**< operation code: 1 set arp status, 0 get arp status */
    u8 arp_en;     /**< ARP双发使能 */
    u32 rsvd;     /**< 保留域段 */
};

#endif /* HINIC5_NIC_CMD_STRUCTS_H */

