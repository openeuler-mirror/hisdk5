/*
 * Copyright (c) 2016 Hisilicon Limited.
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

#ifndef _HIHTR_HW_V3_CQ_H
#define _HIHTR_HW_V3_CQ_H

#include <rdma/ib_umem.h>
#include <rdma/uverbs_ioctl.h>
#include "hihtr_device.h"

#define SGE_LENGTH_ERR 8 // 参考Hi1872V100 RDMA数据结构.xlsm中subtype页签
#define CQC_CQE_BA_L_SHIFT 3
#define CQC_CQE_BA_H_SHIFT 35
#define CQC_CQE_DB_RECORD_ADDR_L_SHIFT 1
#define CQC_CQE_DB_RECORD_ADDR_H_SHIFT 32

void update_cq_db(struct hihtr_dev *hr_dev, struct hihtr_cq *hr_cq);
void __hihtr_v3_cq_clean(struct hihtr_cq *hr_cq, u32 qpn, struct hihtr_srq *srq);
void hihtr_v3_cq_clean(struct hihtr_cq *cq, unsigned int qpn, struct hihtr_srq *srq);
int hihtr_v3_query_cqc(struct hihtr_dev *hr_dev, u32 cqn, void *buffer);
void hihtr_v3_write_cqc(struct hihtr_dev *hr_dev, struct hihtr_cq *hr_cq, void *mb_buf, u64 *mtts,
    dma_addr_t dma_handle);
int hihtr_v3_poll_cq(struct ib_cq *ibcq, int num_entries, struct ib_wc *wc);
int hihtr_v3_req_notify_cq(struct ib_cq *ibcq, enum ib_cq_notify_flags flags);
int hihtr_v3_modify_cq(struct ib_cq *cq, u16 cq_count, u16 cq_period);
void hihtr_inv_work_handler(struct work_struct *work);

#endif /* _HIHTR_HW_V3_CQ_H */