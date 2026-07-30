/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef HINIC5_WQ_H
#define HINIC5_WQ_H

#include <linux/types.h>

#include "hinic5_common.h"

/**
 * @brief struct hinic5_wq
 * @details 用于描述一个工作队列的相关信息; 修改需同步到用户态的struct sdk_cmdq_info
 */
struct hinic5_wq {
    u16 cons_idx;           /**< 消费者索引 */
    u16 prod_idx;           /**< 生产者索引 */

    u32 q_depth;            /**< 队列深度 */
    u16 idx_mask;           /**< 队列索引掩码 */
    u16 wqebb_size_shift;   /**< wqebb的位移 */
    u16 rsvd1;
    u16 num_wq_pages;       /**< wq页面的数量 */
    u32 wqebbs_per_page;    /**< 每页的wqe数量 */
    u16 wqebbs_per_page_shift;
    u16 wqebbs_per_page_mask;

    struct hinic5_dma_addr_align *wq_pages;     /**< 指向DMA地址对齐结构体的指针，用于描述wq页面的相关信息 */

    dma_addr_t wq_block_paddr;  /**< wq块的物理地址 */
    u64 *wq_block_vaddr;        /**< wq块的虚拟地址 */

    void *dev_hdl;          /**< 指向pcidev->dev或Handler的指针 */
    u32 wq_page_size;       /**< wq页面大小 */
    u16 wqebb_size;         /**< wqebb大小 */
} ____cacheline_aligned;


/**
 * @brief 定义一个宏，用于计算给定队列中特定索引的掩码值
 * @param wq 队列对象
 * @param idx 索引值
 *
 * @return 返回计算后的掩码值
 */
#define WQ_MASK_IDX(wq, idx) (((u16)(idx)) & (wq)->idx_mask)
/**
 * @brief 根据工作队列和页索引计算掩码页
 * @param wq 工作队列
 * @param pg_idx 页索引
 *
 * @return 如果页索引小于工作队列的页数，则返回页索引，否则返回0
 */
#define WQ_MASK_PAGE(wq, pg_idx) (((pg_idx) < (wq)->num_wq_pages) ? (pg_idx) : 0)
/**
 * @brief 计算页面索引
 * @param wq 等待队列
 * @param idx 索引
 *
 * @return 返回计算后的页面索引
 */
#define WQ_PAGE_IDX(wq, idx) ((idx) >> (wq)->wqebbs_per_page_shift)
/**
 * @brief 计算队列中元素的偏移量
 * @param wq 队列指针
 * @param idx 元素索引
 *
 * @return 返回元素在页面中的偏移量
 */
#define WQ_OFFSET_IN_PAGE(wq, idx) ((idx) & (wq)->wqebbs_per_page_mask)
/**
 * @brief 获取WQEBB地址
 * @param wq 工作队列
 * @param pg_idx 页索引
 * @param idx_in_pg 页内索引
 *
 * @return u8* 返回WQEBB地址
 */
#define WQ_GET_WQEBB_ADDR(wq, pg_idx, idx_in_pg) \
    ((u8 *)(wq)->wq_pages[(pg_idx)].align_vaddr + (((u64)(idx_in_pg)) << (wq)->wqebb_size_shift))
/**
 * @brief 判断队列是否为0级别
 * @param wq 队列指针
 *
 * @return
 *      @retval true 如果队列是0级别
 *      @retval false 如果队列不是0级别
 */
#define WQ_IS_0_LEVEL_CLA(wq) ((wq)->num_wq_pages == 1)

/**
 * @brief free wq wqebb
 * @param wq: pointer of wq control struct
 *
 * @return free wqebb number
 */
static inline u16 hinic5_wq_free_wqebbs(struct hinic5_wq *wq)
{
    return (u16)(wq->q_depth - ((wq->q_depth + wq->prod_idx - wq->cons_idx) & wq->idx_mask) - 1);
}

/**
 * @brief check wq empty
 * @param wq: pointer of wq control struct
 *
 * @return true or false
 */
static inline bool hinic5_wq_is_empty(const struct hinic5_wq *wq)
{
    return WQ_MASK_IDX(wq, wq->prod_idx) == WQ_MASK_IDX(wq, wq->cons_idx);
}

/**
 * @brief get wq multi wqebbs
 * @param wq: pointer of wq control struct
 * @param wqebb_cnt: the number of wqebbs
 * @param pi: producer index
 *
 * @return first wqebb address
 **/
static inline void *hinic5_wq_get_wqebbs(struct hinic5_wq *wq, u16 wqebb_cnt, u16 *pi)
{
    *pi = WQ_MASK_IDX(wq, wq->prod_idx);
    wq->prod_idx += wqebb_cnt;

    return WQ_GET_WQEBB_ADDR(wq, WQ_PAGE_IDX(wq, *pi), WQ_OFFSET_IN_PAGE(wq, *pi));
}

/**
 * @brief get  wq  one wqebb
 * @param wq: pointer of wq control struct
 * @param pi: producer index
 **/
static inline void *hinic5_wq_get_one_wqebb(struct hinic5_wq *wq, u16 *pi)
{
    return hinic5_wq_get_wqebbs(wq, 1, pi);
}

/**
 * @brief get wq multi wqebbs
 * @param wq: pointer of wq control struct
 * @param num_wqebbs: the number of wqebbs
 * @param prod_idx: producer index
 * @param second_part_wqebbs_addr: second part wqebbs address
 * @param first_part_wqebbs_num: first part wqebbs address
 *
 * @return wqebbs address
 */
static inline void *hinic5_wq_get_multi_wqebbs(struct hinic5_wq *wq, u16 num_wqebbs,
					       u16 *prod_idx, void **second_part_wqebbs_addr,
					       u16 *first_part_wqebbs_num)
{
    u32 pg_idx, off_in_page;

    *prod_idx = WQ_MASK_IDX(wq, wq->prod_idx);
    wq->prod_idx += num_wqebbs;

    pg_idx = WQ_PAGE_IDX(wq, *prod_idx);
    off_in_page = WQ_OFFSET_IN_PAGE(wq, *prod_idx);

    if (off_in_page + num_wqebbs > wq->wqebbs_per_page) {
        /* wqe across wq page boundary */
        *second_part_wqebbs_addr = WQ_GET_WQEBB_ADDR(wq, WQ_MASK_PAGE(wq, pg_idx + 1), 0);
        *first_part_wqebbs_num = (u16)(wq->wqebbs_per_page - off_in_page);
    } else {
        *second_part_wqebbs_addr = NULL;
        *first_part_wqebbs_num = num_wqebbs;
    }

    return WQ_GET_WQEBB_ADDR(wq, pg_idx, off_in_page);
}

/**
 * @brief put  wq  wqebb
 * @param wq: pointer of wq control struct
 * @param num_wqebbs: number of wqebb
 */

static inline void hinic5_wq_put_wqebbs(struct hinic5_wq *wq, u16 num_wqebbs)
{
    wq->cons_idx += num_wqebbs;
}

/**
 * @brief get  wq  wqebb address
 * @param wq: pointer of wq control struct
 * @param idx: wqbb idx
 */

static inline void *hinic5_wq_wqebb_addr(struct hinic5_wq *wq, u16 idx)
{
    return WQ_GET_WQEBB_ADDR(wq, WQ_PAGE_IDX(wq, idx), WQ_OFFSET_IN_PAGE(wq, idx));
}

/**
 * @brief read one wqebb
 * @param wq: pointer of wq control struct
 * @param cons_idx: wqbb index
 *
 * @return wqe page address
 */
static inline void *hinic5_wq_read_one_wqebb(struct hinic5_wq *wq, u16 *cons_idx)
{
    *cons_idx = WQ_MASK_IDX(wq, wq->cons_idx);

    return hinic5_wq_wqebb_addr(wq, *cons_idx);
}

/**
 * @brief get the first wqe page address
 * @param wq: pointer of wq control struct
 *
 * @return wqe page address
 */
static inline u64 hinic5_wq_get_first_wqe_page_addr(struct hinic5_wq *wq)
{
    return wq->wq_pages[0].align_paddr;
}

/**
 * @brief reset wq  zero wq page data
 * @param wq: pointer of wq control struct
 */
static inline void hinic5_wq_reset(struct hinic5_wq *wq)
{
    u16 pg_idx;

    wq->cons_idx = 0;
    wq->prod_idx = 0;

    for (pg_idx = 0; pg_idx < wq->num_wq_pages; pg_idx++)
        memset(wq->wq_pages[pg_idx].align_vaddr, 0, wq->wq_page_size);
}

/**
 * @brief initial wq struct and alloc wq page
 * @param udkdev: device pointer to udkdev
 * @param wq: pointer of wq control struct
 * @param q_depth: wq depth
 * @param wqebb_size: the size of wqebb
 * @return
 *      @retval zero: success
 *      @retval non-zero: failure
 */
int hinic5_wq_create(void *hwdev, struct hinic5_wq *wq, u32 q_depth,
		     u16 wqebb_size);

/**
 * @brief release wqe pages
 * @param udkdev: device pointer to udkdev
 * @param wq: pointer of wq control struct
 */
void hinic5_wq_destroy(struct hinic5_wq *wq);

#endif
