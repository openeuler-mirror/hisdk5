/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2025 Huawei Technologies Co., Ltd */

#ifndef HINIC5_CQM_H
#define HINIC5_CQM_H

#include <linux/types.h>
#include <linux/completion.h>

#include "hinic5_crm.h"
#include "hinic5_cqm_adpt.h"
#include "hinic5_vram_api.h"

#define CQM_SUCCESS                     0       /**< Success result code */
#define CQM_FAIL                        (-1)    /**< Failure result code */
#define CQM_CONTINUE                    1       /**< Continue result code */

#define CQM_WQE_WF_NORMAL               0       /**< Normal WQE Format */
#define CQM_WQE_WF_LINK                 1       /**< Link WQE format */

#define CQM_QUEUE_LINK_MODE             0       /**< 链式队列模式 */
#define CQM_QUEUE_RING_MODE             1       /**< RING式队列模式 */
#define CQM_QUEUE_TOE_SRQ_LINK_MODE     2       /**< SRQ队列模式 */
#define CQM_QUEUE_RDMA_QUEUE_MODE       3       /**< RDMA队列模式 */

/**
 * @brief Link WQE common structure
 */
typedef struct tag_cqm_linkwqe {
    u32 rsv1 : 14;              /**< Reserved */
    u32 wf : 1;                 /**< WQE format */
    u32 rsv2 : 14;              /**< Reserved */
    u32 ctrlsl : 2;             /**< Length of the control segment */
    u32 o : 1;                  /**< Owner bit */

    u32 rsv3 : 31;              /**< Reserved */
    u32 lp : 1;                 /**< Loop Back valid */

    u32 next_page_gpa_h;        /**< 记录下一个页面的物理地址高32b,给芯片使用 */
    u32 next_page_gpa_l;        /**< 记录下一个页面的物理地址低32b,给芯片使用 */

    u32 next_buffer_addr_h;     /**< 记录下一个页面的虚拟地址高32b,给驱动使用 */
    u32 next_buffer_addr_l;     /**< 记录下一个页面的虚拟地址低32b,给驱动使用 */
} cqm_linkwqe_s;

/**
 * @brief SRQ Link WQE structure
 * @note  wqe大小需要保证不超过普通RQE大小
 */
typedef struct tag_cqm_srq_linkwqe {
    cqm_linkwqe_s linkwqe;      /**< Link WQE common data */
    u32 current_buffer_gpa_h;   /**< 记录当前页面的物理地址高32b, 驱动释放container取消映射时使用 */
    u32 current_buffer_gpa_l;   /**< 记录当前页面的物理地址低32b, 驱动释放container取消映射时使用 */
    u32 current_buffer_addr_h;  /**< 记录当前页面的虚拟地址高32b，驱动释放container时使用 */
    u32 current_buffer_addr_l;  /**< 记录当前页面的虚拟地址低32b，驱动释放container时使用 */

    u32 fast_link_page_addr_h;  /**< 记录container地址所在fastlink 页的虚拟地址高32b，驱动释放fastlink时使用 */
    u32 fast_link_page_addr_l;  /**< 记录container地址所在fastlink 页的虚拟地址低32b，驱动释放fastlink时使用 */

    u32 fixed_next_buffer_addr_h;  /**< 记录下一个contianer的虚拟地址高32b，用于驱动资源释放，驱动不可修改 */
    u32 fixed_next_buffer_addr_l;  /**< 记录下一个contianer的虚拟地址低32b，用于驱动资源释放，驱动不可修改 */
} cqm_srq_linkwqe_s;

/**
 * @brief 标准128B WQE的前64B
 */
typedef union tag_cqm_linkwqe_first64B {
    cqm_linkwqe_s basic_linkwqe;        /**< Link WQE common data */
    cqm_srq_linkwqe_s toe_srq_linkwqe;  /**< srq linkwqe结构 */
    u32 value[16];                      /**< 保留字段 */
} cqm_linkwqe_first64B_s;

/**
 * @brief 标准128B WQE的后64B
 */
typedef struct tag_cqm_linkwqe_second64B {
    u32 rsvd0[4];               /**< 第一个16B, Reserved */
    u32 rsvd1[4];               /**< 第二个16B, Reserved */

    union {
        struct {
            u32 rsvd0[3];       /**< Reserved */
            u32 rsvd1 : 29;     /**< Reserved */
            u32 toe_o : 1;      /**< TOE owner bit */
            u32 resvd2 : 2;     /**< Reserved */
        } bs;
        u32 value[4];
    } third_16B;                /**< 第三个16B */

    union {
        struct {
            u32 rsvd0[2];       /**< Reserved */
            u32 rsvd1 : 31;     /**< Reserved */
            u32 ifoe_o : 1;     /**< IFoE onwer bit */
            u32 rsvd2;          /**< Reserved */
        } bs;
        u32 value[4];
    } forth_16B;                /**< 第四个16B */
} cqm_linkwqe_second64B_s;

/**
 * @brief 标准128B WQE结构
 */
typedef struct tag_cqm_linkwqe_128B {
    cqm_linkwqe_first64B_s  first64B;   /**< 标准128B WQE的前64B */
    cqm_linkwqe_second64B_s second64B;  /**< 标准128B WQE的后64B */
} cqm_linkwqe_128B_s;

/**
 * @brief AEQ类型定义
 */
typedef enum {
    CQM_AEQ_BASE_T_NIC = 0,      /**< NIC分15个event:0~14 */
    CQM_AEQ_BASE_T_DMMU = 15,    /**< DMMU分1个event:15 */
    CQM_AEQ_BASE_T_ROCE = 16,    /**< ROCE分32个event:16~47 */
    CQM_AEQ_BASE_T_FC = 48,      /**< FC分8个event:48~55 */
    CQM_AEQ_BASE_T_IOE = 56,     /**< IOE分8个event:56~63 */
    CQM_AEQ_BASE_T_TOE = 64,     /**< TOE分16个event:64~79 */
    CQM_AEQ_BASE_T_UB = 80,      /**< UB分16个event:80~95 */
    CQM_AEQ_BASE_T_VBS = 96,     /**< VBS分16个event:96~111 */
    CQM_AEQ_BASE_T_IPSEC = 112,  /**< IPSEC分16个event:112~127 */
    CQM_AEQ_BASE_T_MAX = 128     /**< 最大定义128种event */
} cqm_aeq_event_type_e;

/**
 * @brief CQM 业务扩展描述
 */
typedef struct tag_service_register_template {
    u32 service_type;      /**< 业务类型 */
    u32 srq_ctx_size;      /**< srq context大小 */
    u32 scq_ctx_size;      /**< scq context大小 */
    void *service_handle;  /**< ceq/aeq回调函数时传给service driver的指针 */
    void (*shared_cq_ceq_callback)(void *service_handle, u32 cqn, void *cq_priv);        /**< ceq回调:shared cq */
    void (*embedded_cq_ceq_callback)(void *service_handle, u32 xid, void *qpc_priv);     /**< ceq回调:embedded cq */
    void (*no_cq_ceq_callback)(void *service_handle, u32 xid, u32 qid, void *qpc_priv);  /**< ceq回调:no cq */
    u8 (*aeq_level_callback)(void *service_handle, u8 event_type, u8 *val);              /**< aeq level回调 */
    void (*aeq_callback)(void *service_handle, u8 event_type, u8 *val);                  /**< aeq回调 */
} service_register_template_s;

/**
 * @brief CQM object type
 */
typedef enum cqm_object_type {
    CQM_OBJECT_ROOT_CTX = 0,              /**< Root context. 留在以后兼容root ctx管理 */
    CQM_OBJECT_SERVICE_CTX,               /**< QPC, Service context, 连接管理对象 */
    CQM_OBJECT_MPT,                       /**< RDMA Memory Protection Table */

    CQM_OBJECT_NONRDMA_EMBEDDED_RQ = 10,  /**< 非RDMA业务的RQ，用LINKWQE管理 */
    CQM_OBJECT_NONRDMA_EMBEDDED_SQ,       /**< 非RDMA业务的SQ，用LINKWQE管理 */
    CQM_OBJECT_NONRDMA_SRQ,               /**< 非RDMA业务的SRQ，用MTT管理，但要CQM自己申请MTT */
    CQM_OBJECT_NONRDMA_EMBEDDED_CQ,       /**< 非RDMA业务的embedded CQ，用LINKWQE管理 */
    CQM_OBJECT_NONRDMA_SCQ,               /**< 非RDMA业务的SCQ，用LINKWQE管理 */

    CQM_OBJECT_RESV = 20,                 /**< Reserved */

    CQM_OBJECT_RDMA_QP = 30,              /**< RDMA Queue Pair */
    CQM_OBJECT_RDMA_SRQ,                  /**< RDMA Shared Receive Queue */
    CQM_OBJECT_RDMA_SCQ,                  /**< RDMA Shared Completion Queue */

    CQM_OBJECT_MTT = 50,                  /**< RDMA Memory Translation Table */
    CQM_OBJECT_RDMARC,                    /**< RDMA Reliable Connection */
} cqm_object_type_e;

/**
 * @brief BITMAP表申请的失败返回值
 */
#define CQM_INDEX_INVALID ~(0U)

/**
 * @brief 新增字段的定义兼容XID=0xFFFFFFFF的默认XID申请规则 宏命名为低3bit比较位
 */
#define CQM_XID_LOW_BIT_1_1_1          0x0  /**< mask is 0x7 */
#define CQM_XID_LOW_BIT_0_1_1          0x4  /**< mask is 0x3 */
#define CQM_XID_LOW_BIT_0_1_0          0x5  /**< mask is 0x2 */
#define CQM_XID_LOW_BIT_0_0_1          0x6  /**< mask is 0x1 */
#define CQM_XID_LOW_BIT_NONE           0x7  /**< mask is 0x0 */
#define CQM_XID_SEARCH_RANGE           0x0
#define CQM_XID_SEARCH_ALL             0x1

#define CQM_XID_SEARCH_MODE_SHIFT      27
#define CQM_XID_LB_MODE_SHIFT          24
#define CQM_XID_LOW_BITS_SHIFT         21
#define CQM_XID_SEARCH_MODE_MASK       0x1
#define CQM_XID_LB_MODE_MASK           0x7
#define CQM_XID_LOW_BITS_MASK          0x7
#define CQM_DYNAMIC_XID_MASK           0x1FFFFF

/**
 * @brief 构造 XID
 * @param[in]  search_mode 搜索模式
 * @param[in]  lb_mode 负载均衡模式
 * @param[in]  xid_low XID的低两位
 *
 * @details search_mode: 0---指定XID范围查找，范围为[bp_start, bp_end)；1---整个动态区查找
 *          lb_mode:
 *          0--动态分配XID时，选择xid[2:0]=xid_low[2:0]
 *          4--动态分配XID时，选择xid[1:0]=xid_low[1:0]
 *          5--动态分配XID时，选择xid[0]=xid_low[0]
 *          6--动态分配XID时，选择xid[1]=xid_low[1]
 *          7--所有xid均可申请
 *          xid_low: 用于匹配的xid_low[2:0]
 *
 * @return 返回生成的XID
 */
#define CQM_DYNAMIC_XID_MOD(search_mode, lb_mode, xid_low) \
    ((((search_mode) & CQM_XID_SEARCH_MODE_MASK) << CQM_XID_SEARCH_MODE_SHIFT) | \
     (((lb_mode) & CQM_XID_LB_MODE_MASK) << CQM_XID_LB_MODE_SHIFT) | \
     (((xid_low) & CQM_XID_LOW_BITS_MASK) << CQM_XID_LOW_BITS_SHIFT) | CQM_DYNAMIC_XID_MASK)

#define CQM_RDMA_Q_ROOM_1 (1)           /**< 为支持ROCE的Q buffer resize，第一个Q buffer空间 */
#define CQM_RDMA_Q_ROOM_2 (2)           /**< 为支持ROCE的Q buffer resize，第二个Q buffer空间 */

#define CQM_HARDWARE_DOORBELL (1)       /**< 当前Q选择的doorbell方式，硬件doorbell */
#define CQM_SOFTWARE_DOORBELL (2)       /**< 当前Q选择的doorbell方式，软件doorbell */
#define CQM_SECURE_BUFFER_EN  (1)       /**< 标识Buffer是从安全内存中申请的 */

/**
 * @brief CQM buffer单节点结构
 */
typedef struct tag_cqm_buf_list {
    void *va;                   /**< 虚拟地址 */
    dma_addr_t pa;              /**< 物理地址 */
    u32 refcount;               /**< buf的引用计数，内部buf管理用 */
} cqm_buf_list_s;

/**
 * @brief CQM buffer单节点结构，适配WIN
 */
struct huge_buf_addr {
    void *huge_buf_vaddr;       /**< 虚拟地址 */
    dma_addr_t huge_buf_paddr;  /**< 物理地址 */
    u32 huge_buf_size;          /**< 单节点buffer的大小 */
};

/**
 * @brief CQM buffers 管理结构
 */
typedef struct tag_cqm_buf {
    cqm_buf_list_s *buf_list;   /**< buffer 链表 */
    cqm_buf_list_s direct;      /**< 将 buf_list 重新映射为连续的虚拟地址，其成员仅 va 有效 */
    u32 page_number;            /**< 总物理页数量 */
    u32 buf_number;             /**< buffer 链表长度 */
    u32 buf_size;               /**< buffer 大小 */
#ifdef __WIN__
    struct huge_buf_addr *bufs_addr;    /**< buffer链表 */
    u32 huge_buf_number;                /**< buffer链表节点个数 */
#endif
    u32 secure_mem_flag;        /**< 安全内存标志，默认为 0 (不使用安全内存) */
    struct vram_buf_info buf_info;
} cqm_buf_s;

/**
 * @brief CQM object 结构，对 context/queue/table 的抽象
 */
typedef struct tag_cqm_object {
    u32 service_type;           /**< 业务类型 */
    u32 object_type;            /**< 对象类型，如context,queue,mpt,mtt等 */
    u32 object_size;            /**< 对象大小，
                                     对于非RDMA的队列，是队列的深度；
                                     对于queue/ctx/MPT，单位Byte；
                                     对于MTT/RDMARC，单位entry个数；
                                     对于container，单位conainer个数 */
    atomic_t refcount;          /**< 引用计数 */
    struct completion free;     /**< 释放完成量 */
    void *cqm_handle;           /**< cqm_handle */
} cqm_object_s;

/**
 * @brief QPC/MPT object
 */
typedef struct tag_cqm_qpc_mpt {
    cqm_object_s object;        /**< 对象基类 */
    u32 xid;                    /**< XID.
                                     xid[20:0] < 1M 时，表示静态申请的xid;
                                     xid[20:0] 全1为动态申请;
                                     xid[22:21] 指定低2bit;
                                     xid[24:23] 为lb_mode;
                                     xid[25]为search_mode */
    dma_addr_t paddr;           /**< QPC/MTT内存的物理地址 */
    void *priv;                 /**< service driver的该对象的私有信息 */
    u8 *vaddr;                  /**< QPC/MTT内存的虚拟地址 */
} cqm_qpc_mpt_s;

/**
 * @brief queue header结构
 */
typedef struct tag_cqm_queue_header {
    u64 doorbell_record;        /**< SQ/RQ的db内容 */
    u64 ci_record;              /**< CQ的db内容 */
    u64 rsv1;                   /**< 该区域为驱动和微码传递信息的自定义区 */
    u64 rsv2;                   /**< 该区域为驱动和微码传递信息的自定义区 */
} cqm_queue_header_s;

/**
 * @brief 队列管理结构
 * @details 非 RDMA 业务，embeded 队列用 linkwqe 管理，SRQ 和 SCQ 用 MTT 管理，MTT 由 CQM 申请;
 *          RDMA 业务的队列，用 MTT 管理
 */
typedef struct tag_cqm_queue {
    cqm_object_s object;                 /**< 对象基类 */
    u32 index;                           /**< embeded队列、QP没有index，SRQ和SCQ有 */
    void *priv;                          /**< service driver的该对象的私有信息 */
    u32 current_q_doorbell;              /**< 当前queue选择的doorbell类型，roce QP同时用HW/SW */
    u32 current_q_room;                  /**< roce:当前有效的room buf */
    cqm_buf_s q_room_buf_1;              /**< nonrdma:只能选择q_room_buf_1为q_room_buf */
    cqm_buf_s q_room_buf_2;              /**< RDMA的CQ会重新分配queue room的大小 */
    cqm_queue_header_s *q_header_vaddr;  /**< queue header虚拟地址 */
    dma_addr_t q_header_paddr;           /**< queue header物理地址 */
    u8 *q_ctx_vaddr;                     /**< SRQ和SCQ的ctx虚拟地址 */
    dma_addr_t q_ctx_paddr;              /**< SRQ和SCQ的ctx物理地址 */
    u32 valid_wqe_num;                   /**< 创建成功的有效wqe个数 */
    u8 *tail_container;                  /**< SRQ container的尾指针 */
    u8 *head_container;                  /**< SRQ container的首针 */
    u8 queue_link_mode;                  /**< 队列创建时确定连接模式:link,ring等 */
} cqm_queue_s;

/**
 * @brief MTT/RDMARC管理结构
 */
typedef struct tag_cqm_mtt_rdmarc {
    cqm_object_s object;        /**< 对象基类 */
    u32 index_base;             /**< index_base */
    u32 index_number;           /**< index_number */
    u8 *vaddr;                  /**< buffer虚拟地址 */
} cqm_mtt_rdmarc_s;

/**
 * @brief 发送命令结构
 */
typedef struct tag_cqm_cmd_buf {
    void *buf;                  /**< 命令buf虚拟地址 */
    dma_addr_t dma;             /**< 命令buf物理地址 */
    u16 size;                   /**< 命令buf大小 */
} cqm_cmd_buf_s;

/**
 * @brief 发送ACK方式定义
 */
typedef enum {
    CQM_CMD_ACK_TYPE_CMDQ = 0,       /**< ack回写到cmdq */
    CQM_CMD_ACK_TYPE_SHARE_CQN = 1,  /**< ack通过root ctx的scq上报 */
    CQM_CMD_ACK_TYPE_APP_CQN = 2     /**< ack通过业务的scq上报 */
} cqm_cmd_ack_type_e;

/**
 * @brief CQM 初始化
 * @param[in]  ex_handle        设备句柄
 *
 * @return 是否成功
 *     @retval  0 success
 *     @retval -1 failure
 */
s32 cqm5_init(void *ex_handle);

/**
 * @brief CQM 反初始化
 * @param[in]  ex_handle        设备句柄
 */
void cqm5_uninit(void *ex_handle);

/**
 * @brief CQM 初始化指定 Fake VF
 * @param[in]  ex_handle        设备句柄
 * @param[in]  vf_id            待初始化的 function id
 *
 * @return 是否成功
 *     @retval  0       success
 *     @retval -1       failure
 *     @retval -EINVAL  Invalid argument
 */
int cqm5_init_fake_vf(void *ex_handle, u32 vf_id);

/**
 * @brief 注册业务扩展能力
 * @param[in]  ex_handle        设备句柄
 * @param[in]  service_template 业务扩展描述
 *
 * @return 是否成功
 *     @retval  0 success
 *     @retval -1 failure
 */
s32 cqm5_service_register(void *ex_handle, service_register_template_s *service_template);

/**
 * @brief 注销业务扩展能力
 * @param[in]  ex_handle        设备句柄
 * @param[in]  service_type     业务类型
 */
void cqm5_service_unregister(void *ex_handle, u32 service_type);

/**
 * @brief 声明设备管理的 Fake VF 数量
 * @param[in]  ex_handle        设备句柄
 * @param[in]  fake_vf_num_cfg  Fake VF 数量，该值不能大于设备支持的最大值
 *
 * @return 是否成功
 *     @retval  0 success
 *     @retval -1 failure
 */
s32 cqm5_fake_vf_num_set(void *ex_handle, u16 fake_vf_num_cfg);

/**
 * @brief 创建 FC SRQ
 * @param[in]  ex_handle        设备句柄
 * @param[in]  service_type     业务类型
 * @param[in]  object_type      对象类型
 * @param[in]  wqe_number       wqe 数目
 * @param[in]  wqe_size         wqe 大小
 * @param[in]  object_priv      对象私有数据指针
 *
 * @details 队列中有效wqe个数必须要满足传入的wqe个数。
 *          因为linkwqe只能填在页尾，真实有效个数超过需求，需要告知业务多创建的个数
 *
 * @return 队列结构指针
 */
cqm_queue_s *cqm5_object_fc_srq_create(void *ex_handle, u32 service_type,
                                       cqm_object_type_e object_type,
                                       u32 wqe_number, u32 wqe_size,
                                       void *object_priv);

/**
 * @brief 创建 RQ
 * @param[in]  ex_handle        设备句柄
 * @param[in]  service_type     业务类型
 * @param[in]  object_type      对象类型
 * @param[in]  init_rq_num      container 数目
 * @param[in]  container_size   container 大小
 * @param[in]  wqe_size         wqe 大小
 * @param[in]  object_priv      对象私有数据指针
 *
 * @details 在使用SRQ时,RQ队列创建
 *
 * @return 队列结构指针
 */
cqm_queue_s *cqm5_object_recv_queue_create(void *ex_handle, u32 service_type,
                                           cqm_object_type_e object_type,
                                           u32 init_rq_num, u32 container_size,
                                           u32 wqe_size, void *object_priv);

/**
 * @brief 创建 TOE SRQ
 * @param[in]  ex_handle        设备句柄
 * @param[in]  service_type     业务类型
 * @param[in]  object_type      对象类型
 * @param[in]  container_number container 数目
 * @param[in]  container_size   container 大小
 * @param[in]  wqe_size         wqe 大小
 *
 * @return 队列结构指针
 */
cqm_queue_s *cqm5_object_share_recv_queue_create(void *ex_handle, u32 service_type,
                                                 cqm_object_type_e object_type,
                                                 u32 container_number,
                                                 u32 container_size, u32 wqe_size);

/**
 * @brief 创建 QPC/MPT
 * @param[in]  ex_handle        设备句柄
 * @param[in]  service_type     业务类型
 * @param[in]  object_type      对象类型
 * @param[in]  object_size      对象大小，单位Byte
 * @param[in]  object_priv      对象私有数据指针
 * @param[in]  index            根据该值申请预留的qpn，如果要自动分配需填入CQM_INDEX_INVALID
 * @param[in]  bitmap_start     范围申请xid的起始index
 * @param[in]  bitmap_end       范围申请xid的结束index
 *
 * @attention 此接口可能会休眠
 *
 * @return QPC/MPT 结构指针
 */
cqm_qpc_mpt_s *cqm5_object_qpc_mpt_create(void *ex_handle, u32 service_type,
                                          cqm_object_type_e object_type,
                                          u32 object_size, void *object_priv,
                                          u32 index, u32 bitmap_start, u32 bitmap_end);

/**
 * @brief 创建非RDMA业务的队列
 * @param[in]  ex_handle        设备句柄
 * @param[in]  service_type     业务类型
 * @param[in]  object_type      对象类型
 * @param[in]  wqe_number       包含link wqe的数目
 * @param[in]  wqe_size         定长，大小为2^n
 * @param[in]  object_priv      对象私有数据指针
 *
 * @attention 此接口可能会休眠
 *
 * @return 队列结构指针
 */
cqm_queue_s *cqm5_object_nonrdma_queue_create(void *ex_handle, u32 service_type,
                                              cqm_object_type_e object_type,
                                              u32 wqe_number, u32 wqe_size,
                                              void *object_priv);

/**
 * @brief 创建RDMA业务的队列
 * @param[in]  ex_handle        设备句柄
 * @param[in]  service_type     业务类型
 * @param[in]  object_type      对象类型
 * @param[in]  object_size      对象大小
 * @param[in]  object_priv      对象私有数据指针
 * @param[in]  room_header_alloc 是否要申请queue room和header空间
 * @param[in]  xid              根据该值申请预留的qpn，如果要自动分配需填入CQM_INDEX_INVALID
 * @param[in]  bitmap_start     范围申请xid的起始index
 * @param[in]  bitmap_end       范围申请xid的结束index
 *
 * @attention 此接口可能会休眠
 *
 * @return 队列结构指针
 */
cqm_queue_s *cqm5_object_rdma_queue_create(void *ex_handle, u32 service_type,
                                           cqm_object_type_e object_type,
                                           u32 object_size, void *object_priv,
                                           bool room_header_alloc, u32 xid,
                                           u32 bitmap_start, u32 bitmap_end);

/**
 * @brief 创建RDMA业务的 MTT/RDMARC
 * @param[in]  ex_handle        设备句柄
 * @param[in]  service_type     业务类型
 * @param[in]  object_type      对象类型
 * @param[in]  index_base       起始index编号
 * @param[in]  index_number     index数量
 *
 * @return MTT/RDMARC 结构指针
 */
cqm_mtt_rdmarc_s *cqm5_object_rdma_table_get(void *ex_handle, u32 service_type,
                                            cqm_object_type_e object_type,
                                            u32 index_base, u32 index_number);

/**
 * @brief 申请一个 cmd buffer
 * @param[in]  ex_handle        设备句柄
 *
 * @attention buffer大小固定 2K，buffer 内容没有清零，需要业务清零
 *
 * @return cmd buffer 指针
 */
cqm_cmd_buf_s *cqm5_cmd_alloc(void *ex_handle);

/**
 * @brief 释放一个 cmd buffer
 * @param[in]  ex_handle        设备句柄
 * @param[in]  cmd_buf          待释放的 cmd buffer 指针
 */
void cqm5_cmd_free(void *ex_handle, cqm_cmd_buf_s *cmd_buf);

/**
 * @brief 发送 cmd
 * @param[in]  ex_handle        设备句柄
 * @param[in]  mod              模块
 * @param[in]  cmd              命令字
 * @param[in]  buf_in           输入命令 buffer
 * @param[out] buf_out          输出命令 buffer
 * @param[out] out_param        命令返回的 udata(user data)
 * @param[in]  timeout          命令超时时间，单位ms
 * @param[in]  channel          调用者 channel id
 *
 * @details 以 box 方式发送一个 cmdq cmd
 *
 * @attention 该接口会挂完成量，造成休眠
 *
 * @return 是否成功
 *     @retval  0 success
 *     @retval -1 failure
 */
s32 cqm5_send_cmd_box(void *ex_handle, u8 mod, u8 cmd,
                      cqm_cmd_buf_s *buf_in, cqm_cmd_buf_s *buf_out,
                      u64 *out_param, u32 timeout, u16 channel);

/**
 * @brief 发送 cmd
 * @param[in]  ex_handle        设备句柄
 * @param[in]  mod              模块
 * @param[in]  cmd              命令字
 * @param[in]  cos_id           CMDQ 队列
 * @param[in]  buf_in           输入命令 buffer
 * @param[out] buf_out          输出命令 buffer
 * @param[out] out_param        命令返回的 udata(user data)
 * @param[in]  timeout          命令超时时间，单位ms
 * @param[in]  channel          调用者 channel id
 *
 * @details 指定 CMDQ 队列并以 box 方式发送一个 cmdq cmd
 *
 * @attention 该接口会挂完成量，造成休眠
 *
 * @return 是否成功
 *     @retval  0 success
 *     @retval -1 failure
 */
s32 cqm5_lb_send_cmd_box(void *ex_handle, u8 mod, u8 cmd, u8 cos_id,
                         cqm_cmd_buf_s *buf_in, cqm_cmd_buf_s *buf_out,
                         u64 *out_param, u32 timeout, u16 channel);

/**
 * @brief 发送 cmd
 * @param[in]  ex_handle        设备句柄
 * @param[in]  mod              模块
 * @param[in]  cmd              命令字
 * @param[in]  buf_in           输入命令 buffer
 * @param[out] out_param        命令返回的 udata(user data)
 * @param[in]  timeout          命令超时时间，单位ms
 * @param[in]  channel          调用者 channel id
 *
 * @details 以 imm 方式发送一个 cmdq cmd
 *
 * @attention 该接口会挂完成量，造成休眠
 *
 * @return 是否成功
 *     @retval  0 success
 *     @retval -1 failure
 */
s32 cqm5_send_cmd_imm(void *ex_handle, u8 mod, u8 cmd,
                      cqm_cmd_buf_s *buf_in,
                      u64 *out_param, u32 timeout, u16 channel);

/**
 * @brief 申请硬件 doorbell 和 dwqe
 * @param[in]  ex_handle        设备句柄
 * @param[out] db_addr          doorbell 物理地址
 * @param[out] dwqe_addr        dwqe 物理地址
 *
 * @details 申请一页硬件doorbell和dwqe，具有相同的index，得到的均为物理地址，每个function最多有1K个
 *
 * @return 是否成功
 *     @retval  0 success
 *     @retval -1 failure
 */
s32 cqm5_db_addr_alloc(void *ex_handle, void __iomem **db_addr, void __iomem **dwqe_addr);

/**
 * @brief 释放硬件 doorbell 和 dwqe
 * @param[in]  ex_handle        设备句柄
 * @param[in]  db_addr          doorbell 物理地址
 * @param[in]  dwqe_addr        dwqe 物理地址
 */
void cqm5_db_addr_free(void *ex_handle, const void __iomem *db_addr, void __iomem *dwqe_addr);

/**
 * @brief 获得硬件 doorbell 虚拟地址
 * @param[in]  ex_handle        设备句柄
 * @param[in]  service_type     业务类型
 *
 * @return doorbell 虚拟地址
 */

void *cqm5_get_db_addr(void *ex_handle, u32 service_type);

/**
 * @brief 获得硬件 doorbell 物理地址
 * @param[in]  ex_handle        设备句柄
 * @param[out] addr             保存 doorbell 物理地址的指针
 * @param[in]  service_type     业务类型
 *
 * @details 获得硬件doorbell物理地址
 *
 * @return doorbell地址
 */
s32 cqm5_get_hardware_db_addr(void *ex_handle, u64 *addr, enum hinic5_service_type service_type);

/**
 * @brief Ring a hardware DB
 * @param[in]  ex_handle        设备句柄
 * @param[in]  service_type     业务类型
 * @param[in]  db_count         doorbell中超出64b的PI[7:0]
 * @param[in]  db               The content of hardware doorbell
 *
 * @return 是否成功
 *     @retval  0 success
 *     @retval -1 failure
 */
s32 cqm5_ring_hardware_db(void *ex_handle, u32 service_type, u8 db_count, u64 db);

/**
 * @brief Ring a direct wqe hardware DB to chip
 * @param[in]  ex_handle        设备句柄
 * @param[in]  service_type     业务类型
 * @param[in]  db_count         The bit[7:0] of PI can't be store in 64-bit db
 * @param[in]  direct_wqe       The content of direct_wqe
 *
 * @return 是否成功
 *     @retval  0 success
 *     @retval -1 failure
 */
s32 cqm5_ring_direct_wqe_db(void *ex_handle, u32 service_type, u8 db_count, void *direct_wqe);

/**
 * @brief Ring a software DB
 * @param[in]  ex_handle        设备句柄
 * @param[in]  object           对象指针
 * @param[in]  db_record        The content of software doorbell
 *
 * @return 是否成功
 *     @retval  0 success
 *     @retval -1 failure
 */
s32 cqm5_ring_software_db(cqm_object_s *object, u64 db_record);

/**
 * @brief bloom filter 增加引用计数
 * @param[in]  ex_handle        设备句柄
 * @param[in]  id               bloom filter id
 *
 * @details 由 0 -> 1 时发送API置位
 *
 * @attention 此接口可能会休眠
 *
 * @return 是否成功
 *     @retval  0 success
 *     @retval -1 failure
 */
s32 cqm5_bloomfilter_inc(void *ex_handle, u16 func_id, u64 id);

/**
 * @brief bloom filter 减少引用计数
 * @param[in]  ex_handle        设备句柄
 * @param[in]  id               bloom filter id
 *
 * @details 减为 0 时发送 API 清零
 *
 * @attention 此接口可能会休眠
 *
 * @return 是否成功
 *     @retval  0 success
 *     @retval -1 failure
 */
s32 cqm5_bloomfilter_dec(void *ex_handle, u16 func_id, u64 id);

/**
 * @brief 获取 SMF Timer spoke list 的基址
 * @param[in]  ex_handle        设备句柄
 *
 * @return 虚拟地址
 */
void *cqm5_timer_base(void *ex_handle);

/**
 * @brief 清零 SMF Timer spoke list
 * @param[in]  ex_handle        设备句柄
 * @param[in]  function_id      function id
 */
void cqm5_function_timer_clear(void *ex_handle, u32 function_id);

/**
 * @brief 清零 hash buffer
 * @param[in]  ex_handle        设备句柄
 * @param[in]  global_funcid    function id
 */
void cqm5_function_hash_buf_clear(void *ex_handle, s32 global_funcid);

/**
 * @brief SRQ 申请新的 container，创建后好挂链
 * @param[in]  common           队列结构指针
 *
 * @return 是否成功
 *     @retval  0 success
 *     @retval -1 failure
 */
s32 cqm5_object_share_recv_queue_add_container(cqm_queue_s *common);

/**
 * @brief SRQ 申请新的 container，创建后不挂链，由业务完成挂链
 * @param[in]  common           队列结构指针
 * @param[out] container_addr   返回的 container 地址
 *
 * @return 是否成功
 *     @retval  0 success
 *     @retval -1 failure
 */
s32 cqm5_object_srq_add_container_free(cqm_queue_s *common, u8 **container_addr);

/**
 * @brief 通过 index 获得对象
 * @param[in]  ex_handle        设备句柄
 * @param[in]  object_type      对象类型
 * @param[in]  index            index支持qpn,mptn,scqn,srqn
 * @param[in]  bh               是否禁用中断下半部
 *
 * @return 对象指针
 */
cqm_object_s *cqm5_object_get(void *ex_handle, cqm_object_type_e object_type,
                              u32 index, bool bh);

/**
 * @brief 释放对象
 * @param[in]  object           对象指针
 */
void cqm5_object_put(cqm_object_s *object);

/**
 * @brief 删除对象
 * @param[in]  object           对象指针
 *
 * @details 删除创建的对象，该函数会休眠等待所有对该对象的操作完成才返回
 *
 * @attention 此接口可能会休眠
 */
void cqm5_object_delete(cqm_object_s *object);

/**
 * @brief 获得对象的所属 function ID
 * @param[in]  object           对象指针
 *
 * @return
 *      @retval >=0 function ID
 *      @retval -1 失败
 */
s32 cqm5_object_funcid(cqm_object_s *object);

/**
 * @brief 给对象申请一块新空间
 * @param[in]  object           对象指针
 * @param[in]  object_size      新buffer大小
 *
 * @details 目前只对roce业务有用，调整CQ的buffer大小，但cqn和cqc不变，
 *          申请新的buffer空间，不释放旧buffer空间，当前有效buffer仍为旧buffer
 *
 * @return 是否成功
 *     @retval  0 success
 *     @retval -1 failure
 */
s32 cqm5_object_resize_alloc_new(cqm_object_s *object, u32 object_size);

/**
 * @brief 给对象释放新申请buffer空间
 * @param[in]  object           对象指针
 *
 * @details 本函数释放新申请buffer空间，用于业务的异常处理分支
 */
void cqm5_object_resize_free_new(cqm_object_s *object);

/**
 * @brief 给对象释旧buffer空间
 * @param[in]  object           对象指针
 *
 * @details 本函数释放旧的buffer，并将当前有效buffer设置为新buffer
 */
void cqm5_object_resize_free_old(cqm_object_s *object);

/**
 * @brief 释放container
 * @param[in]  object           对象指针
 * @param[in]  container        要释放的container指针
 *
 * @details 释放container
 */
void cqm5_srq_used_rq_container_delete(cqm_object_s *object, u8 *container);

/**
 * @brief 获得对象buffer指定偏移处的物理地址和虚拟地址
 * @param[in]  object           对象指针
 * @param[in]  offset           对于rdma table，offset为index绝对编号
 * @param[out] paddr            仅对rdma table才返回物理地址
 *
 * @details 仅支持rdma table查找，获得对象buffer指定偏移处的物理地址和虚拟地址
 *
 * @return u8 *buffer指定偏移处的虚拟地址
 */
u8 *cqm5_object_offset_addr(cqm_object_s *object, u32 offset, dma_addr_t *paddr);

/**
 * @brief 创建 DTOE SRQ
 * @param[in]  ex_handle        设备句柄
 * @param[in]  contex_size      上下文大小
 * @param[out] index_count      申请的 index 数量
 * @param[out] index            申请的 index 起始
 *
 * @return 是否成功
 *     @retval  0 success
 *     @retval -1 failure
 */
s32 cqm5_dtoe_share_recv_queue_create(void *ex_handle, u32 contex_size,
				      u32 *index_count, u32 *index);

/**
 * @brief 释放 DTOE SRQ bitmap
 * @param[in]  ex_handle        设备句柄
 * @param[in]  index_count      释放的 index 数量
 * @param[in]  index            释放的 index 起始
 */
void cqm5_dtoe_free_srq_bitmap_index(void *ex_handle, u32 index_count, u32 index);

#endif /* HINIC5_CQM_H */
