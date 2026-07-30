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
#ifndef _HIHTR_HW_V3_EQ_H
#define _HIHTR_HW_V3_EQ_H

#include "hihtr_device.h"

#define EQC_EQE_BA_L_SHIFT 3
#define EQC_EQE_BA_H_SHIFT 35
#define EQC_CUR_EQE_BA_L_SHIFT 12
#define EQC_CUR_EQE_BA_M_SHIFT 28
#define EQC_CUR_EQE_BA_H_SHIFT 60
#define EQC_NEX_EQE_BA_L_SHIFT 12
#define EQC_NEX_EQE_BA_H_SHIFT 44

int hihtr_v3_init_eq_table(struct hihtr_dev *hr_dev);
void hihtr_v3_cleanup_eq_table(struct hihtr_dev *hr_dev);
int hihtr_v3_modify_eq(struct hihtr_dev *hr_dev, struct hihtr_eq *eq,
			  u16 eq_count, u16 eq_period, u16 type);
int hihtr_v3_query_aeqc(struct hihtr_dev *hr_dev, void *buffer);

#endif /* _HIHTR_HW_V3_EQ_H */