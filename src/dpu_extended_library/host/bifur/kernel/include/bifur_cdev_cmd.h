/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026-2026. All rights reserved.
 * Description: hibifur5's cdev ioctl cmd related structure and macro defined here
 * Create: 2026/02/05
 */

#ifndef BIFUR_CDEV_CMD_H
#define BIFUR_CDEV_CMD_H

#define BIFUR_MSG_MAX_BUF_SIZE     (2048 * 1024)
#define	BIFUR_CMDQ_BUF_MAX_SIZE    (2048U)

enum bifur_dpdk_cmd_type {
    /* DPDK调用BIFUR驱动使用的命令字 */

    /* 队列池化 */
    GET_USER_QUEUE_ID = 0x0,              /**< 队列池化中获取用户态队列ID */
    DEL_USER_QUEUE_ID,                    /**< 队列池化中删除用户态队列ID */
    CFG_RSS_TEMPLATE,                     /**< 队列池化中申请/释放RSS模板 */
    SET_RSS_INDIR_TBL,                    /**< 队列池化中设置RSS间接表 */
    GET_RSS_INDIR_TBL,                    /**< 队列池化中获取RSS间接表 */
    GET_QUEUE_START_PA,                   /**< 队列池化中获取队列的PHY ADDR */
    GET_KERN_DEV_DATA,                    /**< 队列池化中获取队列深度和MTU */
};

struct drv_npu_cmd_st {
    uint32_t mod : 8;
    uint32_t cmd : 8;
    uint32_t ack_type : 3;
    uint32_t direct_resp : 1;
    uint32_t len : 12;
};

struct drv_mpu_cmd_st {
    uint32_t api_type : 8;
    uint32_t mod : 8;
    uint32_t cmd : 16;
};

struct bifur_ioctl_msg {
    char device_name[IFNAMSIZ];
    u32 module;
    union {
        u32 msg_format; /* for driver */
        struct drv_npu_cmd_st npu_cmd;
        struct drv_mpu_cmd_st mpu_cmd;
    };
    u32 timeout; /* for mpu/npu cmd */
    u32 func_idx;
    u32 buf_in_size;
    u32 buf_out_size;
    void *in_buf;
    void *out_buf;
    int bus_num;
    u32 lcore_id;
    u16 qid;
    u16 rsvd1;
    u32 rsvd2[3];
};

struct flow_proc_file_t {
    struct list_head node;
    pid_t pid;
    u16 lcore_id;
    u16 cnt;
    int slot;
};

struct flow_proc_file_list_t {
    struct mutex lock;
    struct list_head list;

    u8 lcore_bitmap;
};

struct drv_cmd_paddr_get {
    u16 lcore_id;
    u16 func_id;
    u16 qid;
    u8 type;
    u8 rsvd;
    unsigned long vm_start;
    unsigned long vma_size;
};

struct mag_cmd_user_queue_get {
    u16 lcore_id;
    u16 func_id;
    u16 qid;
    u16 local_qid;
};

struct mag_cmd_cfg_rss_temp {
    u16 lcore_id;
    u16 func_id;
    u16 opcode;
    u16 rsvd;
};

struct mag_cmd_rss_indir_tbl {
    u16 lcore_id;
    u16 func_id;
    u16 indir_length;
    struct nic_rss_indirect_tbl rss_indir;
};

struct tcam_ext_result_187x {
    union {
        struct {
            u32 qid : 16;
            u32 rsvd : 3;
            u32 q_grp_id : 12;
            u32 have_qgrp_id : 1;
        } bs;
        u32 qid;
        qid_u fdir_info;
    };
    u32 ext;
};

struct bifur_tcam_cfg_rule {
    u32 index;
    struct tcam_ext_result_187x data;
    struct tcam_key_x_y key;
};

struct bifur_fdir_add_rule {
    struct hinic5_mgmt_msg_head head;

    u16 func_id;
    u8 type;
    u8 fdir_ext; /* 0x1: flow bifur en bit */
    struct bifur_tcam_cfg_rule rule;
};

struct bifur_fdir_del_rule {
    struct hinic5_mgmt_msg_head msg_head;

    u16 func_id;
    u8 type;
    u8 rsvd;
    u32 index_start;
    u32 index_num;
};

struct bifur_tcam_block {
    struct hinic5_mgmt_msg_head msg_head;

    u16 func_id;
    u8  alloc_en; /* 0: free tcam block, 1: alloc tcam block */
    u8  tcam_type;
    u16 tcam_block_index;
    u16 rsvd;
};


struct drv_cmd_kernel_nic_data {
    u32 rx_q_depth;
    u32 tx_q_depth;
    u16 mtu;
    u8 link_status;
    u8 group_num;
};

#endif