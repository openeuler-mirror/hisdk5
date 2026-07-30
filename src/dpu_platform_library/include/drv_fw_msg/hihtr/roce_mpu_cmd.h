/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2022. All rights reserved.
 * Description   : RoCE MPU cmd
 * Author        : /
 * Create        : /
 * Notes         : /
 * History       : /
 */

#ifndef ROCE_MPU_CMD_H
#define ROCE_MPU_CMD_H

/* Commands between RoCE driver to MPU */
enum {
    /* FUNC CFG */
    ROCE_MPU_CMD_CFG_IRQ_IDX = 0, /**< 1872 Only, config irq idx to make cpi send right irq to host */
    ROCE_MPU_CMD_CFG_PERX_IDX,  /**< 1872 Only, config PERX ROUND BIT */
    ROCE_MPU_CMD_ADD_MCAST_NODE, /**< 1872 Only, Add multicast node to htn elb */
    ROCE_MPU_CMD_DEL_MCAST_NODE, /**< 1872 Only, Del multicast node to htn elb */
    ROCE_MPU_CMD_CFG_BYPASS,     /**< 1872 Only, config roce bypass */
    ROCE_MPU_CMD_STOP_FLOW,      /**< 1872 Only, stop flow */
    ROCE_MPU_CMD_MAX
};

#endif /* ROCE_MPU_CMD_H */