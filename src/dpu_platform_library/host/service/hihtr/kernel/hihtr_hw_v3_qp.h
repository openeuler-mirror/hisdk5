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

#ifndef _HIHTR_HW_V3_QP_H
#define _HIHTR_HW_V3_QP_H

#include "comm_defs.h"
#include "hinic5_lld.h"
#include "hinic5_hw.h"
#include "hihtr_device.h"
#include "hihtr_hw_v3.h"

#define WQE_SGE_BA_SHIFT 3
#define QPC_WQE_SGE_BA_H_SHIFT 35
#define QPC_TRRL_BA_L_SHIFT 4
#define TRRL_BA_SHIFT 20
#define QPC_TRRL_BA_H_SHIFT 52
#define IRRL_BA_SHIFT 6
#define QPC_IRRL_BA_H_SHIFT 38
#define QPC_DMAC_H_INDEX 4
#define QPC_LP_SGEN_INI_VALUE 3
#define QPC_MAILBOX_ENTRY_CNT 2
#define DEFAULT_QP_TRRL_DEPTH 128

#if defined(ROCE_DESTROY_QP_INT_HAVE_UDATA)
int hihtr_v3_destroy_qp(struct ib_qp *ibqp, struct ib_udata *udata);
#else
int hihtr_v3_destroy_qp(struct ib_qp *ibqp);
#endif

int hihtr_v3_query_qp(struct ib_qp *ibqp, struct ib_qp_attr *qp_attr, int qp_attr_mask,
    struct ib_qp_init_attr *qp_init_attr);
int hihtr_v3_query_qpc(struct hihtr_dev *hr_dev, u32 qpn, void *buffer);
int hihtr_v3_modify_qp(struct ib_qp *ibqp, const struct ib_qp_attr *attr, int attr_mask, enum ib_qp_state cur_state,
    enum ib_qp_state new_state, struct ib_udata *udata);
int hihtr_v3_qp_modify(struct hihtr_dev *hr_dev, struct hihtr_v3_qp_context *context,
                struct hihtr_v3_qp_context *qpc_mask, struct hihtr_qp *hr_qp);
int hihtr_v3_set_dca_buf(struct hihtr_dev *hr_dev, struct hihtr_qp *hr_qp);
bool hihtr_v3_chk_dca_buf_inactive(struct hihtr_dev *hr_dev, struct hihtr_qp *hr_qp);
int hihtr_hw_v3_get_cos(struct hihtr_dev *hr_dev, const struct rdma_ah_attr *ah_attr, u8 *priority);
void dca_detach_qp_buf(struct hihtr_dev *hr_dev, struct hihtr_qp *hr_qp);
int get_cur_qp(struct hihtr_cq *hr_cq, struct hihtr_v3_cqe *cqe, struct hihtr_qp **cur_qp);
int hihtr_v3_destroy_qp_common(struct hihtr_dev *hr_dev, struct hihtr_qp *hr_qp, struct ib_udata *udata);
int dca_attach_qp_buf(struct hihtr_dev *hr_dev, struct hihtr_qp *hr_qp);
bool check_dca_attach_enable(struct hihtr_qp *hr_qp);
int hihtr_v3_query_sccc(struct hihtr_dev *hr_dev, u32 qpn, void *buffer);
#endif /* _HIHTR_HW_V3_QP_H */