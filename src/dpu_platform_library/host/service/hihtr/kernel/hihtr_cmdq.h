/*
 * Copyright (c) 2016-2017 Hisilicon Limited.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _HIHTR_CMDQ_H
#define _HIHTR_CMDQ_H

#include "comm_defs.h"
#include "hinic5_lld.h"
#include "hinic5_hw.h"

#include "hihtr_common.h"
#include "hihtr_device.h"

#define HIHTR_CMDQ_MAX_NUM 5
#define HIHTR_CMDQ_MAX_SIZE (sizeof(struct hihtr_cmq_desc) * HIHTR_CMDQ_MAX_NUM)

struct hihtr_cmq_req {
	__le32 data[6];
};

enum {
	HIHTR_CMD_DESC_NUM_0,
	HIHTR_CMD_DESC_NUM_1,
	HIHTR_CMD_DESC_NUM_2,
	HIHTR_CMD_DESC_NUM_3,
	HIHTR_CMD_DESC_NUM_4,
	HIHTR_CMD_DESC_MAX,
};

struct hihtr_cmq_desc {
	union {
		struct {
			__le16 opcode;
			__le16 flag;    // flag赋值详见 EN_HIHTR_CMD_FLAG
			__le32 rsv;
		}; // 下发请求字段 request
		struct {
			__le16 ret_opcode;
			__le16 ret_val;
			__le16 vf_id;
			__le16 resp_tag;
		} response; // 错误返回信息检测
	};
	union {
		__le32 data[6];
		struct {
			__le32 own_func_num;
			__le32 own_mac_id;
			__le32 rsv[4];
		} func_info;
	};
};

#define CMQ_REQ_FIELD_LOC(h, l) FIELD_LOC(struct hihtr_cmq_req, h, l)
/*
 * Fields of HIHTR_OPC_QUERY_PF_RES, HIHTR_OPC_QUERY_VF_RES
 * and HIHTR_OPC_ALLOC_VF_RES
 */
#define FUNC_RES_A_VF_ID CMQ_REQ_FIELD_LOC(7, 0)
#define FUNC_RES_A_QPC_BT_IDX CMQ_REQ_FIELD_LOC(42, 32)
#define FUNC_RES_A_QPC_BT_NUM CMQ_REQ_FIELD_LOC(59, 48)
#define FUNC_RES_A_SRQC_BT_IDX CMQ_REQ_FIELD_LOC(72, 64)
#define FUNC_RES_A_SRQC_BT_NUM CMQ_REQ_FIELD_LOC(89, 80)
#define FUNC_RES_A_CQC_BT_IDX CMQ_REQ_FIELD_LOC(104, 96)
#define FUNC_RES_A_CQC_BT_NUM CMQ_REQ_FIELD_LOC(121, 112)
#define FUNC_RES_A_MPT_BT_IDX CMQ_REQ_FIELD_LOC(136, 128)
#define FUNC_RES_A_MPT_BT_NUM CMQ_REQ_FIELD_LOC(153, 144)
#define FUNC_RES_A_EQC_BT_IDX CMQ_REQ_FIELD_LOC(168, 160)
#define FUNC_RES_A_EQC_BT_NUM CMQ_REQ_FIELD_LOC(185, 176)
#define FUNC_RES_B_SMAC_IDX CMQ_REQ_FIELD_LOC(39, 32)
#define FUNC_RES_B_SMAC_NUM CMQ_REQ_FIELD_LOC(48, 40)
#define FUNC_RES_B_SGID_IDX CMQ_REQ_FIELD_LOC(71, 64)
#define FUNC_RES_B_SGID_NUM CMQ_REQ_FIELD_LOC(80, 72)
#define FUNC_RES_B_QID_IDX CMQ_REQ_FIELD_LOC(105, 96)
#define FUNC_RES_B_QID_NUM CMQ_REQ_FIELD_LOC(122, 112)
#define FUNC_RES_V_QID_NUM CMQ_REQ_FIELD_LOC(115, 112)
#define FUNC_RES_B_SCCC_BT_IDX CMQ_REQ_FIELD_LOC(136, 128)
#define FUNC_RES_B_SCCC_BT_NUM CMQ_REQ_FIELD_LOC(145, 137)
#define FUNC_RES_B_GMV_BT_IDX CMQ_REQ_FIELD_LOC(167, 160)
#define FUNC_RES_B_GMV_BT_NUM CMQ_REQ_FIELD_LOC(176, 168)
#define FUNC_RES_V_GMV_BT_NUM CMQ_REQ_FIELD_LOC(184, 176)

#define CFG_MB_IN_PARAM0 CMQ_REQ_FIELD_LOC(31, 0)
#define CFG_MB_IN_PARAM1 CMQ_REQ_FIELD_LOC(63, 32)
#define CFG_MB_OUT_PARAM0 CMQ_REQ_FIELD_LOC(95, 64)
#define CFG_MB_OUT_PARAM1 CMQ_REQ_FIELD_LOC(127, 96)
#define CFG_MB_CMD CMQ_REQ_FIELD_LOC(135, 128)
#define CFG_MB_TAG CMQ_REQ_FIELD_LOC(159, 136)
#define CFG_MB_EVENT_EN CMQ_REQ_FIELD_LOC(176, 176)

/* Fields of HIHTR_OPC_CFG_BT_ATTR */
#define CFG_BT_ATTR_QPC_BA_PGSZ CMQ_REQ_FIELD_LOC(3, 0)
#define CFG_BT_ATTR_QPC_BUF_PGSZ CMQ_REQ_FIELD_LOC(7, 4)
#define CFG_BT_ATTR_QPC_HOPNUM CMQ_REQ_FIELD_LOC(9, 8)
#define CFG_BT_ATTR_SRQC_BA_PGSZ CMQ_REQ_FIELD_LOC(35, 32)
#define CFG_BT_ATTR_SRQC_BUF_PGSZ CMQ_REQ_FIELD_LOC(39, 36)
#define CFG_BT_ATTR_SRQC_HOPNUM CMQ_REQ_FIELD_LOC(41, 40)
#define CFG_BT_ATTR_CQC_BA_PGSZ CMQ_REQ_FIELD_LOC(67, 64)
#define CFG_BT_ATTR_CQC_BUF_PGSZ CMQ_REQ_FIELD_LOC(71, 68)
#define CFG_BT_ATTR_CQC_HOPNUM CMQ_REQ_FIELD_LOC(73, 72)
#define CFG_BT_ATTR_MPT_BA_PGSZ CMQ_REQ_FIELD_LOC(99, 96)
#define CFG_BT_ATTR_MPT_BUF_PGSZ CMQ_REQ_FIELD_LOC(103, 100)
#define CFG_BT_ATTR_MPT_HOPNUM CMQ_REQ_FIELD_LOC(105, 104)
#define CFG_BT_ATTR_SCCC_BA_PGSZ CMQ_REQ_FIELD_LOC(131, 128)
#define CFG_BT_ATTR_SCCC_BUF_PGSZ CMQ_REQ_FIELD_LOC(135, 132)
#define CFG_BT_ATTR_SCCC_HOPNUM CMQ_REQ_FIELD_LOC(137, 136)

#define CFG_BT_BYPASS_ENABLE CMQ_REQ_FIELD_LOC(1, 0)

#define CFG_LLM_A_BA_L CMQ_REQ_FIELD_LOC(31, 0)
#define CFG_LLM_A_BA_H CMQ_REQ_FIELD_LOC(63, 32)
#define CFG_LLM_A_DEPTH CMQ_REQ_FIELD_LOC(76, 64)
#define CFG_LLM_A_PGSZ CMQ_REQ_FIELD_LOC(83, 80)
#define CFG_LLM_A_INIT_EN CMQ_REQ_FIELD_LOC(84, 84)
#define CFG_LLM_A_HEAD_BA_L CMQ_REQ_FIELD_LOC(127, 96)
#define CFG_LLM_A_HEAD_BA_H CMQ_REQ_FIELD_LOC(147, 128)
#define CFG_LLM_A_HEAD_NXTPTR CMQ_REQ_FIELD_LOC(159, 148)
#define CFG_LLM_A_HEAD_PTR CMQ_REQ_FIELD_LOC(171, 160)
#define CFG_LLM_B_TAIL_BA_L CMQ_REQ_FIELD_LOC(31, 0)
#define CFG_LLM_B_TAIL_BA_H CMQ_REQ_FIELD_LOC(63, 32)
#define CFG_LLM_B_TAIL_PTR CMQ_REQ_FIELD_LOC(75, 64)

/* Fields of HIHTR_OPC_CFG_GLOBAL_PARAM */
#define CFG_GLOBAL_PARAM_1US_CYCLES CMQ_REQ_FIELD_LOC(9, 0)
#define CFG_GLOBAL_PARAM_UDP_PORT CMQ_REQ_FIELD_LOC(31, 16)

/* Fields of HIHTR_OPC_QUERY_PF_TIMER_RES */
#define PF_TIMER_RES_QPC_ITEM_IDX CMQ_REQ_FIELD_LOC(43, 32)
#define PF_TIMER_RES_QPC_ITEM_NUM CMQ_REQ_FIELD_LOC(60, 48)
#define PF_TIMER_RES_CQC_ITEM_IDX CMQ_REQ_FIELD_LOC(74, 64)
#define PF_TIMER_RES_CQC_ITEM_NUM CMQ_REQ_FIELD_LOC(91, 80)

/* Fields of HIHTR_OPC_CFG_GMV_BT */
#define CFG_GMV_BT_BA_L CMQ_REQ_FIELD_LOC(31, 0)
#define CFG_GMV_BT_BA_H CMQ_REQ_FIELD_LOC(51, 32)
#define CFG_GMV_BT_IDX CMQ_REQ_FIELD_LOC(95, 64)

/* Fields of HIHTR_QUERY_RAM_ECC */
#define QUERY_RAM_ECC_1BIT_ERR CMQ_REQ_FIELD_LOC(31, 0)
#define QUERY_RAM_ECC_RES_TYPE CMQ_REQ_FIELD_LOC(63, 32)
#define QUERY_RAM_ECC_TAG CMQ_REQ_FIELD_LOC(95, 64)

/* Fields of HIHTR_OPC_CFG_ENTRY_SIZE */
#define CFG_HEM_ENTRY_SIZE_TYPE CMQ_REQ_FIELD_LOC(31, 0)
#define CFG_HEM_ENTRY_SIZE_VALUE CMQ_REQ_FIELD_LOC(191, 160)

/* Fields of HIHTR_OPC_CFG_GMV_BT */
#define CFG_GMV_BT_BA_L CMQ_REQ_FIELD_LOC(31, 0)
#define CFG_GMV_BT_BA_H CMQ_REQ_FIELD_LOC(51, 32)
#define CFG_GMV_BT_IDX CMQ_REQ_FIELD_LOC(95, 64)

/* Fields of HIHTR_OPC_CFG_CNP_TX_PERIOD */
#define CFG_CNP_TX_PERIOD CMQ_REQ_FIELD_LOC(15, 0)

enum {
	HIHTR_CFG_QPC_SIZE = BIT(0),
	HIHTR_CFG_SCCC_SIZE = BIT(1),
};

int hihtr5_cmq_send(struct hihtr_dev *hr_dev,
	struct hihtr_cmq_desc *desc, int num);
int hihtr_cmq_mbox(struct hihtr_dev *hr_dev, u64 in_param, u64 out_param,
    u8 cmd, unsigned long tag);

#define hihtr_v3_msg_to_mgmt_sync(hwdev, cmd, buf_in, in_size, buf_out, out_size) \
    hinic5_msg_to_mgmt_sync(hwdev, HINIC5_MOD_HIHTR, cmd, buf_in, in_size, buf_out, out_size, 0, HINIC5_CHANNEL_HIHTR)
#endif /* _HIHTR_CMDQ_H */
