#ifndef SDK_PUB_CMD_H
#define SDK_PUB_CMD_H

#include "base_type.h"
#include "hinic5_mt.h"

/**
 * @brief struct wqe_info
 * @details 用于存储工作队列的相关信息
 */
struct cmdq_wqe_info {
    int q_id;               /**< 队列ID */
    void *rsvd;
    unsigned int wqebb_id;    /**< 工作队列元素ID */
};

#define CMDQ_WQE_DW_NUMBER (16)

struct sdk_cmdq_wqe_desc {
    u32 data[CMDQ_WQE_DW_NUMBER];
    u32 wqebb_size;
};

enum sdk_cmd_type {
    SDK_CMD_SET_FREQ_REDUCE_RATIO = SERVICE_DRV_BASE_CMD,   /**< 设置芯片时间统计的降频比 */
    SDK_CMD_SET_TIME_DIFF_ENABLE,    /**< 设置芯片时间差统计的使能 */
    SDK_CMD_GET_TIME_DIFF,           /**< 获取芯片时间差统计的差值 */
    SDK_CMD_GET_CMDQ_INFO,           /**< 获取cmdq队列的metadata信息 */
    SDK_CMD_GET_CMDQ_WQE_DESC,       /**< 获取cmdq队列的wqe信息 */
    SDK_CMD_CMDQ_CHANNEL_DETECT,     /**< cmdq通道连通性测试 */
    SDK_CMD_ATTACK_TEST,             /**< SDK 攻击测试 */

    SDK_CMD_EXTEND_RSV_START = 0x200,
    /* SDK工具保留命令字，产品使用该范围内的命令字 */
    SDK_CMD_EXTEND_RSV_END = 0x2FF,
};
#endif