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

#include <linux/types.h>
#include <rdma/ib_cache.h>
#include "hihtr_cmdq.h"
#include "hihtr_hw_v3.h"
#include "hihtr_device.h"
#include "hihtr_common.h"
#include "hihtr_cmd.h"
#include "hihtr_dca.h"
#include "hihtr_hw_v3_qp.h"
#include "hihtr_hw_v3_post.h"

static int check_recv_valid(struct hihtr_dev *hr_dev,
			    struct hihtr_qp *hr_qp)
{
	struct ib_device *ibdev = &hr_dev->ib_dev;
	struct ib_qp *ibqp = &hr_qp->ibqp;
	int ret;

	if (unlikely(ibqp->qp_type != IB_QPT_RC &&
		     ibqp->qp_type != IB_QPT_GSI &&
		     ibqp->qp_type != IB_QPT_UD)) {
		ibdev_err(ibdev, "unsupported qp type, qp_type = %d.\n",
			  ibqp->qp_type);
		return -EOPNOTSUPP;
	}

	if (unlikely(hr_dev->state >= HIHTR_DEVICE_STATE_RST_DOWN))
		return -EIO;

	if (hr_qp->state == IB_QPS_RESET)
		return -EINVAL;

	if (check_dca_attach_enable(hr_qp)) {
		ret = dca_attach_qp_buf(hr_dev, hr_qp);
		if (unlikely(ret)) {
			ibdev_err(ibdev,
				  "failed to attach DCA for QP-%lu recv!\n",
				  hr_qp->qpn);
			return ret;
		}
	}

	return 0;
}

static inline void set_data_seg_v3(struct hihtr_v3_wqe_data_seg *dseg,
				   struct ib_sge *sg)
{
	dseg->lkey = cpu_to_le32(sg->lkey);
	dseg->addr = cpu_to_le64(sg->addr);
	dseg->len  = cpu_to_le32(sg->length);
}

int fill_recv_sge_to_wqe(const struct ib_recv_wr *wr, void *wqe,
				 u32 max_sge, bool rsv)
{
	struct hihtr_v3_wqe_data_seg *dseg = wqe;
	u32 i, cnt;
	u64 data_size = 0; // 统计wr中sge携带的数据量大小
	int ret = 0;

	for (i = 0, cnt = 0; i < wr->num_sge; i++) {
		/* Skip zero-length sge */
		if (!wr->sg_list[i].length)
			continue;
		// 增加wr中sge携带消息总长度判断，确保消息长度不大于芯片规格：最大消息长度2G
		if (unlikely(data_size + wr->sg_list[i].length > HIHTR_INVALID_SGE_LENGTH)) {
			ret = -EINVAL;
			cnt = 0;
			goto invalid;
		}
		data_size += wr->sg_list[i].length;

		set_data_seg_v3(dseg + cnt, wr->sg_list + i);
		cnt++;
	}

	if (!rsv) {
		/* Clear remaining segments to make ROCEE ignore sges */
		if (cnt < max_sge)
			memset(dseg + cnt, 0,
			       (max_sge - cnt) * HIHTR_SGE_SIZE);
		return ret;
	}

invalid:
	/* Fill a reserved sge to make hw stop reading remaining segments */
	dseg[cnt].lkey = cpu_to_le32(HIHTR_INVALID_LKEY);
	dseg[cnt].addr = 0;
	dseg[cnt].len = cpu_to_le32(HIHTR_INVALID_SGE_LENGTH);
	return ret;
}

static int fill_rq_wqe(struct hihtr_qp *hr_qp, const struct ib_recv_wr *wr,
			u32 wqe_idx, u32 max_sge)
{
	int ret;
	void *wqe = NULL;

	wqe = hihtr_get_recv_wqe(hr_qp, wqe_idx);
	ret = fill_recv_sge_to_wqe(wr, wqe, max_sge, hr_qp->rq.rsv_sge);
	return ret;
}

static void update_rq_db(struct hihtr_dev *hr_dev,
				struct hihtr_qp *qp)
{
	if (unlikely(qp->state == IB_QPS_ERR)) {
		flush_cqe(hr_dev, qp);
	} else {
		if (likely(qp->en_flags & HIHTR_QP_CAP_RQ_RECORD_DB)) {
			// 1872 only support rq record db
			*qp->rdb.db_record =
					qp->rq.head & V3_DB_PRODUCER_IDX_M;
		} else {
			struct hihtr_v3_db rq_db = {};

			hr_reg_write(&rq_db, DB_TAG, qp->qpn);
			hr_reg_write(&rq_db, DB_CMD, HIHTR_V3_RQ_DB);
			hr_reg_write(&rq_db, DB_PI, qp->rq.head);

			hihtr_write64(hr_dev, (__le32 *)&rq_db,
					 qp->rq.db_reg);
		}
	}
}

int hihtr_v3_post_recv(struct ib_qp *ibqp, const struct ib_recv_wr *wr, const struct ib_recv_wr **bad_wr)
{
    struct hihtr_dev *hr_dev = to_hr_dev(ibqp->device);
	struct hihtr_qp *hr_qp = to_hr_qp(ibqp);
	struct ib_device *ibdev = &hr_dev->ib_dev;
	u32 wqe_idx, nreq, max_sge;
	unsigned long flags;
	int ret;

	ret = check_recv_valid(hr_dev, hr_qp);
	if (unlikely(ret)) {
		*bad_wr = wr;
		return ret;
	}

	spin_lock_irqsave(&hr_qp->rq.lock, flags);

	max_sge = hr_qp->rq.max_gs - hr_qp->rq.rsv_sge;
	for (nreq = 0; wr; ++nreq, wr = wr->next) {
		if (unlikely(hihtr_wq_overflow(&hr_qp->rq, nreq,
						  hr_qp->ibqp.recv_cq))) {
			ret = -ENOMEM;
			*bad_wr = wr;
			goto out;
		}

		if (unlikely(wr->num_sge > max_sge)) {
			ibdev_err(ibdev, "num_sge = %d >= max_sge = %u.\n",
				  wr->num_sge, max_sge);
			ret = -EINVAL;
			*bad_wr = wr;
			goto out;
		}

		wqe_idx = (hr_qp->rq.head + nreq) & (hr_qp->rq.wqe_cnt - 1);
		ret = fill_rq_wqe(hr_qp, wr, wqe_idx, max_sge);
		if (unlikely(ret)) {
			*bad_wr = wr;
			break;
		}
		hr_qp->rq.wrid[wqe_idx] = wr->wr_id;
	}

out:
	if (likely(nreq)) {
		hr_qp->rq.head += nreq;
		wmb(); // ensure rq wqe written into ddr
		update_rq_db(hr_dev, hr_qp);
	}
	spin_unlock_irqrestore(&hr_qp->rq.lock, flags);

	return ret;
}

/*------------------------------------- post send -------------------------------------*/

static int check_send_valid(struct hihtr_dev *hr_dev,
			    struct hihtr_qp *hr_qp)
{
	struct ib_device *ibdev = &hr_dev->ib_dev;
	struct ib_qp *ibqp = &hr_qp->ibqp;
	int ret;

	if (unlikely(ibqp->qp_type != IB_QPT_RC &&
		     ibqp->qp_type != IB_QPT_GSI &&
		     ibqp->qp_type != IB_QPT_UD)) {
		ibdev_err(ibdev, "Not supported QP(0x%x)type!\n",
			  ibqp->qp_type);
		return -EOPNOTSUPP;
	} else if (unlikely(hr_qp->state == IB_QPS_RESET ||
		   hr_qp->state == IB_QPS_INIT ||
		   hr_qp->state == IB_QPS_RTR)) {
		ibdev_err(ibdev, "failed to post WQE, QP state %u!\n",
			  hr_qp->state);
		return -EINVAL;
	} else if (unlikely(hr_dev->state >= HIHTR_DEVICE_STATE_RST_DOWN)) {
		ibdev_err(ibdev, "failed to post WQE, dev state %d!\n",
			  hr_dev->state);
		return -EIO;
	}

	if (check_dca_attach_enable(hr_qp)) {
		ret = dca_attach_qp_buf(hr_dev, hr_qp);
		if (unlikely(ret)) {
			ibdev_err(&hr_dev->ib_dev,
				  "failed to attach DCA for QP-%ld send!\n",
				  hr_qp->qpn);
			return ret;
		}
	}

	return 0;
}

static int check_sq_enabled(struct hihtr_dev *hr_dev, struct hihtr_qp *qp,
			    const struct ib_send_wr *wr, int nreq)
{
	if (hihtr_wq_overflow(&qp->sq, nreq, qp->ibqp.send_cq))
		return -ENOMEM;

	if (unlikely(wr->num_sge > qp->sq.max_gs)) {
		ibdev_err(&hr_dev->ib_dev, "num_sge=%d > qp->sq.max_gs=%u\n",
			  wr->num_sge, qp->sq.max_gs);
		return -EINVAL;
	}

	return 0;
}

static inline void update_sq_db(struct hihtr_dev *hr_dev,
				struct hihtr_qp *qp)
{
	if (unlikely(qp->state == IB_QPS_ERR)) {
		flush_cqe(hr_dev, qp);
	} else {
		struct hihtr_v3_db sq_db = {0};
		hr_reg_write(&sq_db, DB_Q_NUM, qp->doorbell_qpn);
		hr_reg_write(&sq_db, DB_CN, HIHTR_V3_DB_SQ_CN);
		hr_reg_write(&sq_db, DB_COS, qp->sl);
		hr_reg_write(&sq_db, DB_SRV, HIHTR_V3_DB_SQ_SRV);
		hr_reg_write(&sq_db, DB_SQ_PI, V3_DB_SQ_GET_PI_HIGH(qp->sq.head));
		hihtr_write64(hr_dev, (__le32 *)&sq_db, hr_dev->reg_base + (V3_DB_SQ_GET_PI_LOW(qp->sq.head) << 3)); // 3 表示 每个doorbell 8bytes
	}
}

static void hihtr_write512(struct hihtr_dev *hr_dev, u64 *val,
			      u64 __iomem *dest)
{
#define HIHTR_WRITE_TIMES 8
	int i;
	for (i = 0; i < HIHTR_WRITE_TIMES; i++) {
		writeq_relaxed(*(val + i), dest + i);
	}
#undef HIHTR_WRITE_TIMES
}

static void write_dwqe(struct hihtr_dev *hr_dev, struct hihtr_qp *qp, void *wqe,
                       int sge_num)
{
	char *dwqe_addr = (char *)((uintptr_t)(hr_dev->reg_base) + DIRECT_WQE_SHIFT);
	int i = 0;
	// do not modify content of original wqe
	struct hihtr_dwqe dwqe = {0};
	uint32_t *pdw = (uint32_t *)(&dwqe);

	if (unlikely(qp->state == IB_QPS_ERR)) {
		flush_cqe(hr_dev, qp);
		return;
	}

	memcpy(pdw, wqe, sizeof(dwqe));
	hr_reg_write(&dwqe, DIRECT_WQE_Q_NUM, qp->doorbell_qpn);
	hr_reg_write(&dwqe, DIRECT_WQE_CN, HIHTR_V3_DB_SQ_CN);
	hr_reg_write(&dwqe, DIRECT_WQE_COS, qp->sl);
	hr_reg_write(&dwqe, DIRECT_WQE_DDB, DIRECT_WQE_DB_DDB);
	hr_reg_write(&dwqe, DIRECT_WQE_SQ_PI, qp->sq.head);
	hr_reg_write(&dwqe, DIRECT_WQE_SGE_NUM, sge_num);

	// cpi accept big endian data
	for (i = 0; i < (sizeof(dwqe) / sizeof(*pdw)); i++) {
		*(pdw + i) = cpu_to_be32(*(pdw + i));
	}

	hihtr_write512(hr_dev, (u64 *)pdw, (void *)dwqe_addr);
}

static bool is_direct_wqe_supported(struct ib_qp *ibqp, const struct ib_send_wr *wr, int nreq)
{
	struct hihtr_qp *qp = to_hr_qp(ibqp);

	if (!(qp->en_flags & HIHTR_QP_CAP_DIRECT_WQE) || (nreq != 1) || (ibqp->qp_type != IB_QPT_RC)) {
		return false;
	}

	switch (wr->opcode) {
	case IB_WR_SEND:
	case IB_WR_RDMA_WRITE:
	case IB_WR_RDMA_READ:
	case IB_WR_ATOMIC_CMP_AND_SWP:
	case IB_WR_ATOMIC_FETCH_AND_ADD:
		return true;
	default:
		break;
	}
	return false;
}

int hihtr_v3_post_send(struct ib_qp *ibqp,
				 const struct ib_send_wr *wr,
				 const struct ib_send_wr **bad_wr)
{
	struct hihtr_dev *hr_dev = to_hr_dev(ibqp->device);
	struct hihtr_qp *qp = to_hr_qp(ibqp);
	const struct ib_send_wr * first_wr = wr;
	unsigned long flags = 0;
	unsigned int owner_bit;
	unsigned int sge_idx;
	unsigned int wqe_idx;
	unsigned int msg_len;
	void *wqe = NULL;
	u32 nreq;
	int ret;

	ret = check_send_valid(hr_dev, qp);
	if (unlikely(ret)) {
        dev_alert(hr_dev->dev, "check_send_valid -> false\n");
		*bad_wr = wr;
		return ret;
	}

	spin_lock_irqsave(&qp->sq.lock, flags);
	sge_idx = qp->next_sge;

	for (nreq = 0; wr; ++nreq, wr = wr->next) {
		ret = check_sq_enabled(hr_dev, qp, wr, nreq);
		if (unlikely(ret)) {
            dev_alert(hr_dev->dev, "check_sq_enabled -> false\n");
			*bad_wr = wr;
			goto out;
		}

		wqe_idx = (qp->sq.head + nreq) & (qp->sq.wqe_cnt - 1);

		wqe = hihtr_get_send_wqe(qp, wqe_idx);
		qp->sq.wrid[wqe_idx] = wr->wr_id;
		/*
		 * Save invalidate_rkey for LOCAL_INV→NOP mapping.
		 * When poll_cq detects a NOP CQE, it retrieves the saved
		 * rkey from here to perform slow-path MR invalidation.
		 */
		if (wr->opcode == IB_WR_LOCAL_INV) {
			qp->sq.inv_rkey[wqe_idx] = wr->ex.invalidate_rkey;
		}

		owner_bit =
		       ~(((qp->sq.head + nreq) >> ilog2(qp->sq.wqe_cnt)) & 0x1);

		/* Corresponding to the QP type, wqe process separately */
		if (ibqp->qp_type == IB_QPT_RC)
			ret = set_rc_wqe(qp, wr, wqe, &sge_idx, owner_bit, &msg_len);
		else {
            ret = set_ud_wqe(qp, wr, wqe, &sge_idx, owner_bit);
        }

		if (unlikely(ret)) {
            dev_alert(hr_dev->dev, "bad_wr id=%llu, opcode=%x , ret=%d\n", wr->wr_id, wr->opcode, ret);
			*bad_wr = wr;
			goto out;
		}
	}

out:
	if (likely(nreq)) {
		qp->sq.head += nreq;
		qp->next_sge = sge_idx;

		smp_wmb();
		if (is_direct_wqe_supported(ibqp, first_wr, nreq) && msg_len <= DWQE_MAX_INLINE_DATA_SIZE) {
            write_dwqe(hr_dev, qp, wqe, first_wr->num_sge);
        } else {
            update_sq_db(hr_dev, qp);
        }
	}

	spin_unlock_irqrestore(&qp->sq.lock, flags);

	return ret;
}