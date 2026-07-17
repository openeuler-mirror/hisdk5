#ifndef NIC_NPU_WQE_DEFINE_H
#define NIC_NPU_WQE_DEFINE_H

#include "typedef.h"

/**
 * @brief 内部/外部IP类型
 * @details 定义一个枚举类型，用于表示内部/外部IP类型
 */
typedef enum {
    /*
     * 00 - non ip packet or packet type is not defined by software
     * 01 - ipv6 packet
     * 10 - ipv4 packet with no ip checksum offload
     * 11 - ipv4 packet with ip checksum offload
     */
    NON_IP_TYPE = 0,
    TYPE_IPV6,
    TYPE_IPV4,
    TYPE_IPV4_CS_OFF
} qsf_ip_type_e;

/**
 * @brief 定义隧道类型枚举，用于表示不同的隧道类型
 * @details 此枚举用于表示不同的隧道类型，包括无隧道、UDP隧道（无CS）、UDP隧道（有CS）和GRE隧道。
 */
typedef enum {
    /*
     * 0 - no udp / gre tunneling / no tunnel
     * 1 - udp tunneling header with no cs
     * 2 - udp tunneling header with cs
     * 3 - gre tunneling header
     */
    L4_TUNNEL_NO_TUNNEL = 0,
    L4_TUNNEL_UDP_NO_CS,
    L4_TUNNEL_UDP_CS,
    L4_TUNNEL_GRE
} qsf_tunnel_type_e;

/**
 * @brief 定义一个枚举类型，用于表示不同的第四层协议类型
 * @details 这个枚举类型包含了四种不同的第四层协议类型，分别是未知/碎片化的数据包、传输控制协议、流控制传输协议和用户数据报协议。
 */
typedef enum {
    /*
     * 00b - unknown / fragmented packet
     * 01b - tcp
     * 10b - sctp
     * 11b - udp
     */

    L4_TYPE_UNKNOWN = 0,
    L4_TYPE_TCP,
    L4_TYPE_SCTP,
    L4_TYPE_UDP
} qsf_l4_offload_type_e;

/**
 * @struct tag_l2nic_rx_compact_cqe
 * @brief 定义了一个接收完成队列条目（Completion Queue Entry, CQE）的结构体，
 *        该结构体用于描述接收到的数据包的各种属性。
 * @details 这个结构体用于表示接收到的数据包的完成状态，
 *          它包含了关于数据包的各种标志和错误信息。
 */
typedef struct tag_l2nic_rx_compact_cqe {
    /**
     * @union dw0
     * @brief 该联合体用于存储接收到的数据包的各种属性。
     */
    union {
        struct {
#if (BYTE_ORDER == BIG_ENDIAN)
            u32 rx_done : 1;  /**< 接收完成标志 */
            u32 cqe_type : 1; /**< CQE类型 */
            u32 ts_flag : 1;  /**< 时间戳标志 */
            u32 vlan_offload : 1; /**< VLAN硬解析标志 */
            u32 pkt_fmt : 3;      /**< 数据包格式 */
            u32 ip_type : 1;      /**< IP类型 */
            u32 cqe_len : 1;      /**< CQE长度 */
            u32 pkt_mc : 2;       /**< 数据包多播标志 */
            u32 checksum_err : 2; /**< 校验和错误标志 */
            u32 pkt_type : 3;     /**< 数据包类型 */
            u32 pkt_len : 16;     /**< 数据包长度 */
#else
            u32 pkt_len : 16;   /**< 数据包长度 */
            u32 pkt_type : 3;   /**< 数据包类型 */
            u32 checksum_err : 2;   /**< 校验和错误标志 */
            u32 pkt_mc : 2;     /**< 数据包多播标志 */
            u32 cqe_len : 1;    /**< CQE长度 */
            u32 ip_type : 1;    /**< IP类型 */
            u32 pkt_fmt : 3;    /**< 数据包格式 */
            u32 vlan_offload : 1;   /**< VLAN硬解析标志 */
            u32 ts_flag : 1;    /**< 时间戳标志 */
            u32 cqe_type : 1;   /**< CQE类型 */
            u32 rx_done : 1;    /**< 接收完成标志 */
#endif
        } bs;
        u32 value; /**< 存储所有字段的值 */
    } dw0;
 
    /**
     * @union dw1
     * @brief 该联合体用于存储接收到的数据包的RSS哈希值。
     */
    union {
        struct {
            u32 rss_hash_value; /**< RSS哈希值 */
        } bs;
        u32 value; /**< 存储所有字段的值 */
    } dw1;
 
    /**
     * @union dw2
     * @brief 该联合体用于存储接收到的数据包的各种属性。
     */
    union {
        struct {
#if (BYTE_ORDER == BIG_ENDIAN)
            u32 lro_num : 8;       /**< LRO（Large Receive Offload）数量 */
            u32 vlan_tag : 16;     /**< VLAN标签 */
            u32 rsvd2 : 2;         /**< 保留字段 */
            u32 pfe_tx_pkt_en : 1; /**< PFE（Physical Function Engine）传输数据包使能 */
            u32 port_id : 2;       /**< 端口ID */
            u32 flow_mark_vld : 1; /**< 流量标记有效标志 */
            u32 src_function_id_h : 2; /**< 源函数ID的高8位 */
#else
            u32 src_function_id_h : 2;  /**< 源函数ID的高8位 */
            u32 flow_mark_vld : 1;  /**< 流量标记有效标志 */
            u32 port_id : 2;        /**< 端口ID */
            u32 pfe_tx_pkt_en : 1;  /**< PFE（Physical Function Engine）传输数据包使能 */
            u32 rsvd2 : 2;          /**< 保留字段 */
            u32 vlan_tag : 16;      /**< VLAN标签 */
            u32 lro_num : 8;        /**< LRO（Large Receive Offload）数量 */
#endif
        } bs;
        u32 value; /**< 存储所有字段的值 */
    } dw2;
 
    /**
     * @union dw3
     * @brief 该联合体用于存储接收到的数据包的各种属性。
     */
    union {
        struct {
#if (BYTE_ORDER == BIG_ENDIAN)
            u32 src_function_id_l : 8; /**< 源函数ID的低8位 */
            u32 flow_mark : 24;        /**< 流量标记 */
#else
            u32 flow_mark : 24;         /**< 流量标记 */
            u32 src_function_id_l : 8;  /**< 源函数ID的低8位 */
#endif
        } bs;
        u32 value; /**< 存储所有字段的值 */
    } dw3;
}  l2nic_rx_compact_cqe_s;

/**
 * @struct l2nic_rx_cqe_s.
 * @brief L2nic_rx_cqe_s data structure.
 * @details 这个结构体用于表示接收到的数据包的完成状态，
 *          它包含了关于数据包的各种标志和错误信息。
 */
typedef struct tag_l2nic_rx_cqe {
    union {
        struct {
#if (BYTE_ORDER == BIG_ENDIAN)
            u32 rx_done : 1; /**< 表示数据包已经完全接收 */
            u32 bp_en : 1;   /**< 表示缓冲池已经启用 */
            u32 decry_pkt : 1; /**< 表示需要解密数据包 */
            u32 flush : 1; /**< 表示需要刷新数据包 */
            u32 spec_flags : 3; /**< 特殊标志位 */
            u32 rsvd0 : 1; /**< 保留字段 */
            u32 lro_num : 8; /**< 大接收离散连接数 */
            u32 checksum_err : 16; /**< 校验和错误信息 */
#else
            u32 checksum_err : 16; /**< 校验和错误信息 */
            u32 lro_num : 8; /**< 大接收离散连接数 */
            u32 rsvd0 : 1; /**< 保留字段 */
            u32 spec_flags : 3; /**< 特殊标志位 */
            u32 flush : 1; /**< 表示需要刷新数据包 */
            u32 decry_pkt : 1; /**< 表示需要解密数据包 */
            u32 bp_en : 1; /**< 表示缓冲池已经启用 */
            u32 rx_done : 1; /**< 表示数据包已经完全接收 */
#endif
        } bs;
        u32 value; /**< 结构体的值 */
    } dw0;

    union {
        struct {
#if (BYTE_ORDER == BIG_ENDIAN)
            u32 length : 16;    /**< 包长 */
            u32 vlan : 16;  /**< vlan号 */
#else
            u32 vlan : 16;  /**< vlan号 */
            u32 length : 16;    /**< 包长 */
#endif
        } bs;
        u32 value;  /**< 结构体的值 */
    } dw1;

    union {
        struct {
#if (BYTE_ORDER == BIG_ENDIAN)
            u32 rss_type : 8;   /**< rss类型 */
            u32 rsvd0 : 2;  /**< 保留字段 */
            u32 vlan_offload_en : 1;    /**< vlan卸载使能位 */
            u32 umbcast : 2;    /**< umb播 */
            u32 rsvd1 : 7;  /**< 保留字段 */
            u32 pkt_types : 12; /**< 报文类型 */
#else
            u32 pkt_types : 12; /**< 报文类型 */
            u32 rsvd1 : 7;  /**< 保留字段 */
            u32 umbcast : 2;    /**< umb播 */
            u32 vlan_offload_en : 1;    /**< vlan卸载使能位 */
            u32 rsvd0 : 2;  /**< 保留字段 */
            u32 rss_type : 8;   /**< rss类型 */
#endif
        } bs;
        u32 value;  /**< 结构体的值 */
    } dw2;

    union {
        struct {
            u32 rss_hash_value; /**< rss哈希值 */
        } bs;
        u32 value;  /**< 结构体的值 */
    } dw3;

    /**< dw4~dw7 field for nic/ovs multipexing */
    union {
        struct { /**< for nic */
#if (BYTE_ORDER == BIG_ENDIAN)
            u32 if_1588 : 1; /**< 定义一个32位无符号整数，用于表示是否支持1588协议 */
            u32 if_tx_ts : 1; /**< 定义一个32位无符号整数，用于表示是否支持发送时间戳 */
            u32 if_rx_ts : 1; /**< 定义一个32位无符号整数，用于表示是否支持接收时间戳 */
            u32 rsvd : 1; /**< 定义一个32位无符号整数，用于保留 */
            u32 msg_1588_type : 4; /**< 定义一个32位无符号整数，用于表示1588协议的类型 */
            u32 msg_1588_offset : 8; /**< 定义一个32位无符号整数，用于表示1588协议的偏移量 */
            u32 tx_ts_seq : 16; /**< 定义一个32位无符号整数，用于表示发送时间戳的序列号 */
#else
            u32 tx_ts_seq : 16; /**< 定义一个32位无符号整数，用于表示发送时间戳的序列号 */
            u32 msg_1588_offset : 8; /**< 定义一个32位无符号整数，用于表示1588协议的偏移量 */
            u32 msg_1588_type : 4; /**< 定义一个32位无符号整数，用于表示1588协议的类型 */
            u32 rsvd : 1; /**< 定义一个32位无符号整数，用于保留 */
            u32 if_rx_ts : 1; /**< 定义一个32位无符号整数，用于表示是否支持接收时间戳 */
            u32 if_tx_ts : 1; /**< 定义一个32位无符号整数，用于表示是否支持发送时间戳 */
            u32 if_1588 : 1; /**< 定义一个32位无符号整数，用于表示是否支持1588协议 */
#endif
        } bs;

        struct { /**< for ovs */
            u32 reserved;   /**< 保留字段 */
        } ovs_bs;

        struct {
            u32 xid;    /**< x id for crypt*/
        } crypt_bs;

        u32 value;  /**< 结构体的值 */
    } dw4;

    union {
        struct { /**< for nic */
            u32 msg_1588_ts;
        } bs;

        struct { /**< for ovs */
#if (BYTE_ORDER == BIG_ENDIAN)
            u32 mac_type : 2; /**< for ovs. mac_type */
            u32 l3_type : 3;  /**< for ovs. l3_type */
            u32 l4_type : 3;  /**< for ovs. l4_type */
            u32 rsvd0 : 2;  /**< 保留字段 */
            u32 traffic_type : 6;  /**< for ovs. traffic type: 0-default l2nic pkt, 1-fallback traffic, 2-miss upcall
                                      traffic, 2-command */
            u32 traffic_from : 16; /**< for ovs. traffic from: vf_id, only support traffic_type=0(default l2nic) or 2(miss
                                      upcall) */
#else
            u32 traffic_from : 16;  /**< for ovs. traffic from: vf_id, only support traffic_type=0(default l2nic) or 2(miss
                                      upcall) */
            u32 traffic_type : 6;   /**< for ovs. traffic type: 0-default l2nic pkt, 1-fallback traffic, 2-miss upcall
                                      traffic, 2-command */
            u32 rsvd0 : 2;  /**< 保留字段 */
            u32 l4_type : 3;    /**< for ovs. l4_type */
            u32 l3_type : 3;    /**< for ovs. l3_type */
            u32 mac_type : 2;   /**< for ovs. mac_type */
#endif
        } ovs_bs;

        struct { /**< for crypt */
#if (BYTE_ORDER == BIG_ENDIAN)
            u32 rsvd : 16;  /**< 保留字段 */
            u32 decrypt_status : 8; /**< 解密状态 */
            u32 esp_next_head : 8;  /**< 下个头部 */
#else
            u32 esp_next_head : 8;  /**< 下个头部 */
            u32 decrypt_status : 8; /**< 解密状态 */
            u32 rsvd : 16;  /**< 保留字段 */
#endif
        } crypt_bs; /**< 结构体的值 */

        u32 value;
    } dw5;

    union {
        struct { /**< for nic */
            u32 lro_ts; /**< 未使用 */
        } bs;

        struct { /**< for ovs */
            u32 reserved;   /**< 保留字段 */
        } ovs_bs;

        u32 value;  /**< 结构体的值 */
    } dw6;

    union {
        struct { /**< for nic */
            u32 first_len : 13;   /**< Datalen of the first or middle pkt size. */
            u32 last_len : 13;    /**< Data len of the last pkt size. */
            u32 pkt_num : 5;      /**< the number of packet. */
            u32 super_cqe_en : 1; /**< only this bit = 1, other fileds in this DW is valid. */
        } bs;

        struct { /**< for ovs */
            u32 localtag;   /**< local tag */
        } ovs_bs;

        u32 value;  /**< 结构体的值 */
    } dw7;
} l2nic_rx_cqe_s;


#endif
