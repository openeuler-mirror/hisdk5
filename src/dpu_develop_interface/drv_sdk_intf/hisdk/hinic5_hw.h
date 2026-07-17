/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef HINIC5_HW_H
#define HINIC5_HW_H

#include <linux/types.h>

#include "mpu_inband_cmd_defs.h"
#include "hinic5_crm.h"

#ifndef BIG_ENDIAN
#define BIG_ENDIAN 0x4321		/**< 表示大端字节序 */
#endif

#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN 0x1234	/**< 表示小端字节序 */
#endif

#ifdef BYTE_ORDER
#undef BYTE_ORDER
#endif
/* X86 */
#define BYTE_ORDER LITTLE_ENDIAN
/* to use 0-level CLA, page size must be: SQ 16B(wqe) * 64k(max_q_depth) */
#define HINIC5_DEFAULT_WQ_PAGE_SIZE 0x100000	/**< 定义了默认的工作队列页面大小 */
#define HINIC5_HW_WQ_PAGE_SIZE 0x1000			/**< 定义硬件发送队列的页面大小 */
#define HINIC5_MAX_WQ_PAGE_SIZE_ORDER 8			/**< 定义最大工作队列页面大小 */
#define SPU_HOST_ID 4		/**< 表示主机的ID */

#define HINIC5_NIC_RES	BIT(RES_TYPE_NIC)		/**< 表示网络接口控制器（NIC）资源 */

/**
 * @brief enum hinic5_channel_id - 通道类型
 * @details 定义了Hinic5通道的枚举类型
 */
enum hinic5_channel_id {
    HINIC5_CHANNEL_DEFAULT,
    HINIC5_CHANNEL_COMM,
    HINIC5_CHANNEL_NIC,
    HINIC5_CHANNEL_ROCE,
    HINIC5_CHANNEL_TOE,
    HINIC5_CHANNEL_FC,
    HINIC5_CHANNEL_OVS,
    HINIC5_CHANNEL_DSW,
    HINIC5_CHANNEL_MIG,
    HINIC5_CHANNEL_CRYPT,
    HINIC5_CHANNEL_UB,
    HINIC5_CHANNEL_JBOF,
    HINIC5_CHANNEL_MACSEC,
	HINIC5_CHANNEL_DMMU,
	HINIC5_CHANNEL_HIHTR,

    HINIC5_CHANNEL_MAX = 32,
};

/**
 * @brief struct hinic5_cmd_buf - dma buffer 对象
 * @details 定义一个dma缓冲结构体
 */
struct hinic5_cmd_buf {
    void *buf;                    /**< va for buffer */
    dma_addr_t dma_addr;          /**< dma address for buffer */
    u16 size;                     /**< buffer size */
    atomic_t ref_cnt;             /**< buffer reference count */
};

/**
 * @brief struct hinic5_cmdq_cmd_param - cmdq请求对象
 * @details 存储cmdq请求的参数
 */
struct hinic5_cmdq_cmd_param {
    u8 mod;   /**< mod id */
    u8 cmd;   /**< cmd opcode */
    struct hinic5_cmd_buf *buf_in;   /**< input buf, only support 32B/96B/160B for inline data mode */
    struct hinic5_cmd_buf *buf_out;   /**< output buf writed by chip to return data */
    u64 *out_param;  /**< output data, only support 8B */
};

/**
 * @brief enum hinic5_hwdev_init_state
 * @details 定义了硬件设备初始化状态的枚举类型
 */
enum hinic5_hwdev_init_state {
    HINIC5_HWDEV_NONE_INITED = 0,	/**< 设备未初始化 */
    HINIC5_HWDEV_MGMT_INITED,		/**< 设备管理模块已初始化 */
    HINIC5_HWDEV_MBOX_INITED,		/**< 设备mailbox模块已初始化 */
    HINIC5_HWDEV_CMDQ_INITED,		/**< 设备cmdq模块已初始化 */
};

/**
 * @brief struct hinic5_ceq_info - 描述ceq的配置信息
 * @details NA
 */
struct hinic5_ceq_info {
	u32 q_len;  /**< ceq length */
	u32 page_size; /**< ceq page size */
	u16 elem_size; /**< ceq element size */
	u16 num_pages;  /**< number of ceq pages*/
	u32 num_elem_in_pg; /**< number of ceq element in one page */
};

/**
 * @brief enum hinic5_ceq_event - ceqe的类型
 * @details 定义了Hinic5的中断事件类型
 */
enum hinic5_ceq_event {
    HINIC5_NON_L2NIC_SCQ,		/**< 非L2NIC的SCQ中断事件 */
    HINIC5_NON_L2NIC_ECQ,		/**< 非L2NIC的ECQ中断事件 */
    HINIC5_NON_L2NIC_NO_CQ_EQ,	/**< 非L2NIC没有CQ的中断事件 */
    HINIC5_CMDQ,		/**< 命令队列中断事件 */
    HINIC5_L2NIC_SQ,	/**< L2NIC的发送队列中断事件 */
    HINIC5_L2NIC_RQ,	/**< L2NIC的接收队列中断事件 */
    HINIC5_CEQ_EVENT_RSVD,		/**< 保留的中断事件 */
    HINIC5_FAST_MSG_RQ,			/**< 快速消息接收队列中断事件 */
    HINIC5_MAX_CEQ_EVENTS,		/**< ceqe的类型个数 */
};

/**
 * @brief enum hinic5_mbox_seg_errcode
 * @details mailbox错误码枚举类型
 */
enum hinic5_mbox_seg_errcode {
	MBOX_ERRCODE_NO_ERRORS		= 0,
	MBOX_ERRCODE_VF_TO_WRONG_FUNC	= 0x100,	/**< VF发送邮箱数据到错误的目标设备 */
	MBOX_ERRCODE_PPF_TO_WRONG_FUNC	= 0x200,	/**< PPF发送邮箱数据到错误的目标设备 */
	MBOX_ERRCODE_PF_TO_WRONG_FUNC	= 0x300,	/**< PF发送邮箱数据到错误的目标设备 */
	MBOX_ERRCODE_ZERO_DATA_SIZE	= 0x400,		/**< 邮箱数据大小设置为全零 */
	MBOX_ERRCODE_UNKNOWN_SRC_FUNC	= 0x500,	/**< 未知的原设备 */
	MBOX_ERRCODE_UNKNOWN_DES_FUNC	= 0x600,	/**< 未知的目标设备 */
};

/**
 * @brief 处理ceqe事件的回调类型
 * @param pri_handle 回调私有数据
 * @param ceqe_data ceqe数据
 *
 * @return NA
 */
typedef void (*hinic5_ceq_event_cb)(void *pri_handle, u32 ceqe_data);

/**
 * @brief 定义一个名为hinic5_aeq_swe_cb的函数指针类型
 * @param pri_handle 参数为一个void类型的指针，表示私有句柄
 * @param event 参数为一个u8类型，表示事件
 * @param data 参数为一个u8类型的指针，表示数据
 *
 * @return 返回值为u8类型的函数指针
 */
typedef u8 (*hinic5_aeq_swe_cb)(void *pri_handle, u8 event, u8 *data);

/**
 * @brief 定义一个函数指针类型，用于处理虚拟函数邮箱的回调函数
 * @param pri_handle 指向私有句柄的指针
 * @param cmd 命令
 * @param buf_in 输入缓冲区
 * @param in_size 输入缓冲区大小
 * @param buf_out 输出缓冲区
 * @param out_size 输出缓冲区大小的指针
 *
 * @return 是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
typedef int (*hinic5_vf_mbox_cb)(void *pri_handle, u16 cmd,
	     void *buf_in, u16 in_size, void *buf_out, u16 *out_size);

/**
 * @brief 定义一个函数指针类型，用于处理PF邮箱的回调函数
 * @param pri_handle 主句柄
 * @param vf_id 虚拟函数ID
 * @param cmd 命令
 * @param buf_in 输入缓冲区
 * @param in_size 输入缓冲区大小
 * @param buf_out 输出缓冲区
 * @param out_size 输出缓冲区大小
 *
 * @return 是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
typedef int (*hinic5_pf_mbox_cb)(void *pri_handle, u16 vf_id, u16 cmd, void *buf_in,
	     u16 in_size, void *buf_out, u16 *out_size);

/**
 * @brief 定义一个函数指针类型，用于处理PF到VF的邮箱通信
 * @param pri_handle 设备句柄
 * @param pf_idx PF索引
 * @param vf_id VF ID
 * @param cmd 命令
 * @param buf_in 输入缓冲区
 * @param in_size 输入缓冲区大小
 * @param buf_out 输出缓冲区
 * @param out_size 输出缓冲区大小
 *
 * @return 是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
typedef int (*hinic5_ppf_mbox_cb)(void *pri_handle, u16 pf_idx, u16 vf_id, u16 cmd, void *buf_in,
	     u16 in_size, void *buf_out, u16 *out_size);

/**
 * @brief 定义一个函数指针类型，用于接收来自PPF邮箱的消息
 * @param pri_handle 私有句柄
 * @param cmd 命令
 * @param buf_in 输入缓冲区
 * @param in_size 输入缓冲区大小
 * @param buf_out 输出缓冲区
 * @param out_size 输出缓冲区大小
 *
 * @return 返回值
 */
typedef int (*hinic5_pf_recv_from_ppf_mbox_cb)(void *pri_handle, u16 cmd, void *buf_in,
	     u16 in_size, void *buf_out, u16 *out_size);

/**
 * @brief 定义一个函数指针类型，用于处理管理消息
 * @param pri_handle 私有句柄，用于传递给回调函数
 * @param cmd 命令码，表示需要执行的操作
 * @param buf_in 输入缓冲区，用于传递输入参数
 * @param in_size 输入缓冲区大小，用于限制输入参数的长度
 * @param buf_out 输出缓冲区，用于传递输出结果
 * @param out_size 输出缓冲区大小，用于限制输出结果的长度
 *
 * @return 无
 */
typedef void (*hinic5_mgmt_msg_cb)(void *pri_handle, u16 cmd, void *buf_in, u16 in_size,
	      void *buf_out, u16 *out_size);

u8 hinic5_nic_sw_aeqe_stats(void *hwdev, u8 event, u8 *data);

/**
 * @brief 注册微码产生的无状态aeqe的handler
 *
 * @param hwdev 设备对象指针(struct hinic5_hwdev *)
 * @param pri_handle 回调函数私有数据
 * @param stateless_aeq_swe_cb 回调函数
 *
 * @details NA
 *
 * @attention: NA
 *
 * @return: 描述函数返回值.
 *     @retval 0 注册成功
 *     @retval 非0 注册失败
 */
int hinic5_register_stateless_aeqs(void *hwdev, void *pri_handle, hinic5_aeq_swe_cb stateless_aeq_swe_cb);

/**
 * @brief 注销处理微码产生的无状态aeqe的handler
 *
 * @param hwdev 设备对象指针(struct hinic5_hwdev *)
 *
 * @details NA
 *
 * @attention: NA
 *
 * @return: NA
 */
void hinic5_unregister_stateless_aeqs(void *hwdev);

/**
 * @brief hinic5_ceq_register_cb -  register ceq callback
 * @param hwdev: device pointer to hwdev
 * @param event: event type
 * @param callback: callback function
 *
 * @return
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_ceq_register_cb(void *hwdev, void *pri_handle, enum hinic5_ceq_event event,
			   hinic5_ceq_event_cb callback);
/**
 * @brief hinic5_ceq_unregister_cb - unregister ceq callback
 * @param hwdev: device pointer to hwdev
 * @param event: event type
 **/
void hinic5_ceq_unregister_cb(void *hwdev, enum hinic5_ceq_event event);

/**
 * @brief 为PPF注册处理mailbox消息的handler
 *
 * @param hwdev 设备对象指针
 * @param mod 业务模块 值为HINIC5_MOD_XXX宏定义
 * @param pri_handle 回调私有数据
 * @param callback 回调
 *
 * @details NA
 *
 * @attention: NA
 *
 * @return: 描述函数返回值.
 *     @retval 0 成功
 *     @retval 非0 失败
 */
int hinic5_register_ppf_mbox_cb(void *hwdev, u8 mod, void *pri_handle,
				hinic5_ppf_mbox_cb callback);

/**
 * @brief 为PF注册处理mailbox消息的handler
 *
 * @param hwdev 设备对象指针
 * @param mod 业务模块 值为HINIC5_MOD_XXX宏定义
 * @param pri_handle 回调私有数据
 * @param callback 回调
 *
 * @details NA
 *
 * @attention: NA
 *
 * @return: 描述函数返回值.
 *     @retval 0 成功
 *     @retval 非0 失败
 */
int hinic5_register_pf_mbox_cb(void *hwdev, u8 mod, void *pri_handle,
			       hinic5_pf_mbox_cb callback);

/**
 * @brief 为VF注册处理mailbox消息的handler
 *
 * @param hwdev 设备对象指针
 * @param mod 业务模块 值为HINIC5_MOD_XXX宏定义
 * @param pri_handle 回调私有数据
 * @param callback 回调
 *
 * @details NA
 *
 * @attention: NA
 *
 * @return: 描述函数返回值.
 *     @retval 0 成功
 *     @retval 非0 失败
 */
int hinic5_register_vf_mbox_cb(void *hwdev, u8 mod, void *pri_handle,
			       hinic5_vf_mbox_cb callback);

/**
 * @brief 注销PPF的处理mailbox的回调
 *
 * @param hwdev 设备对象指针
 * @param mod 业务模块 值为HINIC5_MOD_XXX宏定义
 *
 * @details NA
 *
 * @attention: NA
 *
 * @return: NA
 */
void hinic5_unregister_ppf_mbox_cb(void *hwdev, u8 mod);

/**
 * @brief 注销PF的处理mailbox的回调
 *
 * @param hwdev 设备对象指针
 * @param mod 业务模块 值为HINIC5_MOD_XXX宏定义
 *
 * @details NA
 *
 * @attention: NA
 *
 * @return: NA
 */
void hinic5_unregister_pf_mbox_cb(void *hwdev, u8 mod);

/**
 * @brief 注销VF的处理mailbox的回调
 *
 * @param hwdev 设备对象指针
 * @param mod 业务模块 值为HINIC5_MOD_XXX宏定义
 *
 * @details NA
 *
 * @attention: NA
 *
 * @return: NA
 */
void hinic5_unregister_vf_mbox_cb(void *hwdev, u8 mod);

/**
 * @brief 注销PF的处理mailbox的回调
 *
 * @param hwdev 设备对象指针
 * @param mod 业务模块 值为HINIC5_MOD_XXX宏定义
 *
 * @details source是PPF
 *
 * @attention: NA
 *
 * @return: NA
 */
void hinic5_unregister_ppf_to_pf_mbox_cb(void *hwdev, u8 mod);

/**
 * @brief 为function注册处理mailbox消息的handler
 *
 * @param hwdev 设备对象指针
 * @param mod 业务模块 值为HINIC5_MOD_XXX宏定义
 * @param pri_handle 回调私有数据
 * @param callback 回调
 *
 * @details source为MPU
 *
 * @attention: NA
 *
 * @return: 描述函数返回值.
 *     @retval 0 成功
 *     @retval 非0 失败
 */
int hinic5_register_mgmt_msg_cb(void *hwdev, u8 mod, void *pri_handle,
				hinic5_mgmt_msg_cb callback);

/**
 * @brief 注销function的处理mailbox的回调
 *
 * @param hwdev 设备对象指针
 * @param mod 业务模块 值为HINIC5_MOD_XXX宏定义
 *
 * @details source是MPU
 *
 * @attention: NA
 *
 * @return: NA
 */
void hinic5_unregister_mgmt_msg_cb(void *hwdev, u8 mod);

/**
 * @brief hinic5_register_service_adapter - register service adapter
 * @param hwdev: device pointer to hwdev
 * @param service_adapter: service adapter
 * @param type: service type
 *
 * @return
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_register_service_adapter(void *hwdev, void *service_adapter,
				    enum hinic5_service_type type);

/**
 * @brief hinic5_unregister_service_adapter - unregister service adapter
 * @param hwdev: device pointer to hwdev
 * @param type: service type
 **/
void hinic5_unregister_service_adapter(void *hwdev,
				       enum hinic5_service_type type);

/**
 * @brief hinic5_get_service_adapter - get service adapter
 * @param hwdev: device pointer to hwdev
 * @param type: service type
 *
 * @return
 * 		@retval non-zero: success
 * 		@retval null: failure
 */
void *hinic5_get_service_adapter(void *hwdev, enum hinic5_service_type type);

/**
 * @brief hinic5_alloc_db_phy_addr - alloc doorbell & direct wqe pyhsical addr
 * @param hwdev: device pointer to hwdev
 * @param db_base: pointer to alloc doorbell base address
 * @param dwqe_base: pointer to alloc direct base address
 *
 * @return
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_alloc_db_phy_addr(void *hwdev, u64 *db_base, u64 *dwqe_base);

/**
 * @brief hinic5_free_db_phy_addr - free doorbell & direct wqe physical address
 * @param hwdev: device pointer to hwdev
 * @param db_base: pointer to free doorbell base address
 * @param dwqe_base: pointer to free direct base address
 **/
void hinic5_free_db_phy_addr(void *hwdev, u64 db_base, u64 dwqe_base);

/**
 * @brief hinic5_alloc_db_addr - alloc doorbell & direct wqe
 * @param hwdev: device pointer to hwdev
 * @param db_base: pointer to alloc doorbell base address
 * @param dwqe_base: pointer to alloc direct base address
 *
 * @return
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_alloc_db_addr(void *hwdev, void __iomem **db_base,
			 void __iomem **dwqe_base);

/**
 * @brief hinic5_free_db_addr - free doorbell & direct wqe
 * @param hwdev: device pointer to hwdev
 * @param db_base: pointer to free doorbell base address
 * @param dwqe_base: pointer to free direct base address
 **/
void hinic5_free_db_addr(void *hwdev, const void __iomem *db_base,
			 void __iomem *dwqe_base);

/**
 * @brief hinic5_alloc_db_phy_addr - alloc physical doorbell & direct wqe
 * @param hwdev: device pointer to hwdev
 * @param db_base: pointer to alloc doorbell base address
 * @param dwqe_base: pointer to alloc direct base address
 *
 * @return
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_alloc_db_phy_addr(void *hwdev, u64 *db_base, u64 *dwqe_base);

/**
 * @brief hinic5_free_db_phy_addr - free physical doorbell & direct wqe
 * @param hwdev: device pointer to hwdev
 * @param db_base: free doorbell base address
 * @param dwqe_base: free direct base address
 **/

void hinic5_free_db_phy_addr(void *hwdev, u64 db_base, u64 dwqe_base);

/**
 * @brief hinic5_set_root_ctxt - set root context
 * @param hwdev: device pointer to hwdev
 * @param rq_depth: rq depth
 * @param sq_depth: sq depth
 * @param rx_buf_sz: rx buffer size
 * @param channel: channel id
 *
 * @return
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_set_root_ctxt(void *hwdev, u32 rq_depth, u32 sq_depth,
			 u16 rx_buf_sz, u16 channel);

/**
 * @brief hinic5_clean_root_ctxt - clean root context
 * @param hwdev: device pointer to hwdev
 * @param channel: channel id
 *
 * @return
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_clean_root_ctxt(void *hwdev, u16 channel);

/**
 * @brief hinic5_alloc_cmd_buf - alloc cmd buffer
 * @param hwdev: device pointer to hwdev
 *
 * @return
 * 		@retval non-zero: success
 * 		@retval null: failure
 * @note
 * 		申请到的内存不支持默认清零, 调用者根据需求进行清零
 */
struct hinic5_cmd_buf *hinic5_alloc_cmd_buf(void *hwdev);

/**
 * @brief hinic5_free_cmd_buf - free cmd buffer
 * @param hwdev: device pointer to hwdev
 * @param cmd_buf: cmd buffer to free
 **/
void hinic5_free_cmd_buf(void *hwdev, struct hinic5_cmd_buf *cmd_buf);

/**
 * @brief 读取16位计数器的值
 * @param hwdev 硬件设备上下文
 * @param node 节点号
 * @param instance 实例号
 * @param ctr_id 计数器ID
 * @param value 存储读取到的计数器值的指针
 *
 * @return 0 成功，其他值表示失败
 */
int hinic5_sm_ctr_rd16(void *hwdev, u8 node, u8 instance, u32 ctr_id, u16 *value);

/**
 * @brief hinic5_sm_ctr_rd32 - small single 32 counter read
 * @param hwdev: device pointer to hwdev
 * @param node: the node id
 * @param instance: instance id
 * @param ctr_id: counter id
 * @param value: read counter value ptr
 *
 * @return
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_sm_ctr_rd32(void *hwdev, u8 node, u8 instance, u32 ctr_id,
		       u32 *value);
/**
 * @brief hinic5_sm_ctr_rd32_clear - small single 32 counter read clear
 * @param hwdev: device pointer to hwdev
 * @param node: the node id
 * @param instance: instance id
 * @param ctr_id: counter id
 * @param value: read counter value ptr
 *
 * @return
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_sm_ctr_rd32_clear(void *hwdev, u8 node, u8 instance,
			     u32 ctr_id, u32 *value);

/**
 * @brief hinic5_sm_ctr_rd64_pair - big pair 128 counter read
 * @param hwdev: device pointer to hwdev
 * @param node: the node id
 * @param instance: instance id
 * @param ctr_id: counter id
 * @param value1: read counter value ptr
 * @param value2: read counter value ptr
 *
 * @return
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_sm_ctr_rd64_pair(void *hwdev, u8 node, u8 instance,
			    u32 ctr_id, u64 *value1, u64 *value2);

/**
 * @brief 读取并清除64位计数器对的值
 * @param hwdev 硬件设备上下文
 * @param node 节点号
 * @param instance 实例号
 * @param ctr_id 计数器ID
 * @param value1 存储读取到的64位计数器对的第一个值的指针
 * @param value2 存储读取到的64位计数器对的第二个值的指针
 *
 * @return 0 成功，其他值 失败
 */
int hinic5_sm_ctr_rd64_pair_clear(void *hwdev, u8 node, u8 instance,
				  u32 ctr_id, u64 *value1, u64 *value2);

/**
 * @brief hinic5_sm_ctr_rd64 - big counter 64 read
 * @param hwdev: device pointer to hwdev
 * @param node: the node id
 * @param instance: instance id
 * @param ctr_id: counter id
 * @param value: read counter value ptr
 *
 * @return
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_sm_ctr_rd64(void *hwdev, u8 node, u8 instance, u32 ctr_id,
		       u64 *value);

/**
 * @brief 清除64位计数器的值
 * @param hwdev 硬件设备的句柄
 * @param node 节点号
 * @param instance 实例号
 * @param ctr_id 计数器ID
 * @param value 存储读取到的计数器值的指针
 *
 * @return 0 成功，其他值表示失败
 */
int hinic5_sm_ctr_rd64_clear(void *hwdev, u8 node, u8 instance,
			     u32 ctr_id, u64 *value);

/**
 * @brief hinic5_api_csr_rd32 - read 32 byte csr
 * @param hwdev: device pointer to hwdev
 * @param dest: hardware node id
 * @param addr: reg address
 * @param val: reg value
 *
 * @return
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_api_csr_rd32(void *hwdev, u8 dest, u32 addr, u32 *val);

/**
 * @brief hinic5_api_csr_wr32 - write 32 byte csr
 * @param hwdev: device pointer to hwdev
 * @param dest: hardware node id
 * @param addr: reg address
 * @param val: reg value
 *
 * @return
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_api_csr_wr32(void *hwdev, u8 dest, u32 addr, u32 val);

/**
 * @brief hinic5_api_csr_rd64 - read 64 byte csr
 * @param hwdev: device pointer to hwdev
 * @param dest: hardware node id
 * @param addr: reg address
 * @param val: reg value
 *
 * @return
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_api_csr_rd64(void *hwdev, u8 dest, u32 addr, u64 *val);

/**
 * @brief hinic5_get_chip_fault_stats - get chip fault stats
 * @param hwdev: device pointer to hwdev
 * @param chip_fault_stats: pointer to memory caller to alloc
 * @param offset: offset
 */
void hinic5_get_chip_fault_stats(const void *hwdev, u8 *chip_fault_stats,
				 u32 offset);

/**
 * @brief hinic5_msg_to_mgmt_sync - msg to management cpu
 * @param hwdev: device pointer to hwdev
 * @param mod: mod type
 * @param cmd: cmd
 * @param buf_in: message buffer in
 * @param in_size: in buffer size
 * @param buf_out: message buffer out
 * @param out_size: out buffer size
 * @param timeout: timeout
 * @param channel: channel id
 *
 * @return
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_msg_to_mgmt_sync(void *hwdev, u8 mod, u16 cmd, void *buf_in,
			    u16 in_size, void *buf_out, u16 *out_size,
			    u32 timeout, u16 channel);

/**
 * @brief hinic5_msg_to_mgmt_async - msg to management cpu async
 * @param hwdev: device pointer to hwdev
 * @param mod: mod type
 * @param cmd: cmd
 * @param buf_in: message buffer in
 * @param in_size: in buffer size
 * @param channel: channel id
 *
 * @details The function does not sleep inside, allowing use in irq context
 * @return
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_msg_to_mgmt_async(void *hwdev, u8 mod, u16 cmd, const void *buf_in,
			     u16 in_size, u16 channel);

/**
 * @brief hinic5_msg_to_mgmt_no_ack - msg to management cpu don't need no ack
 * @param hwdev: device pointer to hwdev
 * @param mod: mod type
 * @param cmd: cmd
 * @param buf_in: message buffer in
 * @param in_size: in buffer size
 * @param channel: channel id
 *
 * @details The function will sleep inside, and it is not allowed to be used in interrupt context
 * @return 发送是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_msg_to_mgmt_no_ack(void *hwdev, u8 mod, u16 cmd, void *buf_in,
			      u16 in_size, u16 channel);

/**
 * @brief 向管理处理链发送异步消息
 * @param hwdev 硬件设备上下文
 * @param mod 消息模块
 * @param cmd 消息命令
 * @param buf_in 输入缓冲区
 * @param in_size 输入缓冲区大小
 *
 * @return 发送是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_msg_to_mgmt_api_chain_async(void *hwdev, u8 mod, u16 cmd,
				       const void *buf_in, u16 in_size);

int hinic5_msg_to_mgmt_api_chain_sync(void *hwdev, u8 mod, u16 cmd,
				      void *buf_in, u16 in_size, void *buf_out,
				      u16 *out_size, u32 timeout);

/**
 * @brief hinic5_mbox_to_pf - vf mbox message to pf
 * @param hwdev: device pointer to hwdev
 * @param mod: mod type
 * @param cmd: cmd
 * @param buf_in: message buffer in
 * @param in_size: in buffer size
 * @param buf_out: message buffer out
 * @param out_size: out buffer size
 * @param timeout: timeout
 * @param channel: channel id
 *
 * @return 是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_mbox_to_pf(void *hwdev, u8 mod, u16 cmd, void *buf_in,
		      u16 in_size, void *buf_out, u16 *out_size,
		      u32 timeout, u16 channel);

/**
 * @brief hinic5_mbox_to_vf - mbox message to vf
 * @param hwdev: device pointer to hwdev
 * @param vf_id: vf index
 * @param mod: mod type
 * @param cmd: cmd
 * @param buf_in: message buffer in
 * @param in_size: in buffer size
 * @param buf_out: message buffer out
 * @param out_size: out buffer size
 * @param timeout: timeout
 * @param channel: channel id
 *
 * @return 是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_mbox_to_vf(void *hwdev, u16 vf_id, u8 mod, u16 cmd, void *buf_in,
		      u16 in_size, void *buf_out, u16 *out_size, u32 timeout,
		      u16 channel);

/**
 * @brief hinic5_mbox_to_vf_without_ack - mbox message to vf without ack
 * @param hwdev: device pointer to hwdev
 * @param vf_id: vf index
 * @param mod: mod type
 * @param cmd: cmd
 * @param buf_in: message buffer in
 * @param in_size: in buffer size
 * @param channel: channel id
 *
 * @return 是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_mbox_to_vf_without_ack(void *hwdev, u16 vf_id, u8 mod, u16 cmd,
								  void *buf_in, u16 in_size,  u16 channel);

/**
 * @brief hinic5_mbox_to_vf_no_ack - mbox message to vf no ack
 * @param hwdev: device pointer to hwdev
 * @param vf_id: vf index
 * @param mod: mod type
 * @param cmd: cmd
 * @param buf_in: message buffer in
 * @param in_size: in buffer size
 * @param buf_out: message buffer out
 * @param out_size: out buffer size
 * @param channel: channel id
 *
 * @return 是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_mbox_to_vf_no_ack(void *hwdev, u16 vf_id, u8 mod, u16 cmd, void *buf_in,
			     u16 in_size, void *buf_out, u16 *out_size, u16 channel);

int hinic5_clp_to_mgmt(void *hwdev, u8 mod, u16 cmd, const void *buf_in,
		       u16 in_size, void *buf_out, u16 *out_size);
/**
 * @brief hinic5_cmdq_async - cmdq asynchronous message
 * @param hwdev: device pointer to hwdev
 * @param mod: mod type
 * @param cmd: cmd
 * @param buf_in: message buffer in
 * @param channel: channel id
 *
 * @return 是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_cmdq_async(void *hwdev, u8 mod, u8 cmd, struct hinic5_cmd_buf *buf_in, u16 channel);

/**
 * @brief hinic5_cmdq_direct_resp - cmdq direct message response
 * @param hwdev: device pointer to hwdev
 * @param mod: mod type
 * @param cmd: cmd
 * @param buf_in: message buffer in
 * @param out_param: message out
 * @param timeout: timeout
 * @param channel: channel id
 *
 * @return 是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_cmdq_direct_resp(void *hwdev, u8 mod, u8 cmd,
			    struct hinic5_cmd_buf *buf_in,
			    u64 *out_param, u32 timeout, u16 channel);

/**
 * @brief hinic5_cmdq_detail_resp - cmdq detail message response
 * @param hwdev: device pointer to hwdev
 * @param mod: mod type
 * @param cmd: cmd
 * @param buf_in: message buffer in
 * @param buf_out: message buffer out
 * @param out_param: inline output data
 * @param timeout: timeout
 * @param channel: channel id
 *
 * @return 是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_cmdq_detail_resp(void *hwdev, u8 mod, u8 cmd,
			    struct hinic5_cmd_buf *buf_in,
			    struct hinic5_cmd_buf *buf_out,
			    u64 *out_param, u32 timeout, u16 channel);

/**
 * @brief hinic5_cmdq_inline_data - cmdq with inline data
 * @param hwdev: device pointer to hwdev
 * @param cmd_param: cmd info, see struct hinic5_cmdq_cmd_param
 * @param timeout: timeout
 * @param channel: channel id
 *
 * @return 是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_cmdq_inline_data(void *hwdev, struct hinic5_cmdq_cmd_param *cmd_param,
			    u32 timeout, u16 channel);

/**
 * @brief hinic5_cos_id_detail_resp - cmdq detail message response
 * @param hwdev: device pointer to hwdev
 * @param mod: mod type
 * @param cmd: cmd
 * @param cos_id: cos id
 * @param buf_in: message buffer in
 * @param buf_out: message buffer out
 * @param out_param: inline output data
 * @param timeout: timeout
 * @param channel: channel id
 *
 * @return 是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_cos_id_detail_resp(void *hwdev, u8 mod, u8 cmd, u8 cos_id,
			      struct hinic5_cmd_buf *buf_in,
			      struct hinic5_cmd_buf *buf_out,
			      u64 *out_param, u32 timeout, u16 channel);

/**
 * @brief hinic5_ppf_tmr_start - start ppf timer
 * @param hwdev: device pointer to hwdev
 *
 * @return 是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_ppf_tmr_start(void *hwdev);

/**
 * @brief hinic5_ppf_tmr_stop - stop ppf timer
 * @param hwdev: device pointer to hwdev
 *
 * @return 是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_ppf_tmr_stop(void *hwdev);

/**
 * @brief hinic5_func_tmr_bitmap_set - set timer bitmap status
 * @param hwdev: device pointer to hwdev
 * @param func_id: global function index
 * @param enable: 0-disable, 1-enable
 *
 * @return 是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_func_tmr_bitmap_set(void *hwdev, u16 func_id, bool en);

/**
 * @brief hinic5_func_vio_en - set current function VIO to enabled/disabled
 * @param hwdev: device pointer to hwdev
 * @param en: 0-disable, 1-enable
 *
 * @return 是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_func_vio_en(void *hwdev, bool en);

/**
 * @brief hinic5_get_board_info - get board info
 * @param hwdev: device pointer to hwdev
 * @param info: board info
 * @param channel: channel id
 *
 * @return 是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_get_board_info(void *hwdev, struct hinic5_board_info *info,
			  u16 channel);

/**
 * @brief hinic5_set_wq_page_size - set work queue page size
 * @param hwdev: device pointer to hwdev
 * @param func_idx: function id
 * @param page_size: page size
 * @param channel: channel id
 *
 * @return 是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_set_wq_page_size(void *hwdev, u16 func_idx, u32 page_size,
			    u16 channel);

/**
 * @brief hinic5_event_callback - evnet callback to notify service driver
 * @param hwdev: device pointer to hwdev
 * @param event: event info to service driver
 */
void hinic5_event_callback(void *hwdev, struct hinic5_event_info *event);

/**
 * @brief hinic5_dbg_lt_rd_16byte - liner table read
 * @param hwdev: device pointer to hwdev
 * @param dest: destine id
 * @param instance: instance id
 * @param lt_index: liner table index id
 * @param data: data
 */
int hinic5_dbg_lt_rd_16byte(void *hwdev, u8 dest, u8 instance,
			    u32 lt_index, u8 *data);

/**
 * @brief hinic5_dbg_lt_wr_16byte_mask - liner table write
 * @param hwdev: device pointer to hwdev
 * @param dest: destine id
 * @param instance: instance id
 * @param lt_index: liner table index id
 * @param data: data
 * @param mask: mask
 */
int hinic5_dbg_lt_wr_16byte_mask(void *hwdev, u8 dest, u8 instance,
				 u32 lt_index, u8 *data, u16 mask);

/**
 * @brief hinic5_link_event_stats - link event stats
 * @param hwdev: device pointer to hwdev
 * @param link: link status
 */
void hinic5_link_event_stats(void *dev, u8 link);

/**
 * @brief hinic5_get_link_down_cnt - link event stats
 * @param hwdev: device pointer to hwdev
 * @param link: link status
 */
int hinic5_get_link_down_cnt(void *dev, int *link_down_cnt);

/**
 * @brief hinic5_get_hw_pf_infos - get pf infos
 * @param hwdev: device pointer to hwdev
 * @param infos: pf infos
 * @param channel: channel id
 */
int hinic5_get_hw_pf_infos(void *hwdev, struct hinic5_hw_pf_infos *infos,
			   u16 channel);

/**
 * @brief hinic5_func_reset - reset func
 * @param hwdev: device pointer to hwdev
 * @param func_id: global function index
 * @param reset_flag: reset flag
 * @param channel: channel id
 */
int hinic5_func_reset(void *dev, u16 func_id, u64 reset_flag, u16 channel);

/**
 * @brief 获取PPF（Physical Function，物理功能）定时器配置
 * @param hwdev 硬件设备上下文
 *
 * @return 是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_get_ppf_timer_cfg(void *hwdev);

/**
 * @brief 设置PCI设备的BDF（总线、设备、功能）信息
 * @param hwdev 硬件设备上下文
 * @param bus PCI总线号
 * @param device PCI设备号
 * @param function PCI功能号
 *
 * @return 是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_set_bdf_ctxt(void *hwdev, u8 bus, u8 device, u8 function);

/**
 * @brief 初始化函数邮箱消息通道
 * @param hwdev 硬件设备
 * @param num_func 函数数量
 *
 * @return 是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_init_func_mbox_msg_channel(void *hwdev, u16 num_func);

/**
 * @brief 获取SML表信息
 * @param hwdev 硬件设备信息
 * @param tbl_id 表ID
 * @param node_id 节点ID
 * @param instance_id 实例ID
 *
 * @return 是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_get_sml_table_info(void *hwdev, u32 tbl_id, u8 *node_id, u8 *instance_id);

/**
 * @brief 从PPF发送邮箱消息到主机
 * @param hwdev 硬件设备
 * @param mod 消息模块
 * @param cmd 消息命令
 * @param host_id 主机ID
 * @param buf_in 输入缓冲区
 * @param in_size 输入缓冲区大小
 * @param buf_out 输出缓冲区
 * @param out_size 输出缓冲区大小
 * @param timeout 超时时间
 * @param channel 通道
 *
 * @return 是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_mbox_ppf_to_host(void *hwdev, u8 mod, u16 cmd, u8 host_id,
			    void *buf_in, u16 in_size, void *buf_out,
			    u16 *out_size, u32 timeout, u16 channel);

/**
 * @brief 强制完成所有的操作
 * @param dev 设备指针
 *
 * @return 无
 */
void hinic5_force_complete_all(void *dev);
/**
 * @brief 获取CEQ页面的物理地址
 * @param hwdev 硬件设备上下文
 * @param q_id 队列ID
 * @param page_idx 页面索引
 * @param page_phy_addr 页面物理地址
 *
 * @return 是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_get_ceq_page_phy_addr(void *hwdev, u16 q_id,
				 u16 page_idx, u64 *page_phy_addr);
				 /**
 * @brief 禁用中断请求
 * @param hwdev 硬件设备
 * @param q_id 队列ID
 *
 * @return 是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_set_ceq_irq_disable(void *hwdev, u16 q_id);
/**
 * @brief 获取ceq信息
 * @param hwdev 硬件设备信息
 * @param q_id 队列ID
 * @param ceq_info ceq信息
 *
 * @return 是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_get_ceq_info(void *hwdev, u16 q_id, struct hinic5_ceq_info *ceq_info);

/**
 * @brief hinic5_init_single_ceq_status
 * @param hwdev: device pointer to hwdev
 * @param q_id: ceq id
 */
int hinic5_init_single_ceq_status(void *hwdev, u16 q_id);

/**
 * @brief 设置API停止
 * @param hwdev 硬件设备指针
 *
 * @return 无
 */
void hinic5_set_api_stop(void *hwdev);

/**
 * @brief 激活固件
 * @param hwdev 硬件设备
 * @param cfg_index 配置索引
 *
 * @return 是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_activate_firmware(void *hwdev, u8 cfg_index);
/**
 * @brief  hinic5_switch_config函数的主要功能是切换固件版本
 * @param  hwdev 设备句柄
 * @param  cfg_index 配置索引
 *
 * @return 是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_switch_config(void *hwdev, u8 cfg_index);

enum hinic5_hw_type {
	HINIC5_HW_TYPE_FPGA = 0,
	HINIC5_HW_TYPE_ASIC = 1,
	HINIC5_HW_TYPE_EMU = 2,
	HINIC5_HW_TYPE_EDA = 3,
	HINIC5_HW_TYPE_INVALID = 0xff,
};

/**
 * @brief 获取设备硬件类型
 *
 * @param hwdev 设备对象指针
 *
 * @details NA
 *
 * @attention: NA
 *
 * @return: 描述函数返回值.
 *     @retval HINIC5_HW_TYPE_FPGA fpga类型
 *     @retval HINIC5_HW_TYPE_ASIC asic类型
 *     @retval HINIC5_HW_TYPE_EMU emu类型
 *     @retval HINIC5_HW_TYPE_EDA eda类型
 *     @retval HINIC5_HW_TYPE_INVALID 无效类型
 */
u8 hinic5_get_hw_type(void *hwdev);

#endif
