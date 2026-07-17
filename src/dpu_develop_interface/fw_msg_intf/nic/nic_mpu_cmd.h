/*
 * 头文件通过MAILBOX基线表格中的脚本自动生成。如有增删，请先修改基线表格，然后自动生成该头文件
 * 修改流程见：https://wiki.huawei.com/domains/1574/wiki/8/WIKI202509298479069
 * File Name     : nic_mpu_cmd.h
 * Description   : NIC Commands between Driver and MPU
 */

#ifndef HINIC5_NIC_CMD_H
#define HINIC5_NIC_CMD_H

/**
 * @brief enum hinic5_nic_cmd
 * @details nic hinic commands
 */
enum hinic5_nic_cmd {
    HINIC5_NIC_CMD_VF_REGISTER = 0, /** <  @see hinic5_cmd_register_vf */
    HINIC5_NIC_CMD_SET_FUNC_TBL = 5, /** < 设置FUNC表初始化、mtu值等 @see hinic5_cmd_set_func_tbl */
    HINIC5_NIC_CMD_SET_VPORT_ENABLE = 6, /** < func使能/去使能标记(OVS使用PF代发） @see hinic5_vport_state */
    HINIC5_NIC_CMD_SET_RX_MODE = 7, /** < 设置单播、组播混杂等标记 @see hinic5_rx_mode_config */
    HINIC5_NIC_CMD_SQ_CI_ATTR_SET = 8, /** < 设置func的CI属性表 @see hinic5_cmd_cons_idx_attr */
    HINIC5_NIC_CMD_GET_VPORT_STAT = 9, /** < 获取func 单播、丢包等统计 @see hinic5_port_stats_info/hinic5_cmd_vport_stats */
    HINIC5_NIC_CMD_CLEAN_VPORT_STAT = 10, /** < 清理func 单薄、丢包等统计 @see hinic5_port_stats_info */
    HINIC5_NIC_CMD_CLEAR_QP_RESOURCE = 11, /** < 清0 func队列的CPI CI值 @see hinic5_cmd_clear_qp_resource */
    HINIC5_NIC_CMD_CFG_FLEX_QUEUE = 12, /** < rsvd */
    HINIC5_NIC_CMD_CFG_RX_LRO = 13, /** < 配置LRO的使能等 @see hinic5_cmd_lro_config */
    HINIC5_NIC_CMD_CFG_LRO_TIMER = 14, /** < 使能LRO的Timer @see hinic5_cmd_lro_timer */
    HINIC5_NIC_CMD_FEATURE_NEGO = 15, /** < func的属性协商 @see hinic5_cmd_feature_nego */
    HINIC5_NIC_CMD_CFG_LOCAL_LRO_STATE = 16, /** < 设置func的local_switch_lro_en @see hinic5_cmd_local_lro_state */
    HINIC5_NIC_CMD_CACHE_OUT_QP_RES = 17, /** < 清理L2NIC/SMMC_CLA Cache资源 @see hinic5_cmd_cache_out_qp_resource */
    HINIC5_NIC_CMD_SET_FUNC_ER_FWD_ID = 18, /** < 设置NIC ER 转发ID，流分叉使用（计算使用，rsvd） */
    HINIC5_NIC_CMD_GET_MAC = 20, /** < func 获取MAC地址 @see hinic5_port_mac_set */
    HINIC5_NIC_CMD_SET_MAC = 21, /** < func 设置MAC地址 @see hinic5_port_mac_set */
    HINIC5_NIC_CMD_DEL_MAC = 22, /** < func 删除MAC地址 @see hinic5_port_mac_set */
    HINIC5_NIC_CMD_UPDATE_MAC = 23, /** < func 更新MAC地址 @see hinic5_port_mac_update */
    HINIC5_NIC_CMD_GET_ALL_DEFAULT_MAC = 24, /** < 获取所有默认MAC地址 @see nic_cmd_mac_info */
    HINIC5_NIC_CMD_CFG_FUNC_VLAN = 25, /** < 添加/删除func的vlan设备 @see hinic5_cmd_vlan_config */
    HINIC5_NIC_CMD_SET_VLAN_FILTER_EN = 26, /** < 设置func的VLAN过滤功能 @see hinic5_cmd_set_vlan_filter */
    HINIC5_NIC_CMD_SET_RX_VLAN_OFFLOAD = 27, /** < 设置func表rx_vlan_offload_en @see hinic5_cmd_vlan_offload */
    HINIC5_NIC_CMD_OUTBAND_SET_FUNC_VLAN = 29, /** < 带外设置func的vlan（计算使用，rsvd） */
    HINIC5_NIC_CMD_CFG_VXLAN_PORT = 30, /** < 设置vxlan_dprot（计算使用，rsvd） */
    HINIC5_NIC_CMD_RX_RATE_CFG = 31, /** < rx限速bios设置（计算使用，rsvd） */
    HINIC5_NIC_CMD_WR_ORDERING_CFG = 32, /** < 设置PCIe的读写是强保序还是乱序（计算使用，rsvd） */
    HINIC5_NIC_CMD_MAC_SYNC = 33, /** < 设置mac同步（计算使用，rsvd） */
    HINIC5_NIC_CMD_SET_RQ_CI_CTX = 34, /** <  @see hinic5_rq_cqe_ctx */
    HINIC5_NIC_CMD_SET_RQ_ENABLE = 35, /** <  @see hinic5_rq_enable */
    HINIC5_NIC_CMD_CFG_VF_VLAN = 40, /** < 添加/删除func的vlan设备（QinQ） @see hinic5_cmd_vf_vlan_config */
    HINIC5_NIC_CMD_SET_SPOOFCHK_STATE = 41, /** <  @see hinic5_cmd_spoofchk_set */
    HINIC5_NIC_CMD_SET_MAX_MIN_RATE = 42, /** < 设置func的限速 @see hinic5_cmd_rate_cfg */
    HINIC5_NIC_CMD_CFG_CQE_COALESCE_OFFLOAD = 43, /** < 设置cqe聚合卸载功能（计算使用，rsvd） */
    HINIC5_NIC_CMD_CFG_CQE_COALESCE_OFFLOAD_TIMER = 44, /** < 设置cqe聚合卸载功能计时（计算使用，rsvd） */
    HINIC5_NIC_CMD_CFG_VF_TRUST = 46, /** < 设置vf trust使能 @see hinic5_set_vf_trust */
    HINIC5_NIC_CMD_RSS_CFG = 60, /** < 设置func的rss使能、rq_pri_num等 @see hinic5_cmd_rss_config */
    HINIC5_NIC_CMD_RSS_TEMP_MGR = 61, /** < 分配/释放func的rss模版表 @see hinic5_rss_template_mgmt */
    HINIC5_NIC_CMD_GET_RSS_CTX_TBL = 62, /** < 设置func的rss context @see hinic5_rss_context_table */
    HINIC5_NIC_CMD_CFG_RSS_HASH_KEY = 63, /** < 设置func的rss key @see hinic5_cmd_rss_hash_key */
    HINIC5_NIC_CMD_CFG_RSS_HASH_ENGINE = 64, /** < 设置func的rss engine @see hinic5_cmd_rss_engine_type */
    HINIC5_NIC_CMD_SET_RSS_CTX_TBL_INTO_FUNC = 65, /** < 设置func的rss type和rss type en @see hinic5_rss_context_table */
    HINIC5_NIC_CMD_IPCS_ERR_RSS_ENABLE_OP = 66, /** < 设置global表的ipcs_err_rss_en @see hinic5_ipcs_err_rss_enable_operation_s */
    HINIC5_NIC_CMD_GTP_INNER_PARSE_STATUS = 67, /** < 控制gtp内层解析状态（计算使用，rsvd） */
    HINIC5_NIC_CMD_ADD_TC_FLOW = 80, /** < 添加TCAM规则 @see nic_cmd_fdir_add_rule */
    HINIC5_NIC_CMD_DEL_TC_FLOW = 81, /** < 删除TCAM规则 @see nic_cmd_fdir_del_rules */
    HINIC5_NIC_CMD_GET_TC_FLOW = 82, /** < 获取TCAM规则 @see nic_cmd_fdir_get_rule */
    HINIC5_NIC_CMD_FLUSH_TCAM = 83, /** < 清除某func所有TCAM规则和block @see nic_cmd_flush_tcam_rules */
    HINIC5_NIC_CMD_CFG_TCAM_BLOCK = 84, /** < 分配/释放func的block资源 @see nic_cmd_ctrl_tcam_block_in/nic_cmd_ctrl_tcam_block_out */
    HINIC5_NIC_CMD_ENABLE_TCAM = 85, /** < 设置func的fdir_tcam_enable @see nic_cmd_set_tcam_enable */
    HINIC5_NIC_CMD_GET_TCAM_BLOCK = 86, /** < 获取某个block被哪个func使用 @see nic_cmd_dfx_fdir_tcam_block_table */
    HINIC5_NIC_CMD_CFG_PPA_TABLE_ID = 87, /** <  @see hinic5_ppa_cfg_table_id_cmd */
    HINIC5_NIC_CMD_SET_PPA_EN = 88, /** < 设置func表的ppa使能 @see hinic5_ppa_cfg_ppa_en_cmd */
    HINIC5_NIC_CMD_CFG_PPA_MODE = 89, /** <  @see hinic5_ppa_cfg_mode_cmd */
    HINIC5_NIC_CMD_CFG_PPA_FLUSH = 90, /** < 设置global表的ppa_flow_flush_en @see hinic5_ppa_flush_en_cmd */
    HINIC5_NIC_CMD_SET_FDIR_STATUS = 91, /** < 设置SML FDIR线性表 @see nic_cmd_set_fdir_status */
    HINIC5_NIC_CMD_GET_PPA_COUNTER = 92, /** <  @see hinic5_ppa_fdir_query_cmd */
    HINIC5_NIC_CMD_SET_FUNC_FLOW_BIFUR_ENABLE = 93, /** < 设置/查询func表的flow_bifur_en（计算使用，rsvd） */
    HINIC5_NIC_CMD_SET_BOND_MASK = 94, /** < 设置bond掩码（计算使用，rsvd） */
    HINIC5_NIC_CMD_GET_BLOCK_TC_FLOWS = 95, /** < 获取某个block下的所有规则？ @see nic_cmd_fdir_get_block_rules */
    HINIC5_NIC_CMD_GET_BOND_MASK = 96, /** < 读取bond掩码（计算使用，rsvd） */
    HINIC5_NIC_CMD_SET_PORT_ENABLE = 100, /** <  @see hinic5_port_state */
    HINIC5_NIC_CMD_CFG_PAUSE_INFO = 101, /** < 设置/查询port的pause状态 @see hinic5_cmd_pause_config */
    HINIC5_NIC_CMD_CFG_PORT_CAR = 102, /** < 设置/查询/使能 arp/icmp的car限速 @see 182x:hinic5_cmd_set_port_car\1872:hinic5_cmd_set_car/hinic5_car_profile */
    HINIC5_NIC_CMD_SET_ER_DROP_PKT = 103, /** < rsvd */
    HINIC5_NIC_CMD_VF_COS = 104, /** <  @see hinic5_cmd_vf_dcb_state */
    HINIC5_NIC_CMD_SETUP_COS_MAPPING = 105, /** < rsvd */
    HINIC5_NIC_CMD_SET_ETS = 106, /** < rsvd */
    HINIC5_NIC_CMD_SET_PFC = 107, /** < rsvd */
    HINIC5_NIC_CMD_QOS_ETS = 108, /** < QoS ETS调度权重配置/查询 @see hinic5_cmd_ets_cfg */
    HINIC5_NIC_CMD_QOS_PFC = 109, /** < QoS PFC配置/查询 @see hinic5_cmd_set_pfc */
    HINIC5_NIC_CMD_QOS_DCB_STATE = 110, /** < QoS 配置func及其所属Port的DCB状态 @see hinic5_cmd_set_dcb_state */
    HINIC5_NIC_CMD_QOS_PORT_CFG = 111, /** < QoS Port的trust信息配置/查询 @see hinic5_cmd_qos_port_cfg */
    HINIC5_NIC_CMD_QOS_MAP_CFG = 112, /** < QoS PCP/DSCP映射配置/查询 @see hinic5_cmd_qos_map_cfg */
    HINIC5_NIC_CMD_FORCE_PKT_DROP = 113, /** < QoS Port的PFC/Pause强制丢包 @see hinic5_force_pkt_drop */
    HINIC5_NIC_CMD_CFG_TX_PROMISC_SKIP = 114, /** < 设置混杂PF是否接收TX未知单播报文 @see hinic5_tx_promisc_cfg */
    HINIC5_NIC_CMD_GET_CIR_DROP = 115, /** < 读取cir（计算使用，rsvd） */
    HINIC5_NIC_CMD_SET_PORT_FLOW_BIFUR_ENABLE = 117, /** < 设置/查询port表的flow_bifur_en（计算使用，rsvd） */
    HINIC5_NIC_CMD_TX_PAUSE_EXCP_NOTICE = 118, /** <  @see nic_cmd_tx_pause_notice */
    HINIC5_NIC_CMD_INQUIRT_PAUSE_CFG = 119, /** < QoS PFC风暴检测参数配置 @see nic_cmd_pause_inquiry_cfg */
    HINIC5_NIC_CMD_BIOS_CFG = 120, /** < 固化BIOS配置/查询 @see nic_cmd_bios_cfg */
    HINIC5_NIC_CMD_SET_FIRMWARE_CUSTOM_PACKETS_MSG = 121, /** < 接收快速故障通知内容 @see fault_msg_s */
    HINIC5_NIC_CMD_QOS_EXTEND_CFG = 122, /** < Qos 扩展配置，支持TC限速配置/查询 */
    HINIC5_NIC_CMD_BOND_LINK_INFO_GET = 130, /** <  @see hinic5_bond_link_info */
    HINIC5_NIC_CMD_MACSEC_PN_EXPIRED_NOTICE = 131, /** <  @see tag_macsec_pn_expired_report_cmd_s */
    HINIC5_NIC_CMD_PASS_ARP_PKT = 132, /** <  @see hinic5_arp_pkt_info */
    HINIC5_NIC_CMD_BOND_DEV_CFG = 133, /** <  @see hinic5_cmd_cfg_bond */
    HINIC5_NIC_CMD_BOND_DEV_CREATE = 134, /** <  @see hinic5_cmd_create_bond */
    HINIC5_NIC_CMD_BOND_DEV_DELETE = 135, /** <  @see hinic5_cmd_delete_bond */
    HINIC5_NIC_CMD_BOND_DEV_OPEN_CLOSE = 136, /** <  @see hinic5_cmd_open_close_bond */
    HINIC5_NIC_CMD_BOND_INFO_GET = 137, /** <  @see hinic5_bond_status_info */
    HINIC5_NIC_CMD_BOND_ACTIVE_INFO_GET = 138, /** <  @see hinic5_bond_active_report_info */
    HINIC5_NIC_CMD_BOND_ACTIVE_NOTICE = 139, /** <  @see nic_cmd_bond_active_report_info */
    HINIC5_NIC_CMD_GET_SM_TABLE = 140, /** < 获取mac表等一些信息 @see nic_cmd_dfx_sm_table */
    HINIC5_NIC_CMD_RD_LINE_TBL = 141, /** < 获取SM线性表的一些信息 @see nic_mpu_lt_opera */
    HINIC5_NIC_CMD_SET_VEB = 143, /** < 查询或配置veb卸载模式 @see hinic5_veb_set */
    HINIC5_NIC_CMD_NIC_VPORT_CNT = 144, /** < 查询或配置VF的vport计数 @see hinic5_nic_vport_cnt_info */
    HINIC5_NIC_CMD_SET_UCAPTURE_OPT = 160, /** < ROCE抓包功能开关 @see nic_cmd_capture_info */
    HINIC5_NIC_CMD_SET_VHD_CFG = 161, /** < 设置func表的vhd_type相关参数 @see nic_cmd_vhd_config */
    HINIC5_NIC_CMD_GET_UCAPTURE_INFO = 162, /** < 获取PF抓取报文功使能情况，遍历PF，返回使能抓包功能的位图（计算使用，rsvd） */
    HINIC5_NIC_CMD_GET_OUTBAND_CFG = 170, /** < 获取带外设置（计算使用，rsvd） */
    HINIC5_NIC_CMD_OUTBAND_CFG_NOTICE = 171, /** < 带外信息获取（计算使用，rsvd） */
    HINIC5_NIC_CMD_FLUSH_TC_FLOW = 176, /** <  @see hinic5_tc_flush_info */
    HINIC5_NIC_CMD_CFG_VXLAN_TBL = 177, /** <  @see hinic5_tc_vxlan_tbl_cfg_info */
    HINIC5_NIC_CMD_MOVE_TC_TBL = 178, /** <  @see hinic5_tc_move_info */
    HINIC5_NIC_CMD_CFG_TC_AGING_TBL = 179, /** <  @see hinic5_tc_aging_info */
    HINIC5_NIC_CMD_PFE_CNT = 180, /** <  @see hinic5_tc_pfe_cnt_info */
    HINIC5_NIC_CMD_GET_PFE_CFG = 181, /** <  @see hinic5_tc_pfe_cfg_reg_info */
    HINIC5_NIC_CMD_CFG_PFE_TCAM = 182, /** <  @see hinic5_tc_tcam_info */
    HINIC5_NIC_CMD_CFG_PFE_VTEP_IP = 183, /** <  @see hinic5_tc_pfe_vtep_ip_cmd */
    HINIC5_NIC_CMD_SET_PFE_DEFAULT_ACTION = 184, /** <  @see hinic5_tc_default_action_info */
    HINIC5_NIC_CMD_PFE_TCAM_FREQ = 185, /** <  @see hinic5_tc_pfe_tcam_freq_info */
    HINIC5_NIC_CMD_CFG_TCAM_CLOCK_GATING = 186, /** <  @see hinic5_tc_tcam_clock_gating_cfg_info */
    HINIC5_NIC_CMD_CFG_TC_FLOW_RULE = 187, /** <  @see hinic5_tc_cfg_info */
    HINIC5_NIC_CMD_SET_PFE_CFG_PROFILE = 188, /** <  @see hinic5_tc_pfe_cfg_profile_info */
    HINIC5_NIC_CMD_GET_PORT_STAT = 200, /** < rsvd */
    HINIC5_NIC_CMD_CLEAN_PORT_STAT = 201, /** < rsvd */
    HINIC5_NIC_CMD_CFG_LOOPBACK_MODE = 202, /** < rsvd */
    HINIC5_NIC_CMD_GET_SFP_QSFP_INFO = 203, /** < rsvd */
    HINIC5_NIC_CMD_SET_SFP_STATUS = 204, /** < rsvd */
    HINIC5_NIC_CMD_GET_LIGHT_MODULE_ABS = 205, /** < rsvd */
    HINIC5_NIC_CMD_GET_LINK_INFO = 206, /** < rsvd */
    HINIC5_NIC_CMD_CFG_AN_TYPE = 207, /** < rsvd */
    HINIC5_NIC_CMD_GET_PORT_INFO = 208, /** <  @see hinic5_cmd_port_info */
    HINIC5_NIC_CMD_SET_LINK_SETTINGS = 209, /** < rsvd */
    HINIC5_NIC_CMD_ACTIVATE_BIOS_LINK_CFG = 210, /** < rsvd */
    HINIC5_NIC_CMD_RESTORE_LINK_CFG = 211, /** < rsvd */
    HINIC5_NIC_CMD_SET_LINK_FOLLOW = 212, /** < rsvd */
    HINIC5_NIC_CMD_GET_LINK_STATE = 213, /** < rsvd */
    HINIC5_NIC_CMD_LINK_STATUS_REPORT = 214, /** < rsvd */
    HINIC5_NIC_CMD_CABLE_PLUG_EVENT = 215, /** < rsvd */
    HINIC5_NIC_CMD_LINK_ERR_EVENT = 216, /** < rsvd */
    HINIC5_NIC_CMD_SET_LED_STATUS = 217, /** < rsvd */
    HINIC5_NIC_CMD_MIG_SET_CEQ_CTRL = 230, /** <  @see mig_nic_set_ceq_ctrl */
    HINIC5_NIC_CMD_MIG_CFG_MSIX_INFO = 231, /** <  @see mig_nic_msix_info_rw */
    HINIC5_NIC_CMD_MIG_CFG_FUNC_VAT_TBL = 232, /** <  @see mig_nic_func_vat_tbl */
    HINIC5_NIC_CMD_MIG_GET_VF_INFO = 233, /** <  @see mig_nic_func_cfg */
    HINIC5_NIC_CMD_MIG_CHK_MBX_EMPTY = 234, /** <  @see mig_nic_chk_mbx_empty */
    HINIC5_NIC_CMD_MIG_SET_VPORT_ENABLE = 235, /** <  @see mig_nic_vport_state */
    HINIC5_NIC_CMD_MIG_CFG_SQ_CI = 236, /** <  @see mig_nic_sq_ci */
    HINIC5_NIC_CMD_MIG_CFG_RSS_TBL = 237, /** <  @see mig_nic_cfg_rss_tbl */
    HINIC5_NIC_CMD_MIG_TMP_SET_CMDQ_CTX = 238, /** <  @see mig_nic_tmp_cfg_cmdq_ctx */
    HINIC5_NIC_CMD_MIG_STOP_SQ = 239, /** <  @see nic_mig_sq_stop */
    HINIC5_NIC_CMD_MIG_CFG_FAST_MSG_ADDR = 240, /** <  @see mig_nic_fast_msg_addr */
    HINIC5_NIC_CMD_MIG_SET_FUNC_FLR_MGMT = 241, /** <  @see hinic5_cmd_set_pcie_flr_mgmt */
    HINIC5_NIC_CMD_LRO_CFG = 242, /** <  @see hinic5_cmd_lro_cfg */
    HINIC5_NIC_CMD_CFG_VF_LAG = 243, /** < 设置vf_lag（计算使用，rsvd） */
    HINIC5_NIC_CMD_VF_LAG_SYNC_BOND_STATE = 244, /** < vf_lag同步bond状态（计算使用，rsvd） */
    HINIC5_NIC_CMD_EXTEND_SECTION1_START = 257, /** < NIC Mbox保留命令字起始，计算产品线使用 */
    HINIC5_NIC_CMD_EXTEND_SECTION1_END = 384, /** < NIC Mbox保留命令字结束，计算产品线使用 */
    HINIC5_NIC_CMD_EXTEND_SECTION2_START = 385, /** < NIC Mbox保留命令字起始，数存产品线使用 */
    HINIC5_NIC_CMD_EXTEND_SECTION2_END = 512  /** < NIC Mbox保留命令字结束，数存产品线使用 */
};

#endif /* HINIC5_NIC_CMD_H */
