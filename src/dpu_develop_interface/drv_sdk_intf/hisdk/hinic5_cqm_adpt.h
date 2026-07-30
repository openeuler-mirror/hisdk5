/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef HINIC5_CQM_ADPT_H
#define HINIC5_CQM_ADPT_H

#define cqm5_init	cqm5_init       /**< 初始化CQM */
#define cqm5_uninit	cqm5_uninit     /**< 反初始化CQM */
#define cqm5_init_fake_vf	cqm5_init_fake_vf     /**< CQM 初始化指定 Fake VF */
#define cqm5_service_register	cqm5_service_register       /**< 注册服务 */
#define cqm5_service_unregister	cqm5_service_unregister     /**< 注销服务 */
#define cqm5_bloomfilter_dec	cqm5_bloomfilter_dec    /**< 减少布隆过滤器的计数 */
#define cqm5_bloomfilter_inc	cqm5_bloomfilter_inc    /**< 增加布隆过滤器的计数 */
#define cqm5_cmd_alloc	cqm5_cmd_alloc              /**< 分配命令 */
#define cqm5_get_hardware_db_addr cqm5_get_hardware_db_addr  /**< 获取硬件数据库地址 */
#define cqm5_cmd_free	cqm5_cmd_free               /**< 释放命令 */
#define cqm5_send_cmd_box	cqm5_send_cmd_box       /**< 发送命令盒 */
#define cqm5_lb_send_cmd_box	cqm5_lb_send_cmd_box    /**< 发送负载均衡命令盒 */
#define cqm5_send_cmd_imm	cqm5_send_cmd_imm       /**< 发送立即命令 */
#define cqm5_db_addr_alloc	cqm5_db_addr_alloc      /**< 分配数据库地址 */
#define cqm5_db_addr_free	cqm5_db_addr_free       /**< 释放数据库地址 */
#define cqm5_ring_hardware_db	cqm5_ring_hardware_db       /**< 环形硬件数据库 */
#define cqm5_ring_software_db	cqm5_ring_software_db       /**< 环形软件数据库 */
#define cqm5_object_fc_srq_create	cqm5_object_fc_srq_create       /**< 创建FC SRQ对象 */
#define cqm5_object_share_recv_queue_create	cqm5_object_share_recv_queue_create     /**< 创建共享接收队列对象 */
#define cqm5_object_share_recv_queue_add_container	cqm5_object_share_recv_queue_add_container  /**< 向共享接收队列添加容器 */
#define cqm5_object_srq_add_container_free	cqm5_object_srq_add_container_free      /**< 释放SRQ添加容器 */
#define cqm5_object_recv_queue_create	cqm5_object_recv_queue_create   /**< 创建接收队列对象 */
#define cqm5_object_qpc_mpt_create	cqm5_object_qpc_mpt_create      /**< 创建QPC MPT对象 */
#define cqm5_object_nonrdma_queue_create	cqm5_object_nonrdma_queue_create    /**< 创建非RDMA队列对象 */
#define cqm5_object_rdma_queue_create	cqm5_object_rdma_queue_create       /**< 创建RDMA队列对象 */
#define cqm5_object_rdma_table_get	cqm5_object_rdma_table_get      /**<获取RDMA表  */
#define cqm5_object_delete	cqm5_object_delete          /**< 删除对象 */
#define cqm5_object_offset_addr	cqm5_object_offset_addr /**< 获取偏移地址 */
#define cqm5_object_get	cqm5_object_get         /**< 获取对象 */
#define cqm5_object_put	cqm5_object_put         /**< 放置对象 */
#define cqm5_object_funcid	cqm5_object_funcid  /**< 获取函数ID */
#define cqm5_object_resize_alloc_new	cqm5_object_resize_alloc_new        /**< 重新分配新大小的对象 */
#define cqm5_object_resize_free_new	cqm5_object_resize_free_new         /**< 释放新大小的对象 */
#define cqm5_object_resize_free_old	cqm5_object_resize_free_old         /**< 释放旧大小的对象 */
#define cqm5_function_timer_clear	cqm5_function_timer_clear           /**< 清除函数计时器 */
#define cqm5_function_hash_buf_clear	cqm5_function_hash_buf_clear        /**< 清除函数哈希缓冲区 */
#define cqm5_srq_used_rq_container_delete	cqm5_srq_used_rq_container_delete   /**< 删除使用的RQ容器 */
#define cqm5_timer_base cqm5_timer_base      /**< 定时器基础 */
#define cqm5_dtoe_free_srq_bitmap_index cqm5_dtoe_free_srq_bitmap_index      /**< 释放SRQ位图索引 */
#define cqm5_dtoe_share_recv_queue_create cqm5_dtoe_share_recv_queue_create  /**< 创建共享接收队列 */
#define cqm5_get_db_addr                    cqm5_get_db_addr             /**< 获取数据库地址 */
#define cqm5_ring_direct_wqe_db             cqm5_ring_direct_wqe_db      /**< 直接环形WQE数据库 */
#define cqm5_fake_vf_num_set                cqm5_fake_vf_num_set         /**< 设置假的VF数量 */

#endif
