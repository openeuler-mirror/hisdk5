/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2022. All rights reserved.
 ******************************************************************************
  History       :
  1.Date        : 2018/3/8
    Modification: Created file
***************************************************************************** */

#ifndef HINIC_RDMA_H__
#define HINIC_RDMA_H__

#ifndef __UEFI__

#define RDMA_ROCE_ENABLE 1
#define RDMA_IWARP_ENABLE 1
#define RDMA_ROCE_DISABLE 0
#define RDMA_IWARP_DISABLE 0

#define RDMA_MPT_FIX_BUG_LKEY 0

struct mutex;
struct tag_cqm_qpc_mpt;
struct tag_cqm_object;
struct net_device;
struct rdma_gid_entry;

#include "hinic5_cqm.h"

enum mtt_check_type_e {
    MTT_CHECK_TYPE_0 = 0,
    MTT_CHECK_TYPE_1
};

struct rdma_rdmarc {
    u32 offset;    /* 分配连续索引的首个索引 */
    u32 order;     /* 分配的rdmarc的order,代表了个数 */
    u32 ext_order; /* 包含rc表和扩展表的个数 */
    dma_addr_t dma_addr;
    void *vaddr;
};

enum rdma_ib_access {
    RDMA_IB_ACCESS_LOCAL_WRITE = 1,
    RDMA_IB_ACCESS_REMOTE_WRITE = (1 << 1),
    RDMA_IB_ACCESS_REMOTE_READ = (1 << 2),
    RDMA_IB_ACCESS_REMOTE_ATOMIC = (1 << 3),
    RDMA_IB_ACCESS_MW_BIND = (1 << 4),
    RDMA_IB_ACCESS_ZERO_BASED = (1 << 5),
    RDMA_IB_ACCESS_ON_DEMAND = (1 << 6),
};

int roce5_rdma_pd_alloc(void *hwdev, u32 *pdn);

void roce5_rdma_pd_free(void *hwdev, u32 pdn);

int roce5_rdma_rdmarc_alloc(void *hwdev, u32 num, struct rdma_rdmarc *rdmarc);

void roce5_rdma_rdmarc_free(void *hwdev, struct rdma_rdmarc *rdmarc);

/* 该接口在pf初始化时调用 */
int roce5_rdma_init_resource(void *hwdev);

/* 该接口在pf卸载时调用 */
void roce5_rdma_cleanup_resource(void *hwdev);

#endif /* __UEFI__ */

#endif /* HINIC_RDMA_H__ */
