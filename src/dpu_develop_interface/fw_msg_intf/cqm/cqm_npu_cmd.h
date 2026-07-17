/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2023. All rights reserved.
 * Description: cqm common command interface define.
 * Author: None
 * Create: 2015/11/13
 */
#ifndef CQM_NPU_CMD_H
#define CQM_NPU_CMD_H

typedef enum {
    CQM_CMD_T_INVALID			= 0,
    CQM_CMD_T_BAT_UPDATE		= 1,
    CQM_CMD_T_CLA_UPDATE		= 2,
    CQM_CMD_T_BLOOMFILTER_SET		= 3,
    CQM_CMD_T_BLOOMFILTER_CLEAR		= 4,
    CQM_CMD_T_COMPACT_SRQ_UPDATE	= 5,
    CQM_CMD_T_CLA_CACHE_INVALID		= 6,
    CQM_CMD_T_BLOOMFILTER_INIT		= 7,
    CQM_CMD_T_CLA_RESET			= 8, /* Reset VF's CLA */
    CQM_CMD_T_MAX
} cqm_cmd_type_e;

#endif /* CQM_NPU_CMD_H */
