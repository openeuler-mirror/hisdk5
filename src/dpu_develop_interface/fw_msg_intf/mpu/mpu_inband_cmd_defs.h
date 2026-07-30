/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
 * Filename      : mpu_inband_cmd_defs.h
 * Version       : Initial Draft
 * Creation time : 2023/09/22
 * Last Modified :
 * Description   : In-band command-related structures between the driver and the MPU
 */

#ifndef MPU_INBAND_CMD_DEFS_H
#define MPU_INBAND_CMD_DEFS_H

#include "mpu_cmd_base_defs.h"

#define HARDWARE_ID_1XX3V200_TAG 32     /**< 1xx3v200 tag */
#define DUMP_16B_PER_LINE	16  /**< dump 16byte对齐 */
#define DUMP_4_VAR_PER_LINE	4   /**< dump 单位4byte */
#define FW_UPDATE_MGMT_TIMEOUT	3000000U    /** mbox消息升级命令超时时间 */

#define FUNC_RESET_FLAG_MAX_VALUE ((1U << (RES_TYPE_MAX + 1)) - 1)  /**< func_reset_flag的边界值 */
struct comm_cmd_func_reset {    /**< 驱动加卸载场景，function reset清理资源 */
    struct mgmt_msg_head head;  /**< mbox消息头 */

    u16 func_id;    /**< 需要reset的function id */
    u16 rsvd1[3];   /**< 保留字段 */
    u64 reset_flag;     /**< 具体reset资源的bitmap */
};

struct comm_cmd_ppf_flr_type_set {  /**< flr场景，设定ppf执行flr的范围 */
    struct mgmt_msg_head head;  /**<  */

    u16 func_id;    /**< flr的function id */
    u8 rsvd1[2];    /**< 保留字段 */
    u32 ppf_flr_type;   /**< ppf执行flr的范围类型，0: 仅ffp, 1: 改ppf下的所有function */
};

enum {
    COMM_F_API_CHAIN                    = 1U << 0,   /**< 属性协商，cpi chain */
    COMM_F_CLP                          = 1U << 1,   /**< 属性协商，clp */
    COMM_F_CHANNEL_DETECT               = 1U << 2,   /**< 属性协商，通道探测 */
    COMM_F_MBOX_SEGMENT                 = 1U << 3,   /**< 属性协商，mbox */
    COMM_F_CMDQ_NUM                     = 1U << 4,   /**< 属性协商，cmdq */
    COMM_F_VIRTIO_VQ_SIZE               = 1U << 5,   /**< 属性协商，vio vq size */
    COMM_F_EXTEND_CAP                   = 1U << 6,   /**< 属性协商，能力集扩展 */
    COMM_F_SMF_CACHE_INVALID            = 1U << 7,   /**< 属性协商，cache invalid */
    COMM_F_ONLY_ENHANCE_CMDQ            = 1U << 8,   /**< 属性协商，增强型cmdq */
    COMM_F_USE_REAL_RX_BUF_SIZE         = 1U << 9,   /**< 属性协商，use real rx buf */
    COMM_F_CMD_BUF_SIZE                 = 1U << 10,  /**< 属性协商，cmd buf size */
    COMM_F_HTN_CMD                      = 1U << 11,  /**< 属性协商，Hard Tile - NIC (硬件nic) */
    COMM_F_MBOX_MSG_HEAD_SUPP_VER1      = 1U << 12,  /**< 属性协商，mode扩展 */
    COMM_F_FAST_MSG                     = 1U << 13,  /**< 属性协商，fast msg */
    COMM_F_UFHD                         = 1U << 14,  /**< 属性协商，Update Firmware from Host DDR - 支持 DDR 微码热升级 */
    COMM_F_VIRTIO_FC_CACHE_MODE         = 1U << 15,  /**< 属性协商，驱动支持Virtio function context cache mode */
    COMM_F_NON_PTP_SYNC                 = 1U << 16,  /**< 属性协商，非ptp同步 */
    COMM_F_HT_GPA                       = 1U << 17,  /**< 属性协商，HT GPA (Bank GPA) */
    COMM_F_UFHD_FLEX_SEG                = 1U << 18,  /**< 属性协商，UFHD 增加支持 segement size 协商能力。
                                                                   该特性不能与 COMM_F_UFHD 特性同时开启。
                                                                   为支持 segment size 协商，需要开启 COMM_F_EXTEND_CAP。 */
};

/**< mode扩展version */
#define CHECK_COMM_F_SUPP_MBOX_MSG_HEAD_VER1(feature) (((feature) & COMM_F_MBOX_MSG_HEAD_SUPP_VER1) > 0)

enum {
    COMM_PLUG_SRV_NIC = 0,   /**< plug nic */
    COMM_PLUG_SRV_VROCE,     /**< plug vroce */
    COMM_PLUG_SRV_UB,        /**< plug ub */
    COMM_PLUG_SRV_BUTT,
};

#define COMM_MAX_FEATURE_QWORD 4
struct comm_cmd_feature_nego {   /**< 属性协商 */
    struct mgmt_msg_head head;  /**< mbox消息头 */

    u16 func_id;   /**< 指定function id */
    u8 opcode;    /**< 1: set, 0: get */
    u8 rsvd[5];   /**< 保留域段 */
    u64 s_feature[COMM_MAX_FEATURE_QWORD];   /**< 协商信息 */
};

struct comm_cmd_clear_doorbell {   /**< 驱动卸载flush db */
    struct mgmt_msg_head head;   /**< mbox消息头 */

    u16 func_id;   /**< 指定function id */
    u16 rsvd1[3];   /**< 保留域段 */
};

struct comm_cmd_clear_resource {   /**< 驱动卸载flush流程，清理资源 */
    struct mgmt_msg_head head;   /**< mbox消息头 */

    u16 func_id;   /**< 指定function id */
    u16 rsvd1[3];   /**< 保留域段 */
};

struct comm_global_attr {   /**< 获取芯片全局属性信息 */
    u8 max_host_num;   /**< 最大host个数 */
    u8 max_pf_num;   /**< 最大pf个数 */
    u16 vf_id_start;   /**< 起始vf id */

    u8 mgmt_host_node_id;    /**< 管理host节点id */
    u8 cmdq_num;   /**< cmdq个数 */
    u16 cmd_buf_size;   /**< cmd buff size */

    u32 rsvd2[8];   /**< 保留域段 */
};

struct comm_cmd_heart_event {   /**< mpu与驱动之间的mbox心跳事件 */
    struct mgmt_msg_head head;   /**< mbox消息头 */

    u8 init_sta;    /**< 0: mpu init ok, 1: mpu init error */
    u8 rsvd1[3];   /**< 保留域段 */
    u32 heart;   /**< 心跳标识 */
    u32 heart_handshake;   /**< should be alwasys: 0x5A5A5A5A */
};

struct comm_cmd_channel_detect {   /**< 通道探测 */
    struct mgmt_msg_head head;   /**< mbox消息头 */

    u16 func_id;   /**< 指定function id */
    u16 rsvd1[3];   /**< 保留域段1 */
    u32 rsvd2[2];   /**< 保留域段2 */
};

struct comm_cmd_func_svc_used_state {   /**< function使用状态 */
    struct mgmt_msg_head head;   /**< mbox消息头 */
    u16 func_id;   /**< 指定function id */
    u16 svc_type;   /**< service类型(暂未使用) */
    u8 used_state;   /**< 使用状态 */
    u8 rsvd[35];   /**<  */
};

struct comm_cmd_get_flr_info {   /**< flrdx信息 */
    struct mgmt_msg_head head;   /**< mbox消息头 */
    u16 func_id;   /**< 指定function id */
    u8 flr_valid;   /**< flr有效位 */
    u8 flr_step;   /**< flr状态机 */
    u32 flr_used_time_ms;   /**< flr耗时时间 */
    u16 max_flr_time_func_id;   /**< 耗时最长function */
    u32 max_flr_used_time_ms;   /**< flr最长耗时 */
    u8 rsvd[30];   /**< 保留域段 */
};

struct sml_table_id_info {   /**< sml表信息 */
    u8 node_id;   /**< 节点id */
    u8 instance_id;   /**< instance id */
};

struct comm_cmd_get_sml_tbl_data {   /**< sml表内容 */
    struct mgmt_msg_head head;    /**<mbox消息头  */
    u8 tbl_data[512];   /**< sml payload */
};

struct comm_cmd_get_glb_attr {   /**< 获取芯片全局信息 */
    struct mgmt_msg_head head;   /**< mbox消息头 */

    struct comm_global_attr attr;   /**< 全局信息 */
};

#define HINIC5_FW_VERSION_LEN 16   /**< version长度 */
#define HINIC5_FW_COMPILE_TIME_LEN 20   /**< 时间长度 */

struct comm_cmd_get_fw_version {   /**< 获取固件版本号 */
    struct mgmt_msg_head head;   /**< mbox消息头 */

    u16 fw_type;   /**< 固件类型 */
    u16 fw_dfx_vld : 1;   /**< 版本类型，0: release, 1: debug */
    u16 rsvd1 : 15;   /**< 保留域段 */
    char ver[HINIC5_FW_VERSION_LEN];   /**< 版本 */
    char time[HINIC5_FW_COMPILE_TIME_LEN];   /**< 时间 */
};

struct cmdq_ctxt_info {   /**< hardware define: cmdq context */
    u64 curr_wqe_page_pfn;   /**< wqe页面信息 */
    u64 wq_block_pfn;   /**< wqe地址 */
};

struct comm_cmd_cmdq_ctxt {   /**< 配置cmdq context */
    struct mgmt_msg_head head;   /**< mbox消息头 */

    u16 func_id;   /**< 指定function id */
    u8 cmdq_id;   /**< cmdq id */
    u8 rsvd1[5];   /**< 保留域段 */

    struct cmdq_ctxt_info ctxt;   /**< ctx信息 */
};

struct enhance_cmdq_ctxt_info {   /**< hardware define: enhance cmdq context */
	u64	eq_cfg;   /**< eq cfg */
	u64	dfx_pi_ci;   /**< 指针pi ci */

	u64	pft_thd;   /**< pft thd */
	u64	pft_ci;   /**< pft ci */

	u64	rsv;   /**< 保留域段 */
	u64	ci_cla_addr;   /**< cla地址 */
};

struct comm_cmd_enhance_cmdq_ctxt {   /**< 增强型cmdq ctx配置 */
    struct mgmt_msg_head head;   /**< mbox消息头 */

    u16 func_id;   /**< 指定function id */
    u8 cmdq_id;   /**< cmdq id */
    u8 rsvd1[5];   /**< 保留域段 */

    struct enhance_cmdq_ctxt_info ctxt;   /**< ctx信息 */
};

struct comm_cmd_virtio_en {   /**< virtio 配置 到加载sdk之后 */
    struct mgmt_msg_head head;   /**< mbox消息头 */
    u8 msien_snap_2_virtio_en;   /**< msien virtio使能 */
    u8 rsv[3];
};

struct cqm_cmd_func_secure_mem {   /**< 安全内存信息获取 */
	struct mgmt_msg_head head;   /**< mbox消息头 */
	u16 func_id;   /**< 指定function id */
	u16 rsvd0;   /**< 保留域段 */
	u32 gpa_hi;   /**< gpa高位地址 */
	u32 gpa_lo;   /**< gpa低位地址 */
	u32 len;   /**< 长度 */
	u8 gpa_mode;   /**< gpa模式 */
    u8 valid;   /**< 有效位 */
	u8 rsvd1[2];   /**< 保留域段 */
};

struct nic_plug_cap {   /**< 热拔插能力 */
    u16 max_sqs;   /**< 最大sq size */
	u16 max_rqs;   /**< 最大rq size */
};

struct comm_cmd_plug_srv {   /**< 热拔插服务 */
	struct mgmt_msg_head head;   /**< mbox消息头 */
	u16 func_id;   /**< 指定function id */
    u8 srv_type;   /**< service类型 */
    u8 attach_en;   /**< 使能标识 */
    struct nic_plug_cap nic_cap;   /**< nic热拔插能力 */
	u32 rsvd;   /**< 保留域段 */
};

struct comm_cmd_fast_msg_cap {   /**< fast msg能力 */
    struct mgmt_msg_head head;   /**< mbox消息头 */
    u32 func_id;   /**< 指定function id */
    u32 fast_msg_depth;    /**< PF:2048, VF:512 */
    u32 fast_msg_page_size;    /**< 消息页面大小256(单位K) */
    u32 rsvd[9];   /**< 保留域段 */
};

#define FAST_MSG_MAX_PAGE_NUM 32   /**< fast msg页面数量 */
struct comm_cmd_fast_msg_rq_addr {
    struct mgmt_msg_head head;
    u32 func_id;
    u32 page_num;
    u32 rsvd[2];
    u64 page_addr[FAST_MSG_MAX_PAGE_NUM];
};

struct fast_msg_rq_addr {   /**< fast msg rq地址 */
    u64 rq_page_addr;   /**< rq页面地址 */
};

struct comm_cmd_set_fast_msg_rq_addr {   /**< fast msg rq地址设置 */
	struct mgmt_msg_head head;   /**< mbox消息头 */
	u32 func_id;   /**< 指定function id */
    u32 page_num;   /**< 页面数量 */
    u32 rsvd[2];   /**< 保留域段 */
    struct fast_msg_rq_addr page_addr[32];   /**< 页面地址 */
};

struct comm_cmd_clear_fast_msg_sml_table {   /**< fast msg 清除表项 */
	struct mgmt_msg_head head;   /**< mbox消息头 */
    u32 func_id;   /**< 指定function id */
    u32 rsvd[5]; /**< 保留域段, 32 Bytes total */
};

struct comm_cmd_root_ctxt {   /**< root ctx配置 */
    struct mgmt_msg_head head;   /**< mbox消息头 */

    u16 func_id;   /**< 指定function id */
    u8 set_cmdq_depth;   /**< cmdq深度设置标识 */
    u8 cmdq_depth;   /**< cmdq深度 */
    u16 rx_buf_sz;   /**< rx buff size */
    u8 lro_en;   /**< lro使能标识 */
    u8 cmdq_mode;   /**< cmdq模式 */
    u16 sq_depth;   /**< sq深度 */
    u16 rq_depth;   /**< rq深度 */
    u32 rsvd1;   /**< 保留域段 */
    u64 rsvd2;   /**< 保留域段 */
};

struct comm_cmd_wq_page_size {   /**< root ctx wqe配置 */
    struct mgmt_msg_head head;   /**< mbox消息头 */

    u16 func_id;   /**< 指定function id */
    u8 opcode;   /**< 操作标识，0：get，1：set */
    u8 page_size;   /**< real_size=4KB*2^page_size, range(0~20) must be checked by driver */
    u32 rsvd1;   /**< 保留域段 */
};

struct comm_cmd_msix_config {   /**< msix中断配置 */
    struct mgmt_msg_head head;   /**< mbox消息头 */

    u16 func_id;   /**< 指定function id */
    u8 opcode;   /**< 操作标识，0：get，1：set */
    u8 rsvd1;   /**< 保留域段 */
    u16 msix_index;   /**< 中断idx */
    u8 pending_cnt;   /**< It specifies the maximum wait time for resending period. */
    u8 coalesce_timer_cnt;   /**< 聚合配置 */
    u8 resend_timer_cnt;   /**< 重传计数 */
    u8 lli_timer_cnt;   /**< 信用补偿配置 */
    u8 lli_credit_cnt;   /**< 信用补充门限 */
    u8 rsvd2[5];   /**< 保留域段 */
};

struct comm_cmd_cfg_msix_num {   /**< msix中断数量配置 */
    struct mgmt_msg_head head;    /**< mbox消息头 */

    u16 func_id;   /**< 指定function id */
    u8 op_code;    /**< 1: alloc 0: free */
    u8 rsvd0;   /**< 保留域段 */

    u16 msix_num;   /**< msix数量 */
    u16 rsvd1;   /**< 保留域段 */
};

struct comm_cmd_dma_attr_config {   /**< dma属性配置(当前暂未使用) */
    struct mgmt_msg_head head;   /**< mbox消息头 */

    u16 func_id;   /**< 指定function id */
    u8 entry_idx;
    u8 st;
    u8 at;
    u8 ph;
    u8 no_snooping;
    u8 tph_en;
    u32 resv1;
};

struct comm_cmd_ppf_tbl_htrp_config {   /**< ppf热替换配置 */
    struct mgmt_msg_head head;   /**< mbox消息头 */

    u32 hotreplace_flag;   /**< 热替换标识 */
};

struct comm_cmd_ceq_ctrl_reg {   /**< ceq配置 */
    struct mgmt_msg_head head;   /**< mbox消息头 */

    u16 func_id;   /**< 指定function id */
    u16 q_id;   /**< q id */
    u32 ctrl0;   /**< ceq ctrl0 */
    u32 ctrl1;   /**< ceq ctrl1 */
    u32 rsvd1;   /**< 保留域段 */
};

struct comm_cmd_func_tmr_bitmap_op {   /**< 使能 smf timr bitmap操作 */
    struct mgmt_msg_head head;   /**< mbox消息头 */

    u16 func_id;   /**< 指定function id */
    u8 opcode;   /**< 1: start, 0: stop */
    u8 rsvd1[5];   /**< 保留域段 */
};

struct comm_cmd_ppf_tmr_op {   /**< smf timr配置 */
    struct mgmt_msg_head head;   /**< mbox消息头 */

    u8 ppf_id;   /**< ppf id */
    u8 opcode;   /**< 1: start, 0: stop */
    u8 rsvd1[6];   /**< 保留域段 */
};

#define HT_GPA_CLEAR 0 /**< gpa clr */
#define HT_GPA_SET 1    /**< gpa set */
struct comm_cmd_ht_gpa {    /**< gpa操作 */
    struct mgmt_msg_head head; /**< mbox消息头 */

    u8 host_id;   /**< 指定host */
    u8 opcode;  /**< 1:set, 0: clear */
    u8 rsvd0[2];   /**< 保留域段 */
    u32 rsvd1[7];   /**< 保留域段 */
    u64 page_pa0;   /**< gpa地址0 */
    u64 page_pa1;   /**< gpa地址1 */
};

struct comm_cmd_get_eqm_num {   /**< mqm eqm配置 */
    struct mgmt_msg_head head;   /**< mbox消息头 */

    u8 host_id;   /**< 指定host */
    u8 rsvd1[3];   /**< 保留域段 */
    u32 chunk_num;   /**< chunk num */
    u32 search_gpa_num;   /**< search gpa num */
};

struct comm_cmd_eqm_cfg {   /**< mqm溢出配置 */
    struct mgmt_msg_head head;   /**< mbox消息头 */

    u8 host_id;   /**< 指定host */
    u8 valid;   /**< 有效位 */
    u16 rsvd1;   /**< 保留域段 */
    u32 page_size;   /**< 页面size */
    u32 rsvd2;   /**< 保留域段 */
};

struct comm_cmd_eqm_search_gpa {   /**< mqm search gpa配置 */
    struct mgmt_msg_head head;   /**< mbox消息头 */

    u8 host_id;   /**< 指定host */
    u8 rsvd1[3];   /**< 保留域段 */
    u32 start_idx;   /**< 起始idx */
    u32 num;   /**< 数量 */
    u32 rsvd2;   /**< 保留域段 */
    u64 gpa_hi52[0];   /**< gpa */
};

struct comm_cmd_set_bat_info {
    struct mgmt_msg_head head;

    u16 func_id;
    u8  smf_id;
    u8  rsvd1;
    u32 bat_offset;
    u32 data_size;
    u8  data[256];
};

struct hinic5_board_info {   /**< 获取板卡信息 */
    u8 board_type;   /**< 板卡类型 */
    u8 port_num;   /**< 网口数量 */
    u8 port_speed;   /**< 板卡速率 */
    u8 host_width;   /**< 网口带宽 */
    u8 host_num;   /**< 支持host数量 */
    u8 pf_num;   /**< 支持pf数量 */
    u16 vf_total_num;   /**< 支持vf数量 */
    u8 tile_num;   /**< 支持tile数量 */
    u8 qcm_num;   /**< 支持qcm数量 */
    u8 core_num;   /**< 支持tile core数量 */
    u8 work_mode;   /**< 板卡工作模式 */
    u8 service_mode;   /**< 板卡支持service模式 */
    u8 board_mode;   /**< 板卡模式 */
    u8 boot_sel;   /**< 启动模式 */
    u8 board_id;   /**< 板卡id */
    u32 cfg_addr;   /**< 配置文件地址 */
    u32 service_en_bitmap;   /**< service使能特性 */
    u8 scenes_id;   /**< 场景id */
    u8 cfg_template_id;   /**< 配置模板id */
    u8 hardware_id;   /**< 硬件id */
    u8 spu_en;   /**< spu使能标识 */
    u16 pf_vendor_id;   /**< 设备厂商id */
    u8 tile_bitmap;   /**< tile使能bitmap */
    u8 sm_bitmap;   /**< sm使能bitmap */
    u8 smf_bitmap_hi; /**< 保存高4bit的smf */
    u8 board_type_hi; /**< 保存高8bit的board_type */
    u8 host_type : 2; /**< 取值参考BUS_TYPE_E 0 : pcie, 1 : ubc */
    u8 pg_grade : 2; /**< 取值参考PARTIAL_GOOD_GRADE_MODE，0 : fg, 1 : pg */
    u8 rsvd0 : 4;
    u8 rsvd;
    u32 service_en_bitmap2; /**< service使能特性扩展 */
};

struct comm_cmd_board_info {   /**< 获取板卡信息 */
    struct mgmt_msg_head head;   /**< mbox消息头 */

    struct hinic5_board_info info;   /**< 板卡信息 */
    u32 rsvd[20];   /**< 保留域段 */
};

struct comm_cmd_sync_time {   /**< 驱动时间同步 */
    struct mgmt_msg_head head;   /**< mbox消息头 */

    u64 mstime;   /**< 时间戳 */
    u64 sync_time;
};

struct comm_cmd_sdi_info {   /**< 获取sdi信息 */
    struct mgmt_msg_head head;   /**< mbox消息头 */
    u32 cfg_sdi_mode;   /**< 配置sdi模式 */
};

enum tool_run_env {
    TOOL_RUN_ENV_HOST,   /**< host侧 */
    TOOL_RUN_ENV_SPU,   /**< spu侧 */
    TOOL_RUN_ENV_INVALID = 0xFF
};
typedef u8 tool_run_env_u8;

enum chip_ver {
    CHIP_VER_HI1823V100,   /**< 芯片1823v100 */
    CHIP_VER_HI1823EV100,   /**< 芯片1823v100e */
    CHIP_VER_HI1823V200,   /**< 芯片1823v200 */
    CHIP_VER_HI1872V100,   /**< 芯片1872v100 */
    CHIP_VER_HI1825V100,   /**< 芯片1825v100 */
    CHIP_VER_INVALID = 0xFF
};
typedef u8 chip_ver_u8;   /**< 芯片版本 */

enum chip_type {
    CHIP_TYPE_FPGA,   /**< 芯片fpga */
    CHIP_TYPE_ASIC,   /**< 芯片asic */
    CHIP_TYPE_EMU,   /**< 芯片emu */
    CHIP_TYPE_EDA,   /**< 芯片eda */
    CHIP_TYPE_INVALID = 0xFF
};
typedef u8 chip_type_u8;   /**< 芯片平台 */

struct comm_cmd_compatible_info {   /**< huoq环境信息 */
    struct mgmt_msg_head head;   /**< mbox消息头 */
    chip_ver_u8 chip_ver;     /**< 芯片版本 */
    tool_run_env_u8 host_env;    /**< host类型 */
    chip_type_u8 chip_type;   /**< 芯片类型/平台 */
    u8 dual_die_flag;   /**< 双die使能标识 0：否(单die)，1：是(双die) */
    u32 mpu_ver;   /**< mpu版本 */
    u32 npu_ver;   /**< 微码版本 */
    u32 rsv1[31];   /**< 保留域段 */
};

/* func flr set */
struct comm_cmd_func_flr_set {   /**< 设置function flr类型 */
    struct mgmt_msg_head head;   /**< mbox消息头 */

    u16 func_id;   /**< 指定function id */
    u8 type;    /**< 1: close 置flush */
    u8 isall;   /**< 是否操作对应pf下的所有vf 1: all vf */
    u32 rsvd;
};

struct comm_cmd_bdf_info {   /**< 获取bdf号 */
    struct mgmt_msg_head head;   /**< mbox消息头 */

    u16 function_idx;   /**< 指定function id */
    u8 rsvd1[2];   /**< 保留域段 */
    u8 bus;   /**< 总线号 */
    u8 device;   /**< 设备号 */
    u8 function;   /**< function号 */
    u8 rsvd2[5];   /**< 保留域段 */
};

struct hw_pf_info {   /**< 硬件pf信息 */
    u16 glb_func_idx;   /**< 全局function id */
    u16 glb_pf_vf_offset;   /**< 该pf的起始vf id */
    u8 p2p_idx;   /**< p2p idx */
    u8 itf_idx;   /**< host id */
    u16 max_vfs;   /**< vf数量 */
    u16 max_queue_num;   /**< 队列数量 */
    u16 vf_max_queue_num;   /**< vf支持的队列数量 */
    u16 port_id;   /**< 网口id */
    u16 rsvd0;   /**< 保留域段 */
    u32 pf_service_en_bitmap;   /**< pf的service en */
    u32 vf_service_en_bitmap;   /**< vf的service en */
    u16 rsvd1[2];   /**< 保留域段 */

    u8 device_type;   /**< 设备类型 */
    u8 bus_num;      /**< 总线号 */
    u16 vf_stride;    /**< vf步长 */
    u16 vf_offset;   /**< vf相对偏移 */
    u8 func_valid_map : 2;    /**< 0：呈现所有function, 1: 呈现奇数function, 2：呈现偶数function, 3：无效值 */
    u8 rsvd2 : 6;   /**< 保留域段 */
    u8 rsvd;   /**< 保留域段 */
};

#define CMD_MAX_MAX_PF_NUM 32   /**< 最大pf数量 */
struct hinic5_hw_pf_infos {   /**< 硬件pf信息 */
    u8 num_pfs;   /**< pf个数 */
    u8 rsvd1[3];   /**< 保留域段 */

    struct hw_pf_info infos[CMD_MAX_MAX_PF_NUM];   /**< 硬件pf信息 */
};

struct comm_cmd_hw_pf_infos {   /**< 硬件pf信息 */
    struct mgmt_msg_head head;   /**< mbox消息头 */

    struct hinic5_hw_pf_infos infos;   /**< pf信息 */
};

#define DD_CFG_TEMPLATE_MAX_IDX 12   /**< 支持的配置模板数量 */
#define DD_CFG_TEMPLATE_MAX_TXT_LEN 64   /**< 支持的配置模板大小 */
#define CFG_TEMPLATE_OP_QUERY 0   /**< 查询配置模板 */
#define CFG_TEMPLATE_OP_SET 1   /**< 设置配置模板 */
#define CFG_TEMPLATE_SET_MODE_BY_IDX 0
#define CFG_TEMPLATE_SET_MODE_BY_NAME 1

struct comm_cmd_cfg_template {   /**< 配置版本配置 */
    struct mgmt_msg_head head;   /**< mbox消息头 */
    u8 opt_type;    /**< 0: query  1: set */
    u8 set_mode;    /**< 0-index mode. 1-name mode. */
    u8 tp_err;   /**< 模板错误标识 */
    u8 rsvd0;   /**< 保留域段 */

    u8 cur_index;      /**< Current cfg tempalte index. */
    u8 cur_max_index;    /**< Max support cfg tempalte index. */
    u8 rsvd1[2];   /**< 保留域段 */
    u8 cur_name[DD_CFG_TEMPLATE_MAX_TXT_LEN];   /**< 当前模板名称 */
    u8 cur_cfg_temp_info[DD_CFG_TEMPLATE_MAX_IDX][DD_CFG_TEMPLATE_MAX_TXT_LEN];   /**< 当前模板信息 */

    u8 next_index;     /**< Next reset cfg tempalte index. */
    u8 next_max_index;    /**< Max support cfg tempalte index. */
    u8 rsvd2[2];   /**< 保留域段 */
    u8 next_name[DD_CFG_TEMPLATE_MAX_TXT_LEN];   /**< 下一个模板名称 */
    u8 next_cfg_temp_info[DD_CFG_TEMPLATE_MAX_IDX][DD_CFG_TEMPLATE_MAX_TXT_LEN];   /**< 下一个模板信息 */
};

#define MQM_SUPPORT_COS_NUM 8   /**< cos数量 */
#define MQM_INVALID_WEIGHT 256   /**< mqm表大小 */
#define MQM_LIMIT_SET_FLAG_READ 0   /**< read */
#define MQM_LIMIT_SET_FLAG_WRITE 1   /**< write */
struct comm_cmd_set_mqm_limit {   /**< mqm限速配置 */
    struct mgmt_msg_head head;   /**< mbox消息头 */

    u16 set_flag;   /**< 置位该标记位表示设置 */
    u16 func_id;   /**< 指定function id */
    u16 cos_weight[MQM_SUPPORT_COS_NUM];    /**< 对应cos_id所占的权重，0-255, 0为SP调度. */
    u32 host_min_rate;        /**< 本host支持的最低限速 */
    u32 func_min_rate;        /**< 本function支持的最低限速，单位Mbps */
    u32 func_max_rate;          /**< 本function支持的最高限速，单位Mbps  */
    u8 rsvd[64];                /**< 保留域段 */
};

enum core_type_e {
    CORE_TYPE_ARM  = 0,
    CORE_TYPE_LINX = 1
};

struct arm_core_reg_info {
    u64 elr;   /**< 通用寄存器 */
    u64 spsr;   /**< 通用寄存器 */
    u64 far;   /**< 通用寄存器 */
    u64 esr;   /**< 通用寄存器 */
    u64 xzr;   /**< 通用寄存器 */
    u64 x30;   /**< 通用寄存器 */
    u64 x29;   /**< 通用寄存器 */
    u64 x28;   /**< 通用寄存器 */
    u64 x27;   /**< 通用寄存器 */
    u64 x26;   /**< 通用寄存器 */
    u64 x25;   /**< 通用寄存器 */
    u64 x24;   /**< 通用寄存器 */
    u64 x23;   /**< 通用寄存器 */
    u64 x22;   /**< 通用寄存器 */
    u64 x21;   /**< 通用寄存器 */
    u64 x20;   /**< 通用寄存器 */
    u64 x19;   /**< 通用寄存器 */
    u64 x18;   /**< 通用寄存器 */
    u64 x17;   /**< 通用寄存器 */
    u64 x16;   /**< 通用寄存器 */
    u64 x15;   /**< 通用寄存器 */
    u64 x14;   /**< 通用寄存器 */
    u64 x13;   /**< 通用寄存器 */
    u64 x12;   /**< 通用寄存器 */
    u64 x11;   /**< 通用寄存器 */
    u64 x10;   /**< 通用寄存器 */
    u64 x09;   /**< 通用寄存器 */
    u64 x08;   /**< 通用寄存器 */
    u64 x07;   /**< 通用寄存器 */
    u64 x06;   /**< 通用寄存器 */
    u64 x05;   /**< 通用寄存器 */
    u64 x04;   /**< 通用寄存器 */
    u64 x03;   /**< 通用寄存器 */
    u64 x02;   /**< 通用寄存器 */
    u64 x01;   /**< 通用寄存器 */
    u64 x00;   /**< 通用寄存器 */
};

struct linx_core_reg_info {
    u32 s0;
    u32 s1;
    u32 s2;
    u32 s3;
    u32 s4;
    u32 s5;
    u32 s6;
    u32 s7;
    u32 s8;
    u32 s9;
    u32 s10;
    u32 s11;
    u32 ra;
    u32 gp;
    u32 tp;
    u32 t0;
    u32 t1;
    u32 t2;
    u32 t3;
    u32 t4;
    u32 t5;
    u32 t6;
    u32 a0;
    u32 a1;
    u32 a2;
    u32 a3;
    u32 a4;
    u32 a5;
    u32 a6;
    u32 a7;
    u32 mepc;
    u32 mstatus;
    u32 mcause;
    u32 rsv[39];   /**< 该结构体总大小与struct arm_core_reg_info保持一致 */
};

#define DATA_LEN_1K 1024
struct comm_info_sw_watchdog {   /**< 软狗超时信息上报接口 */
    struct mgmt_msg_head head;   /**< mbox消息头 */

    /**< 全局信息 */
    u32 curr_time_h;    /**< 发生死循环的时间,cycle */
    u32 curr_time_l;    /**< 发生死循环的时间,cycle */
    u32 task_id;       /**< 发生死循环的任务       */
    u8 core_type;     /**< 参考core_type_e定义   */
    u8 rsv[3];        /**< 保留字段，用于扩展     */

    /**< 寄存器信息，TSK_CONTEXT_S */
    u64 pc;   /**< 通用寄存器 */

    union core_reg {
        struct arm_core_reg_info  arm_reg;
        struct linx_core_reg_info linx_reg;
    } reg_info;

    /**< 堆栈控制信息，STACK_INFO_S */
    u64 stack_top;    /**< 栈顶                   */
    u64 stack_bottom;    /**< 栈底                   */
    u64 sp;      /**< 栈当前SP指针值         */
    u32 curr_used;      /**< 栈当前使用的大小       */
    u32 peak_used;       /**< 栈使用的历史峰值       */
    u32 is_overflow;       /**< 栈是否溢出             */

    /**< 堆栈具体内容 */
    u32 stack_actlen;      /**< 实际的堆栈长度(<=1024) */
    u8 stack_data[DATA_LEN_1K];    /**< 超过1024部分，会被截断 */
};

/**< 临终遗言信息 */
#define XREGS_NUM 31   /**< 寄存器数量 */
typedef struct tag_cpu_tick {   /**< 时间 */
    u32 cnt_hi;    /**<  cycle计数高32位 */
    u32 cnt_lo;   /**< cycle计数低32位 */
} CPU_TICK;

typedef struct tag_ax_exc_reg_info {   /**< 通用寄存器 */
    u64 ttbr0;   /**< 通用寄存器 */
    u64 ttbr1;   /**< 通用寄存器 */
    u64 tcr;   /**< 通用寄存器 */
    u64 mair;   /**< 通用寄存器 */
    u64 sctlr;   /**< 通用寄存器 */
    u64 vbar;   /**< 通用寄存器 */
    u64 current_el;   /**< 通用寄存器 */
    u64 sp;   /**< 通用寄存器 */
    /**< 以下字段的内存布局与TskContext保持一致 */
    u64 elr;      /**< 通用寄存器 */
    u64 spsr;   /**< 通用寄存器 */
    u64 far_r;   /**< 通用寄存器 */
    u64 esr;   /**< 通用寄存器 */
    u64 xzr;   /**< 通用寄存器 */
    u64 xregs[XREGS_NUM];    /**< 寄存器0~30: x30~x0 */
} EXC_REGS_S;

typedef struct exc_call_stack_info {
    u32 depth;  /* 调用栈深度 */
    u64 addrList[10];  /* 调用栈地址列表 */
    char nameList[10][64];  /* 调用栈函数名列表 */
} exc_call_stack_info_s;

typedef struct tag_exc_info {
    char os_ver[48];     /**< OS版本号                                         */
    char app_ver[64];    /**< 产品版本号                                       */
    u32 exc_cause;     /**< 异常原因                                         */
    u32 thread_type;    /**< 异常前的线程类型                                 */
    u32 thread_id;      /**< 异常前线程PID                                    */
    u16 byte_order;     /**< 字节序                                           */
    u16 cpu_type;     /**< CPU类型                                          */
    u32 cpu_id;       /**< CPU ID                                           */
    CPU_TICK cpu_tick;   /**< CPU Tick                                         */
    u32 nest_cnt;      /**< 异常嵌套计数                                     */
    u32 fatal_errno;     /**< 致命错误码，发生致命错误时有效                    */
    u64 uw_sp;         /**< 异常前栈指针                                     */
    u64 stack_bottom;    /**< 异常前栈底     */
    /**< 异常发生时的核内寄存器上下文信息，82\57必须位于152字节处，若改动，需更新sre_platform.eh中的OS_EXC_REGINFO_OFFSET宏 */
    EXC_REGS_S reg_info;
} EXC_INFO_S;

typedef struct tag_exc_info_all {
    char os_ver[48];     /**< OS版本号                                         */
    char app_ver[64];    /**< 产品版本号                                       */
    u32 exc_cause;     /**< 异常原因                                         */
    u32 thread_type;    /**< 异常前的线程类型                                 */
    u32 thread_id;      /**< 异常前线程PID                                    */
    u16 byte_order;     /**< 字节序                                           */
    u16 cpu_type;     /**< CPU类型                                          */
    u32 cpu_id;       /**< CPU ID                                           */
    CPU_TICK cpu_tick;   /**< CPU Tick                                         */
    u32 nest_cnt;      /**< 异常嵌套计数                                     */
    u32 fatal_errno;     /**< 致命错误码，发生致命错误时有效                    */
    u64 uw_sp;         /**< 异常前栈指针                                     */
    u64 stack_bottom;    /**< 异常前栈底     */
    /**< 异常发生时的核内寄存器上下文信息，82\57必须位于152字节处，若改动，需更新sre_platform.eh中的OS_EXC_REGINFO_OFFSET宏 */
    EXC_REGS_S reg_info;
    exc_call_stack_info_s call_stack_info;
} EXC_INFO_ALL_S;

#define MPU_LASTWORD_SIZE 1024   /**< 临终遗言单次数据长度 */
typedef struct tag_comm_info_up_lastword {   /**< 上报给驱动的up lastword模块接口 */
    struct mgmt_msg_head head;   /**< mbox消息头 */

    EXC_INFO_S stack_info;   /**< 堆栈信息 */

    /* 堆栈具体内容 */
    u32 stack_actlen;   /**<实际的堆栈长度(<=1024) */
    u8 stack_data[MPU_LASTWORD_SIZE];    /**< payload 超过1024部分，会被截断 */
} comm_info_up_lastword_s;

typedef struct {
    u32 magic;
    u32 symbol_num;  /**< 符号个数 */
    u32 code_size;  /**< 补丁代码大小 */
    u32 rsvd0[5];  /**< 保留字段 */
    char git_tag[64]; /**< 冷基线的git标签 */
    char compile_time[20]; /**< 编译时间 */
    u32 rsvd1;  /**< 用于8字节对齐的保留字段 */
} patch_head_info_s;

struct hinic5_cmd_update_firmware {   /**< 固件升级 */
    struct mgmt_msg_head msg_head;   /**< mbox消息头 */

    struct {
        u32 sl : 1;   /**< 尾片 */
        u32 sf : 1;   /**< 首片 */
        u32 flag : 1;   /**< 分区标识 */
        u32 bit_signed : 1;   /**< 签名标识 */
        u32 reserved : 12;   /**< 保留域段 */
        u32 fragment_len : 16;   /**< 分片长度 */
    } ctl_info;   /**< 控制信息 */

    struct {
        u32 section_crc;   /**< 子固件crc */
        u32 section_type;   /**< 子固件类型 */
    } section_info;   /**< 子固件信息 */

    u32 total_len;   /**< 镜像长度 */
    u32 section_len;   /**< 子固件长度 */
    u32 section_version;   /**< 子固件版本 */
    u32 section_offset;   /**< 子固件偏移 */
    u32 data[384];   /**< 镜像数据 */
};

struct hinic5_cmd_activate_firmware {   /**< 镜像激活 */
    struct mgmt_msg_head msg_head;   /**< mbox消息头 */
    u8 index;    /**< 配置文件激活idx(默认使用0) */
    u8 data[7];   /**< payload */
};

struct hinic5_cmd_switch_config {   /**< 配置文件切换 */
    struct mgmt_msg_head msg_head;   /**< mbox消息头 */
    u8 index;    /**< 配置文件idx0~1 */
    u8 data[7];   /**< payload */
};

/* start 为适配ub访问寄存器，定义的id，用于接口中识别UB模块, 不能和INTERNAL_RING_NODE_ID_E冲突 */
enum hinic5_ub_mod_id {
    HINIC5_UB_D2H = 64,
    HINIC5_UB_LQ_TP = HINIC5_UB_D2H,
    HINIC5_UB_MISC,
    HINIC5_UB_LQ_MISC,
    HINIC5_UBC0_LQ_NL_DL,
    HINIC5_UBC1_LQ_NL_DL,
    HINIC5_UBC2_LQ_NL_DL,
    /* 1872 */
    HINIC5_UBG_MISC,
    HINIC5_UBG_BA,
    HINIC5_UBG_TM,
    HINIC5_UBG_DLPHY,
    HINIC5_UBG_NL,
    HINIC5_UBG_TA,
    HINIC5_UBG_TP,
    HINIC5_UBG_IMMU,
    HINIC5_UBG_OMMU,
    HINIC5_UBC_D2H,
    HINIC5_UBC_TP,
    HINIC5_UBC_MISC,
    HINIC5_UBC_NL,
    HINIC5_UBC_DLPHY,
    HINIC5_UBC_TA,
    /* 1872 */
    HINIC5_UB_END_IDX,
};
/* end 为适配ub访问寄存器，定义的node id，用于接口中识别UB模块, 不能和INTERNAL_RING_NODE_ID_E冲突 */

#define MAX_DATA_NUM     (240)
struct csr_msg {   /**< csr寄存器读信息 */
    struct {
        u32 node_id             : 5;        /**< 节点id */
        u32 data_width          : 10;       /**< 访问宽度 */
        u32 module_id           : 8;        /**< 模块id，用于识别地址属于哪个模块（部分模块没有node_id） */
        u32 rsvd                : 9;        /* 保留域段 */
    } bits;
    u32 addr;   /**< 地址 */
};

struct comm_cmd_mbox_csr_rd_req {   /**< csr寄存器读请求 */
    struct mgmt_msg_head head;   /**< mbox消息头 */
    struct csr_msg csr_info[MAX_DATA_NUM];   /**< payload */
    u32 data_num;   /**< 寄存器个数 */
};

struct comm_cmd_mbox_csr_rd_ret {   /**< csr寄存器读 */
    struct mgmt_msg_head head;   /**< mbox消息头 */
    u64 value[MAX_DATA_NUM];   /**< 寄存器读取结果 */
};

struct comm_cmd_mbox_csr_rd_req_ex {   /**< csr寄存器读请求 */
    struct mgmt_msg_head head;   /**< mbox消息头 */
    u32 data_num;   /**< 寄存器个数 */
    struct csr_msg csr_info[0];    /**< 根据实际读取个数填充 */
};

struct comm_cmd_mbox_csr_rd_ret_ex {   /**< csr寄存器读 */
    struct mgmt_msg_head head;   /**< mbox消息头 */
    u64 value[0];    /**< 根据实际读取个数填充 */
};

struct comm_cmd_mbox_csr_wt_req {   /**< csr寄存器写 */
    struct mgmt_msg_head head;   /**< mbox消息头 */
    struct csr_msg csr_info;   /**< csr控制寄存器信息 */
    u64 value;   /**< 值 */
};

struct comm_cmd_mbox_csr_wt_ret {   /**< csr寄存器写 */
    struct mgmt_msg_head head;   /**< mbox消息头 */
};

#define INDIR_MAX_INDEX_NUM 480
#define INDIR_MAX_WT_INDEX_NUM 32

struct comm_cmd_mbox_indir_addr {   /**< 间接表操作信息 */
    u32 indir_ctrl_addr;   /**< 控制寄存器 */
    u32 indir_timeout_addr;   /**< 超时寄存器 */
    u32 indir_data_addr;   /**< 数据寄存器 */
};

struct comm_cmd_mbox_indir_tab_rd_req {   /**<  间接表读请求 */
    struct mgmt_msg_head head;   /**< mbox消息头 */

    struct comm_cmd_mbox_indir_addr indir_addr;   /**< 间接表控制信息 */
    u32 tab_width;   /**< 表项宽度 */
    u32 index_num;   /**< offset idx */
    u32 index[INDIR_MAX_INDEX_NUM];   /**< payload */
};

struct comm_cmd_mbox_indir_tab_rd_ret {   /**< 间接表读请求 */
    struct mgmt_msg_head head;   /**< mbox消息头 */
    u32 data[INDIR_MAX_INDEX_NUM];   /**< payload */
};

struct comm_cmd_mbox_indir_tab_wt_req {   /**< 间接表写请求 */
    struct mgmt_msg_head head;   /**< mbox消息头 */

    struct comm_cmd_mbox_indir_addr indir_addr;   /**< 间接表控制信息 */
    u32 tab_width;   /**< 表项宽度 */
    u32 index;   /**< offset idx */
    u32 data[INDIR_MAX_WT_INDEX_NUM];   /**< payload */
};

struct comm_cmd_mbox_indir_tab_wt_ret {   /**< 间接表写请求 */
    struct mgmt_msg_head head;   /**< mbox消息头 */
};

enum {
    MPU_LOG_CLEAR = 0,   /**< 清除mpu日志 */
    SMU_LOG_CLEAR,   /**< 清除smu日志 */
    NPU_LOG_CLEAR,   /**< 清除npu日志 */
    SPU_LOG_CLEAR,   /**< 清除spu日志 */
    MPU_LASTWORD_CLEAR,   /**< 清除mpu临终遗言 */
    NPU_LASTWORD_CLEAR,   /**< 清除微码临终遗言 */
    ALL_LOG_CLEAR,   /**< 清除全量日志&临终遗言 */
    UBC_IMP_LOG_CLEAR,   /**< 清除ubc imp日志 */
    UBC_IMP_LASTWORD_CLEAR,   /**< 清除ubc imp临终遗言 */
    ROCE_IMP_LOG_CLEAR,   /**< 清除roce imp日志 */
    ROCE_IMP_LASTWORD_CLEAR,   /**< 清除roce imp临终遗言 */
    ROCE_SCC_LOG_CLEAR,   /**< 清除roce scc日志 */
    CLEAR_TYPE_BUTT,
};

struct comm_cmd_clear_log {   /**< 清除log */
    struct mgmt_msg_head head;    /**< mbox消息头 */
    u32 type;   /**< 清除日志类型 */
};

struct cmd_sector_info {   /**< 擦除flash */
    struct mgmt_msg_head head;    /**< mbox消息头 */
    u32 offset;                   /**< flash地址 */
    u32 len;                       /**< flash擦除长度 */
};

enum flash_counter_info_req_type {
    FLASH_COUNTER_TYPE_GET_MPU_SIZE,   /**< mpu counter大小获取类型 */
    FLASH_COUNTER_TYPE_GET_MPU_DATA,   /**< mpu counter数据获取类型 */
    FLASH_COUNTER_TYPE_GET_NPU_SIZE,   /**< npu counter大小获取类型 */
    FLASH_COUNTER_TYPE_GET_NPU_DATA,   /**< npu counter数据获取类型 */
    FLASH_COUNTER_TYPE_INVALID   /**< counter获取类型无效值 */
};

struct flash_counter_info_req {   /**< 获取固件counter信息 */
    struct mgmt_msg_head head;   /**< mbox消息头 */
    u8 type;    /**< flash_counter_info_req_type */
    u8 rsv[3];   /**< 保留域段 */
    u32 offset;   /**< 地址偏移 */
    u32 length;   /**< 数据长度 */
};

#define FLASH_COUNTER_TYPE_GET_DATA_MAX_SIZE 1024
struct flash_counter_info_resp {   /**< 获取固件counter信息 */
    struct mgmt_msg_head head;   /**< mbox消息头 */
    u32 length;   /**< 数据长度 */
    u8 data[FLASH_COUNTER_TYPE_GET_DATA_MAX_SIZE];   /**< payload */
};

typedef struct {
    u64 smu_images;   /**< smu镜像 */
    u64 mpu_images;   /**< mpu镜像 */
    u64 npu_images;   /**< npu镜像 */
    u64 ppe_images;   /**< 微码ppe镜像 */
    u64 cfg_images;   /**< 配置文件镜像 */
    u64 patch_images;   /**< mpu补丁镜像 */
    u64 rsvd[4];   /**< 保留域段 */
} module_images;   /**< 固件镜像类型 */

typedef struct {
    struct mgmt_msg_head head;   /**< mbox消息头 */
    u32 rsvd[4];   /**< 保留域段 */
} comm_cmd_query_module_images_req;   /**< 查询镜像类型 */

typedef struct {
    struct mgmt_msg_head head;   /**< mbox消息头 */
    module_images img;   /**< 固件镜像 */
} comm_cmd_query_module_images_rsp;   /**< 查询镜像类型 */

#define SINGLE_EFUSE_BIN_SIZE 512
#define SEND_EFUSE_DATA_SIZE (SINGLE_EFUSE_BIN_SIZE * 3)
struct send_efuse_data_s {   /**< efuse信息烧写 */
    struct mgmt_msg_head head;     /**< mbox消息头 */
    u8 opt_type;                 /**< efuse操作类型：1: burn efuse bin，2: hw rotpk switch to guest rotpk */
    u8 rsvd0[3];   /**< 保留域段 */
    u32 total_len;    /**< entire package leng value */
    u32 data_csum;    /**< entire package data count sum value */
    u8 data[SEND_EFUSE_DATA_SIZE];    /**<  payload 1024B*/
};

typedef enum {
    BURN_EFUSE_BIN = 1,
    REVOKE_SEC_VER_NUM,
    BURN_HISS_EFUSE0_BIN,
    BURN_HISS_EFUSE1_BIN,
    BURN_ALL_EFUSE_BIN,
    NONE_BURN_EFUSE_BIN,
} eufse_option_type_e;

#define DFX_MAG_MAX_REG_NUM (32)
struct comm_info_dfx_mag_reg {
    struct mgmt_msg_head head;
    u16 sel;      /**< 方向: 0 - tx 1 - rx */
    u16 write;    /**< 读写标志: 0 - 读 1 - 写 */
    u32 reg_addr; /**< 寄存器地址 */
    u32 reg_cnt;  /**< 以reg_addr为BASE_ADDR，读取多少个连续的寄存器个数(不是字节数，是寄存器个数)，最大32个 */
    u32 clear;    /**< 读清标志: 0 - 不读清 1 - 读清(写操作时此参数无效) */
    u32 data[DFX_MAG_MAX_REG_NUM]; /**< 返回的数据，最多DFX_MAG_MAX_REG_NUM个，根据reg_cnt来标明实际有效数据个数 */
};

#define UPDATE_CMD_HEAD_NEW_VERSION 0x1  // 工具下发的重复包命令版本

/**< 固件升级错误码定义 */
enum hinic5_update_fw_err_code {
    MPU_FW_UPDATE_OK = 0x00,
    MPU_FW_UPDATE_START = 0x01,
    MPU_FW_UPDATE_READ_FLASH_ERR = 0x02,
    MPU_FW_UPDATE_WRITE_FLASH_ERR = 0x03,
    MPU_FW_UPDATE_OTHER_FAIL = 0x04,
    MPU_FW_UPDATE_BUSY = 0x05,
    MPU_FW_UPDATE_OTHER_OPERAT = 0x06,
    MPU_FW_UPDATE_PARA_CHECK_ERR = 0x07,
    MPU_FW_UPDATE_DUPLICATE_SUBFW = 0x08,
    MPU_FW_UPDATE_FW_CRC_ERR = 0x09,
    MPU_FW_UPDATE_FW_VERIFY_ERR = 0x0a,
    MPU_FW_UPDATE_SUBFW_PARTIAL = 0x0b,
    MPU_FW_UPDATE_REFRESH_STATE_MACHINE_FAIL = 0x0c,
    MPU_FW_UPDATE_BOARD_TYPE_CHECK_FAIL = 0x0d,
    MPU_FW_UPDATE_PERMISSION_DENINED = 0x0e,
    MPU_FW_UPDATE_ALREADY_ACTIVED = 0x0f,
    MPU_FW_UPDATE_ALLOC_MEM_FAIL = 0x10,
    MPU_FW_UPDATE_CHECK_VERSION_FAIL = 0x11,
    MPU_FW_UPDATE_STATE_MACHINE_INVALID = 0x12,
    MPU_FW_UPDATE_INSTALL_PATCH_FAIL = 0x13,
    MPU_FW_UPDATE_UNINSTALL_PATCH_FAIL = 0x14,
    MPU_FW_UPDATE_ACTIVE_PATCH_FAIL = 0x15,
    MPU_FW_UPDATE_DEACTIVE_PATCH_FAIL = 0x16,
    MPU_FW_UPDATE_GIT_TAG_MISMATCH = 0x17,
    MPU_FW_UPDATE_ADD_SYMBOL_FAIL = 0x18,
    MPU_FW_UPDATE_PATCH_REPLACE_EXCLUSIVE = 0x19,
    MPU_FW_UPDATE_PAUSE_TASKS_FAIL = 0x1a,
    MPU_FW_UPDATE_FLR_IS_RUNNING = 0x1b,
    MPU_FW_UPDATE_RESET_CORE_FAIL = 0x1c,
    MPU_FW_UPDATE_INIT_FAIL = 0x1d,
    MPU_FW_UPDATE_HOT_UPDATE_NOT_SUPPORT = 0x1e,
    MPU_FW_UPDATE_FLUSH_FLASH_REPEAT = 0xfb,
    MPU_FW_UPDATE_OTHER_HOST_RST_SPI_BUSY = 0xfc,
    MPU_FW_UPDATE_HOT_ACTIVE_INVALID = 0xfd,
    MPU_FW_UPDATE_HOT_ACTIVE_FAIL = 0xfe,
};

struct fw_update_msg_st {   /**<  */
    struct mgmt_msg_head msg_head;   /**< mbox消息头 */
    struct {
        u32 SL : 1;           /**< 尾片 */
        u32 SF : 1;           /**< 首片 */
        u32 Flag : 1;   /**< 分区标识 */
        u32 Signed : 1;   /**< 签名标识 */
        u32 Repeat : 1;   /**< 分片操作flash */
        u32 updatefw_main_area_flag : 1;   /**< 主区标识 */
        u32 Reserved : 10;   /**< 保留域段 */
        u32 Fragment_Len : 16;    /**分片长度<  */
    } ctl_info;   /**<  */

    struct {
        u32 FW_section_CRC;   /**< 分片操作flash */
        u32 FW_section_type;   /**< 子固件类型 */
    } section_info;   /**< 子固件信息 */

    u32 total_len;            /**< 镜像长度 */
    u32 setion_total_len;   /**< 子固件长度 */
    u32 fw_section_version;   /**< 子固件版本 */
    u32 section_offset;   /**< 子固件偏移 */
    u32 data[384];   /**< 镜像数据 */
};

/* 热激活类型 */
typedef enum {
    HOT_ACTIVE_NONE = 0,
    HOT_ACTIVE_MPU = 1,
    HOT_ACTIVE_NPU = 2,
    HOT_ACTIVE_MNPU = 3,
    HOT_ACTIVE_SCC = 4,
} hot_active_type_e;

/* MPU热激活类型 */
typedef enum {
    MPU_HOT_ACTIVE_NONE,  /**< 还没有进行热升级 */
    MPU_HOT_ACTIVE_PATCH,  /**< 热补丁 */
    MPU_HOT_ACTIVE_REPLACE,  /**< 热替换 */
} mpu_hot_active_type_e;

struct cmd_hot_active_fw {   /**< 热升级激活 */
    struct mgmt_msg_head head;     /**< mbox消息头 */
    u8 type;                      /**< 激活子固件类型，1: mpu; 2: ucode; 3: mpu & npu */
    u8 mpu_hot_active_type;     /**< MPU热激活类型，当type为MPU或MNPU时有效 */
    u8 data[6];                  /**< 保留域段 */
};

struct cmd_bat_set_info {   /**< 热升级bat表项操作过渡信息 */
    struct mgmt_msg_head head;   /**< mbox消息头 */

    u16 func_id;   /**< 指定function id */
    u8  smf_id;   /**< smf idx */
    u8  rsvd1;   /**< 保留域段 */
    u32 bat_offset;   /**< bat偏移 */
    u32 data_size;   /**< 数据大小 */
    u8  data[256];   /**< payload */
};

/* 读取dbf信息 */
typedef struct {
    u32 device_id;
    u32 vendor_id;
} mpu_pcie_pf_info_s;

typedef struct {
    struct mgmt_msg_head head;

    mpu_pcie_pf_info_s pf_info[32];
    u32 bus_id;
    u32 pf_num;
} mpu_pcie_dev_bdf_info_s;

typedef struct {
    struct mgmt_msg_head head;
    u8 valid;  /* 1: valid */
    u8 host_id;  /* 容器归宿host，范围0 ~ 3 */
    u8 rsvd[2];
} comm_cmd_con_sel_sta;

typedef struct pf_bdf_info {
    u8 itf_idx;
    u16 bdf;
    u8 pf_bdf_info_vld;
} comm_pf_bdf_info_s;

typedef struct vf_bdf_info {
    u16 glb_pf_vf_offset; /* global_func_id offset of 1st vf in pf */
    u16 max_vfs; /* vf number */
    u16 vf_stride; /* VF_RID_SETTING.vf_stride */
    u16 vf_offset; /* VF_RID_SETTING.vf_offset */
    u8 bus_num; /* tl_cfg_bus_num */
    u8 rsv[3];
} comm_vf_bdf_info_s;

struct comm_cmd_get_bdf_info_s {
    struct mgmt_msg_head head;
    comm_pf_bdf_info_s  pf_bdf_info[PCIE_MODE_PF_NUM];
    comm_vf_bdf_info_s  vf_bdf_info[PCIE_MODE_PF_NUM];
    u32 vf_num; /**< vf num */
};

typedef struct comm_virtio_dev_cmd {
    u16 device_type;
    u16 device_id;
    u32 devid_switch;
    u32 sub_vendor_id;
    u32 sub_class_code;
    u32 flash_en;
} comm_virtio_dev_cmd_s;

typedef struct comm_virtio_dev_ctl {
    u32 device_type_mark;
    u32 devid_switch_mark;
    u32 sub_vendor_id_mark;
    u32 sub_class_code_mark;
    u32 flash_en_mark;
} comm_virtio_dev_ctl_s;

struct comm_cmd_set_virtio_dev {
    struct mgmt_msg_head head;
    comm_virtio_dev_cmd_s virtio_dev_cmd;
    comm_virtio_dev_ctl_s virtio_dev_ctl;
};

#define PSM_GIT_CHAR_NUM (20)
struct cmd_get_mpu_git_code {
    struct mgmt_msg_head head; /* 8B */
    u8 rsvd[3];                   /* 保留 */
    u8 psm_en;
    char mpu_git_code[64];      /* git 号和编译时间长60个字符 */
    char psm_git_code[PSM_GIT_CHAR_NUM + 1];      /* psm git 号和编译时间长20个字符 */
    u8 rsvd1[3];                /* 保留 */
};

/* 关闭芯片自复位 */
struct comm_cmd_enable_auto_rst_chip {
    struct mgmt_msg_head head;

    u8 op_code; /* 0: get  1: set */
    u8 enable; /* 1: 使能自动复位芯片; 0: 禁止自动复位芯片 */
    u8 rsvd[2];
};

/* 芯片核温结构体定义 */
struct comm_temp_in_info {
    struct mgmt_msg_head head; /* 8B */
    u8 opt_type;                /* 0:read operation 1:cfg operation */
    u8 rsv[3];
    s32 max_temp; /* 芯片核温阈值 */
    s32 min_temp; /* 芯片核温阈值 */
};

struct comm_temp_out_info {
    struct mgmt_msg_head head; /* 8B */
    s32 temp_data;             /* 读出的温度 */
    s32 max_temp_threshold;    /* 芯片核温阈值 */
    s32 min_temp_threshold;    /* 芯片核温阈值 */
    s32 max_temp;              /* 芯片核温历史最大值 */
    s32 min_temp;              /* 芯片核温历史最小值 */
};

/* chip id信息 */
struct comm_chip_id_info {
    struct mgmt_msg_head head;

    u8 chip_id;
    u8 rsvd[3];
};

/* die id模块接口 */
struct comm_cmd_get_die_id {
    struct mgmt_msg_head head;

    u32 die_id_data[8];
};

typedef struct {
    struct mgmt_msg_head head;

    u8 lldp_tx_enable;
    u8 port;
    u8 rsv[2];
} comm_cmd_lldp_tx_set_s;

#define MSIX_INFO_LEN 0x200
struct comm_cmd_msix_info {
    struct mgmt_msg_head head;

    u8 rsvd1;
    u8 flag;  /* 0-second map, 1-actual map, 2-first map entry */
    u16 function_id;
};

enum log_status_operation_type {
    READ_TYPE = 0,
    WRITE_TYPE,
};

enum log_status_type {
    LOG_NORMAL = 0,
    LOG_BUSY,
};
struct mpu_log_status_info {
    struct mgmt_msg_head head;
    u8 type;       /* 0: 读 1：写 */
    u8 log_status; /* 0:空闲 1:忙碌 */
    u8 rsvd[2];
};

#define RQ_CXT_SIZE 64
#define SQ_CXT_SIZE 64
#define CMDQ_COUNT 2
#define CMDQ_CXT_SIZE 16
#define ENHANCE_CMDQ_CXT_SIZE 48
#define ENHANCE_CMDQ_CXT_SIZE_FRIST 16
#define ENHANCE_CMDQ_CXT_SIZE_SECOND 32

typedef struct {
    struct mgmt_msg_head head;
    u32 func_id;
    u32 smf_id;
    u32 queue_id;
    u32 smf_id_valid;            // 传入smf_id是否有效
} comm_cmd_root_ctx_load_req_s;

typedef struct {
    struct mgmt_msg_head head;
    u8 rq_ctx[RQ_CXT_SIZE];
    u8 sq_ctx[SQ_CXT_SIZE];
    u8 cmdq_ctx[CMDQ_CXT_SIZE * CMDQ_COUNT];
    u8 enhance_cmdq_ctx[ENHANCE_CMDQ_CXT_SIZE * CMDQ_COUNT];
} comm_cmd_root_ctx_load_ret_s;

struct cmd_query_fw {
    struct mgmt_msg_head head; // 8B
    u32 offset;                 // 偏移，因为返回的信息比较大，因此，需要多次返回
    u32 len;                    // 要读的数据长度
};

#define MAX_CMD_DATA_LEN (1024 + 512)
struct cmd_fw_info {
    struct mgmt_msg_head head; // 8B
    u32 len;                    // 返回实际读到的数据长度
    u8 data[MAX_CMD_DATA_LEN];  // 一次最多读1536字节数据
};

typedef struct {
    u32 tgt_speed;
    u32 cur_speed;
    u32 tgt_width;
    u32 cur_width;
} pcie_link_info_s;

typedef struct {
    u32 pf_start;
    u32 pf_end;
    u32 pf_num;
    u32 vf_start;
    u32 vf_end;
    u32 vf_num;
} pcie_pf_vf_info_s;

typedef struct {
    u32 p_tx_left_tag;
    u32 np_tx_left_tag;
    u32 cpl_tx_left_Tag;
    u32 p_rx_left_tag;
    u32 np_rx_left_tag;
    u32 cpl_rx_left_tag;
} pcie_dfx_info_s;

typedef struct {
    u32 host_idx;
    u32 core_id;
    u32 port_id;
    pcie_link_info_s link_info;
    pcie_pf_vf_info_s pf_vf_info;
    pcie_dfx_info_s dfx_info;
} pcie_topo_item_s;

#define CMD_PCIE_MAX_HOST_IDX 0xD
typedef struct  {
    struct mgmt_msg_head head;
    u32 cur_host;
    u32 host_cnt;
    pcie_topo_item_s item[CMD_PCIE_MAX_HOST_IDX + 1];
} comm_cmd_get_pcie_topo_s;

typedef struct  {
    struct mgmt_msg_head head;
    u32 type;
    u32 condition1;
    u32 condition2;
    u32 opc;
    u64 data;
    u32 ret;
} comm_cmd_pcie_option_s;

#define MAX_TYPE_NAME_LEN 8

/* VF mapping flags for mqm vf_map type */
#define VF_MAP_FLAG_FUNC_ID_SET 0x01
#define VF_MAP_FLAG_VNIC_ID_SET 0x02
#define VF_MAP_FLAG_VNIC_GRP_ID_SET 0x04

struct cmd_mpu_set_shaper {
    struct mgmt_msg_head head; /* 8B */
    char option[MAX_TYPE_NAME_LEN];
    char module_name[MAX_TYPE_NAME_LEN];
    char shaper_mod[MAX_TYPE_NAME_LEN];
    char type_name[MAX_TYPE_NAME_LEN];
    u8 pqm_mod;
    u8 port_id;
    u8 tc_id;
    u8 cos_id;
    u8 mqm_type;
    u8 rsvd[3];
    u32 mqm_shaper_id;
    u32 vnic_vnic_grp_id;
    u32 cir;
    u32 cbs;
    u32 pir;
    u32 pbs;
    u32 xir;
    u32 xbs;
    u32 func_id;
    u32 vnic_id;
    u32 vnic_group_id;
    u32 vf_map_flags;
    u32 rsvds[16];
};

#define VF_SQ_RQ_MAX_NUM 128
#define PF_SQ_RQ_MAX_NUM 256
#define CFG_VF_MAX_NUM 63
#define CFG_VF_TOTAL_NUM 126
#define CFG_MSIX_MAX_NUM 3072 /* NIC QP + AEQ = 3K, NOT CONTAIN RoCE */
#define CFG_PF_MAX_NUM 16
#define CFG_INVALID_VALUE 0xFFFF
#define PF_VF_TOTAL_QUEUE_MAX_NUM 1744

#define CFG_BAR_INDEX0 0
#define CFG_BAR_INDEX1 1
#define CFG_BAR_INDEX2 2
#define CFG_BAR_INDEX3 3
#define CFG_BAR_INDEX4 4
#define CFG_BAR_INDEX5 5

#define PF_TYPE 0
#define VF_TYPE 1
#define CFG_BAR_INDEX_NUM 6
#define CFG_BAR_SIZE_MIN_NUM   4
#define CFG_PF_BAR_SIZE_MAX_NUM   64
#define CFG_PF_BAR3_SIZE_MAX_NUM   128
#define CFG_VF_BAR_SIZE_MAX_NUM   64
#define CFG_VF_BAR4_SIZE_MAX_NUM 4096
#define CFG_BAR_SIZE_INVALID_VALUE 0xFFFFFFFF
#define CFG_BAR_TRANSLATE_KB_TO_BYTE(bar_size) (((bar_size) * 1024) - 1)
#define CFG_BAR_TRANSLATE_BYTE_TO_KB(bar_size) (((bar_size) + 1) / 1024)

#define CFG_DATA_OP_GET 0
#define CFG_DATA_OP_SET 1
#define CFG_DATA_OP_CLEAR 2
#define CFG_DATA_OP_BAR_GET 3
#define CFG_DATA_OP_BAR_SET 4
#define CFG_DATA_OP_BAR_CLEAR 5
#define QUEUE_BIT_PF_SQ_RQ 0
#define QUEUE_BIT_VF_SQ_RQ 1
#define QUEUE_BIT_VF_NUM 2

#define CFG_BAR_MODE_TEMP 0
#define FLASH_BAR_MODE_TEMP 0xFFFFFFFF
#define CLEAN_BAR_REBOOT_TWICE (FLASH_BAR_MODE_TEMP - 2)
#define CFG_BAR_MODE_PERM 1
#define FLASH_BAR_MODE_PERM 0x1

#define CFG_BIT(x) (0x1U << (x))
#define CFG_GET_BIT(val, bit) (((val) >> (bit)) & 0x1)
#define CFG_SET_BIT(val, bit) ((val) |= CFG_BIT(bit))
#define CFG_CLEAR_BIT(val, bit) ((val) &= ~CFG_BIT(bit))

typedef struct mpu_nic_func_queue_s {
    u32 magic_func_sq_rq_queue;
    u16 pf_sq_rq;
    u16 vf_sq_rq;
    u16 vf_num;
    u16 rsvd0;
} mpu_nic_func_queue;

typedef struct mpu_nic_bar_s {
    u32 magic_bar_set;
    u8 pf_bar_index;    /**< 0-3 */
    u8 vf_bar_index;    /**< 0,2,4 */
    u8 pf_bar_set_flag;
    u8 vf_bar_set_flag;
    u32 bar_mode;       /**< temp: 0 in tool, 0xFFFFFFFF in flash ; permanently: 1 in tool, 0x1 in flash */
    u32 pf_bar_size[CFG_BAR_INDEX_NUM];
    u32 vf_bar_size[CFG_BAR_INDEX_NUM];
} mpu_nic_bar;

struct comm_cmd_cfg_data {
    struct mgmt_msg_head head;
    u8 opt_type; /**< operation type 0: query 1: set 2: clear 4: bar set 5: bar clear */
    u8 pf_index;
    u8 queue_bitmap; /**< 0: pf_sq_rq 1: vf_sq_rq 2: vf_num */
    u8 pf_num;
    u16 pf_sq_rq;
    u16 vf_sq_rq;
    u16 vf_num;
    u16 total_queue_num;
    u8 is_set_diff_template;
    u8 rsvd0[3];
    u32 rsvd1[210];
    mpu_nic_bar bar_info_current;
    mpu_nic_bar bar_info_default;
    mpu_nic_bar bar_info;
    mpu_nic_func_queue cur_func_queue[CFG_PF_MAX_NUM];
    mpu_nic_func_queue next_func_queue[CFG_PF_MAX_NUM];
};

enum voltage_type_e {
    VOLTAGE_TYPE_VRD,
    VOLTAGE_TYPE_VSENSOR,
};
typedef struct comm_cmd_voltage_info_s {
    struct mgmt_msg_head head;
    u8 type; // 参考voltage_type_e
    u8 rsv[3];
    u16 vol_integer;
    u16 vol_decimal;
} comm_cmd_voltage_info;

typedef enum {
    RTOS_INFO_TYPE_TASK_INFO,
    RTOS_INFO_TYPE_CPU_PER,
    RTOS_INFO_TYPE_VER_INFO,
    RTOS_INFO_TYPE_HWI_INFO,
    RTOS_INFO_TYPE_SEM_INFO,
    RTOS_INFO_TYPE_BUTT,
} rtos_info_type_e;

typedef struct {
    struct mgmt_msg_head head;
    u8 type;
    u8 rsv[3];
    u32 para;
} cmd_query_rtos_info;

#define MAX_RTOS_INFO_LEN (2000)
typedef struct {
    struct mgmt_msg_head head;
    u8 data[MAX_RTOS_INFO_LEN];
} cmd_rtos_info;

#define MAX_RTOS_ID_NUM 200
typedef struct {
    u16 num;
    u16 rsv;
    u32 id[MAX_RTOS_ID_NUM];
} rtos_list_info;

#define RTOS_TASK_NAME_LEN 16
typedef struct {
    u32 task_pid;
    char name[RTOS_TASK_NAME_LEN];
    u16 status;
    u16 prio;
    u64 pc;
    u64 sp;
    u32 sem_id;
} rtos_task_info;

typedef struct {
    u32 pid;
    u16 usage;
    u16 rsv;
    char name[RTOS_TASK_NAME_LEN];
} thread_cpup_info;

#define RTOS_MAX_THREAD_CPUP_NUM 50
typedef struct {
    u32 core_id;
    u32 cpup;
} core_cpup_info;

#define RTOS_MAX_CORE_NUM 4
typedef struct {
    u32 core_num;
    u32 thread_num;
    core_cpup_info cpup_info[RTOS_MAX_CORE_NUM];
    thread_cpup_info thread_info[RTOS_MAX_THREAD_CPUP_NUM];
} rtos_cpup_info;

#define RTOS_VER_INFO_LEN 1024
typedef struct {
    char version[RTOS_VER_INFO_LEN];
} rtos_ver_info;

typedef struct {
    u32 no;
    u16 type;
    u16 prio;
    u64 para;
} hwi_info;

#define RTOS_MAX_HWI_NUM 100
typedef struct {
    u32 num;
    hwi_info info[RTOS_MAX_HWI_NUM];
} rtos_hwi_info;

/*
 * @ingroup OS_sem
 * 信号量类型。
 */
typedef enum {
    RTOS_SEM_TYPE_COUNT, /* 计数型信号量 */
    RTOS_SEM_TYPE_BIN, /* 二进制信号量 */
    RTOS_SEM_TYPE_BUTT
} rtos_sem_type_e;

/*
 * @ingroup OS_sem
 * 信号量模块被阻塞线程唤醒方式。
 */
typedef enum {
    RTOS_SEM_MODE_FIFO,  // 信号量FIFO唤醒模式
    RTOS_SEM_MODE_PRIOR, // 信号量优先级唤醒模式
    RTOS_SEM_MODE_BUTT   // 信号量非法唤醒方式
} rtos_sem_mode_e;

typedef struct {
    u32 count;
    u32 owner;
    u16 sem_id;
    u8 mode; // 信号量唤醒模式
    u8 type;
} sem_info;

#define RTOS_MAX_SEM_NUM 128
typedef struct {
    u32 num;
    sem_info info[RTOS_MAX_SEM_NUM];
} rtos_sem_info;

#define FW_RESTORE_ENABLE 1
#define FW_RESTORE_DISABLE 0
#define FW_RESTORE_SET_MAX_NUM (200)
#define FW_RESTORE_INSTALL_SDK_MAX_TIMEOUT 30
#define FW_RESTORE_INSTALL_SDK_MIN_TIMEOUT 1
#define FW_RESTORE_MAX_FAIL_COUNT 20
#define FW_RESTORE_MIN_FAIL_COUNT 1

#define SWITCH_RESET_OPT_READ 0
#define SWITCH_RESET_OPT_WRITE 1

typedef enum {
    RESTORE_SET_TYPE_SWITCH = 0,  // 启动切区标志位
    RESTORE_SET_TYPE_RESET, // 切区后自动复位标志位
    RESTORE_SET_TYPE_FAIL_COUNT, // 设置检测SDK加载失败次数
    RESTORE_SET_TYPE_TIMEOUT, // 设置SDK加载超时时间
    RESTORE_SET_TYPE_BUTT
} fw_restore_set_e;

struct cmd_chip_switch_reset {
    struct mgmt_msg_head head;
    u8 op_code; /* 0: set  1: get */
    u8 type; /* fw_restore_set_e, 读时复用作为fail_count返回值 */
    u8 value; /* 待设置的值, 读时复用作为switch&reset返回值 */
    u8 read_value; /* 只在读时使用，为timeout返回值 */
};

/* integrity */
#define GRAY_INFO_MAGIC_NUM 0xc380f8dd
#define HASH_SIG_SIZE 512
#define KEY_HASH_SIZE 32
#define PUBKEY_SIZE 1024
#define INTEGERITY_VERIFY_ENABLE 0x5A /* 使用魔法数字标识完整性验证开启 */
typedef enum {
    INTEGERITY_CMD_ENABLE = 0,
    INTEGERITY_CMD_UPDATE,
    INTEGERITY_CMD_DISABLE,
    INTEGERITY_CMD_MAX,
} integrity_cmd_type;

typedef struct {
    u8 integrity_type;          /* 用户固件完整性保护开关 */
    u8 rsvd[3];                 /* 预留字段*/
    u8 key_hash[KEY_HASH_SIZE]; /* 客户根公钥哈希值 */
    u8 pubkey[PUBKEY_SIZE];
    u8 keysig[HASH_SIG_SIZE];
    u8 newpubkey[PUBKEY_SIZE];
    u8 newkeysig[HASH_SIG_SIZE]; /* 更新操作用到的签名文件 */
} cskey_status;

typedef struct {
    u32 magic_num;
    cskey_status key_hash_sign;
    u32 crc;
} gray_card_info_s;
/* integrity */

/* update err code */
typedef enum MPU_INTEGRITY_STATUS {
    MPU_INTEGRITY_OK = 0,
    MPU_INTEGRITY_NOT_ENABLE = 101,
    MPU_INTEGRITY_IS_ENABLE,
    MPU_INTEGRITY_MEMCPY_FAIL,
    MPU_INTEGRITY_WRITE_FLASH_FAIL,
    MPU_INTEGRITY_SMU_VERIFY_FAIL,
} MPU_INTEGRITY_STATUS_ENUM;

#endif