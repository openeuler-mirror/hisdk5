/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef HINIC5_PROFILE_H
#define HINIC5_PROFILE_H

#include <linux/types.h>

/**
 * @brief Check the device whether matches the specific scene
 * @param device Generic device pointer
 *
 * @return match or not
 */
typedef bool (*hinic5_is_match_prof)(void *device);

/**
 * @brief Init profile attributes of the device
 * @param device Generic device pointer
 *
 * @return Nullable, profile attributes
 */
typedef void *(*hinic5_init_prof_attr)(void *device);

/**
 * @brief Denit profile attributes of the device
 * @param porf_attr Profile attributes
 */
typedef void (*hinic5_deinit_prof_attr)(void *porf_attr);

/**
 * @brief Profile adapter types
 */
enum prof_adapter_type {
	PROF_ADAP_TYPE_INVALID,		/**< Invalid adapter type */
	PROF_ADAP_TYPE_PANGEA = 1,	/**< PANGEA */

	/* Add prof adapter type before default */
	PROF_ADAP_TYPE_DEFAULT,
};

/**
 * struct hinic5_prof_adapter - custom scene's profile adapter
 * @type: adapter type
 * @match: Check whether the current function is used in the custom scene.
 *	Implemented in the current source file
 * @init: When @match return true, the initialization function called in probe.
 *	Implemented in the source file of the custom scene
 * @deinit: When @match return true, the deinitialization function called when
 *	remove. Implemented in the source file of the custom scene
 */
struct hinic5_prof_adapter {
	enum prof_adapter_type	type;
	hinic5_is_match_prof	match;
	hinic5_init_prof_attr	init;
	hinic5_deinit_prof_attr	deinit;
};

struct hinic5_prof_ops {
	void		(*fault_recover)(void *data, u16 src, u16 level);
	int		(*get_work_cpu_affinity)(void *data, u32 work_type);
	void		(*probe_success)(void *data);
	void		(*remove_pre_handle)(void *hwdev);
};

struct hinic5_prof_attr {
	void *priv_data;
	u64 hw_feature_cap;
	u64 sw_feature_cap;
	u64 dft_hw_feature;
	u64 dft_sw_feature;

	struct hinic5_prof_ops *ops;
};

/**
 * @brief Get profile adapter by device
 * @param hwdev The hwdev pointer
 * @return Nullable, the adapter for the device
 */
const struct hinic5_prof_adapter *hinic5_get_prof_adapter(void *hwdev);

/**
 * @brief Helper function for verifing a profile adapter
 * @param adapter Adapter pointer
 * @return True if the adapter is valid
 */
static inline bool hinic5_verify_prof_adapter(const struct hinic5_prof_adapter *adapter)
{
	bool has_init, has_deinit;

	if (!adapter)
		return true;

	has_init   = adapter->init != NULL;
	has_deinit = adapter->deinit != NULL;
	if (has_init != has_deinit)
		return false;

	return true;
}

/**
 * @brief Helper function for finding a adapter by the device and calling adapter's init function
 * @param device Generic device's pointer
 * @param adap_objs Adapter array
 * @param num_adap Adapter array size
 * @param prof_attr [Out] Result of init function
 *
 * @return Nullable, the adapter for the device
 */
static inline struct hinic5_prof_adapter *hinic5_prof_init(void *device,
							   struct hinic5_prof_adapter *adap_objs,
							   int num_adap, void **prof_attr)
{
	struct hinic5_prof_adapter *prof_obj = NULL;
	int i;

	for (i = 0; i < num_adap; i++) {
		prof_obj = &adap_objs[i];
		if (!(prof_obj->match && prof_obj->match(device)))
			continue;

		*prof_attr = prof_obj->init ? prof_obj->init(device) : NULL;

		return prof_obj;
	}

	return NULL;
}

/**
 * @brief  反初始化一个 hinic5 的 profile 对象
 * @param  prof_obj 参数说明
 * @param  prof_attr 参数说明
 *
 * @return 无
 */
static inline void hinic5_prof_deinit(const struct hinic5_prof_adapter *prof_obj, void *prof_attr)
{
	if (!prof_obj)
		return;

	if (prof_obj->deinit)
		prof_obj->deinit(prof_attr);
}

/* module-level interface */
#ifdef CONFIG_MODULE_PROF
/**
 * @brief model-level interface
 */
struct hinic5_module_ops {
	int (*module_prof_pre_init)(void);				/**< pre初始化模块的配置文件,成功返回0，否则返回错误码 */
	int (*module_prof_post_init)(void);				/**< post初始化模块的配置文件,成功返回0，否则返回错误码 */
	void (*module_prof_pre_exit)(void);				/**< pre退出模块的配置文件 */
	void (*module_prof_post_exit)(void);				/**< post退出模块的配置文件 */
	void (*probe_fault_process)(void *pdev, u16 level);	/**< 处理探测故障 */
	int (*probe_pre_process)(void *pdev);			/**< 预处理探测 */
	void (*probe_pre_unprocess)(void *pdev);		/**< 预处理探测取消 */
};

/**
 * @brief 获取模块性能操作的函数
 *
 * @return 返回一个指向hinic5_module_ops结构体的指针
 */
struct hinic5_module_ops *hinic5_get_module_prof_ops(void);

/**
 * @brief  用于处理设备的探测错误
 * @param  pdev 设备指针
 * @param  level 错误级别
 *
 * @return 无
 */
static inline void hinic5_probe_fault_process(void *pdev, u16 level)
{
	struct hinic5_module_ops *ops = hinic5_get_module_prof_ops();

	if (ops && ops->probe_fault_process)
		ops->probe_fault_process(pdev, level);
}

/**
 * @brief 模块预初始化函数
 *
 * @return 返回0表示成功，返回其他值表示失败
 */
static inline int hinic5_module_pre_init(void)
{
	struct hinic5_module_ops *ops = hinic5_get_module_prof_ops();

	if (!ops || !ops->module_prof_pre_init)
		return -EINVAL;

	return ops->module_prof_pre_init();
}

/**
 * @brief 模块退出后的处理函数
 *
 * @return void 无返回值
 */
static inline void hinic5_module_post_exit(void)
{
	struct hinic5_module_ops *ops = hinic5_get_module_prof_ops();

	if (ops && ops->module_prof_post_exit)
		ops->module_prof_post_exit();
}

/**
 * @brief  在驱动程序中进行设备探测前的预处理
 * @param  pdev 设备指针
 *
 * @return 成功返回0，失败返回-EINVAL
 */
static inline int hinic5_probe_pre_process(void *pdev)
{
	struct hinic5_module_ops *ops = hinic5_get_module_prof_ops();

	if (!ops || !ops->probe_pre_process)
		return -EINVAL;

	return ops->probe_pre_process(pdev);
}

/**
 * @brief  设备移除或者加载失败后处理，和hinic5_probe_pre_process匹配
 * @param  pdev 设备指针
 *
 * @return 无
 */
static inline void hinic5_probe_pre_unprocess(void *pdev)
{
	struct hinic5_module_ops *ops = hinic5_get_module_prof_ops();

	if (ops && ops->probe_pre_unprocess)
		ops->probe_pre_unprocess(pdev);
}

/**
 * @brief 模块后初始化函数
 *
 * @return 返回0表示成功，返回其他值表示失败
 */
static inline int hinic5_module_post_init(void)
{
	struct hinic5_module_ops *ops = hinic5_get_module_prof_ops();
 
	if (!ops || !ops->module_prof_post_init)
		return -EINVAL;
 
	return ops->module_prof_post_init();
}
 
/**
 * @brief 模块退出前的处理函数
 *
 * @return void 无返回值
 */
static inline void hinic5_module_pre_exit(void)
{
	struct hinic5_module_ops *ops = hinic5_get_module_prof_ops();
 
	if (ops && ops->module_prof_pre_exit)
		ops->module_prof_pre_exit();
}

#else

/**
 * @brief  hinic5_probe_fault_process 函数的作用
 * @param  pdev 设备指针，用于操作设备
 * @param  level 错误级别
 *
 * @return 无
 */
static inline void hinic5_probe_fault_process(void *pdev, u16 level) { };


/**
 * @brief module pre initial
 *
 * @return
 *		@retval zero: success
 *      @retval non-zero: failure
 */

static inline int hinic5_module_pre_init(void)
{
	return 0;
}

/**
 * @brief 模块退出后的处理函数
 *
 * @return 无
 */
static inline void hinic5_module_post_exit(void) { };

/**
 * @brief module pre process
 * @param pdev: pointer to dev
 *
 * @retval
 *		@retval zero: success
 *      @retval non-zero: failure
 */

static inline int hinic5_probe_pre_process(void *pdev)
{
	return 0;
}

/**
 * @brief module pre unprocess
 * @param pdev: pointer to dev
 *
 * @return
 *		@retval zero: success
 *      @retval non-zero: failure
 */
static inline void hinic5_probe_pre_unprocess(void *pdev) { };

/**
 * @brief 模块后初始化函数
 *
 * @return 返回0表示成功，返回其他值表示失败
 */
static inline int hinic5_module_post_init(void)
{
	return 0;
}
 
/**
 * @brief 模块退出前的处理函数
 *
 * @return void 无返回值
 */
static inline void hinic5_module_pre_exit(void) { };

#endif

/**
 * @brief 获取struct hinic5_prof_attr指针
 *
 * @param hwdev sdk驱动内部的硬件设备指针
 *
 * @return: 命令执行结果.
 *     @retval NULL 获取失败
 *     @retval 非NULL 获取成功
 */
struct hinic5_prof_attr *hinic5_get_prof_attr(void *hwdev);

#endif
