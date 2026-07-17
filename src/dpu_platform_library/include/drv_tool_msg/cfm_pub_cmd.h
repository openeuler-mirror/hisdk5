/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 * Description   : tool与MPU、NPU共用头
 * Creation time : 2025/11/05
 */

#ifndef CFM_PUB_CMD_H
#define CFM_PUB_CMD_H

#include "mpu_cmd_base_defs.h"
#include "qos_base_mpu_defs.h"

typedef enum tag_cfm_qos_vport_shaper_set_type {
    CFM_QOS_VPORT_SHAPER_SET_BW_INGRESS = 0,
    CFM_QOS_VPORT_SHAPER_SET_PPS_INGRESS
} cfm_qos_vport_shaper_set_type_e;

/* DFT_QOS 消息发送 */
typedef struct tag_qos_vport_req {
    struct mgmt_msg_head head; /* DFT 消息响应头 */
    cfm_qos_policing_cmd_s shaper;
} qos_vport_req_s;

/* DFT_QOS 消息返回 */
typedef struct tag_qos_vport_rsp {
    struct mgmt_msg_head head; /* DFT 消息响应头 */
    cfm_qos_policing_cmd_s bps;
    cfm_qos_policing_cmd_s pps;
} qos_vport_rsp_s;

#endif /* CFM_PUB_CMD_H */