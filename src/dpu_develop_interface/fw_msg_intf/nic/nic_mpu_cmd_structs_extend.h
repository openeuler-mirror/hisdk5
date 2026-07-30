#ifndef HINIC5_NIC_CMD_STRUCTS_EXTEND_H
#define HINIC5_NIC_CMD_STRUCTS_EXTEND_H

#if defined(__LINUX__) || defined(__VMWARE__)
#include <linux/types.h>
#endif

#include "mpu_cmd_base_defs.h"
#include "nic_mpu_cmd_structs.h"

#ifndef FUNC_MAX_CLEAR_QP_NUM
#define FUNC_MAX_CLEAR_QP_NUM 256
/**
 * @brief 定义了一个按队列级别清理qp res结构体
 * @details qp_num表示要清理的队列个数，qp表示要清理队列的在func下的local_id
 */
struct hinic5_cmd_clear_assign_qp_res {
    struct hinic5_mgmt_msg_head msg_head;

    u16 func_id;
    u16 qp_num;
    u32 rsvd[4];
    u16 qp[FUNC_MAX_CLEAR_QP_NUM];
};
#endif
#endif /* HINIC5_NIC_CMD_STRUCTS_EXTEND_H */