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

#ifndef _HIHTR_CMD_H
#define _HIHTR_CMD_H

#define HIHTR_MAILBOX_SIZE		4096
#define HIHTR_CMD_TIMEOUT_MSECS	10000

enum {
	/* QPC BT commands */
	HIHTR_CMD_WRITE_QPC_BT0	= 0x0,
	HIHTR_CMD_WRITE_QPC_BT1	= 0x1,
	HIHTR_CMD_WRITE_QPC_BT2	= 0x2,
	HIHTR_CMD_READ_QPC_BT0	= 0x4,
	HIHTR_CMD_READ_QPC_BT1	= 0x5,
	HIHTR_CMD_READ_QPC_BT2	= 0x6,
	HIHTR_CMD_DESTROY_QPC_BT0	= 0x8,
	HIHTR_CMD_DESTROY_QPC_BT1	= 0x9,
	HIHTR_CMD_DESTROY_QPC_BT2	= 0xa,

	/* QPC operation */
	HIHTR_CMD_MODIFY_QPC		= 0x41,
	HIHTR_CMD_QUERY_QPC		= 0x42,

	HIHTR_CMD_MODIFY_CQC		= 0x52,
	HIHTR_CMD_QUERY_CQC		= 0x53,
	/* CQC BT commands */
	HIHTR_CMD_WRITE_CQC_BT0	= 0x10,
	HIHTR_CMD_WRITE_CQC_BT1	= 0x11,
	HIHTR_CMD_WRITE_CQC_BT2	= 0x12,
	HIHTR_CMD_READ_CQC_BT0	= 0x14,
	HIHTR_CMD_READ_CQC_BT1	= 0x15,
	HIHTR_CMD_READ_CQC_BT2	= 0x1b,
	HIHTR_CMD_DESTROY_CQC_BT0	= 0x18,
	HIHTR_CMD_DESTROY_CQC_BT1	= 0x19,
	HIHTR_CMD_DESTROY_CQC_BT2	= 0x1a,

	/* MPT BT commands */
	HIHTR_CMD_WRITE_MPT_BT0	= 0x20,
	HIHTR_CMD_WRITE_MPT_BT1	= 0x21,
	HIHTR_CMD_WRITE_MPT_BT2	= 0x22,
	HIHTR_CMD_READ_MPT_BT0	= 0x24,
	HIHTR_CMD_READ_MPT_BT1	= 0x25,
	HIHTR_CMD_READ_MPT_BT2	= 0x26,
	HIHTR_CMD_DESTROY_MPT_BT0	= 0x28,
	HIHTR_CMD_DESTROY_MPT_BT1	= 0x29,
	HIHTR_CMD_DESTROY_MPT_BT2	= 0x2a,

	/* CQC TIMER commands */
	HIHTR_CMD_WRITE_CQC_TIMER_BT0 = 0x23,
	HIHTR_CMD_READ_CQC_TIMER_BT0  = 0x27,

	/* MPT commands */
	HIHTR_CMD_QUERY_MPT		= 0x62,

	/* SRQC BT commands */
	HIHTR_CMD_WRITE_SRQC_BT0	= 0x30,
	HIHTR_CMD_WRITE_SRQC_BT1	= 0x31,
	HIHTR_CMD_WRITE_SRQC_BT2	= 0x32,
	HIHTR_CMD_READ_SRQC_BT0	= 0x34,
	HIHTR_CMD_READ_SRQC_BT1	= 0x35,
	HIHTR_CMD_READ_SRQC_BT2	= 0x36,
	HIHTR_CMD_DESTROY_SRQC_BT0	= 0x38,
	HIHTR_CMD_DESTROY_SRQC_BT1	= 0x39,
	HIHTR_CMD_DESTROY_SRQC_BT2	= 0x3a,

	/* QPC TIMER commands */
	HIHTR_CMD_WRITE_QPC_TIMER_BT0 = 0x33,
	HIHTR_CMD_READ_QPC_TIMER_BT0  = 0x37,

	/* EQC commands */
	HIHTR_CMD_CREATE_AEQC	= 0x80,
	HIHTR_CMD_MODIFY_AEQC	= 0x81,
	HIHTR_CMD_QUERY_AEQC		= 0x82,
	HIHTR_CMD_DESTROY_AEQC	= 0x83,
	HIHTR_CMD_CREATE_CEQC	= 0x90,
	HIHTR_CMD_MODIFY_CEQC	= 0x91,
	HIHTR_CMD_QUERY_CEQC		= 0x92,
	HIHTR_CMD_DESTROY_CEQC	= 0x93,

	HIHTR_CMD_QUERY_SCCC     = 0xa2,
	/* SCC CTX BT commands */
	HIHTR_CMD_READ_SCCC_BT0	= 0xa4,
	HIHTR_CMD_WRITE_SCCC_BT0	= 0xa5,
};

enum {
	/* TPT commands */
	HIHTR_CMD_CREATE_MPT		= 0xd,
	HIHTR_CMD_DESTROY_MPT	= 0xf,

	/* CQ commands */
	HIHTR_CMD_CREATE_CQC		= 0x16,
	HIHTR_CMD_DESTROY_CQC	= 0x17,

	/* QP/EE commands */
	HIHTR_CMD_RST2INIT_QP	= 0x19,
	HIHTR_CMD_INIT2RTR_QP	= 0x1a,
	HIHTR_CMD_RTR2RTS_QP		= 0x1b,
	HIHTR_CMD_RTS2RTS_QP		= 0x1c,
	HIHTR_CMD_2ERR_QP		= 0x1e,
	HIHTR_CMD_RTS2SQD_QP		= 0x1f,
	HIHTR_CMD_SQD2RTS_QP		= 0x20,
	HIHTR_CMD_2RST_QP		= 0x21,
	HIHTR_CMD_QUERY_QP		= 0x22,
	HIHTR_CMD_SQD2SQD_QP		= 0x38,
	HIHTR_CMD_CREATE_SRQ		= 0x70,
	HIHTR_CMD_MODIFY_SRQC	= 0x72,
	HIHTR_CMD_QUERY_SRQC		= 0x73,
	HIHTR_CMD_DESTROY_SRQ	= 0x74,
};

struct hihtr_cmd_mailbox *hihtr_alloc_cmd_mailbox(struct hihtr_dev *hr_dev);
void hihtr_free_cmd_mailbox(struct hihtr_dev *hr_dev,
			       struct hihtr_cmd_mailbox *mailbox);

#endif /* _HIHTR_CMD_H */
