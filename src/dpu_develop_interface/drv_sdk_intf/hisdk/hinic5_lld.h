/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef HINIC5_LLD_H
#define HINIC5_LLD_H

#include <linux/pci.h>
#include "hinic5_crm.h"

/**
 * @brief function device bus type
 * @details NA
 */
enum hinic5_dev_type {
	HINIC5_DEVICE_T_PCI,   /**< 设备通过pci bus连接 */
	HINIC5_DEVICE_T_UB,    /**< 设备通过ub bus连接 */
	HINIC5_DEVICE_T_MAX,   /**< 支持的bus类型个数  */
};

#define HINIC5_CARD_ID_OFFSET  16
#define HINIC5_CARD_ID_MASK    0xffffffffffff
#define HINIC5_GUID_LEN (16)
/**
 * @brief guid
 * @details NA
 */
struct hinic5_guid {
	u8 id[HINIC5_GUID_LEN];
};

/**
 * @brief 每个function的设备信息
 * @details NA
 */
struct hinic5_device_info {
	u64 id;                        /* DPU唯一性标识编号 */

	struct hinic5_guid guid;
};

/**
 * @brief struct hinic5_lld_dev - 暴露给uld的设备对象
 * @details uld使用的设备对象，包含设备类型、硬件设备指针
 */
struct hinic5_lld_dev {
	void *hwdev;                      /**< sdk驱动内部的硬件设备指针 */
	struct device *dev;               /**< 关联的struct device      */
	enum hinic5_dev_type dev_type;    /**< device bus type         */
};

/**
 * @brief struct hinic5_uld_info
 * @details 定义了一个结构体，用于存储用户层驱动（uld）的信息
 */
struct hinic5_uld_info {
	/* When the function does not need to initialize the corresponding uld,
	 * @probe needs to return 0 and uld_dev is set to NULL;
	 * if uld_dev is NULL, @remove will not be called when uninstalling
	 */
	int (*probe)(struct hinic5_lld_dev *lld_dev, void **uld_dev, char *uld_dev_name);	/**< 初始化用户层驱动的函数 */
	void (*remove)(struct hinic5_lld_dev *lld_dev, void *uld_dev);	/**< 移除用户层驱动的函数 */
	int (*suspend)(struct hinic5_lld_dev *lld_dev, void *uld_dev, pm_message_t state);	/**< 挂起用户层驱动的函数 */
	int (*resume)(struct hinic5_lld_dev *lld_dev, void *uld_dev);	/**< 恢复用户层驱动的函数 */
	void (*event)(struct hinic5_lld_dev *lld_dev, void *uld_dev,	/**< 处理事件的函数 */
		      struct hinic5_event_info *event);
	int (*ioctl)(void *uld_dev, u32 cmd, const void *buf_in, u32 in_size,		/**< 执行ioctl操作的函数 */
		     void *buf_out, u32 *out_size);
};

/**
 * @brief hinic5_get_card_nic_uld_array - get nic uld array
 * @param lld_dev: device pointer to pcie
 * @param dev_cnt: uld cnt
 * @param array: uld array
 *
 * @return
 *      @retval zero: success
 *      @retval non-zero: failure
 */

int hinic5_get_card_nic_uld_array(struct hinic5_lld_dev *lld_dev, uint32_t *dev_cnt, void *array[]);

/**
 * @brief 注册用户层驱动
 * @param type 服务类型
 * @param uld_info 用户层驱动信息
 *
 * @details 此函数用于注册用户层驱动，根据提供的服务类型和用户层驱动信息进行注册
 *
 * @return
 *      @retval zero: success
 *      @retval non-zero: failure
 */
int hinic5_register_uld(enum hinic5_service_type type, struct hinic5_uld_info *uld_info);

/**
 * @brief 注销用户定义的上层驱动模块
 * @param type 服务类型枚举
 *
 * @details 此函数用于注销用户定义的上层驱动模块
 *
 * @return 无
 */
void hinic5_unregister_uld(enum hinic5_service_type type);

/**
 * @brief 等待LLD设备节点的改变完成
 *
 * @details 在调用此函数前，需要确保没有任何设备节点正在改变
 *
 * @return 无
 */
void lld_hold(void);
/**
 * @brief 此函数用于释放全局锁
 *
 * @details 通过原子操作递减设备引用计数，如果引用计数为0，则表示没有设备在使用该锁，可以进行释放
 *
 * @return 无
 */
void lld_put(void);

/**
 * @brief hinic5_get_lld_dev_by_chip_name - get lld device by chip name
 * @param chip_name: chip name
 *
 * @details The value of lld_dev reference increases when lld_dev is obtained. The caller needs
 * 	    to release the reference by calling hinic5_lld_dev_put.
 *
 * @return 返回lld设备
 */
struct hinic5_lld_dev *hinic5_get_lld_dev_by_chip_name(const char *chip_name);

/**
 * @brief hinic5_lld_dev_hold - get reference to lld_dev
 * @param dev: lld device
 *
 * @details Hold reference to device to keep it from being freed
 */
void hinic5_lld_dev_hold(struct hinic5_lld_dev *dev);

/**
 * @brief hinic5_lld_dev_put - release reference to lld_dev
 * @param dev: lld device
 *
 * @details Release reference to device to allow it to be freed
 */
void hinic5_lld_dev_put(struct hinic5_lld_dev *dev);

/**
 * @brief hinic5_get_lld_dev_by_dev_name - get lld device by uld device name
 * @param dev_name: uld device name
 * @param type: uld service type, When the type is SERVICE_T_MAX, try to match all ULD names to get uld_dev
 *
 * @details The value of lld_dev reference increases when lld_dev is obtained. The caller needs
 * 	    to release the reference by calling hinic5_lld_dev_put.
 *
 * @return 如果成功，返回LLD设备，否则返回NULL
 */
struct hinic5_lld_dev *hinic5_get_lld_dev_by_dev_name(const char *dev_name,
						      enum hinic5_service_type type);

/**
 * @brief hinic5_get_lld_dev_by_dev_name_unsafe - get lld device by uld device name
 * @param dev_name: uld device name
 * @param type: uld service type, When the type is SERVICE_T_MAX, try to match all ULD names to get uld_dev
 *
 * @details hinic5_get_lld_dev_by_dev_name_unsafe() is completely analogous to
 * 			hinic5_get_lld_dev_by_dev_name(), The only difference is that the reference
 * 			of lld_dev is not increased when lld_dev is obtained.
 *			The caller must ensure that lld_dev will not be freed during the remove process
 * 			when using lld_dev.
 *
 * @return 如果成功，返回LLD设备，否则返回NULL
 */
struct hinic5_lld_dev *hinic5_get_lld_dev_by_dev_name_unsafe(const char *dev_name,
							     enum hinic5_service_type type);

/**
 * @brief hinic5_get_lld_dev_by_chip_and_port - get lld device by chip name and port id
 * @param chip_name: chip name
 * @param port_id: port id
 *
 * @return 如果成功，返回LLD设备，否则返回NULL
 */
struct hinic5_lld_dev *hinic5_get_lld_dev_by_chip_and_port(const char *chip_name, u8 port_id);

/**
 * @brief hinic5_get_lld_dev_with_l3i_enabled - get lld device which enables BAT L3I
 * @param chip_name: chip name
 *
 * @return 如果成功，返回LLD设备，否则返回NULL
 */
struct hinic5_lld_dev *hinic5_get_lld_dev_with_l3i_enabled(const char *chip_name);

/**
 * @brief hinic5_get_ppf_lld_dev - get ppf lld device by current function's lld device
 * @param lld_dev: current function's lld device
 *
 * @details The value of lld_dev reference increases when lld_dev is obtained. The caller needs
 * 			to release the reference by calling hinic5_lld_dev_put.
 *
 * @return 如果成功，返回LLD设备，否则返回NULL
 */
struct hinic5_lld_dev *hinic5_get_ppf_lld_dev(struct hinic5_lld_dev *lld_dev);

/**
 * @brief hinic5_get_ppf_lld_dev_unsafe - get ppf lld device by current function's lld device
 * @param lld_dev: current function's lld device
 *
 * @details hinic5_get_ppf_lld_dev_unsafe() is completely analogous to hinic5_get_ppf_lld_dev(),
 * 			The only difference is that the reference of lld_dev is not increased when lld_dev is obtained.
 *			The caller must ensure that ppf's lld_dev will not be freed during the remove process
 * 			when using ppf lld_dev.
 *
 * @return 如果成功，返回LLD设备，否则返回NULL
 */
struct hinic5_lld_dev *hinic5_get_ppf_lld_dev_unsafe(struct hinic5_lld_dev *lld_dev);

/**
 * @brief hinic5_get_ppf_hw_dev_unsafe - get any ppf hw device in current host by current function's hw device
 * @param hwdev: current function's hw device
 *
 * @details The caller must ensure that ppf's hw_dev will not be freed during the remove process
 * 			when using ppf hw_dev.
 */
void* hinic5_get_ppf_hw_dev_unsafe(void* hwdev);

/**
 * @brief uld5_dev_hold - get reference to uld_dev
 * @param lld_dev: lld device
 * @param type: uld service type
 *
 * @details Hold reference to uld device to keep it from being freed
 */
void uld5_dev_hold(struct hinic5_lld_dev *lld_dev, enum hinic5_service_type type);

/**
 * @brief uld5_dev_put - release reference to lld_dev
 * @param dev: lld device
 * @param type: uld service type
 *
 * @details Release reference to uld device to allow it to be freed
 */
void uld5_dev_put(struct hinic5_lld_dev *lld_dev, enum hinic5_service_type type);

/**
 * @brief hinic5_get_uld_dev - get uld device by lld device
 * @param lld_dev: lld device
 * @param type: uld service type
 *
 * @details The value of uld_dev reference increases when uld_dev is obtained. The caller needs
 * 			to release the reference by calling uld5_dev_put.
 */
void *hinic5_get_uld_dev(struct hinic5_lld_dev *lld_dev, enum hinic5_service_type type);

/**
 * @brief hinic5_get_uld_dev_unsafe - get uld device by lld device
 * @param lld_dev: lld device
 * @param type: uld service type
 *
 * @details hinic5_get_uld_dev_unsafe() is completely analogous to hinic5_get_uld_dev(),
 * 			The only difference is that the reference of uld_dev is not increased when uld_dev is obtained.
 *			The caller must ensure that uld_dev will not be freed during the remove process when using uld_dev.
 */
void *hinic5_get_uld_dev_unsafe(struct hinic5_lld_dev *lld_dev, enum hinic5_service_type type);

/**
 * @brief hinic5_get_chip_name - get chip name by lld device
 * @param lld_dev: lld device
 * @param chip_name: String for storing the chip name
 * @param max_len: Maximum number of characters to be copied for chip_name
 *
 * @return 0 成功，其他值 失败
 */
int hinic5_get_chip_name(struct hinic5_lld_dev *lld_dev, char *chip_name, u16 max_len);

/**
 * @brief 获取SDK硬件设备
 * @param lld_dev 低层驱动设备
 *
 * @return 返回SDK硬件设备
 */
void *hinic5_get_sdk_hwdev_by_lld(struct hinic5_lld_dev *lld_dev);

/**
 * @brief 设置VF业务(service)的使能开关，仅PF调用
 * @param lld_dev 设备结构体指针
 * @param service 服务类型
 * @param vf_srv_load 是否启用虚拟函数服务负载
 *
 * @return 返回0表示成功，否则返回错误码
 */
int hinic5_set_vf_service_load(struct hinic5_lld_dev *lld_dev, u16 service,
			       bool vf_srv_load);

/**
 * @brief 设置该服务的VF加载使能标志位
 * @param lld_dev 物理设备
 * @param vf_func_id 虚拟函数ID
 * @param service 服务类型
 * @param en 是否启用
 *
 * @return 返回0表示成功，否则返回错误码
 */
int hinic5_set_vf_service_state(struct hinic5_lld_dev *lld_dev, u16 vf_func_id,
				u16 service, bool en);

/**
 * @brief 设置VF的加载使能标志
 * @param lld_dev 设备结构体指针
 * @param vf_load_state 虚拟函数加载状态
 *
 * @return 返回0表示成功，否则返回错误码
 */
int hinic5_set_vf_load_state(struct hinic5_lld_dev *lld_dev, bool vf_load_state);

/**
 * @brief 绑定NIC设备
 * @param lld_dev 低层设备结构体指针
 *
 * @return 返回0表示成功，否则返回错误码
 */
int hinic5_attach_nic(struct hinic5_lld_dev *lld_dev);

/**
 * @brief  解除网卡
 * @param  lld_dev 设备的底层驱动信息
 *
 * @return 无
 */
void hinic5_detach_nic(const struct hinic5_lld_dev *lld_dev);

/**
 * @brief  将指定的服务类型附加到设备上
 * @param  lld_dev 设备的底层驱动信息
 * @param  type 服务类型
 *
 * @return 成功返回0，失败返回错误码
 */
int hinic5_attach_service(const struct hinic5_lld_dev *lld_dev, enum hinic5_service_type type);
/**
 * @brief  hinic5_detach_service 函数的作用是解除服务
 * @param  lld_dev 设备的底层驱动信息
 * @param  type 服务类型
 *
 * @return 无
 */
void hinic5_detach_service(const struct hinic5_lld_dev *lld_dev, enum hinic5_service_type type);

/**
 * @brief 注销uld前调用，执行cleanup回调
 *
 * @param type ULD服务类型
 * @param cleanup 回调
 *
 * @details 加载了ULD的所有function，按顺序调用cleanup回调
 *
 * @attention: NA
 *
 * @return: NA
 */
void hinic5_uld_cleanup_before_unregister(enum hinic5_service_type type, void (*cleanup)(void *));

/**
 * @brief  hinic5_get_vf_num 函数的作用是获取pci/ub设备使能的vf数
 * @param  lld_dev 设备的底层驱动信息
 *
 * @return 使能的vf数
 */
int hinic5_get_vf_num(struct hinic5_lld_dev *lld_dev);

/**
 * @brief  hinic5_get_chip_node_id 函数的作用是获取pci/ub设备归属的chip_node id
 * @param  lld_dev 设备的底层驱动信息
 *
 * @return chip_node id
 */
int hinic5_get_chip_node_id(struct hinic5_lld_dev *lld_dev, u64 *chip_node_id);

/**
 * @brief 获取设备信息
 *
 * @param[in] lld_dev 设备
 * @param[out] info 返回设备信息
 *
 * @details NULL
 *
 * @attention: NULL
 *
 * @return: 描述函数返回值.
 *     @retval 0 成功
 *     @retval 非0 错误码
 */
int hinic5_get_device_info(struct hinic5_lld_dev *lld_dev, struct hinic5_device_info *info);

#endif
