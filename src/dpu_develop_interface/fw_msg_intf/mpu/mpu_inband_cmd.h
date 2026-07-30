/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
 * Filename      : mpu_inband_cmd.h
 * Version       : Initial Draft
 * Creation time : 2023/09/22
 * Last Modified :
 * Description   : In-band commands between the driver and the MPU
 */

#ifndef MPU_INBAND_CMD_H
#define MPU_INBAND_CMD_H

/**< COMM Commands between Driver to MPU */
enum hinic5_mgmt_cmd {
    /**< flr及资源清理相关命令 */
    COMM_MGMT_CMD_FUNC_RESET = 0,   /**< 驱动加卸载function reset清理资源 */
    COMM_MGMT_CMD_FEATURE_NEGO,   /**< fw与驱动兼容性属性协商 */
    COMM_MGMT_CMD_FLUSH_DOORBELL,   /**< 驱动卸载flush流程，清理资源 */
    COMM_MGMT_CMD_START_FLUSH,   /**< 驱动加卸载flush握手 */
    COMM_MGMT_CMD_SET_FUNC_FLR,   /**< 驱动主动通过mbox流程触发flr */
    COMM_MGMT_CMD_GET_GLOBAL_ATTR,   /**< 获取sm全局表 */
    COMM_MGMT_CMD_SET_PPF_FLR_TYPE,   /**< 设置ppf flr执行范围 */
    COMM_MGMT_CMD_SET_FUNC_SVC_USED_STATE,   /**< 设置对应function驱动是否加载/使用 */
    COMM_MGMT_CMD_GET_FUNC_FLR_INFO,   /**< 获取flr执行dfx信息 */

    /**< 驱动中断资源 */
    COMM_MGMT_CMD_CFG_MSIX_NUM = 10,   /**< 获取驱动msix中断信息 */

    /**< 驱动相关配置命令 */
    COMM_MGMT_CMD_SET_CMDQ_CTXT = 20,   /**< 设置cmdq cxt */
    COMM_MGMT_CMD_SET_VAT,   /**< 设置vat表 */
    COMM_MGMT_CMD_CFG_PAGESIZE,   /**< 配置root cxt 页面大小 */
    COMM_MGMT_CMD_CFG_MSIX_CTRL_REG,   /**< 配置中断msix */
    COMM_MGMT_CMD_SET_CEQ_CTRL_REG,   /**< 配置ceq */
    COMM_MGMT_CMD_SET_DMA_ATTR,   /**< 配置驱动dma属性 */
    COMM_MGMT_CMD_SET_ENHANCE_CMDQ_CTXT,   /**< 配置增强型cmdq */
    COMM_MGMT_CMD_GET_FUNC_SECURE_MEM,   /**< 获取安全内存标识 */
    COMM_MGMT_CMD_SET_FUNC_PLUG_SRV,   /**< 设置热拔插bmp */
    COMM_MGMT_CMD_GET_FUNC_PLUG_SRV,   /**< 获取热拔插bmp */
    COMM_MGMT_CMD_SET_PPF_TBL_HTR_FLG,   /**< 设置热替换标识 */
    COMM_MGMT_CMD_GET_FAST_MSG_CAP,   /**< 获取fast msg能力 */
    COMM_MGMT_CMD_SET_FAST_MSG_RQ_ADDR,   /**< 设置fast msg地址 */
    COMM_MGMT_CMD_CLEAR_FAST_MSG_SML,   /**< 清除fast msg表项 */

    /**< INFRA配置相关命令字 */
    COMM_MGMT_CMD_GET_MQM_FIX_INFO = 40,   /**< mqm获取chunk num */
    COMM_MGMT_CMD_SET_MQM_CFG_INFO,   /**< 从驱动接收ppf和page_size */
    COMM_MGMT_CMD_SET_MQM_SRCH_GPA,   /**< 从驱动接收search gpa地址 */
    COMM_MGMT_CMD_SET_PPF_TMR,   /**< 配置smf timer */
    COMM_MGMT_CMD_SET_HT_GPA,   /**< 设置 ht gpa (bank gpa) 地址 */
    COMM_MGMT_CMD_SET_FUNC_TMR_BITMAT,   /**< 使能smf timer */
    COMM_MGMT_CMD_SET_MBX_CRDT,   /**< 设置mbox信用，dfx性能调优用 */
    COMM_MGMT_CMD_CFG_TEMPLATE,   /**< 设置/获取温度告警阈值 */
    COMM_MGMT_CMD_SET_MQM_LIMIT,   /**< 设置/获取mqm限速配置，1823V100盘古驱动使用. */
    COMM_MGMT_CMD_SET_BAT_INFO,   /**< 设置bat信息 */
    COMM_MGMT_CMD_SET_VIO_EN, /**< 设置cpi MSI使能*/
    COMM_MGMT_CMD_CFG_DATA, /**< 配置模板中的Function相关参数 */

    /**< 信息获取相关命令字 */
    COMM_MGMT_CMD_GET_FW_VERSION = 60,   /**< 获取fw版本信息 */
    COMM_MGMT_CMD_GET_BOARD_INFO,   /**< 获取单板信息 */
    COMM_MGMT_CMD_SYNC_TIME,   /**< 同步驱动时间戳 */
    COMM_MGMT_CMD_GET_HW_PF_INFOS,   /**< 获取硬件pf信息 */
    COMM_MGMT_CMD_SEND_BDF_INFO,   /**< 接收驱动获取到的dbf信息 */
    COMM_MGMT_CMD_GET_VIRTIO_BDF_INFO,   /**< virtio场景获取dbf信息 */
    COMM_MGMT_CMD_GET_SML_TABLE_INFO,   /**< 获取smlb表项信息 */
    COMM_MGMT_CMD_GET_SDI_INFO,   /**< 获取sdI信息(裸机/虚机) */
    COMM_MGMT_CMD_ROOT_CTX_LOAD,   /**< 获取root cxt信息 */
    COMM_MGMT_CMD_GET_HW_BOND = 69, /* 1823V100 */
    COMM_MGMT_CMD_MPU_AND_NPU_VER = 70, /* 1823V100 */
    COMM_MGMT_CMD_GET_PF_BY_FUNC = 71, /* 1823V100 */
    COMM_MGMT_CMD_GET_PF_BUS_BY_DEV = 72, /* 1823V100 */

    /**< 升级相关命令字 */
    COMM_MGMT_CMD_UPDATE_FW = 80,   /**< fw升级 */
    COMM_MGMT_CMD_ACTIVE_FW,   /**< fw冷激活 */
    COMM_MGMT_CMD_HOT_ACTIVE_FW,   /**< fw热激活 */
    COMM_MGMT_CMD_HOT_ACTIVE_DONE_NOTICE,   /**< fw热激活完成(当前未使用) */
    COMM_MGMT_CMD_SWITCH_CFG,   /**< 配置文件切换(当前未使用) */
    COMM_MGMT_CMD_CHECK_FLASH,   /**< 存储场景flash静默检测 */
    COMM_MGMT_CMD_CHECK_FLASH_RW,   /**< 存储场景升级前 */
    COMM_MGMT_CMD_RESOURCE_CFG,   /**< 原配置模板(当前未使用) */
    COMM_MGMT_CMD_UPDATE_BIOS,   /**< bios升级命令(已废弃，bios升级和网卡升级合并) */
    COMM_MGMT_CMD_MPU_GIT_CODE,   /**< 获取版本git号 */
    COMM_MGMT_QUERY_MODULE_IMAGES,   /**< 镜像握手，获取固件信息 */
    COMM_MGMT_CMD_UPDATE_CUSTOM_FW = 98,  /* 升级CUSTOM_FW固件，不可修改该值 */
    COMM_MGMT_CMD_ACTIVE_CUSTOM_FW = 99,  /* 激活CUSTOM_FW固件 */

    /**< chip reset相关 */
    COMM_MGMT_CMD_FAULT_REPORT = 100,   /**< mpu发往驱动信息，异常告警 */
    COMM_MGMT_CMD_WATCHDOG_INFO,   /**< mpu发往驱动信息，看门狗告警 */
    COMM_MGMT_CMD_MGMT_RESET,   /**< mpu发往驱动信息，mpu LastWord */
    COMM_MGMT_CMD_FFM_SET,    /**< mpu发往驱动消息，异常中断显示信息 */

    /**< chip info/log 相关 */
    COMM_MGMT_CMD_GET_LOG = 120,   /**< 获取固件log */
    COMM_MGMT_CMD_TEMP_OP,   /**< 获取芯片温度 */
    COMM_MGMT_CMD_EN_AUTO_RST_CHIP,   /**< 使能芯片跟随perst复位自动复位 */
    COMM_MGMT_CMD_CFG_REG,   /**< 配置芯片寄存器(当前未使用) */
    COMM_MGMT_CMD_GET_CHIP_ID,   /**< 获取芯片id(多芯片场景) */
    COMM_MGMT_CMD_SYSINFO_DFX,   /**< 获取芯片软件系统dfx信息 */
    COMM_MGMT_CMD_PCIE_DFX_NTC,   /**< 通知驱动收集pcie dfx信息 */
    COMM_MGMT_CMD_DICT_LOG_STATUS,    /**< 获取log收集状态 */
    COMM_MGMT_CMD_MSIX_INFO,   /**< 配置msix信息 */
    COMM_MGMT_CMD_CHANNEL_DETECT,   /**< mbox通道探测 */
    COMM_MGMT_CMD_DICT_COUNTER_STATUS,   /**< 获取flash counter计数 */
    COMM_MGMT_CMD_UCODE_SM_COUNTER,   /**< 获取sm counter计数 */
    COMM_MGMT_CMD_CLEAR_LOG = 132, /* 1823V100 */
    COMM_MGMT_CMD_UCODE_SM_COUNTER_PER = 133, /* 1823V100 */

    /**< switch workmode 相关 */
    COMM_MGMT_CMD_CHECK_IF_SWITCH_WORKMODE = 140,   /**< 配置切换(卡内多份配置文件，已废弃) */
    COMM_MGMT_CMD_SWITCH_WORKMODE,   /**< 切换工作模式(已废弃) */

    /**< mpu 相关 */
    COMM_MGMT_CMD_MIGRATE_DFX_HPA = 150,   /**< 热迁移hpa dfx */
    COMM_MGMT_CMD_BDF_INFO,   /**< 获取pcie bdf号 */
    COMM_MGMT_CMD_NCSI_CFG_INFO_GET_PROC,   /**< 获取ncsi配置信息 */
    COMM_MGMT_CMD_CPI_TCAM_DBG,   /**< cpi tcam信息调试(当前暂未使用) */
    COMM_MGMT_CMD_LLDP_TX_FUNC_SET_PROC,   /**< lldp使能 */
    COMM_MGMT_CMD_FUNC_ENABLE_INFO = 155, /* 1823V100 */
    COMM_MGMT_CMD_FUNC_VIRTIO_INFO = 156, /* 1823V100 */
    COMM_MGMT_CMD_NCSI_LOW_POWER_PROC,   /* 1872V100 NCSI 低功耗使能 */

    /**< rsvd0 section */
    COMM_MGMT_CMD_SECTION_RSVD_0 = 160,
    COMM_MGMT_CMD_SWITCH_RESET_CFG,

    /**< rsvd1 section */
    COMM_MGMT_CMD_SECTION_RSVD_1 = 170,

    /**< rsvd2 section */
    COMM_MGMT_CMD_SECTION_RSVD_2 = 180,

    /**< rsvd3 section */
    COMM_MGMT_CMD_SECTION_RSVD_3 = 190,
    COMM_MGMT_CMD_GET_INDIR_TABLE,   /**< tool read indirect table */
    COMM_MGMT_CMD_SET_INDIR_TABLE,   /**< tool write indirect table */

    /**< move to DFT mode */
    COMM_MGMT_CMD_GET_TDIE_ID = 199,   /**< 获取totem die id */
    COMM_MGMT_CMD_GET_UDIE_ID = 200,   /**< 获取unic die id */
    COMM_MGMT_CMD_GET_EFUSE_TEST,   /**< efuse测试(当前暂未使用) */
    COMM_MGMT_CMD_EFUSE_INFO_CFG,   /**< 烧写efuse信息 */
    COMM_MGMT_CMD_GPIO_CTL,   /**< gpio测试(当前暂未使用) */
    COMM_MGMT_CMD_HI30_SERLOOP_START,    /**< hi30开始环回 */
    COMM_MGMT_CMD_HI30_SERLOOP_STOP,      /**< hi30停止环回 */
    COMM_MGMT_CMD_HI30_MBIST_SET_FLAG,     /**< hi30 bist测试(当前暂未使用) */
    COMM_MGMT_CMD_HI30_MBIST_GET_RESULT,   /**< hi30 获取bist测试结果(当前暂未使用) */
    COMM_MGMT_CMD_ECC_TEST,   /**< 芯片ecc测试 */
    COMM_MGMT_CMD_FUNC_BIST_TEST,    /**< 芯片function bist测试 */

    COMM_MGMT_CMD_VPD_SET = 210,   /**< 写vpd信息到flash */
    COMM_MGMT_CMD_VPD_GET,   /**< 工具读取flash vpd信息 */

    COMM_MGMT_CMD_ERASE_FLASH,   /**< 擦除flash(dfx版本开启) */
    COMM_MGMT_CMD_QUERY_FW_INFO,   /**< 查询固件状态机信息 */
    COMM_MGMT_CMD_GET_CFG_INFO,   /**< 获取配置信息(功能暂未开启) */
    COMM_MGMT_CMD_GET_UART_LOG,   /**< 串口重定向输出 */
    COMM_MGMT_CMD_SET_UART_CMD,   /**< 串口重定向输入 */
    COMM_MGMT_CMD_SPI_TEST,   /**< spi测试(dfx版本开启) */

    COMM_MGMT_CMD_HEART_EVENT,      /**< 心跳检测(当前暂未使用) */
    COMM_MGMT_CMD_NCSI_OEM_GET_DRV_INFO,   /**< 发往驱动的消息，获取bdf号(方案已替换) */
    COMM_MGMT_CMD_LASTWORD_GET,   /**< 发往驱动的消息，mpu临终遗言 */
    COMM_MGMT_CMD_READ_BIN_DATA,    /**< 获取固件bin数据(已废弃) */
    COMM_MGMT_CMD_GET_REG_VAL,   /**< 读取寄存器(dfx版本开启) */
    COMM_MGMT_CMD_SET_REG_VAL,   /**< 写寄存器(dfx版本开启) */

    /**< COMM_MGMT_CMD_WWPN_GET, TBD: move to FC? */
    /**< COMM_MGMT_CMD_WWPN_SET, TBD: move to FC? 229 */

    /**< check if needed */
    COMM_MGMT_CMD_SET_VIRTIO_DEV = 230,   /**< 设置vitro设备类型 */
    COMM_MGMT_CMD_SET_MAC,   /**< 设置固化mac */
    COMM_MGMT_CMD_LOAD_PATCH,   /**< load mpu patch(已废弃) */
    COMM_MGMT_CMD_REMOVE_PATCH,   /**< 移除mpu patch(已废弃) */
    COMM_MGMT_CMD_PATCH_ACTIVE,   /**< 激活mpu patch(已废弃) */
    COMM_MGMT_CMD_PATCH_DEACTIVE,   /**< 去激活mpu patch(已废弃) */
    COMM_MGMT_CMD_PATCH_SRAM_OPTIMIZE,   /**< pathc空间刷新(已废弃) */
    COMM_MGMT_CMD_CONTAINER_HOST_PROC,   /**< 存储容器场景，设置主host */
    COMM_MGMT_CMD_NCSI_COUNTER_PROC,   /**< 获取ncsi counter信息 */
    COMM_MGMT_CMD_CHANNEL_STATUS_CHECK,    /**< 存储场景，通道探测 */

    /**< 热补丁保留命令字 */
    COMM_MGMT_CMD_RSVD_0 = 240,   /**< 热补丁保留命令0 */
    COMM_MGMT_CMD_RSVD_1,   /**< 热补丁保留命令1 */
    COMM_MGMT_CMD_RSVD_2,   /**< 热补丁保留命令2 */
    COMM_MGMT_CMD_RSVD_3,   /**< 热补丁保留命令3 */
    COMM_MGMT_CMD_SECTION_INTEGRITY, /**客户固件完整性验证 */

    COMM_MGMT_CMD_SEND_API_ACK_BY_UP,   /**< 无效字段，版本收编删除，编译使用 */

    COMM_MGMT_CMD_GET_VER_COMPATIBLE_INFO = 254,   /**< for tool ver compatible info */

    /**< 注：添加cmd，不能修改已有命令字的值，请在前方rsv section中添加；原则上所有分支cmd表完全一致 */
    COMM_MGMT_CMD_MAX = 255,   /**<  */
};

#endif
