/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2024 Huawei Technologies Co., Ltd */

#ifndef HINIC5_FAST_MSG_H
#define HINIC5_FAST_MSG_H

#include <linux/types.h>
#include <linux/workqueue.h>
#include "fast_msg_common_define.h"
#include "comm_defs.h"
#include "hinic5_hw.h"

#define FAST_MSG_ENTRY_SIZE             2
#define FAST_MSG_ENTRY_UNIT             1024
#define FAST_MSG_ENTRY_SIZE_B           (FAST_MSG_ENTRY_UNIT * FAST_MSG_ENTRY_SIZE)
#define HISDK5_FAST_MSG_MAX_PAGE_NUM    32
#define MAX_FAST_MSG_RQ_DEPTH           2048
#define FAST_MSG_RQ_OFFSET_MASK         0xFFF
#define FAST_MSG_RECV_MAX_CONCURRENT    10

/* 标识fast msg上层承载消息格式类型 */
enum hisdk5_fast_msg_ulp_format {
    HISDK5_FAST_MSG_ULP_FROMAT_NONE = 0,
    HISDK5_FAST_MSG_ULP_FROMAT_MIG = 1,
    /* 4bit位宽，类型需小于16 */
    HISDK5_FAST_MSG_ULP_FROMAT_MAX = 16
};

typedef void (*hinic5_fast_msg_rq_cb)(hisdk5_fast_msg_buf *rq_msg, void *fast_msg_rq_data);
typedef void (*hinic5_fast_msg_forward_cb)(void *data);

enum hisdk5_fast_msg_work_entry_type {
    MSG_WORK_ENTRY_RECV_FAST_MSG,
    MSG_WORK_ENTRY_FORWARDING
};

struct hisdk5_fast_msg_recv_entry {
    struct list_head entry;
    u32 type;
    union {
        u32 rq_offset;
        struct {
            void *forward_data;
            hinic5_fast_msg_forward_cb forward_cb;
        };
    };
};

struct hisdk5_fast_msg_recv_work {
    struct work_struct work;
    struct hisdk5_fast_msg_to_func *fast_msg_to_func;
    struct list_head msg_head;
    spinlock_t lock;
};

struct hisdk5_fast_msg_to_func {
    void *hwdev;

	u32 fast_msg_rq_depth;
	u32 fast_msg_rq_page_size;
	u32 fast_msg_rq_page_num;

	void *rq_mem[HISDK5_FAST_MSG_MAX_PAGE_NUM];
    dma_addr_t rq_mem_paddr[HISDK5_FAST_MSG_MAX_PAGE_NUM];

    struct workqueue_struct *workq;

    hinic5_fast_msg_rq_cb fast_msg_rq_cb[HINIC5_MOD_MAX];
	void *fast_msg_rq_data[HINIC5_MOD_MAX];

    u32 num_concurrent_work;
    struct hisdk5_fast_msg_recv_work *recv_concurrent_work;
    struct hisdk5_fast_msg_recv_entry *recv_entries;
};

void hinic5_fast_msg_rq_handler(void *pri_handle, u32 ceqe_data);
void hinic5_fast_msg_recv_handler(struct work_struct *work);

int hinic5_fast_msg_register_cb(void *hwdev, u8 mod, hinic5_fast_msg_rq_cb callback, void *pri_data);
void hinic5_fast_msg_unregister_cb(void *hwdev, u8 mod);
int hinic5_fast_msg_send(void *hwdev, struct hinic5_cmd_buf *cmd_buf, u64 *out_parm);

/**
 * @brief fast msg消息重新保序执行
 *
 * @param hwdev SDK句柄
 * @param src_func_id fastmsg消息的源function id
 * @param data 回调函数的私有数据
 * @param callback 回调函数指针
 *
 * @details 某个function接收到fastmsg消息之后，若消息需要等另一个function前面的消息都处理完，
 *      则fastmsg需要重新挂链确保消息在另一个function的任务里串行处理
 *
 * @return: 0 - 消息入链表成功， other - 消息入链失败
 */
int hinic5_fast_msg_forward(void *hwdev, u16 src_func_id, void *data, hinic5_fast_msg_forward_cb callback);

/**
 * @brief 获取本function是否支持fast msg
 *
 * @param hwdev SDK句柄
 *
 * @return: true - 支持， false - 不支持.
 */
bool hinic5_support_fast_msg(void *hwdev);

#endif