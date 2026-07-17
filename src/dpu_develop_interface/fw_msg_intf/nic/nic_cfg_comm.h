/******************************************************************************

  Copyright (C), 2001-2021, Huawei Tech. Co., Ltd.

 ******************************************************************************
  File Name     : nic_cfg_comm.h
  Version       : Initial Draft
  Description   : nic config common header file
  Function List :
  History       :
    Modification: Created file

******************************************************************************/

#ifndef NIC_CFG_COMM_H
#define NIC_CFG_COMM_H

#if defined(__LINUX__) || defined(__VMWARE__)
#include <linux/types.h>
#endif

#include "nic_mpu_cmd_structs.h"
#include "nic_mpu_cmd_structs_extend.h"
/* rss */
#define HINIC5_RSS_TYPE_VALID_SHIFT 23 /**< 定义RSS（Receive Side Scaling）类型有效位的偏移量 */
#define HINIC5_RSS_TYPE_TCP_IPV6_EXT_SHIFT 24 /**< 定义RSS类型TCP IPv6扩展的偏移量 */
#define HINIC5_RSS_TYPE_IPV6_EXT_SHIFT 25 /**< 定义RSS类型IPv6扩展头的偏移量 */
#define HINIC5_RSS_TYPE_TCP_IPV6_SHIFT 26 /**< 定义RSS类型TCP IPv6的偏移量 */
#define HINIC5_RSS_TYPE_IPV6_SHIFT 27 /**< 定义RSS类型IPv6的偏移量 */
#define HINIC5_RSS_TYPE_TCP_IPV4_SHIFT 28 /**< 定义RSS类型TCP IPv4的偏移量 */
#define HINIC5_RSS_TYPE_IPV4_SHIFT 29 /**< 定义RSS类型IPv4的偏移量 */
#define HINIC5_RSS_TYPE_UDP_IPV6_SHIFT 30 /**< 定义RSS类型UDP IPv6的偏移量 */
#define HINIC5_RSS_TYPE_UDP_IPV4_SHIFT 31 /**< 定义RSS类型UDP IPv4的偏移量 */

/* vlan */
#define NIC_CVLAN_INSERT_ENABLE 0x1
#define NIC_QINQ_INSERT_ENABLE  0x3
#define NIC_CONFIG_ALL_QUEUE_VLAN_CTX 0xffff

/**
 * @brief 定义一个宏，用于设置RSS类型
 * @param val 要设置的值
 * @param member 要设置的成员
 * @return 返回设置后的结果
 */
#define HINIC5_RSS_TYPE_SET(val, member) (((u32)(val) & 0x1) << HINIC5_RSS_TYPE_##member##_SHIFT)
/**
 * @brief 定义一个宏，用于获取RSS类型
 * @param val 要获取的值
 * @param member 要获取的成员
 * @return 返回获取到的结果
 */
#define HINIC5_RSS_TYPE_GET(val, member) (((u32)(val) >> HINIC5_RSS_TYPE_##member##_SHIFT) & 0x1)

/**
 * @brief 定义RSS哈希类型枚举
 * @details 此枚举用于表示网络接口卡(NIC)的接收方硬件分布(RSS)哈希类型
 */
enum nic_rss_hash_type {
    NIC_RSS_HASH_TYPE_XOR = 0,  /**< XOR哈希类型 */
    NIC_RSS_HASH_TYPE_TOEP,     /**< TOEP哈希类型 */

    NIC_RSS_HASH_TYPE_MAX       /**< MUST BE THE LAST ONE */
};

/**
 * @brief 定义CQE合一场景下微码上报的csum_err类型枚举
 * @details CQE合一场景下, CQE中csum_err由原来的9bit压缩到2bit, 与驱动保持以下配合
 * NIC_RX_CSUM_IPSU_OTHER_ERR时, 即非CQE合一场景csum_err bit[8]不为0 ---> CQE合一场景csum_err修改成2
 * NIC_RX_CSUM_HW_BYPASS_ERR时, 即非CQE合一场景csum_err bit[7]不为0 ---> CQE合一场景csum_err修改成3
 * l3或l4层报文csum错误时, 非CQE合一场景csum_err bit[0]~bit[6]不为0 ---> CQE合一场景csum_err修改成1
 */
enum nic_compact_cqe_csum_err_type {
    NIC_RX_COMPACT_CSUM_NO_ERROR = 0,
    NIC_RX_COMPACT_L3_L4_CSUM_ERROR,
    NIC_RX_COMPACT_CSUM_OTHER_ERROR,
    NIC_RX_COMPACT_HW_BYPASS_ERROR
};

#define NIC_RSS_INDIR_SIZE      256     /**< 定义RSS间接表的大小为256 */
#define NIC_RSS_KEY_SIZE        40      /**< 定义RSS ket大小为40 */

/* *
 * Definition of the NIC receiving mode
 */
#define NIC_RX_MODE_UC          0x01 /**< 单播模式 */
#define NIC_RX_MODE_MC          0x02 /**< 组播模式 */
#define NIC_RX_MODE_BC          0x04 /**< 广播模式 */
#define NIC_RX_MODE_MC_ALL      0x08 /**< 全组播模式 */
#define NIC_RX_MODE_PROMISC     0x10 /**< 混杂模式，接收所有的数据包 */

/* IEEE 802.1Qaz std */
#define NIC_DCB_DSCP_NUM        0x8 /**< 定义网络接口控制数据包(NIC DCB)的最大DSCP值 */
#define NIC_DCB_IP_PRI_MAX      0x40    /**< 定义网络接口控制数据包(NIC DCB)的最大IP优先级 */

#define NIC_DCB_PRIO_DWRR       0x0 /**< 定义一个宏，表示优先级的分配方式为带宽的严格分配 */
#define NIC_DCB_PRIO_STRICT     0x1 /**< 定义一个宏，表示优先级的分配方式为严格的优先级 */

#define NIC_DCB_MAX_PFC_NUM     0x4 /**< 定义一个宏，表示最大优先级流控制（PFC）的数量 */

#ifndef ETH_ALEN
#define ETH_ALEN 6  /**< 定义单mac地址长度为6B */
#endif

#ifndef BIT
/**
 * @brief 定义一个宏，用于将一个数的二进制形式左移n位
 * @param n 要左移的位数
 * @return 返回左移n位后的结果
 */
#define BIT(n) (1UL << (n))
#endif

/**
 * @brief 网络接口卡功能能力枚举定义
 *
 * @details 枚举类型 nic_feature_cap 定义了网络接口卡的各种功能能力。
 *          每个枚举值都是一个位掩码的BIT位，可以通过位操作来设置或者检查某个功能是否被支持。
 */

enum nic_feature_cap {
    NIC_F_CSUM_BIT = 0,                    /**< 校验和计算 */
    NIC_F_SCTP_CRC_BIT = 1,                /**< SCTP CRC校验 */
    NIC_F_TSO_BIT = 2,                     /**< TCP Segmentation Offload */
    NIC_F_LRO_BIT = 3,                     /**< Large Receive Offload */
    NIC_F_UFO_BIT = 4,                     /**< UDP Fragmentation Offload */
    NIC_F_RSS_BIT = 5,                     /**< Receive Side Scaling */
    NIC_F_RX_VLAN_FILTER_BIT = 6,          /**< 接收VLAN过滤 */
    NIC_F_RX_VLAN_STRIP_BIT = 7,           /**< 接收VLAN去除 */
    NIC_F_TX_VLAN_INSERT_BIT = 8,          /**< 发送VLAN插入 */
    NIC_F_VXLAN_OFFLOAD_BIT = 9,           /**< VXLAN Offload */
    NIC_F_IPSEC_OFFLOAD_BIT = 10,          /**< IPsec Offload */
    NIC_F_FDIR_BIT = 11,                   /**< Flow Director */
    NIC_F_PROMISC_BIT = 12,                /**< 混杂模式 */
    NIC_F_ALLMULTI_BIT = 13,               /**< 接收所有组播 */
    NIC_F_XSFP_REPORT_BIT = 14,            /**< XSFP状态报告 */
    NIC_F_VF_MAC_BIT = 15,                 /**< 虚拟函数MAC地址 */
    NIC_F_RATE_LIMIT_BIT = 16,             /**< 速率限制 */
    NIC_F_RXQ_RECOVERY_BIT = 17,           /**< 接收队列恢复 */
    NIC_F_PTP_1588_V2_BIT = 18,            /**< PTP 1588v2 */
    NIC_F_TX_WQE_COMPACT_TASK_BIT = 19,    /**< 发送WQE压缩 */
    NIC_F_RX_HW_COMPACT_CQE_BIT = 20,      /**< HTN合一CQE */
    NIC_F_HTN_CMDQ_BIT = 21,               /**< HTN命令队列 */
    NIC_F_GENEVE_OFFLOAD_BIT = 22,         /**< Geneve Offload */
    NIC_F_IPXIP_OFFLOAD_BIT = 23,          /**< IPXIP Offload */
    NIC_F_TC_FLOWER_OFFLOAD_BIT = 24,      /**< TCAM流控卸载 */
    NIC_F_HTN_FDIR_BIT = 25,               /**< HTN FDIR功能 */
    NIC_F_SQ_RQ_CI_COALESCE_BIT = 26,      /**< SQ RQ CI共用 */
    NIC_F_RX_SW_COMPACT_CQE_BIT = 27,      /**< ucode合一CQE */
    NIC_F_HALF_BOND_OFFLOAD_BIT = 28,      /**< 半Bond卸载 */
    NIC_F_MACSEC_OFFLOAD_BIT = 29,         /**< MACSec卸载 */
    NIC_F_VEB_OFFLOAD_BIT = 30,            /**< VEB卸载 */
    NIC_F_GET_COUNTER_BY_CMDQ_BIT = 31,    /**< 支持通过CMDQ读取vport计数 */
    NIC_F_HTN_CMDQ_CAR_BIT = 32,           /**< 支持通过CMDQ设置CAR限速 */
    NIC_F_ARP_DUAL_BIT = 33,                    /**< 支持arp双发 */
};

#define NIC_F_BIT(bit)    ((u64)1 << (bit))
#define NIC_F(name)       NIC_F_BIT(NIC_F_##name##_BIT)

#define NIC_F_CSUM          NIC_F(CSUM)
#define NIC_F_SCTP_CRC      NIC_F(SCTP_CRC)
#define NIC_F_TSO           NIC_F(TSO)
#define NIC_F_LRO           NIC_F(LRO)
#define NIC_F_UFO           NIC_F(UFO)
#define NIC_F_RSS           NIC_F(RSS)
#define NIC_F_RX_VLAN_FILTER NIC_F(RX_VLAN_FILTER)
#define NIC_F_RX_VLAN_STRIP NIC_F(RX_VLAN_STRIP)
#define NIC_F_TX_VLAN_INSERT NIC_F(TX_VLAN_INSERT)
#define NIC_F_VXLAN_OFFLOAD NIC_F(VXLAN_OFFLOAD)
#define NIC_F_IPSEC_OFFLOAD NIC_F(IPSEC_OFFLOAD)
#define NIC_F_FDIR          NIC_F(FDIR)
#define NIC_F_PROMISC       NIC_F(PROMISC)
#define NIC_F_ALLMULTI      NIC_F(ALLMULTI)
#define NIC_F_XSFP_REPORT   NIC_F(XSFP_REPORT)
#define NIC_F_VF_MAC        NIC_F(VF_MAC)
#define NIC_F_RATE_LIMIT    NIC_F(RATE_LIMIT)
#define NIC_F_RXQ_RECOVERY  NIC_F(RXQ_RECOVERY)
#define NIC_F_PTP_1588_V2   NIC_F(PTP_1588_V2)
#define NIC_F_TX_WQE_COMPACT_TASK NIC_F(TX_WQE_COMPACT_TASK)
#define NIC_F_RX_HW_COMPACT_CQE NIC_F(RX_HW_COMPACT_CQE)
#define NIC_F_HTN_CMDQ      NIC_F(HTN_CMDQ)
#define NIC_F_GENEVE_OFFLOAD NIC_F(GENEVE_OFFLOAD)
#define NIC_F_IPXIP_OFFLOAD NIC_F(IPXIP_OFFLOAD)
#define NIC_F_TC_FLOWER_OFFLOAD NIC_F(TC_FLOWER_OFFLOAD)
#define NIC_F_HTN_FDIR      NIC_F(HTN_FDIR)
#define NIC_F_SQ_RQ_CI_COALESCE NIC_F(SQ_RQ_CI_COALESCE)
#define NIC_F_RX_SW_COMPACT_CQE NIC_F(RX_SW_COMPACT_CQE)
#define NIC_F_HALF_BOND_OFFLOAD NIC_F(HALF_BOND_OFFLOAD)
#define NIC_F_MACSEC_OFFLOAD NIC_F(MACSEC_OFFLOAD)
#define NIC_F_VEB_OFFLOAD   NIC_F(VEB_OFFLOAD)
#define NIC_F_GET_COUNTER_BY_CMDQ NIC_F(GET_COUNTER_BY_CMDQ)
#define NIC_F_HTN_CMDQ_CAR      NIC_F(HTN_CMDQ_CAR)
#define NIC_F_ARP_DUAL      NIC_F(ARP_DUAL)

#define NIC_F_1823_MASK 0x1FFEF   /**< 1823的所有属性 */
#define NIC_F_1825_MASK (NIC_F_CSUM | NIC_F_SCTP_CRC | NIC_F_TSO | NIC_F_LRO | NIC_F_RSS | NIC_F_RX_VLAN_FILTER | \
            NIC_F_RX_VLAN_STRIP | NIC_F_TX_VLAN_INSERT | NIC_F_VXLAN_OFFLOAD | NIC_F_IPSEC_OFFLOAD | NIC_F_FDIR | \
            NIC_F_PROMISC | NIC_F_ALLMULTI | NIC_F_XSFP_REPORT | NIC_F_VF_MAC | NIC_F_RATE_LIMIT | \
            NIC_F_TX_WQE_COMPACT_TASK | NIC_F_RX_SW_COMPACT_CQE | NIC_F_HALF_BOND_OFFLOAD | NIC_F_GET_COUNTER_BY_CMDQ)

#define NIC_F_182X_MASK (NIC_F_1823_MASK | NIC_F_1825_MASK) /**< 182x的所有属性 */

#define NIC_F_1872_PF_MASK (NIC_F_CSUM | NIC_F_SCTP_CRC | NIC_F_TSO | NIC_F_TX_WQE_COMPACT_TASK | NIC_F_RX_HW_COMPACT_CQE | \
            NIC_F_HTN_CMDQ | NIC_F_PROMISC | NIC_F_ALLMULTI | NIC_F_VXLAN_OFFLOAD | NIC_F_GENEVE_OFFLOAD |  \
            NIC_F_IPXIP_OFFLOAD | NIC_F_RX_VLAN_STRIP | NIC_F_TX_VLAN_INSERT | NIC_F_RSS | NIC_F_RX_VLAN_FILTER |   \
            NIC_F_LRO | NIC_F_FDIR | NIC_F_HTN_FDIR | NIC_F_SQ_RQ_CI_COALESCE | NIC_F_PTP_1588_V2 | NIC_F_TC_FLOWER_OFFLOAD | \
            NIC_F_MACSEC_OFFLOAD | NIC_F_VEB_OFFLOAD | NIC_F_RATE_LIMIT | NIC_F_HTN_CMDQ_CAR | NIC_F_ARP_DUAL | \
            NIC_F_XSFP_REPORT) /**< 187x PF的所有属性 */

#define NIC_F_1872_VF_MASK (NIC_F_CSUM | NIC_F_SCTP_CRC | NIC_F_TSO | NIC_F_TX_WQE_COMPACT_TASK | NIC_F_RX_HW_COMPACT_CQE | \
            NIC_F_HTN_CMDQ | NIC_F_ALLMULTI | NIC_F_VXLAN_OFFLOAD | NIC_F_GENEVE_OFFLOAD | NIC_F_IPXIP_OFFLOAD | \
            NIC_F_RX_VLAN_STRIP | NIC_F_TX_VLAN_INSERT | NIC_F_RSS | NIC_F_RX_VLAN_FILTER | NIC_F_LRO | NIC_F_FDIR | \
            NIC_F_HTN_FDIR | NIC_F_SQ_RQ_CI_COALESCE | NIC_F_TC_FLOWER_OFFLOAD | NIC_F_MACSEC_OFFLOAD | \
            NIC_F_VEB_OFFLOAD | NIC_F_RATE_LIMIT | NIC_F_HTN_CMDQ_CAR | NIC_F_ARP_DUAL) /**< 187x VF的所有属性 */

#define NIC_F_1872_MASK (NIC_F_1872_PF_MASK | NIC_F_1872_VF_MASK)
#define NIC_F_ALL_MASK (NIC_F_182X_MASK | NIC_F_1872_MASK)

#define HINIC5_TCAM_BLOCK_ENABLE      1 /**< TCAM块启用 */
#define HINIC5_TCAM_BLOCK_DISABLE     0 /**< TCAM块禁用 */
#define HINIC5_MAX_TCAM_RULES_NUM   4096 /**< TCAM的最大规则数量 */

/**
 * @brief 定义一个枚举类型，用于表示NIC TCAM块的类型。
 * @details 这个枚举类型包含了两种类型：NIC_TCAM_BLOCK_TYPE_LARGE和NIC_TCAM_BLOCK_TYPE_SMALL。
 *          NIC_TCAM_BLOCK_TYPE_LARGE表示块大小为16，NIC_TCAM_BLOCK_TYPE_SMALL表示块大小为0。
 */
enum {
    NIC_TCAM_BLOCK_TYPE_LARGE = 0, /**< block_size: 16 */
    NIC_TCAM_BLOCK_TYPE_SMALL,     /**< block_size: 0 */
    NIC_TCAM_BLOCK_TYPE_MAX
};

/**
 * @struct hinic5_tcam_key_ipv4_mem
 * @brief 定义了一个用于存储IPv4 TCAM键的结构体
 * @details 该结构体包含了IPv4 TCAM键的各个字段，包括保留位、隧道类型、IP协议类型、IP类型、
 *          Func ID、源IPv4地址、目的IPv4地址、目的端口号、源端口号、外部源IPv4地址、外部目的IPv4地址、VNI等信息。
 */
struct hinic5_tcam_key_ipv4_mem {
    u32 rsvd1 : 4; /**< 保留位1 */
    u32 tunnel_type : 4; /**< 隧道类型 */
    u32 ip_proto : 8; /**< IP协议类型 */
    u32 rsvd0 : 16; /**< 保留位0 */
    u32 sipv4_h : 16; /**< 源IPv4地址的高16位 */
    u32 ip_type : 1; /**< IP类型 */
    u32 function_id : 15; /**< Func ID */
    u32 dipv4_h : 16; /**< 目的IPv4地址的高16位 */
    u32 sipv4_l : 16; /**< 源IPv4地址的低16位 */
    u32 rsvd2 : 16; /**< 保留位2 */
    u32 dipv4_l : 16; /**< 目的IPv4地址的低16位 */
    u32 rsvd3; /**< 保留位3 */
    u32 dport : 16; /**< 目的端口号 */
    u32 rsvd4 : 16; /**< 保留位4 */
    u32 rsvd5 : 16; /**< 保留位5 */
    u32 sport : 16; /**< 源端口号 */
    u32 outer_sipv4_h : 16; /**< 外部源IPv4地址的高16位 */
    u32 rsvd6 : 16; /**< 保留位6 */
    u32 outer_dipv4_h : 16; /**< 外部目的IPv4地址的高16位 */
    u32 outer_sipv4_l : 16; /**< 外部源IPv4地址的低16位 */
    u32 vni_h : 16; /**< VNI的高16位 */
    u32 outer_dipv4_l : 16; /**< 外部目的IPv4地址的低16位 */
    u32 rsvd7 : 16; /**< 保留位7 */
    u32 vni_l : 16; /**< VNI的低16位 */
};

/**
 * @struct hinic5_tcam_key_ipv6_mem
 * @brief 定义了一个用于存储IPv6 TCAM键的结构体
 * @details 该结构体用于存储IPv6 TCAM键，包含了源IPv6地址和目标IPv6地址的各个部分，
 *          以及相关的协议类型、端口号等信息。
 */
struct hinic5_tcam_key_ipv6_mem {
    u32 rsvd1 : 3; /**< 保留位1 */
    u32 outer_ip_type : 1; /**< 外部IP类型 */
    u32 tunnel_type : 4; /**< 隧道类型 */
    u32 ip_proto : 8; /**< IP协议类型 */
    u32 rsvd0 : 16; /**< 保留位0 */
    u32 sipv6_key0 : 16; /**< 源IPv6地址的低16位 */
    u32 ip_type : 1; /**< IP类型 */
    u32 function_id : 15; /**< 功能ID */
    u32 sipv6_key2 : 16; /**< 源IPv6地址的第二部分 */
    u32 sipv6_key1 : 16; /**< 源IPv6地址的第一部分 */
    u32 sipv6_key4 : 16; /**< 源IPv6地址的第四部分 */
    u32 sipv6_key3 : 16; /**< 源IPv6地址的第三部分 */
    u32 sipv6_key6 : 16; /**< 源IPv6地址的第六部分 */
    u32 sipv6_key5 : 16; /**< 源IPv6地址的第五部分 */
    u32 dport : 16; /**< 目标端口 */
    u32 sipv6_key7 : 16; /**< 源IPv6地址的第七部分 */
    u32 dipv6_key0 : 16; /**< 目标IPv6地址的低16位 */
    u32 sport : 16; /**< 源端口 */
    u32 dipv6_key2 : 16; /**< 目标IPv6地址的第二部分 */
    u32 dipv6_key1 : 16; /**< 目标IPv6地址的第一部分 */
    u32 dipv6_key4 : 16; /**< 目标IPv6地址的第四部分 */
    u32 dipv6_key3 : 16; /**< 目标IPv6地址的第三部分 */
    u32 dipv6_key6 : 16; /**< 目标IPv6地址的第六部分 */
    u32 dipv6_key5 : 16; /**< 目标IPv6地址的第五部分 */
    u32 rsvd2 : 16; /**< 保留位2 */
    u32 dipv6_key7 : 16; /**< 目标IPv6地址的第七部分 */
};

/**
 * @struct hinic5_tcam_key_vxlan_ipv6_mem
 * @brief 定义了一个用于存储VXLAN IPv6 TCAM键的结构体
 * @details 该结构体包含了VXLAN IPv6 TCAM键的各个字段，包括保留位、隧道类型、IP协议类型等。
 */
struct hinic5_tcam_key_vxlan_ipv6_mem {
    u32 rsvd1 : 4; /**< 保留位1 */
    u32 tunnel_type : 4; /**< 隧道类型 */
    u32 ip_proto : 8; /**< IP协议类型 */
    u32 rsvd0 : 16; /**< 保留位0 */

    u32 dipv6_key0 : 16; /**< IPv6目标地址的低16位 */
    u32 ip_type : 1; /**< IP类型 */
    u32 function_id : 15; /**< 功能ID */

    u32 dipv6_key2 : 16; /**< IPv6目标地址的第二部分 */
    u32 dipv6_key1 : 16; /**< IPv6目标地址的第一部分 */

    u32 dipv6_key4 : 16; /**< IPv6目标地址的第四部分 */
    u32 dipv6_key3 : 16; /**< IPv6目标地址的第三部分 */

    u32 dipv6_key6 : 16; /**< IPv6目标地址的第六部分 */
    u32 dipv6_key5 : 16; /**< IPv6目标地址的第五部分 */

    u32 dport : 16; /**< 目标端口 */
    u32 dipv6_key7 : 16; /**< IPv6目标地址的第七部分 */

    u32 rsvd2 : 16; /**< 保留位2 */
    u32 sport : 16; /**< 源端口 */

    u32 outer_sipv4_h : 16; /**< 外部源IPv4地址的高16位 */
    u32 rsvd3 : 16; /**< 保留位3 */

    u32 outer_dipv4_h : 16; /**< 外部目标IPv4地址的高16位 */
    u32 outer_sipv4_l : 16; /**< 外部源IPv4地址的低16位 */

    u32 vni_h : 16; /**< VXLAN网络标识的高16位 */
    u32 outer_dipv4_l : 16; /**< 外部目标IPv4地址的低16位 */

    u32 rsvd4 : 16; /**< 保留位4 */
    u32 vni_l : 16; /**< VXLAN网络标识的低16位 */
};

/**
 * @struct hinic5_tcam_key_mem_htn
 * @brief 定义了一个用于存储htn TCAM键的结构体
 * @details 该结构体包含了htn TCAM键的各个字段，包括保留位、隧道类型、IP协议类型等。
 */
struct hinic5_tcam_key_mem_htn {
	u32 function_id_h : 5; /**< function id高5位 */
	u32 tunnel_type : 3;   /**< 存储隧道类型 */
	u32 ip_proto : 8;      /**< 存储IP协议类型 */
	u32 rsvd0 : 16;        /**< 保留16位 */

	u32 outer_sipv4_h : 16; /**< 外部源IPv4的高16位 */
	u32 rsvd1 : 8;          /**< 保留8位 */
	u32 outer_ip_type : 1;  /**< 存储外部IP类型 */
	u32 ip_type : 2;        /**< 存储IP类型 */
	u32 function_id_l : 5;  /**< function id低5位 */

	u32 outer_dipv4_h : 16; /**< 外部目的IPv4的高16位 */
	u32 outer_sipv4_l : 16; /**< 外部源IPv4的低16位 */

	u32 vni_h : 8; /**< 高8位存储虚拟网络标识符 */
	u32 rsvd2 : 8; /**< 保留8位 */
	u32 outer_dipv4_l : 16; /**< 外部目的IPv4的低16位 */

	u32 sipv4_h : 16; /**< 源IPv4的高16位 */
	u32 vni_l : 16;   /**< 低8位存储虚拟网络标识符 */

	u32 rsvd5 : 16; /**< 保留16位 */
	u32 sipv4_l : 16; /**< 源IPv4的低16位 */

	u32 rsvd6; /**< 保留 */
	u32 rsvd7; /**< 保留 */

	u32 dipv4_h : 16; /**< 目的IPv4的高16位 */
	u32 rsvd8 : 16;   /**< 保留16位 */

	u32 sport : 16; /**< 源端口 */
	u32 dipv4_l : 16; /**< 目的IPv4的低16位 */

	u32 rsvd9 : 16; /**< 保留16位 */
	u32 dport : 16; /**< 目的端口 */
};

/**
 * @struct hinic5_tcam_key_ipv6_mem_htn
 * @brief 定义了一个用于存储htn ipv6 TCAM键的结构体
 * @details 该结构体包含了htn ipv6 TCAM键的各个字段，包括保留位、隧道类型、IP协议类型等。
 */
struct hinic5_tcam_key_ipv6_mem_htn {
	u32 function_id_h : 5;  /**< function id高5位 */
	u32 tunnel_type : 3;    /**< 存储隧道类型 */
	u32 ip_proto : 8;       /**< 存储IP协议类型 */
	u32 rsvd0 : 16;         /**< 保留16位 */

	u32 sipv6_key0 : 16; /**< 源IPv6的第一部分 */
	u32 rsvd1 : 8; /**< 保留字段1，未使用 */
	u32 outer_ip_type : 1; /**< 外部IP类型，1位 */
	u32 ip_type : 2; /**< IP类型，2位 */
	u32 function_id_l : 5; /**< function id低5位 */

	u32 sipv6_key2 : 16; /**< 源IPv6的第二部分 */
	u32 sipv6_key1 : 16; /**< 源IPv6的第一部分 */

	u32 sipv6_key4 : 16; /**< 源IPv6的第四部分 */
	u32 sipv6_key3 : 16; /**< 源IPv6的第三部分 */

	u32 sipv6_key6 : 16; /**< 源IPv6的第六部分 */
	u32 sipv6_key5 : 16; /**< 源IPv6的第五部分 */

	u32 dipv6_key0 : 16; /**< 目的IPv6的第一部分 */
	u32 sipv6_key7 : 16; /**< 源IPv6的第七部分 */

	u32 dipv6_key2 : 16; /**< 目的IPv6的第二部分 */
	u32 dipv6_key1 : 16; /**< 目的IPv6的第一部分 */

	u32 dipv6_key4 : 16; /**< 目的IPv6的第四部分 */
	u32 dipv6_key3 : 16; /**< 目的IPv6的第三部分 */

	u32 dipv6_key6 : 16; /**< 目的IPv6的第六部分 */
	u32 dipv6_key5 : 16; /**< 目的IPv6的第五部分 */

	u32 sport : 16; /**< 源端口 */
	u32 dipv6_key7 : 16; /**< 目的IPv6的第七部分 */

	u32 rsvd2 : 16; /**< 保留字段2，未使用 */
	u32 dport : 16; /**< 目的端口 */
};

/**
 * @struct hinic5_tcam_key_vxlan_ipv6_mem_htn
 * @brief 定义了一个用于存储htn vxlan ipv6 TCAM键的结构体
 * @details 该结构体包含了htn vxlan ipv6 TCAM键的各个字段，包括保留位、隧道类型、IP协议类型等。
 */
struct hinic5_tcam_key_vxlan_ipv6_mem_htn {
	u32 function_id_h : 5; /**< function id高5位 */
	u32 tunnel_type : 3;  /**< 隧道类型 */
	u32 ip_proto : 8;     /**< IP协议 */
	u32 rsvd0 : 16;       /**< 保留16位 */

	u32 outer_sipv4_h : 16; /**< 高16位外部源IPv4地址 */
	u32 rsvd1 : 8;         /**< 保留8位 */
	u32 outer_ip_type : 1; /**< 外部IP类型 */
	u32 ip_type : 2;       /**< IP类型 */
	u32 function_id_l : 5; /**< function id低5位 */

	u32 outer_dipv4_h : 16; /**< 高16位外部目的IPv4地址 */
	u32 outer_sipv4_l : 16; /**< 低16位外部源IPv4地址 */

	u32 vni_h : 8; /**< 高8位虚拟网络标识符 */
	u32 rsvd2 : 8; /**< 保留8位 */
	u32 outer_dipv4_l : 16; /**< 低16位外部目的IPv4地址 */

	u32 rsvd3 : 16; /**< 保留16位 */
	u32 vni_l : 16; /**< 低16位虚拟网络标识符 */

	u32 dipv6_key0 : 16; /**< 第0部分的目的IPv6地址 */
	u32 rsvd4 : 16;      /**< 保留16位 */

	u32 dipv6_key2 : 16; /**< 第2部分的目的IPv6地址 */
	u32 dipv6_key1 : 16; /**< 第1部分的目的IPv6地址 */

	u32 dipv6_key4 : 16; /**< 第4部分的目的IPv6地址 */
	u32 dipv6_key3 : 16; /**< 第3部分的目的IPv6地址 */

	u32 dipv6_key6 : 16; /**< 第6部分的目的IPv6地址 */
	u32 dipv6_key5 : 16; /**< 第5部分的目的IPv6地址 */

	u32 sport : 16; /**< 源端口 */
	u32 dipv6_key7 : 16; /**< 第7部分的目的IPv6地址 */

	u32 rsvd5 : 16; /**< 保留16位 */
	u32 dport : 16; /**< 目的端口 */
};

/**
 * @struct tcam_key_ctrl_mem
 * @brief 定义了一个用于存储htn 控制报文 TCAM键的结构体
 * @details 该结构体包含了htn vxlan ipv6 TCAM键的各个字段，包括保留位、隧道类型、IP协议类型等。
 */
struct tcam_key_ctrl_mem {
    u32 function_id1 : 5;   /**< function id高5位 */
    u32 pkt_fmt : 3;    /**< 报文格式 */
    u32 packet_type : 8;    /**< 报文类型 */
    u32 rsvd0 : 16;     /**< 保留字段0 */

    u32 rsvd2 : 16;     /**< 保留字段2 */
    u32 rsvd1 : 8;      /**< 保留字段1 */
    u32 outer_type : 1;     /**< 外层类型 */
    u32 inner_type : 2;     /**< 内层类型 */
    u32 function_id2 : 5;   /**< function id低5位 */

    u32 rsvd3;      /**< 保留字段3 */
    u32 rsvd4;      /**< 保留字段4 */
    u32 rsvd5;      /**< 保留字段5 */
    u32 rsvd6;      /**< 保留字段6 */
    u32 rsvd7;      /**< 保留字段7 */
    u32 rsvd8;      /**< 保留字段8 */
    u32 rsvd9;      /**< 保留字段9 */
    u32 rsvd10;     /**< 保留字段10 */
    u32 rsvd11;     /**< 保留字段11 */
};

/**
 * @struct tag_tcam_key
 * @brief TCAM键结构体，用于存储TCAM键的信息和掩码
 * @details 该结构体包含两个联合体，分别用于存储IPv4键的信息和掩码。
 */
struct tag_tcam_key {
    /**
     * @union
     * @brief TCAM键信息
     */
    union {
        struct hinic5_tcam_key_ipv4_mem key_info;
        struct tcam_key_ctrl_mem key_info_ctrl;
        struct hinic5_tcam_key_ipv6_mem key_info_ipv6;
        struct hinic5_tcam_key_vxlan_ipv6_mem key_info_vxlan_ipv6;
        struct hinic5_tcam_key_mem_htn key_info_htn;
        struct hinic5_tcam_key_ipv6_mem_htn key_info_ipv6_htn;
        struct hinic5_tcam_key_vxlan_ipv6_mem_htn key_info_vxlan_ipv6_htn;
    };

    /**
     * @union
     * @brief TCAM键掩码
     */
    union {
        struct hinic5_tcam_key_ipv4_mem key_mask;
        struct tcam_key_ctrl_mem key_mask_ctrl;
        struct hinic5_tcam_key_ipv6_mem key_mask_ipv6;
        struct hinic5_tcam_key_vxlan_ipv6_mem key_mask_vxlan_ipv6;
        struct hinic5_tcam_key_mem_htn key_mask_htn;
        struct hinic5_tcam_key_ipv6_mem_htn key_mask_ipv6_htn;
        struct hinic5_tcam_key_vxlan_ipv6_mem_htn key_mask_vxlan_ipv6_htn;
    };
};

#define TCAM_RULE_FDIR_TYPE 0 /**< 定义TCAM规则的类型，FDIR类型对应值为0 */
#define TCAM_RULE_PPA_TYPE  1 /**< 定义TCAM规则的类型，PPA类型对应值为1 */
#define TCAM_RULE_BIFURCATION_TYPE  2 /**< 定义TCAM规则的类型，BIFURCATION类型对应值为1 */

/**
 * @struct hinic5_phy_fpga_port_stats
 * @brief 定义了一个结构体，用于存储PHY FPGA端口的统计信息
 * @details 这里是结构体的详细描述
 */
struct hinic5_phy_fpga_port_stats {
    u64 mac_rx_total_octs_port; /**< 接收的总字节数 */
    u64 mac_tx_total_octs_port; /**< 发送的总字节数 */
    u64 mac_rx_under_frame_pkts_port; /**< 接收的帧长度小于64字节的数据包数 */
    u64 mac_rx_frag_pkts_port; /**< 接收的碎片数据包数 */
    u64 mac_rx_64_oct_pkts_port; /**< 接收的64字节数据包数 */
    u64 mac_rx_127_oct_pkts_port; /**< 接收的127字节数据包数 */
    u64 mac_rx_255_oct_pkts_port; /**< 接收的255字节数据包数 */
    u64 mac_rx_511_oct_pkts_port; /**< 接收的511字节数据包数 */
    u64 mac_rx_1023_oct_pkts_port; /**< 接收的1023字节数据包数 */
    u64 mac_rx_max_oct_pkts_port; /**< 接收的最大长度数据包数 */
    u64 mac_rx_over_oct_pkts_port; /**< 接收的超长数据包数 */
    u64 mac_tx_64_oct_pkts_port; /**< 发送的64字节数据包数 */
    u64 mac_tx_127_oct_pkts_port; /**< 发送的127字节数据包数 */
    u64 mac_tx_255_oct_pkts_port; /**< 发送的255字节数据包数 */
    u64 mac_tx_511_oct_pkts_port; /**< 发送的511字节数据包数 */
    u64 mac_tx_1023_oct_pkts_port; /**< 发送的1023字节数据包数 */
    u64 mac_tx_max_oct_pkts_port; /**< 发送的最大长度数据包数 */
    u64 mac_tx_over_oct_pkts_port; /**< 发送的超长数据包数 */
    u64 mac_rx_good_pkts_port; /**< 接收的无错误数据包数 */
    u64 mac_rx_crc_error_pkts_port; /**< 接收的CRC错误数据包数 */
    u64 mac_rx_broadcast_ok_port; /**< 接收的广播数据包数 */
    u64 mac_rx_multicast_ok_port; /**< 接收的多播数据包数 */
    u64 mac_rx_mac_frame_ok_port; /**< 接收的MAC帧数据包数 */
    u64 mac_rx_length_err_pkts_port; /**< 接收的长度错误数据包数 */
    u64 mac_rx_vlan_pkts_port; /**< 接收的VLAN数据包数 */
    u64 mac_rx_pause_pkts_port; /**< 接收的暂停数据包数 */
    u64 mac_rx_unknown_mac_frame_port; /**< 接收的未知MAC帧数据包数 */
    u64 mac_tx_good_pkts_port; /**< 发送的无错误数据包数 */
    u64 mac_tx_broadcast_ok_port; /**< 发送的广播数据包数 */
    u64 mac_tx_multicast_ok_port; /**< 发送的多播数据包数 */
    u64 mac_tx_underrun_pkts_port; /**< 发送的缓冲区不足数据包数 */
    u64 mac_tx_mac_frame_ok_port; /**< 发送的MAC帧数据包数 */
    u64 mac_tx_vlan_pkts_port; /**< 发送的VLAN数据包数 */
    u64 mac_tx_pause_pkts_port; /**< 发送的暂停数据包数 */
};

/**
 * @struct hinic5_port_stats
 * @brief 定义了hinic5端口统计信息的结构体
 * @details 该结构体包含了管理消息头部信息和物理端口统计信息
 */
struct hinic5_port_stats {
    struct hinic5_mgmt_msg_head msg_head;   /**< 管理消息头部信息 */

    struct hinic5_phy_fpga_port_stats stats;    /**< 物理端口统计信息 */
};

/**
 * @struct hinic5_rss_indir_table
 * @brief 定义了一个RSS（Receive Side Scaling，接收端缩放）的间接查找表结构体
 * @details 该结构体用于存储RSS的相关信息，包括管理消息头部、函数ID、保留字段和间接查找表。
 */
struct hinic5_rss_indir_table {
    struct hinic5_mgmt_msg_head msg_head;   /**< 管理消息头部 */

    u16 func_id;    /**< func id */
    u16 rsvd1;  /** 保留字段1 */
    u8 indir[NIC_RSS_INDIR_SIZE];   /**< 间接查找表 */
};

#define NIC_RSS_CMD_TEMP_ALLOC 0x01     /**< 用于临时分配rss资源 */
#define NIC_RSS_CMD_TEMP_FREE 0x02      /**< 用于临时释放rss资源 */

/**
 * @struct hinic5_func_tbl_cfg_bitmap
 * @brief 函数表配置位图结构体
 * @details 该结构体用于表示函数表配置位图，包含了初始化配置、接收缓冲区大小配置和最大传输单元配置。
 */
enum hinic5_func_tbl_cfg_bitmap {
	FUNC_CFG_INIT,              /**< 初始化配置 */
	FUNC_CFG_RX_BUF_SIZE,       /**< 接收缓冲区大小配置 */
	FUNC_CFG_MTU,               /**< 最大传输单元配置 */
    FUNC_CFG_ISOLATION_VF_MAC,    /**< 集群模式VF MAC配置 */
    FUNC_CFG_ISOLATION_VF_SVLAN,    /**< 集群模式VF SVLAN配置 */
};

typedef struct mac_table_cnt {
    u32 valid_table_cnt;  /**< 有效的MAC表数量 */
    u32 mac_table_cnt;    /**< 最大支持的MAC表数量 */
    u16 uc_mac_cnt;       /**< 单播计数 */
    u16 mc_mac_cnt;       /**< 组播计数 */
} mac_table_cnt_s;

#define NIC_FUNC_MAX_NUM 4096
typedef struct mac_table_res_stat {
    u16 uc_mac_cnt;                               /**< 单播表资源使用统计 */
    u16 mc_mac_cnt;                               /**< 组播表资源使用统计 */
    u16 share_mac_res_cur_cnt;                    /**< 当前共享资源池使用统计 */
    u16 share_mac_res_total;                      /**< 共享资源池总大小 */
    u16 func_uc_mac_cnt[NIC_FUNC_MAX_NUM];        /**< Func粒度单播表资源使用统计 */
} mac_table_res_stat_s;

#define HINIC5_CMD_OP_SET	1   /**< cmd操作类型为set */
#define HINIC5_CMD_OP_GET	0   /**< cmd操作类型为get */

#define HINIC5_CMD_OP_ADD	1   /**< cmd操作类型为add */
#define HINIC5_CMD_OP_DEL	0   /**< cmd操作类型为del */

/**
 * @brief 定义枚举类型，用于表示不同的命令类型
 * @details 这个枚举类型定义了一系列的命令类型，用于在不同的上下文中表示不同的操作。
 */
enum {
    PPA_TABLE_ID_CLEAN_CMD = 0, /**< 清理PPA表的命令*/
    PPA_TABLE_ID_ADD_CMD,   /**< 添加PPA表的命令*/
    PPA_TABLE_ID_DEL_CMD,   /**< 删除PPA表的命令*/
    FDIR_TABLE_ID_ADD_CMD,  /**< 添加FDIR表的命令*/
    FDIR_TABLE_ID_DEL_CMD,  /**< 删除FDIR表的命令*/
    PPA_TABEL_ID_MAX    /**< PPA表的最大值*/
};

/**
 * @brief 定义一个枚举类型，用于表示网卡NVM数据的类型
 * @details 这个枚举类型包含了多个标志位，每个标志位代表一种网卡NVM数据的类型。
 */
enum {
    NIC_NVM_DATA_SET = BIT(0), /**< 1-save, 0-read */
    NIC_NVM_DATA_PXE = BIT(1),  /**< PXE */
    NIC_NVM_DATA_VLAN = BIT(2), /**< VLAN */
    NIC_NVM_DATA_VLAN_PRI = BIT(3), /**< VLAN PRI */
    NIC_NVM_DATA_VLAN_ID = BIT(4),  /**< VLAN ID */
    NIC_NVM_DATA_WORK_MODE = BIT(5),    /**< 任务类型 */
    NIC_NVM_DATA_PF_SPEED_LIMIT = BIT(6),   /**< PPF限速 */
    NIC_NVM_DATA_GE_MODE = BIT(7),  /**< GE模式 */
    NIC_NVM_DATA_AUTO_NEG = BIT(8), /**< AUTO NEG */
    NIC_NVM_DATA_LINK_FEC = BIT(9), /**< LINK FEC */
    NIC_NVM_DATA_PF_ADAPTIVE_LINK = BIT(10),    /**< PF自适应link */
    NIC_NVM_DATA_SRIOV_CONTROL = BIT(11),   /**< SRIOV CONTROL */
    NIC_NVM_DATA_EXTEND_MODE = BIT(12), /**< 拓展模式 */
    NIC_NVM_DATA_RESET = BIT(31),   /**< RESET */
};

#define BIOS_CFG_SIGNATURE                  0x1923E518    /**< 定义BIOS配置签名 */
#define BIOS_OP_CFG_ALL(op_code_val)        ((((op_code_val) >> 1) & (0xFFFFFFFF)) != 0)    /**< 定义BIOS操作码全部配置的宏 */
#define BIOS_OP_CFG_WRITE(op_code_val)      ((((op_code_val) & NIC_NVM_DATA_SET)) != 0)    /**< 定义BIOS操作码写入配置的宏 */
#define BIOS_OP_CFG_PXE_EN(op_code_val)     (((op_code_val) & NIC_NVM_DATA_PXE) != 0)    /**< 定义BIOS操作码PXE启用的宏 */
#define BIOS_OP_CFG_VLAN_EN(op_code_val)    (((op_code_val) & NIC_NVM_DATA_VLAN) != 0)    /**< 定义BIOS操作码VLAN启用的宏 */
#define BIOS_OP_CFG_VLAN_PRI(op_code_val)   (((op_code_val) & NIC_NVM_DATA_VLAN_PRI) != 0)    /**< 定义BIOS操作码VLAN优先级的宏 */
#define BIOS_OP_CFG_VLAN_ID(op_code_val)    (((op_code_val) & NIC_NVM_DATA_VLAN_ID) != 0)    /**< 定义BIOS操作码VLAN ID的宏 */
#define BIOS_OP_CFG_WORK_MODE(op_code_val)  (((op_code_val) & NIC_NVM_DATA_WORK_MODE) != 0)    /**< 定义BIOS操作码工作模式的宏 */
#define BIOS_OP_CFG_PF_BW(op_code_val)      (((op_code_val) & NIC_NVM_DATA_PF_SPEED_LIMIT) != 0)    /**< 定义BIOS操作码PF带宽的宏 */
#define BIOS_OP_CFG_GE_SPEED(op_code_val)   (((op_code_val) & NIC_NVM_DATA_GE_MODE) != 0)    /**< 定义BIOS操作码GE速度的宏 */
#define BIOS_OP_CFG_AUTO_NEG(op_code_val)   (((op_code_val) & NIC_NVM_DATA_AUTO_NEG) != 0)    /**< 定义BIOS操作码自动协商的宏 */
#define BIOS_OP_CFG_LINK_FEC(op_code_val)   (((op_code_val) & NIC_NVM_DATA_LINK_FEC) != 0)    /**< 定义BIOS操作码链路FEC的宏 */
#define BIOS_OP_CFG_AUTO_ADPAT(op_code_val) (((op_code_val) & NIC_NVM_DATA_PF_ADAPTIVE_LINK) != 0)    /**< 定义BIOS操作码自动适应的宏 */
#define BIOS_OP_CFG_SRIOV_ENABLE(op_code_val) (((op_code_val) & NIC_NVM_DATA_SRIOV_CONTROL) != 0)    /**< 定义BIOS操作码SR-IOV启用的宏 */
#define BIOS_OP_CFG_EXTEND_MODE(op_code_val)  (((op_code_val) & NIC_NVM_DATA_EXTEND_MODE) != 0)    /**< 定义BIOS操作码扩展模式的宏 */
#define BIOS_OP_CFG_RST_DEF_SET(op_code_val)  (((op_code_val) & (u32)NIC_NVM_DATA_RESET) != 0)    /**< 定义BIOS操作码重置默认设置的宏 */

#define ENHANCED_CMDQ_CTX_SIZE 0x30     /**< 定义增强型cmdq context size为48 */

#define FLOW_BIFURCATE_BIT  1U << 10 /* 1872流分叉bit位为10 */

#define HINIC5_LRO_DEFAULT_COAL_PKT_SIZE	32
#define HINIC5_LRO_DEFAULT_TIME_LIMIT		16

#define HINIC5_SET_PORT_CAR_PROFILE 0
#define HINIC5_SET_PORT_CAR_STATE 1
#define HINIC5_GET_PORT_CAR_LIMIT_SPEED 2

#define HINIC5_SET_CAR_PROFILE 0
#define HINIC5_GET_CAR_PROFILE 1

#define HINIC5_FUNC_CAR_ID_OFFSET 16

#define HINIC5_HTN_CMD_SET_CAR    0x26
#define HINIC5_HTN_CMD_GET_CAR    0x27

#define CAR_PROFILE_SIZE 	32
#define CAR_INDEX_UNIT		16

#define NIC_MPU_LT_RD_NOT_SUPPORT_ERROR 253
#define NIC_MPU_LT_OPERA_RANGE_ERROR 254

#define CMD_QOS_ETS_COS_TC     BIT(0)
#define CMD_QOS_ETS_TC_BW      BIT(1)
#define CMD_QOS_ETS_COS_PRIO   BIT(2)
#define CMD_QOS_ETS_COS_BW     BIT(3)
#define CMD_QOS_ETS_TC_PRIO    BIT(4)

#define CMD_QOS_PORT_TRUST     BIT(0)
#define CMD_QOS_PORT_DFT_COS   BIT(1)

#define CMD_QOS_MAP_PCP2COS     BIT(0)
#define CMD_QOS_MAP_DSCP2COS    BIT(1)

#define STD_SFP_INFO_MAX_SIZE 640

#define HINIC5_PF_SET_VF_ALREADY 0x4

typedef enum {
    HINIC5_GET_CNT = 0,
    HINIC5_GET_CNT_RES,
    HINIC5_ADD_CNT,
    HINIC5_DEL_CNT,
    HINIC5_DEL_ALL_CNT,
    HINIC5_RESET_CNT,
    HINIC5_RESET_ALL_CNT,
    HINIC5_NIC_OP_MAX,
} nic_cnt_op_e;

#define VEB_OFFLOAD_QUERY      0
#define VEB_OFFLOAD_SET        1
#define VEB_OFFLOAD_STATUS_OFF      0
#define VEB_OFFLOAD_STATUS_ON       1
#define VEB_OFFLOAD_STATUS_INVALID  2

enum hinic5_port_car_type {
    HINIC5_PORT_CAR_TYPE_PORT = 0,
    HINIC5_PORT_CAR_TYPE_FUNC,
    HINIC5_PORT_CAR_TYPE_VNIC_GROUP,
};

enum hinic5_port_car_pkt_type {
    HINIC5_PORT_CAR_PKT_TYPE_TCP = 0,
    HINIC5_PORT_CAR_PKT_TYPE_UDP,
    HINIC5_PORT_CAR_PKT_TYPE_ARP,
    HINIC5_PORT_CAR_PKT_TYPE_ICMP,
    HINIC5_PORT_CAR_PKT_TYPE_MAX,
};

enum hinic5_port_car_level {
    HINIC5_PORT_CAR_LEVEL_256M = 0,
    HINIC5_PORT_CAR_LEVEL_500M,
    HINIC5_PORT_CAR_LEVEL_1G,
    HINIC5_PORT_CAR_LEVEL_2G,
    HINIC5_PORT_CAR_LEVEL_INVALID_NUM = 0xFF,
};

typedef enum {
    NIC_SOFT_LRO_EN_OPERATE = 0,   /* 软件LRO使能操作 */
    NIC_HW_LRO_LEN_OPERATE,        /* 硬件LRO聚合长度操作 */
    NIC_HW_LRO_NUM_OPERATE,        /* 硬件LRO聚合数量操作 */
    NIC_HW_LRO_TIMER_OPERATE,      /* 硬件LRO聚合时间操作 */
    NIC_LRO_CFG_OPERATE_MAX
} lro_cfg_operate_type_u;

struct cmd_mac_info_set_s {
    struct mgmt_msg_head head;

    u16 is_valid;
    u16 rsvd0;
    u8 mac_addr[ETH_ALEN];
    u8 rsvd1[2];
};

#endif
