/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2022. All rights reserved.
 ******************************************************************************
 * @file hinic5_srv_nic.h
 * @details nic service interface
  History       :
  1.Date        : 2018/3/8
    Modification: Created file
***************************************************************************** */

#ifndef HINIC5_SRV_NIC_H
#define HINIC5_SRV_NIC_H

#include "nic_cfg_comm.h"
#include "drv_nic_api.h"
#if !defined(__UEFI__) && !defined(__WIN__)
#include <linux/netdevice.h>
#include "hinic5_lld.h"
#endif

/**
 * @brief struct hinic5_event_link_info 端口link事件信息
 * @details link事件上报后获取的端口link信息
 */
struct hinic5_event_link_info {
	u8 valid; /**< 结构体数据是否有效 */
	u8 port_type; /**< 端口类型 */
	u8 autoneg_cap; /**< 自动协商能力 */
	u8 autoneg_state; /**< 自动协商状态 */
	u8 duplex;  /**< 双工模式 */
	u8 speed; /**< 端口速率 */
};

enum link_err_type {
	LINK_ERR_MODULE_UNRECOGENIZED, /**< 未识别的模块错误类型 */
	LINK_ERR_NUM,
};

enum port_module_event_type {
	HINIC5_PORT_MODULE_CABLE_PLUGGED, /**< 端口电缆已插入的事件 */
	HINIC5_PORT_MODULE_CABLE_UNPLUGGED, /**< 端口电缆已拔出的事件 */
	HINIC5_PORT_MODULE_LINK_ERR, /**< 端口链路出现错误的事件 */
	HINIC5_PORT_MODULE_MAX_EVENT,
};

/**
 * @brief struct hinic5_port_module_event 端口事件信息
 * @details DCB事件上报的DCB信息
 */
struct hinic5_port_module_event {
	enum port_module_event_type type; /**< 端口电缆事件类型 */
	enum link_err_type err_type; /**< Link错误事件类型 */
};

/**
 * @brief struct hinic5_dcb_info DCB信息
 * @details DCB事件上报的DCB信息
 */
struct hinic5_dcb_info {
	u8 dcb_on; /**< DCB使能状态 */
	u8 default_cos; /**< 默认cos */
	u8 up_cos[NIC_DCB_COS_MAX]; /**< 优先级到cos映射 */
};

enum hinic5_nic_event_type {
	EVENT_NIC_LINK_DOWN, /**< Link down事件 */
	EVENT_NIC_LINK_UP, /**< Link up事件 */
	EVENT_NIC_PORT_MODULE_EVENT, /**< 线缆插拔事件 */
	EVENT_NIC_DCB_STATE_CHANGE,  /**< DCB状态变化事件 */
};

#if !defined(__UEFI__) && !defined(__VMWARE__)
/**
 * @brief 根据netdev获取lld_dev结构体指针
 *
 * @param netdev netdev结构体指针
 *
 * @details 根据netdev匹配查找查找lld_dev
 *
 * @attention: 该接口返回不会对lld_dev引用计数++,使用过程中如果lld_dev被释放可能导致访问野指针
 *
 * @return: 成功匹配到netdev的lld_dev时返回lld_dev结构体指针，否则返回NULL
 */
struct hinic5_lld_dev *hinic5_get_lld_dev_by_netdev(struct net_device *netdev);
#endif

/**
 * @brief 删除设备mac接口
 *
 * @param hwdev device pointer to hwdev
 * @param mac_addr mac地址
 * @param vlan_id vlan id 范围[0~4095]
 * @param func_id global function index
 * @param channel channel id，mailbox发送使用的channel id
 *
 * @details 删除对应function的mac地址
 *
 * @attention: 函数内部涉及发送mailbox消息会休眠，禁止中断上下文等不允许休眠的流程中调用
 *
 * @return: 删除MAC返回成功或者失败.
 *     @retval 0 成功
 *     @retval 非0 失败
 */
int hinic5_del_mac(void *hwdev, const u8 *mac_addr, u16 vlan_id, u16 func_id, u16 channel);

/**
 * @brief 获取设备DCB状态
 *
 * @param hwdev device pointer to hwdev
 * @param dcb_state：DCB状态信息
 *
 * @details 获取设备DCB状态
 *
 * @attention: NA
 * @return: DCB状态获取返回成功或者失败
 *     @retval 0 成功
 *     @retval 非0 失败
 */
int hinic5_get_dcb_state(void *hwdev, struct hinic5_dcb_state *dcb_state);

/**
 * @brief 获取PF DCB状态
 *
 * @param hwdev device pointer to hwdev
 * @param dcb_state：DCB状态信息
 *
 * @details VF通过mailbox信息发送给PF，获取PF DCB状态信息
 *
 * @attention: 仅VF支持，PF调用返回失败；函数内部涉及发送mailbox消息会休眠，禁止中断上下文等不允许休眠的流程中调用
 *
 * @return: VF获取PF DCB状态返回成功或者失败
 *     @retval 0 成功
 *     @retval 非0 失败
 */
int hinic5_get_pf_dcb_state(void *hwdev, struct hinic5_dcb_state *dcb_state);

/**
 * @brief 根据优先级获取对应cos值
 *
 * @param hwdev device pointer to hwdev
 * @param pri 优先级 PCP模式[0~7] DSCP模式[0~63]
 * @param cos 输出cos值 [0~7]
 *
 * @details 通过用户输入的pri查询对应的cos值，PCP模式下，pri合法值为0~7, DSCP模式下，pri合法值0~63
 *
 * @attention: NA
 *
 * @return: pri映射cos查询成功或者失败.
 *     @retval 0 成功
 *     @retval 非0 失败
 */
int hinic5_get_cos_by_pri(void *hwdev, u8 pri, u8 *cos);

/* TO DO 以下接口待删除 */
#if !defined(__UEFI__) && !defined(__VMWARE__)
typedef u8 (*hinic5_cqe_cb)(void *lld_dev, void *data);

int hinic5_register_cqe_cb(struct hinic5_lld_dev *lld_dev, enum hinic5_service_type event,
			       hinic5_cqe_cb cqe_cb);
void hinic5_unregister_cqe_cb(struct hinic5_lld_dev *lld_dev, enum hinic5_service_type event);
#endif

enum hinic5_bonding_en {
        HINIC5_BONDING_OFFLOAD_DISABLE = 0,
        HINIC5_BONDING_OFFLOAD_ENABLE
};

enum hinic5_bonding_event_e {
        BOND_EVENT_LINK_DOWN = 0,
        BOND_EVENT_LINK_UP = 1,
        BOND_EVENT_OPEN = 2,
        BOND_EVENT_CLOSE = 3
};
/* *
 * @brief hinic5_bonding_register_service_func - bonding event register
 * @param type: hinic5 service type
 * @param func: register function
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic5_bonding_register_service_func(enum hinic5_service_type type, void (*func)(void *netdev,
									     u32 bond_id, u8 new_slaves, enum hinic5_bonding_event_e event));

/* *
 * @brief hinic5_bonding_unregister_service_func - bonding event unregister
 * @param type: hinic5 service type
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic5_bonding_unregister_service_func(enum hinic5_service_type type);

/* *
 * @brief hinic5_offload_bond_en_get - get bonding offload status
 * @param type: void
 * @retval zero: bonding offload disable
 * @retval non-zero: bonding offload enable
 */
int hinic5_offload_bond_en_get(void);

/* *
 * @brief hinic5_bond_offload_get_uplink_id - get bonding uplink id
 * @param type: u16 bond_id, u32 *uplink_id
 * @retval zero success
 * @retval non-zero failure
 */
int hinic5_bond_offload_get_uplink_id(u16 bond_id, u32 *uplink_id);

/* *
 * @brief hinic5_bond_offload_get_slaves - get bonding slaves info
 * @param type: u16 bond_id, void *drv_msg, u8 *slaves
 * @retval zero success
 * @retval non-zero failure
 */
int hinic5_bond_offload_get_slaves(u16 bond_id, void *drv_msg, u8 *slaves);

#if !defined(__UEFI__) && !defined(__WIN__) && !defined(__VMWARE__)
int hinic5_get_phy_port_id_by_netdev(struct net_device *netdev, uint8_t *phy_port_id);
#endif

/* *
 * @brief hinic5_get_phy_port_stats - get port stats
 * @param hwdev: device pointer to hwdev
 * @param stats: port stats
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic5_get_phy_port_stats(void *hwdev, struct mag_cmd_port_stats *stats);

#endif
