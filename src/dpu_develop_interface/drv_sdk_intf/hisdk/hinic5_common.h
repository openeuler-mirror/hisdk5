/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef HINIC5_COMMON_H
#define HINIC5_COMMON_H

#include <asm/byteorder.h>
#include <linux/types.h>

/**
 * @brief struct hinic5_dma_addr_align
 * @details DMA地址对齐结构体
 */
struct hinic5_dma_addr_align {
    u32 real_size;			/**< 真实大小 */

    void *ori_vaddr;		/**< 原始虚拟地址 */
    dma_addr_t ori_paddr;	/**< 原始物理地址 */

    void *align_vaddr;		/**< 对齐的虚拟地址 */
    dma_addr_t align_paddr;	/**< 对齐的物理地址 */
};

/**
 * @brief enum hinic5_wait_return - 等待处理的返回值枚举类
 * @details 有三种情况，处理完成，正在处理，处理出错
 */
enum hinic5_wait_return {
    WAIT_PROCESS_CPL = 0,		/**< 表示处理完成，可以进行下一步操作 */
    WAIT_PROCESS_WAITING = 1,	/**< 表示正在处理，需要继续等待 */
    WAIT_PROCESS_ERR = 2,		/**< 表示处理出错，需要进行错误处理 */
};

/**
 * @brief struct hinic5_sge
 * @details 硬件标识符
 */
struct hinic5_sge {
	u32 hi_addr;  /**< 地址的高32位 */
	u32 lo_addr;  /**< 地址的低32位 */
	u32 len;      /**< 数据大小 */
};

/**
 * @brief 分配一块设备相关的内存，并且内存地址需要按照一定的对齐方式进行对齐
 * @param dev_hdl：分配内存的设备句柄
 * @param size：需要分配的内存大小
 * @param align：内存地址对齐方式
 * @param flag：分配内存的标志
 * @param mem_align：返回的内存地址对齐信息
 *
 * @details 分配一块设备相关的内存，并且内存地址需要按照一定的对齐方式进行对齐
 *
 * @return 是否成功
 * 		@retval zero: success
 * 		@retval non-zero: failure
 */
int hinic5_dma_zalloc_coherent_align(void *dev_hdl, u64 size, u64 align,
				     unsigned int flag,
				     struct hinic5_dma_addr_align *mem_align);

/**
 * @brief 释放DMA的内存
 * @param dev_hdl：设备句柄
 * @param mem_align：内存对齐结构体指针
 *
 * @details 此函数用于释放通过hinic5_dma_alloc_coherent_align()函数分配的DMA内存
 *
 * @return void
 */
void hinic5_dma_free_coherent_align(void *dev_hdl,
				    struct hinic5_dma_addr_align *mem_align);


/**
 * @brief 定义一个名为wait_cpl_handler的函数指针类型
 * @param priv_data：私有数据，可以是任何类型的数据
 *
 * @return 返回hinic5_wait_return枚举类型
 */
typedef enum hinic5_wait_return (*wait_cpl_handler)(void *priv_data);

/**
 * @brief  在等待一定时间后，检查是否完成
 * @param  priv_data：用于传递私有数据
 * @param  handler：等待操作的处理函数
 * @param  wait_total_ms：等待总时间，单位：毫秒
 * @param  wait_once_us：每次等待时间，单位：微秒
 *
 * @details 在等待一定时间后，检查是否完成
 *
 * @return 返回检查结果
 *		@retval 0：成功
 *		@retval -EINVAL：参数无效
 *		@retval -EIO：处理过程出错
 *		@retval -ETIMEDOUT：超时
 */
int hinic5_wait_for_timeout(void *priv_data, wait_cpl_handler handler,
			    u32 wait_total_ms, u32 wait_once_us);

/**
 * @brief 将数据从CPU字节序转换为大端字节序
 * @param data 要转换的数据
 * @param len 数据长度
 *
 * @details 此函数将数据从CPU字节序转换为大端字节序。
 * 			参数data表示要转换的数据，len表示数据长度。
 *
 * @return void
 */
static inline void hinic5_cpu_to_be32(void *data, int len)
{
	int i, chunk_sz = sizeof(u32);
	int data_len = len;
	u32 *mem = (u32 *)data;

	if (!data)
		return;

	data_len = data_len / chunk_sz;

	for (i = 0; i < data_len; i++) {
		*mem = cpu_to_be32(*mem);
		mem++;
	}
}

/**
 * @brief 将大端模式的32位数据转换为当前CPU的字节序
 * @param data 要转换的数据的指针
 * @param len 要转换的数据的长度
 *
 * @details 此函数将大端模式的32位数据转换为当前CPU的字节序。
 * 			参数data指向要转换的数据，len表示要转换的数据的长度。
 *
 * @return void
 */
static inline void hinic5_be32_to_cpu(void *data, int len)
{
	int i, chunk_sz = sizeof(u32);
	int data_len = len;
	u32 *mem = (u32 *)data;

	if (!data)
		return;

	data_len = data_len / chunk_sz;

	for (i = 0; i < data_len; i++) {
		*mem = be32_to_cpu(*mem);
		mem++;
	}
}

/**
 * @brief 设置hinic5_sge结构体的值
 * @param sge 要设置的hinic5_sge结构体指针
 * @param addr 要设置的地址
 * @param len 要设置的长度
 *
 * @return void
 */
static inline void hinic5_set_sge(struct hinic5_sge *sge, dma_addr_t addr,
				  u32 len)
{
	sge->hi_addr = upper_32_bits(addr);
	sge->lo_addr = lower_32_bits(addr);
	sge->len = len;
}

#ifdef HW_CONVERT_ENDIAN
#define hinic5_hw_be32(val) (val)		/**< 将32位值转换为大端序，此处直接返回原值 */
#define hinic5_hw_cpu64(val) (val)		/**< 将64位值转换为CPU字节序，此处直接返回原值 */
#define hinic5_hw_cpu32(val) (val)		/**< 将32位值转换为CPU字节序，此处直接返回原值 */
#define hinic5_hw_cpu16(val) (val)		/**< 将16位值转换为CPU字节序，此处直接返回原值 */
#else
#define hinic5_hw_be32(val) cpu_to_be32(val)	/**< 将32位值转换为大端序，此处调用cpu_to_be32函数进行转换 */
#define hinic5_hw_cpu64(val) be64_to_cpu(val)	/**< 将64位值转换为CPU字节序，此处调用be64_to_cpu函数进行转换 */
#define hinic5_hw_cpu32(val) be32_to_cpu(val)	/**< 将32位值转换为CPU字节序，此处调用be32_to_cpu函数进行转换 */
#define hinic5_hw_cpu16(val) be16_to_cpu(val)	/**< 将16位值转换为CPU字节序，此处调用be16_to_cpu函数进行转换 */
#endif

/**
 * @brief 将数据从主机字节序转换为网络字节序
 * @param data 要转换的数据
 * @param len 数据长度
 *
 * @return 无
 */
static inline void hinic5_hw_be32_len(void *data, int len)
{
#ifndef HW_CONVERT_ENDIAN
	int i, chunk_sz = sizeof(u32);
	int data_len = len;
	u32 *mem = (u32 *)data;

	if (!data)
		return;

	data_len = data_len / chunk_sz;

	for (i = 0; i < data_len; i++) {
		*mem = hinic5_hw_be32(*mem);
		mem++;
	}
#endif
}

/**
 * @brief 该函数用于将数据从CPU端转换为HW端的32位数据
 * @param data 要转换的数据
 * @param len 要转换的数据的长度
 *
 * @return 无
 */
static inline void hinic5_hw_cpu32_len(void *data, int len)
{
#ifndef HW_CONVERT_ENDIAN
	int i, chunk_sz = sizeof(u32);
	int data_len = len;
	u32 *mem = (u32 *)data;

	if (!data)
		return;

	data_len = data_len / chunk_sz;

	for (i = 0; i < data_len; i++) {
		*mem = hinic5_hw_cpu32(*mem);
		mem++;
	}
#endif
}

#endif
