/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
 * Filename      : macsec_mpu_cmd_defs.h
 * Creation time : 2024/02/23
 * Description   : COMM Commands between host and MPU(macsec)
 * Version       : 1.0
 */

#ifndef MACSEC_MPU_CMD_DEFS_H
#define MACSEC_MPU_CMD_DEFS_H

#include "mpu_cmd_base_defs.h"

#define MACSEC_PORT_NUM 4 /**< macsec最大的端口规格 */
#define MACSEC_SC_NUM 32 /**< macsec最大的SC规格 */
#define MACSEC_SA_NUM 64 /**< macsec最大的SA规格 */
#define MACSEC_MAX_FEATURE_QWORD 4 /**< macsec最大特性规格 */
#define MACSEC_WHITELIST_DMAC_ID_MAX 16 /**< macsec dmac id最大模板数量 */
#define MACSEC_WHITELIST_TOOL_IN_BUF_MAX 1024
#define MACSEC_WHITELIST_TOOL_OUT_BUF_MAX 4096
#define MACSEC_WHITELIST_DMAC_ENABLE 1 /**< macsec dmac使能 */
#define MACSEC_WHITELIST_DMAC_DISABLE 0 /**< macsec dmac不使能 */
#define MACSEC_WHITELIST_SERVICE_ENABLE 1 /**< macsec 使能 */
#define MACSEC_WHITELIST_SERVICE_DISABLE 0 /**< macsec 不使能 */
#define WHITELIST_FUNC_ENABLE 0x1
#define WHITELIST_FUNC_DISABLE 0x0
#define MACSEC_DMAC_TEMPLATE_NUM 16 /**< macsec dmac最大模板数量 */
#define MACSEC_ETYPE_TEMPLATE_NUM 16 /**< macsec etype最大模板数量 */
#define MACSEC_VLANID_MAX 4094 /**< macsec vlanid最大值 */
#define MACSEC_VLANID_MIN 1 /**< macsec vlanid最小值 */
#define MACSEC_RXVLANID_MASK_MAX 0xFFF /**< macsec rxvlan mask最大值 */
#define MACSEC_DMAC_VALUE_MAX 0xFFFFFFFFFFFF /**< macsec dmac最大值 */
#define MACSEC_DMAC_MASK_MAX 0xFF /**< macsec dmac mask最大值 */

/**
 * @brief macsec_sa_info_s - SA结构体
 * @details 通过该结构体进行SA的管理
 */
typedef struct tag_macsec_sa_info_s {
    // encryption
    u64 pn_th; /**< The threshold of PN */
    u64 next_pn; /**< NEXTPN number */
    u8 enable_transmit; /**< Indicates if the transmit SA can be used to receive frames */
    u8 rsvd0[7]; /**< for expanding */

    // decryption
    u8 replay_protect; /**< Enable or disable replay protection function */
    u8 enable_receive; /**< Indicates if the transmit SA can be used to receive frames */
    u8 rsvd1[2];
    u32 replay_window; /**< Indicates the replay protection window size */
    u64 lowest_pn; /**< LOWESTPN number */

    // common
    u8 an; /**< an value used by sa */
    u8 confidentiality_offset; /**< The confidentaility protection offset options of the cipher suite */
    u8 current_key_length; /**< Indicate the key length the SA is currently using */
    u8 extended_pn_enable; /**< Indicates to enable the extended packet number while receiving MACsec frames */
    u8 current_crypto_algo; /**< Indicates the crypto algorithm which the SA is currently using */
    u8 rsvd2[7]; /**< for expanding */
    u32 ssci; /**< If extended_pn_enable is 1, use SSCI to compose IV */
    u32 salt[3]; /**< Used to generate IV in extended Packet Number mode, 96bit */
    u32 sak_crc; /**< sak key crc val */
    u64 sci; /**< 64-bit SCI value used to identify an SC */
    u32 sak[8]; /**< cipher suite key,max 256bit */
} macsec_sa_info_s;

/**
 * @brief macsec_sc_info_s - SC结构体
 * @details 通过该结构体进行SC的管理
 */
typedef struct tag_macsec_sc_info_s {
    // encryption
    u8 protect_frames; /**< Protect function for frames */
    u8 protection_mode; /**< Protection options of the Cipher Suite */
    u8 include_sci_enable; /**< Indicates to include the SCI information in SecTAG field */
    u8 use_es_enable; /**< Indicates to enable the ES bit in SecTAG */
    u8 use_scb_enable; /**< Indicates to enable the SCB bit in SecTAG field */
    u8 encoding_sa; /**< The current transmitting SA in use */
    u8 rsvd0[2]; /**< for expanding */

    // decryption
    u8 validate_frames; /**< Uses to control the validation function for frames per SC */
    u8 rsvd1[3]; /**< for expanding */

    // common
    u8 sa_an[4]; /**< AN number used by SA, 4 refer to max num */
    u64 sci; /**< 64-bit SCI value used to identify an SC */
} macsec_sc_info_s; /**< Common head info */

/**
 * @brief macsec_cmd_sc_operation_s - 与主机驱动之间的SC管理信息
 * @details 通过该信息的传递，进行SC的创建、删除和更新，各SC之间独立。
 */
typedef struct tag_macsec_cmd_sc_operation_s {
    struct mgmt_msg_head head; /**< Common head info */
    u8 op_code; /**< Operation code, 0: enc get/1: enc set/2: enc update/3: dec get/4: dec set/5: dec update */
    u8 rsvd[3]; /**< reserved for expanding */
    macsec_sc_info_s sc_info; /**< 与主机驱动之间传递的sc信息 */
} macsec_cmd_sc_operation_s;

/**
 * @brief macsec_cmd_sa_operation_s - 与主机驱动之间的SA管理信息
 * @details 用作SA的创建、删除和更新，其中包括密钥管理、重放管理以及PN值等信息
 */
typedef struct tag_macsec_cmd_sa_operation_s {
    struct mgmt_msg_head head;  /**< Common head info */
    u8 op_code; /**< Operation code, 0: enc get/1: enc set/2: enc update/3: dec get/4: dec set/5: dec update */
    u8 rsvd[3]; /**< reserved for expanding */
    macsec_sa_info_s sa_info; /**< 与主机驱动之间传递的sa信息 */
} macsec_cmd_sa_operation_s;

/**
 * @brief macsec_port_mib_info_s - PORT MIB具体内容
 * @details 包括PORT级别的SecTAG异常、完整性保护和加密保护的统计值
 */
typedef struct {
    u64 macsec_mib_txpkts_untagged; /**< 传输的不带SecTAG的报文数 */
    u64 macsec_mib_txoctets_protected; /**< 传输的只做完整性保护不做加密的字节数 */
    u64 macsec_mib_txoctets_encrypted; /**< 传输的即做完整性保护也做加密的字节数 */
    u64 macsec_mib_rxpkts_untagged; /**< 接收不带SecTAG的报文数 not strict mode */
    u64 macsec_mib_rxpkts_notag; /**< 接收不带SecTAG的报文数 strict mode */
    u64 macsec_mib_rxpkts_badtag; /**< 接收的无效SecTAG或ICV无效的报文数，V、SC、SL出现错误 */
    u64 macsec_mib_rxpkts_nosa; /**< 接收的未知SCI或未使用的SA的报文数 未设置validateEnable&C&strict mode */
    u64 macsec_mib_rxpkts_nosaerror; /**< 接收的未知SCI或未使用的SA的报文数 设置validateEnable&C&strict mode */
    u64 macsec_mib_rxoctets_validated; /**< 接收到的只做完整性保护不做加密的字节数 */
    u64 macsec_mib_rxoctets_decrypted; /**< 接收到的即做完整性保护也做加密的字节数 */
} macsec_port_mib_info_s;

/**
 * @brief macsec_sc_mib_info_s - SC MIB具体内容
 * @details 包括SC级别的完整性保护和加密保护的统计值、重放以及安全校验相关的统计值
 */
typedef struct {
    u64 macsec_mib_txscpkts_protected; /**< 传输的只做完整性保护不做加密的报文数 */
    u64 macsec_mib_txscpkts_encrypted; /**< 传输的即做完整性保护也做加密的报文数 */
    u64 macsec_mib_rxscpkts_ok; /**< 成功验证且在重放窗口内接收的报文数 */
    u64 macsec_mib_rxscpkts_delayed; /**< 报文的PN小于可接受的最小PN的报文数 */
    u64 macsec_mib_rxscpkts_late; /**< ReplayProtect为true，且报文的PN小于重放检查PN的下限的报文数 */
    u64 macsec_mib_rxscpkts_unchecked; /**< validateFrames disable，SecTAG中的C位没有设置，且该数据包不是重放数据包 */
    u64 macsec_mib_rxscpkts_notvalid; /**< ICV验证失败，但由于validateFrames为strtic或数据已加密而被丢弃的报文数 */
    u64 macsec_mib_rxscpkts_invalid; /**< validateFrames标识check，但是数据没有加密（原始帧可能是recovered）的报文数 */
} macsec_sc_mib_info_s;

/**
 * @brief macsec_cmd_port_mib_operation_s - PORT MIB管理信息
 * @details 遵循macsec标准协议，获取指定端口的MIB信息
 */
typedef struct tag_macsec_cmd_port_mib_operation_s {
    struct mgmt_msg_head head;  /**< Management msg header info, 8B */
    macsec_port_mib_info_s port_mib; /**< port mib info */
    u64 reserved; /**< reserved for expanding */
} macsec_cmd_port_mib_operation_s;

/**
 * @brief macsec_cmd_sc_mib_operation_s - SC MIB管理信息
 * @details 遵循macsec标准协议，获取单个SC的MIB信息
 */
typedef struct tag_macsec_cmd_sc_mib_operation_s {
    struct mgmt_msg_head head;  /**< Management msg header info, 8B */
    macsec_sc_mib_info_s sc_mib; /**< sc mib info */
    u64 sci; /**< 64-bit SCI value used to identify an SC */
    u8 reserved[8]; /**< reserved for expanding */
} macsec_cmd_sc_mib_operation_s;

/**
 * @brief macsec_enc_cnt_u - MACSEC加密侧的DFX统计值
 * @details 包括加密侧可纠和不可纠异常、数据路径的sop和eop异常以及根据profile id进行不同处理的统计值等
 */
typedef union {
    struct {
        u32 macsec_enc_msop_cnt; /**< MSOP中断产生的次数 */
        u32 macsec_enc_meop_cnt; /**< MEOP中断产生的次数 */
        u32 macsec_enc_iadp_ierr_c_cnt; /**< IADP中断可纠错误统计 */
        u32 macsec_enc_iadp_ierr_u_cnt; /**< IADP中断不可纠错误统计 */
        u32 macsec_enc_oadp_ierr_c_cnt; /**< OADP中断可纠错误统计 */
        u32 macsec_enc_oadp_ierr_u_cnt; /**< OADP中断不可纠错误统计 */
        u32 macsec_enc_cs_ierr_c_cnt; /**< CS中断可纠错误统计 */
        u32 macsec_enc_cs_ierr_u_cnt; /**< CS中断不可纠错误统计 */
        u32 macsec_enc_mib_ierr_c_cnt; /**< MIB中断可纠错误统计 */
        u32 macsec_enc_mib_ierr_u_cnt; /**< MIB中断不可纠错误统计 */
        u32 macsec_enc_dpp_ierr_c_cnt; /**< DPP中断可纠错误统计 */
        u32 macsec_enc_dpp_ierr_u_cnt; /**< DPP中断不可纠错误统计 */
        u32 macsec_enc_dpp_wrongsa_cnt; /**< DPP错误SA统计 */
        u32 macsec_enc_dpp_illegal_pkt_cnt; /**< 非法报文统计 */
        u64 macsec_enc_dpp_profile_id_0_bypass_pkt_cnt; /**< profile id为0的bypass报文统计 */
        u32 macsec_enc_dpp_profile_read_all0_bypass_pkt_cnt; /**< porfile id非0但profile信息为0被bypass报文统计 */
        u32 macsec_enc_dpp_profile_id_1_cnt; /**< profile id为1的报文统计 */
        u32 macsec_enc_dpp_profile_id_2_cnt; /**< profile id为2的报文统计 */
        u32 macsec_enc_dpp_profile_id_3_cnt; /**< profile id为3的报文统计 */
        u64 macsec_enc_white_list_plaintext_pkt_cnt; /**< 白名单放通报文统计 */
        u64 macsec_enc_in_sop_cnt; /**< 加密引擎入端口产生SOP次数 */
        u64 macsec_enc_in_eop_cnt; /**< 加密引擎入端口产生EOP次数 */
        u64 macsec_enc_in_abort_cnt; /**< 加密引擎入端口丢弃的次数 */
        u64 macsec_enc_out_sop_cnt; /**< 加密引擎出端口产生SOP次数 */
        u64 macsec_enc_out_eop_cnt; /**< 加密引擎出端口产生EOP次数 */
        u64 macsec_enc_out_abort_cnt; /**< 加密引擎出端口丢弃的次数 */
        u32 macsec_enc_drop_pkt_cnt; /**< 外部端口丢包的数量 */
        u32 macsec_enc_oadp_out_msop_cnt; /**< output sop 统计次数 */
        u32 macsec_enc_oadp_out_meop_cnt; /**< output eop 统计次数 */
    } enc_cnt_s;
    u32 cnt_info[0];
} macsec_enc_cnt_u;

/**
 * @brief macsec_dec_cnt_u - MACSEC解密侧的DFX统计值
 * @details 包括解密侧可纠和不可纠异常、数据路径的sop和eop异常以及根据profile id进行不同处理的统计值等
 */
typedef union {
    struct {
        u32 macsec_dec_msop_cnt; /**< MSOP中断产生的次数 */
        u32 macsec_dec_meop_cnt; /**< MEOP中断产生的次数 */
        u32 macsec_dec_iadp_ierr_c_cnt; /**< IADP中断可纠错误统计 */
        u32 macsec_dec_iadp_ierr_u_cnt; /**< IADP中断不可纠错误统计 */
        u32 macsec_dec_oadp_ierr_c_cnt; /**< OADP中断可纠错误统计 */
        u32 macsec_dec_oadp_ierr_u_cnt; /**< OADP中断不可纠错误统计 */
        u32 macsec_dec_pa_ierr_c_cnt; /**< PA中断可纠错误统计 */
        u32 macsec_dec_pa_ierr_u_cnt; /**< PA中断不可纠错误统计 */
        u32 macsec_dec_pg_ierr_c_cnt; /**< PG中断可纠错误统计 */
        u32 macsec_dec_pg_ierr_u_cnt; /**< PG中断不可纠错误统计 */
        u32 macsec_dec_mib_ierr_c_cnt; /**< MIB中断可纠错误统计 */
        u32 macsec_dec_mib_ierr_u_cnt; /**< MIB中断不可纠错误统计 */
        u32 macsec_dec_dpp_ierr_u_cnt; /**< DPP中断可纠错误统计 */
        u32 macsec_dec_dpp_ierr_c_cnt; /**< DPP中断不可纠错误统计 */
        u32 macsec_dec_dpp_ec01_pkt_cnt; /**< E=0且C=1统计 */
        u32 macsec_dec_dpp_ec10_pkt_cnt; /**< E=1且C=0统计 */
        u32 macsec_dec_dpp_unknownsci_bypass_pkt_cnt; /**< 未知SCI bypass报文统计 */
        u32 macsec_dec_dpp_illegal_pad_pkt_cnt; /**< 非法的padding报文统计 */
        u64 macsec_dec_dpp_white_list_plaintext_pkt_cnt; /**< 白名单放通报文统计 */
        u64 macsec_dec_dpp_profile_id_0_bypass_pkt_cnt;      /**< profile id为0的放通报文统计 */
        u32 macsec_dec_dpp_profile_read_all0_bypass_pkt_cnt; /**< porfile id非0但profile信息为0被bypass报文统计 */
        u32 macsec_dec_dpp_profile_id_1_pkt_cnt; /**< profile id为1的报文统计 */
        u32 macsec_dec_dpp_profile_id_2_pkt_cnt; /**< profile id为2的报文统计 */
        u32 macsec_dec_dpp_profile_id_3_pkt_cnt; /**< profile id为3的报文统计 */
        u32 macsec_dec_dpp_profile_id_4_pkt_cnt; /**< profile id为4的报文统计 */
        u32 macsec_dec_dpp_illegal_pkt_cnt;      /**< 非法报文统计 */
        u64 macsec_dec_intf_in_sop_cnt; /**< DEC_INTF入端口sop的统计 */
        u64 macsec_dec_intf_in_eop_cnt; /**< DEC_INTF入端口eop的统计 */
        u64 macsec_dec_intf_in_abort_cnt; /**< DEC_INTF入端口丢弃的统计 */
        u64 macsec_dec_out_sop_cnt; /**< DEC_INTF出端口sop的统计 */
        u64 macsec_dec_out_eop_cnt; /**< DEC_INTF出端口eop的统计 */
        u64 macsec_dec_out_abort_cnt; /**< DEC_INTF出端口丢弃的统计 */
        u32 macsec_dec_intf_drop_pkt_cnt; /**< 端口disable丢弃包统计 */
        u32 macsec_dec_oadp_out_msop_cnt; /**< out msop计数统计 */
        u32 macsec_dec_oadp_out_meop_cnt; /**< out meop计数统计 */
    } dec_cnt_s;
    u32 cnt_info[0];
} macsec_dec_cnt_u;

/**
 * @brief macsec_cmd_err_cnt_operation_s - macsec内部的dfx cnt信息获取
 * @details 包括加密和解密侧的统计信息，用于问题定位
 */
typedef struct tag_macsec_cmd_err_cnt_operation_s {
    struct mgmt_msg_head head;  /**< Management msg header info, 8B */
    macsec_enc_cnt_u enc_module_cnt; /**< enc cnt info */
    macsec_dec_cnt_u dec_module_cnt; /**< dec cnt info */
    u8 reserved[4]; /**< reserved for expanding */
} macsec_cmd_err_cnt_operation_s;

/**
 * @brief macsec_cmd_service_operation_s - macsec服务管理操作
 * @details 包括驱动加卸载时对macsec的处理
 */
typedef struct tag_macsec_cmd_service_operation_s {
    struct mgmt_msg_head head;  /**< Management msg header info, 8B */
    u8 op_code; /**< Operation code, 0: disable macsec/1: enable macsec */
    u8 reserved[7]; /**< reserved for expanding */
} macsec_cmd_service_operation_s;

/**
 * @brief macsec_feature_nego_cmd_s - feature negotiation command struct defination
 * @details 包括驱动加载时对macsec特性协商的处理
 */
typedef struct tag_macsec_feature_nego_cmd {
    struct mgmt_msg_head head; /**< Management msg header info, 8B */
    u64 s_feature[MACSEC_MAX_FEATURE_QWORD]; /**< macsec features */
    u8 op_code; /**< Operation code, 0: get macsec feature/1: set macsec feature */
    u8 rsvd[7]; /**< reserved for expanding */
} macsec_feature_nego_cmd_s;

/**
 * @brief tag_macsec_flush_cmd_s - MACSEC resource actively cleans up parameters
 * @details 下发flush命令时包括的参数
 */
typedef struct tag_macsec_flush_cmd {
    struct mgmt_msg_head head; /**< Management msg header info, 8B */
    u64 sci; /**< 64-bit SCI value used to identify an SC */
    u8 op_code; /**< Operation code, 0: flush sc and sa, 1: flush sa */
    u8 reserved[7]; /**< reserved for expanding */
} tag_macsec_flush_cmd_s;

// macsec 白名单工具支持的操作类型
typedef enum macsec_whitelist_tool_op {
    MACSEC_WHITELIST_TOOL_OP_ENABLE = 0,
    MACSEC_WHITELIST_TOOL_OP_SET,
    MACSEC_WHITELIST_TOOL_OP_LIST,
    MACSEC_WHITELIST_TOOL_OP_MAX
} macsec_whitelist_tool_op_e;

// macsec 白名单的配置场景
typedef enum macsec_whitelist_tool_mode {
    MACSEC_WHITELIST_TOOL_MODE_DMAC = 0,
    MACSEC_WHITELIST_TOOL_MODE_TXVLAN,
    MACSEC_WHITELIST_TOOL_MODE_RXVLAN,
    MACSEC_WHITELIST_TOOL_MODE_ETYPE,
    MACSEC_WHITELIST_TOOL_MODE_MAX
} macsec_whitelist_tool_mode_e;

// inbuf 包含一个hdr
typedef struct macsec_whitelist_cmd_hdr {
    macsec_whitelist_tool_op_e cmd_type;
    macsec_whitelist_tool_mode_e mode_type;
} macsec_whitelist_cmd_hdr_t;

// inbuf 定义
typedef struct macsec_whitelist_info {
    u64 dmac;/**< dmac value */
    u32 vlanid;/**< vlanid value */
    u32 mask;
    u32 dmac_id;
    u32 port;
    u32 dmac_mask;
    u8 dmac_service;
    u8 service;
    u8 etype_control;
    u8 dmac_flag;
    u8 dmac_service_flag;
    u8 vlanid_flag;
    u8 mask_flag;
    u8 etype_flag;
    u8 etype_control_flag;
    u8 rsvd[3];
} macsec_whitelist_info_t;

typedef struct macsec_whitelist_cmd_in {
    struct mgmt_msg_head head;
    macsec_whitelist_cmd_hdr_t hdr;
    macsec_whitelist_info_t buf;
} macsec_whitelist_cmd_in_t;

// outbuf 定义
typedef struct macsec_whitelist_cmd_out {
    struct mgmt_msg_head head;
    u64 dmac_list[MACSEC_DMAC_TEMPLATE_NUM];
    u32 dmac_mask_list[MACSEC_DMAC_TEMPLATE_NUM];
    u32 dmac_valid_num;
    u8 dmac_port_list[MACSEC_PORT_NUM];
    u32 txvlan_list[MACSEC_PORT_NUM];
    u32 txvlan_valid_num;
    u32 rxvlan_list[MACSEC_PORT_NUM];
    u32 rxvlan_mask_list[MACSEC_PORT_NUM];
    u32 rxvlan_valid_num;
    u32 etype_list[MACSEC_ETYPE_TEMPLATE_NUM];
} macsec_whitelist_cmd_out_s;

#endif /* MACSEC_MPU_CMD_DEFS_H */