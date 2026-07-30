/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 * Description   : nic驱动提供的半卸载bond接口定义
 * Author        : /
 * Create        : 2025/9/9
 * Notes         :
 * History       : Init
 */

#ifndef DRV_BOND_API
#define DRV_BOND_API

#include <net/bonding.h>
#include <linux/netdevice.h>
#include <linux/types.h>
#include "bond_common_defs.h"

/**
 * @brief struct hinic5_bond_info_s
 * @details 用户获取的bond信息结构体
 */
struct hinic5_bond_info_s {
	u8 slaves;			/**< bond port slave bitmap */
	u8 cnt;				/**< bond slave 的数量 */
	u8 rsvd[2];
	char slaves_name[BOND_PORT_MAX_NUM][BOND_NAME_MAX_LEN]; /**< slave设备名 */
};

/**
 * @brief struct netdev_lower_state_info
 * @details ndev状态信息
 */
struct netdev_lower_state_info {
	u8 link_up : 1;			/**< slave设备 link up状态 */
	u8 tx_enabled : 1;		/**< 可用于发送数据的slave设备 */
	u8 rsvd : 6;
} __attribute__((__packed__));

/**
 * @brief struct bond_tracker
 * @details bond设备信息结构体
 */
struct bond_tracker {
	struct netdev_lower_state_info netdev_state[BOND_PORT_MAX_NUM]; /**< bond slave设备信息 */
	struct net_device *ndev[BOND_PORT_MAX_NUM]; /**< bond slave设备指针 */
	u8 cnt;	/**< bond slave 设备数量 */
	bool is_bonded; /**< bond是否可以发送至mpu创建并激活bond */
};

/**
 * @brief struct bond_attr
 * @details bond基础属性
 */
struct bond_attr {
	u16 bond_mode;	/**< bond模式 */
	u16 bond_id;	/**< bond id */
	u16 up_delay;	/**< bond up时开始工作的延迟时间 */
	u16 down_delay; /**< bond donw时不可用的延迟时间 */
	u8 active_slaves;	/**< 激活可用的slave bitmap */
	u8 slaves;			/**< 组bond时原始的slave bitmap */
	u8 lacp_collect_slaves; /**< lacp协议配置的slave bitmap */
	u8 xmit_hash_policy;	/**< bond选路的hash策略 */
	u32 first_roce_func;	/**< bond中的第一个func，仅roce使用 */
	u32 bond_pf_bitmap;	/**< 组bond的func */
	u32 user_bitmap;	/**< 当前使用bond的用户bitmap */
};

/**
 * @brief 用户注册的bond绑定处理接口
 * @param[in] bond: 内核协议中bonding结构体指针
 * @details 当用户注册此接口后，协议栈bond触发bond事件（增删slave等）会调用此接口进行
 * 是否可以绑定bond的判断，如果判断可以则bond驱动会为用户尝试绑定bond
 * @attention N/A
 * @return	如果返回true，代表可以绑定bond，如果返回false代表不可绑定bond
 **/
typedef bool (*attach_func)(struct bonding *bond);

/**
 * @brief 用户注册的bond事件处理接口
 * @param[in] bond_name: bond名
 * @param[in] attr: bond属性
 * @param[in] err: bond激活后/修改后/去激活后的处理结果，0为成功，非0失败
 * @details 用户注册此接口，当bond的创建/删除/更新前后，bond驱动会调用做业务自身的处理
 * @attention N/A
 * @return void
 **/
typedef void (*event_func)(const char *bond_name, struct bond_attr *attr, int err);

/**
 * @brief struct bond_srv_func
 * @details 业务注册的bond处理接口集合
 */
struct bond_srv_func {
	event_func before_active; /**< bond激活前srv处理 */
	event_func after_active; /**< bond激活后srv处理 */
	event_func before_modify; /**< bond修改前srv处理 */
	event_func after_modify; /**< bond修改后srv处理 */
	event_func before_deactive; /**< bond去激活前srv处理 */
	event_func after_deactive; /**< bond去激活后srv处理 */
	attach_func can_attach; /**< 注册此接口代表当协议栈bond更新CFM会为srv绑定bond */
};

/**
 * @brief 用户绑定协议栈bonding
 * @param[in] name bond设备名
 * @param[in] user 绑定的用户
 * @param[out] bond_id 绑定的bondid
 * @details 用户在协议栈bonding创建完成后，可以下发此接口绑定对应bond名的bond，返回芯片内部管理的bondid
 * @attention N/A
 * @return	返回绑定结果，0为成功，非0失败
 **/
int hinic5_bond_attach(const char *name, enum hinic5_bond_user user, u16 *bond_id);

/**
 * @brief 用户解绑协议栈bonding
 * @param[in] bond_id bond id
 * @param[in] user 绑定的用户
 * @details 用户解绑协议栈bonding，如果无使用的用户则会销毁nic驱动中的bond设备
 * @attention N/A
 * @return	返回解绑结果，0为成功，非0失败
 **/
int hinic5_bond_detach(u16 bond_id, enum hinic5_bond_user user);

/**
 * @brief 解绑此用户所有绑定的bond
 * @param[in] user 用户
 * @details 为所有的bond设备解绑此用户
 * @attention N/A
 * @return	void
 **/
void hinic5_bond_clean_user(enum hinic5_bond_user user);

/**
 * @brief 获取bond设备的BDF标识
 * @param[in] bond_id bond id
 * @param[out] uplink_id 返回的bdf id
 * @details 获取bond设备的BDF标识，支持pci/ub设备
 * @attention N/A
 * @return	返回获取结果 0为成功，非0失败
 **/
int hinic5_bond_get_uplink_id(u16 bond_id, u32 *uplink_id);

/**
 * @brief bond用户注册处理接口接口
 * @param[in] user 用户
 * @param[in] func 处理接口集合，包含事件处理和绑定处理
 * @details 用户注册处理接口，处理接口见bond_srv_func结构体内的接口定义
 * @attention N/A
 * @return	返回注册结果 0为成功，非0失败
 **/
int hinic5_bond_register_service_func(enum hinic5_bond_user user, struct bond_srv_func *func);

/**
 * @brief bond用户解注册处理接口接口
 * @param[in] user 用户
 * @details 用户解注册处理接口，处理接口见bond_srv_func结构体内的接口定义
 * @attention N/A
 * @return	返回解注册结果 0为成功，非0失败
 **/
int hinic5_bond_unregister_service_func(enum hinic5_bond_user user);

/**
 * @brief 获取bond slave信息
 * @param[in] bond_id bond id
 * @param[in] info bond slave信息，详见hinic5_bond_info_s
 * @details 获取bond slave信息
 * @attention N/A
 * @return	返回获取结果 0为成功，非0失败
 **/
int hinic5_bond_get_slaves(u16 bond_id, struct hinic5_bond_info_s *info);

/**
 * @brief 获取bond slave的ndev设备
 * @param[in] bond_name bond名
 * @param[in] port_id port id
 * @details 获取bond中对应portid的ndev设备指针
 * @attention N/A
 * @return 返回ndev指针，返回非NULL获取成功，返回NULL获取失败
 **/
struct net_device *hinic5_bond_get_netdev_by_portid(const char *bond_name, u8 port_id);

/**
 * @brief 获取bond slave的设备信息
 * @param[in] bond_name bond名
 * @param[out] tracker bond设备信息，见bond_tracker
 * @details 获取bond中所有slave设备的设备信息
 * @attention N/A
 * @return 返回获取结果 0为成功，非0失败
 **/
int hinic5_get_bond_tracker_by_name(const char *name, struct bond_tracker *tracker);

#endif