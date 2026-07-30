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

#include <asm/atomic.h>
#include "hihtr_device.h"
#include "hihtr_cmd.h"
#include "hihtr_hw_v3.h"
#include "hihtr_hw_v3_post.h"
#include "hihtr_hw_v3_srq.h"

static int hihtr_v3_write_srqc_index_queue(struct hihtr_srq *srq,
					      struct hihtr_srq_context *ctx)
{
	struct hihtr_idx_que *idx_que = &srq->idx_que;
	struct ib_device *ibdev = srq->ibsrq.device;
	struct hihtr_dev *hr_dev = to_hr_dev(ibdev);
	u64 mtts_idx[MTT_MIN_COUNT] = {};
	dma_addr_t dma_handle_idx = 0;
	int ret;

	/* Get physical address of idx que buf */
	ret = hihtr_mtr_find(hr_dev, &idx_que->mtr, 0, mtts_idx,
				ARRAY_SIZE(mtts_idx), &dma_handle_idx);
	if (ret < 1) {
		ibdev_err(ibdev, "failed to find mtr for SRQ idx, ret = %d.\n",
			  ret);
		return -ENOBUFS;
	}

	hr_reg_write(ctx, SRQC_IDX_HOP_NUM,
		     to_hr_hem_hopnum(hr_dev->caps.idx_hop_num, srq->wqe_cnt));

	hr_reg_write(ctx, SRQC_IDX_BT_BA_L, dma_handle_idx >> DMA_IDX_SHIFT);
	hr_reg_write(ctx, SRQC_IDX_BT_BA_H,
		     upper_32_bits(dma_handle_idx >> DMA_IDX_SHIFT));

	hr_reg_write(ctx, SRQC_IDX_BA_PG_SZ,
		     to_hr_hw_page_shift(idx_que->mtr.hem_cfg.ba_pg_shift));
	hr_reg_write(ctx, SRQC_IDX_BUF_PG_SZ,
		     to_hr_hw_page_shift(idx_que->mtr.hem_cfg.buf_pg_shift));

	hr_reg_write(ctx, SRQC_IDX_CUR_BLK_ADDR_L,
		     to_hr_hw_page_addr(mtts_idx[0]));
	hr_reg_write(ctx, SRQC_IDX_CUR_BLK_ADDR_H,
		     upper_32_bits(to_hr_hw_page_addr(mtts_idx[0])));

	hr_reg_write(ctx, SRQC_IDX_NXT_BLK_ADDR_L,
		     to_hr_hw_page_addr(mtts_idx[1]));
	hr_reg_write(ctx, SRQC_IDX_NXT_BLK_ADDR_H,
		     upper_32_bits(to_hr_hw_page_addr(mtts_idx[1])));

	return 0;
}

int hihtr_v3_write_srqc(struct hihtr_srq *srq, void *mb_buf)
{
    struct ib_device *ibdev = srq->ibsrq.device;
	struct hihtr_dev *hr_dev = to_hr_dev(ibdev);
	struct hihtr_srq_context *ctx = mb_buf;
	u64 mtts_wqe[MTT_MIN_COUNT] = {};
	dma_addr_t dma_handle_wqe;
	int ret;

	memset(ctx, 0, sizeof(*ctx));

	/* Get the physical address of srq buf */
	ret = hihtr_mtr_find(hr_dev, &srq->buf_mtr, 0, mtts_wqe,
				ARRAY_SIZE(mtts_wqe), &dma_handle_wqe);
	if (ret < 1) {
		ibdev_err(ibdev, "failed to find mtr for SRQ WQE, ret = %d.\n",
			  ret);
		return ENOBUFS;
	}

	dma_handle_wqe = hihtr_get_mtr_ba(&srq->buf_mtr);

	hr_reg_write(ctx, SRQC_SRQ_ST, 1);
	hr_reg_write_bool(ctx, SRQC_SRQ_TYPE,
			  srq->ibsrq.srq_type == IB_SRQT_XRC);
	hr_reg_write(ctx, SRQC_PD, to_hr_pd(srq->ibsrq.pd)->pdn);
	hr_reg_write(ctx, SRQC_SRQN, srq->srqn);
	hr_reg_write(ctx, SRQC_XRCD, srq->xrcdn);
	hr_reg_write(ctx, SRQC_XRC_CQN, srq->cqn);
	hr_reg_write(ctx, SRQC_SHIFT, ilog2(srq->wqe_cnt));
	hr_reg_write(ctx, SRQC_RQWS,
		     srq->max_gs <= 0 ? 0 : fls(srq->max_gs - 1));

	hr_reg_write(ctx, SRQC_WQE_HOP_NUM,
		     to_hr_hem_hopnum(hr_dev->caps.srqwqe_hop_num,
				      srq->wqe_cnt));

	hr_reg_write(ctx, SRQC_WQE_BT_BA_L, dma_handle_wqe >> DMA_WQE_SHIFT);
	hr_reg_write(ctx, SRQC_WQE_BT_BA_H,
		     upper_32_bits(dma_handle_wqe >> DMA_WQE_SHIFT));

	hr_reg_write(ctx, SRQC_WQE_BA_PG_SZ,
		     to_hr_hw_page_shift(srq->buf_mtr.hem_cfg.ba_pg_shift));
	hr_reg_write(ctx, SRQC_WQE_BUF_PG_SZ,
		     to_hr_hw_page_shift(srq->buf_mtr.hem_cfg.buf_pg_shift));

	if (srq->cap_flags & HIHTR_SRQ_CAP_RECORD_DB) {
		hr_reg_enable(ctx, SRQC_DB_RECORD_EN);
		hr_reg_write(ctx, SRQC_DB_RECORD_ADDR_L,
			     lower_32_bits(srq->db.dma) >> 1);
		hr_reg_write(ctx, SRQC_DB_RECORD_ADDR_H,
			     upper_32_bits(srq->db.dma));
	}

	return hihtr_v3_write_srqc_index_queue(srq, ctx);
}

int hihtr_v3_modify_srq(struct ib_srq *ibsrq,
				  struct ib_srq_attr *srq_attr,
				  enum ib_srq_attr_mask srq_attr_mask,
				  struct ib_udata *udata)
{
    struct hihtr_dev *hr_dev = to_hr_dev(ibsrq->device);
	struct hihtr_srq *srq = to_hr_srq(ibsrq);
	struct hihtr_srq_context *srq_context;
	struct hihtr_srq_context *srqc_mask;
	struct hihtr_cmd_mailbox *mailbox;
	int ret = 0;

	/* Resizing SRQs is not supported yet */
	if (srq_attr_mask & IB_SRQ_MAX_WR) {
		ret = -EOPNOTSUPP;
		goto out;
	}

	if (srq_attr_mask & IB_SRQ_LIMIT) {
		dev_info(hr_dev->dev, "enter kernel modify srq, srq->wqe_cnt = %u\n", srq->wqe_cnt);
		if (srq_attr->srq_limit > srq->wqe_cnt) {
			ret = -EINVAL;
			goto out;
		}

		mailbox = hihtr_alloc_cmd_mailbox(hr_dev);
		if (IS_ERR(mailbox)) {
			ret = PTR_ERR(mailbox);
			goto out;
		}

		srq_context = mailbox->buf;
		srqc_mask = (struct hihtr_srq_context *)mailbox->buf + 1;

		memset(srqc_mask, 0xff, sizeof(*srqc_mask));

		hr_reg_write(srq_context, SRQC_LIMIT_WL, srq_attr->srq_limit);
		hr_reg_clear(srqc_mask, SRQC_LIMIT_WL);

		ret = hihtr_cmq_mbox(hr_dev, mailbox->dma, 0,
					HIHTR_CMD_MODIFY_SRQC, srq->srqn);
		hihtr_free_cmd_mailbox(hr_dev, mailbox);
		if (ret)
			ibdev_err(&hr_dev->ib_dev,
				  "failed to handle cmd of modifying SRQ, ret = %d.\n",
				  ret);
	}

out:
	if (ret)
	 	atomic64_inc(&hr_dev->dfx_cnt[HIHTR_DFX_SRQ_MODIFY_ERR_CNT]);
	return ret;
}

static int hihtr_srqwq_overflow(struct hihtr_srq *srq)
{
	struct hihtr_idx_que *idx_que = &srq->idx_que;
	return (idx_que->head - idx_que->tail) >= srq->wqe_cnt;
}

static int check_post_srq_valid(struct hihtr_srq *srq, u32 max_sge,
				const struct ib_recv_wr *wr)
{
	struct ib_device *ib_dev = srq->ibsrq.device;

	if (unlikely(wr->num_sge > max_sge)) {
		ibdev_err(ib_dev,
			  "failed to check sge, wr->num_sge = %d, max_sge = %u.\n",
			  wr->num_sge, max_sge);
		return -EINVAL;
	}

	if (unlikely(hihtr_srqwq_overflow(srq))) {
		ibdev_err(ib_dev,
			  "failed to check srqwq status, srqwq is full.\n");
		return -ENOMEM;
	}

	return 0;
}

static int get_srq_wqe_idx(struct hihtr_srq *srq, u32 *wqe_idx)
{
	struct hihtr_idx_que *idx_que = &srq->idx_que;
	u32 pos;

	pos = find_first_zero_bit(idx_que->bitmap, srq->wqe_cnt);
	if (unlikely(pos == srq->wqe_cnt))
		return -ENOSPC;

	bitmap_set(idx_que->bitmap, pos, 1);
	*wqe_idx = pos;
	return 0;
}

static void *get_srq_wqe_buf(struct hihtr_srq *srq, u32 n)
{
	return hihtr_buf_offset(srq->buf_mtr.kmem, n << srq->wqe_shift);
}

static void *get_idx_buf(struct hihtr_idx_que *idx_que, u32 n)
{
	return hihtr_buf_offset(idx_que->mtr.kmem,
				   n << idx_que->entry_shift);
}

static void fill_wqe_idx(struct hihtr_srq *srq, unsigned int wqe_idx)
{
	struct hihtr_idx_que *idx_que = &srq->idx_que;
	unsigned int head;
	__le32 *buf;

	head = idx_que->head & (srq->wqe_cnt - 1);

	buf = get_idx_buf(idx_que, head);
	*buf = cpu_to_le32(wqe_idx);

	idx_que->head++;
}

static void update_srq_db(struct hihtr_v3_db *db, struct hihtr_srq *srq)
{
	hr_reg_write(db, DB_TAG, srq->srqn);
	hr_reg_write(db, DB_CMD, HIHTR_V3_SRQ_DB);
	hr_reg_write(db, DB_PI, srq->idx_que.head);
}

int hihtr_v3_post_srq_recv(struct ib_srq *ibsrq,
				     const struct ib_recv_wr *wr,
				     const struct ib_recv_wr **bad_wr)
{
    struct hihtr_dev *hr_dev = to_hr_dev(ibsrq->device);
	struct hihtr_srq *srq = to_hr_srq(ibsrq);
	struct hihtr_v3_db srq_db;
	unsigned long flags;
	int ret = 0;
	u32 max_sge;
	u32 wqe_idx;
	void *wqe;
	u32 nreq;

	spin_lock_irqsave(&srq->lock, flags);

	max_sge = srq->max_gs - srq->rsv_sge;
	for (nreq = 0; wr; ++nreq, wr = wr->next) {
		ret = check_post_srq_valid(srq, max_sge, wr);
		if (ret) {
			*bad_wr = wr;
			break;
		}

		ret = get_srq_wqe_idx(srq, &wqe_idx);
		if (unlikely(ret)) {
			*bad_wr = wr;
			break;
		}

		wqe = get_srq_wqe_buf(srq, wqe_idx);

		ret = fill_recv_sge_to_wqe(wr, wqe, max_sge, srq->rsv_sge);
		if (unlikely(ret)) {
			*bad_wr = wr;
			break;
		}
		fill_wqe_idx(srq, wqe_idx);
		srq->wrid[wqe_idx] = wr->wr_id;
	}

	if (likely(nreq)) {
		wmb();
		// 1872 roce driver srq only support record doorbell
        if (srq->cap_flags & HIHTR_SRQ_CAP_RECORD_DB) {
            *srq->db.db_record = srq->idx_que.head & V3_DB_PRODUCER_IDX_M;
        } else {
            update_srq_db(&srq_db, srq);
		    hihtr_write64(hr_dev, (__le32 *)&srq_db, srq->db_reg);
        }
	}

	spin_unlock_irqrestore(&srq->lock, flags);

	return ret;
}

int hihtr_v3_query_srqc(struct hihtr_dev *hr_dev, u32 srqn,
				 void *buffer)
{
	struct hihtr_srq_context *context;
	struct hihtr_cmd_mailbox *mailbox;
	int ret;
	int count = hr_dev->srq_table.table.obj_count;

	if (srqn >= count) {
		return -EINVAL;
	}
	mailbox = hihtr_alloc_cmd_mailbox(hr_dev);
	if (IS_ERR(mailbox))
		return PTR_ERR(mailbox);

	context  = mailbox->buf;
    ret = hihtr_cmq_mbox(hr_dev, 0, mailbox->dma, HIHTR_CMD_QUERY_SRQC, srqn);
    if (ret) {
        ibdev_err(&hr_dev->ib_dev, "failed to process cmd of querying SRQC, ret = %d.\n", ret);
        goto out;
    }

    memcpy(buffer, context, sizeof(*context));

out:
	hihtr_free_cmd_mailbox(hr_dev, mailbox);
	return ret;
}

int hihtr_v3_query_srq(struct ib_srq *ibsrq, struct ib_srq_attr *attr)
{
	int ret;
	struct hihtr_dev *hr_dev = to_hr_dev(ibsrq->device);
	struct hihtr_srq *srq = to_hr_srq(ibsrq);

	struct hihtr_srq_context srq_context;
	ret = hr_dev->hw->query_srqc(hr_dev, srq->srqn, (void *)&srq_context);
	if (ret) {
		ibdev_err(&hr_dev->ib_dev,
			  "failed to process cmd of querying SRQ, ret = %d.\n",
			  ret);
		return -EBUSY;
	}
	attr->srq_limit = hr_reg_read(&srq_context, SRQC_LIMIT_WL);
	attr->max_wr = srq->wqe_cnt;
	attr->max_sge = srq->max_gs - srq->rsv_sge;

	return ret;
}