/*
 * Copyright (c) 2016 Hisilicon Limited.
 * Copyright (c) 2007, 2008 Mellanox Technologies. All rights reserved.
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
#include "hihtr_device.h"
#include "hihtr_hw_v3.h"
#include "hihtr_common.h"
#include "hihtr_cmd.h"
#include "hihtr_cmdq.h"
#include "roce_mpu_cmd_defs.h"
#include "roce_mpu_cmd.h"
#include "hihtr_hw_v3_eq.h"

static const struct {
	const char *name;
	u8 read_bt0_op;
	u8 write_bt0_op;
} fmea_ram_res[] = {
	{ "ECC_RESOURCE_QPC",
	  HIHTR_CMD_READ_QPC_BT0, HIHTR_CMD_WRITE_QPC_BT0 },
	{ "ECC_RESOURCE_CQC",
	  HIHTR_CMD_READ_CQC_BT0, HIHTR_CMD_WRITE_CQC_BT0 },
	{ "ECC_RESOURCE_MPT",
	  HIHTR_CMD_READ_MPT_BT0, HIHTR_CMD_WRITE_MPT_BT0 },
	{ "ECC_RESOURCE_SRQC",
	  HIHTR_CMD_READ_SRQC_BT0, HIHTR_CMD_WRITE_SRQC_BT0 },
	/* ECC_RESOURCE_GMV is handled by cmdq, not mailbox */
	{ "ECC_RESOURCE_GMV",
	  0, 0 },
	{ "ECC_RESOURCE_QPC_TIMER",
	  HIHTR_CMD_READ_QPC_TIMER_BT0, HIHTR_CMD_WRITE_QPC_TIMER_BT0 },
	{ "ECC_RESOURCE_CQC_TIMER",
	  HIHTR_CMD_READ_CQC_TIMER_BT0, HIHTR_CMD_WRITE_CQC_TIMER_BT0 },
	{ "ECC_RESOURCE_SCCC",
	  HIHTR_CMD_READ_SCCC_BT0, HIHTR_CMD_WRITE_SCCC_BT0 },
};

static void init_eq_config(struct hihtr_dev *hr_dev, struct hihtr_eq *eq)
{
	eq->db_reg = hr_dev->reg_base;
	eq->cons_index = 0;
	eq->over_ignore = HIHTR_V3_EQ_OVER_IGNORE_0;
	eq->coalesce = HIHTR_V3_EQ_COALESCE_0;
	eq->arm_st = HIHTR_V3_EQ_ALWAYS_ARMED;
	eq->shift = ilog2((unsigned int)eq->entries);
}

static int config_eqc(struct hihtr_dev *hr_dev, struct hihtr_eq *eq,
		      void *mb_buf)
{
	u64 eqe_ba[MTT_MIN_COUNT] = { 0 };
	struct hihtr_eq_context *eqc;
	u64 bt_ba = 0;
	int count;

	eqc = mb_buf;
	memset(eqc, 0, sizeof(struct hihtr_eq_context));

	init_eq_config(hr_dev, eq);

	/* if not multi-hop, eqe buffer only use one trunk */
	count = hihtr_mtr_find(hr_dev, &eq->mtr, 0, eqe_ba, MTT_MIN_COUNT,
				  &bt_ba);
	if (count < 1) {
		dev_err(hr_dev->dev, "failed to find EQE mtr\n");
		return -ENOBUFS;
	}

	hr_reg_write(eqc, EQC_EQ_ST, HIHTR_V3_EQ_STATE_VALID);
	hr_reg_write(eqc, EQC_EQE_HOP_NUM, eq->hop_num);
	hr_reg_write(eqc, EQC_OVER_IGNORE, eq->over_ignore);
	hr_reg_write(eqc, EQC_COALESCE, eq->coalesce);
	hr_reg_write(eqc, EQC_ARM_ST, eq->arm_st);
	hr_reg_write(eqc, EQC_EQN, eq->eqn);
	hr_reg_write(eqc, EQC_EQE_CNT, HIHTR_EQ_INIT_EQE_CNT);
	hr_reg_write(eqc, EQC_EQE_BA_PG_SZ,
		     to_hr_hw_page_shift(eq->mtr.hem_cfg.ba_pg_shift));
	hr_reg_write(eqc, EQC_EQE_BUF_PG_SZ,
		     to_hr_hw_page_shift(eq->mtr.hem_cfg.buf_pg_shift));
	hr_reg_write(eqc, EQC_EQ_PROD_INDX, HIHTR_EQ_INIT_PROD_IDX);
	hr_reg_write(eqc, EQC_EQ_MAX_CNT, eq->eq_max_cnt);

	if (HIHTR_GET_PCI_REVISION(hr_dev) == PCI_REVISION_ID_HIP08) {
		if (eq->eq_period * HIHTR_CLOCK_ADJUST > USHRT_MAX) {
			dev_info(hr_dev->dev, "eq_period(%u) reached the upper limit, adjusted to 65.\n",
				 eq->eq_period);
			eq->eq_period = HIHTR_MAX_EQ_PERIOD;
		}
		eq->eq_period *= HIHTR_CLOCK_ADJUST;
	}

	hr_reg_write(eqc, EQC_EQ_PERIOD, eq->eq_period);
	hr_reg_write(eqc, EQC_EQE_REPORT_TIMER, HIHTR_EQ_INIT_REPORT_TIMER);
	hr_reg_write(eqc, EQC_EQE_BA_L, bt_ba >> EQC_EQE_BA_L_SHIFT);
	hr_reg_write(eqc, EQC_EQE_BA_H, bt_ba >> EQC_EQE_BA_H_SHIFT);
	hr_reg_write(eqc, EQC_SHIFT, eq->shift);
	hr_reg_write(eqc, EQC_MSI_INDX, HIHTR_EQ_INIT_MSI_IDX);
	hr_reg_write(eqc, EQC_CUR_EQE_BA_L, eqe_ba[0] >> EQC_CUR_EQE_BA_L_SHIFT);
	hr_reg_write(eqc, EQC_CUR_EQE_BA_M, eqe_ba[0] >> EQC_CUR_EQE_BA_M_SHIFT);
	hr_reg_write(eqc, EQC_CUR_EQE_BA_H, eqe_ba[0] >> EQC_CUR_EQE_BA_H_SHIFT);
	hr_reg_write(eqc, EQC_EQ_CONS_INDX, HIHTR_EQ_INIT_CONS_IDX);
	hr_reg_write(eqc, EQC_NEX_EQE_BA_L, eqe_ba[1] >> EQC_NEX_EQE_BA_L_SHIFT);
	hr_reg_write(eqc, EQC_NEX_EQE_BA_H, eqe_ba[1] >> EQC_NEX_EQE_BA_H_SHIFT);
	hr_reg_write(eqc, EQC_EQE_SIZE, eq->eqe_size == HIHTR_V3_EQE_SIZE);

	return 0;
}

static int alloc_eq_buf(struct hihtr_dev *hr_dev, struct hihtr_eq *eq)
{
	struct hihtr_buf_attr buf_attr = {};
	int err;

	if (hr_dev->caps.eqe_hop_num == HIHTR_HOP_NUM_0)
		eq->hop_num = 0;
	else
		eq->hop_num = hr_dev->caps.eqe_hop_num;

	buf_attr.page_shift = hr_dev->caps.eqe_buf_pg_sz + PAGE_SHIFT;
	buf_attr.region[0].size = eq->entries * eq->eqe_size;
	buf_attr.region[0].hopnum = eq->hop_num;
	buf_attr.region_count = 1;

	err = hihtr_mtr_create(hr_dev, &eq->mtr, &buf_attr,
				  hr_dev->caps.eqe_ba_pg_sz + PAGE_SHIFT, NULL,
				  0, NULL);
	if (err)
		dev_err(hr_dev->dev, "Failed to alloc EQE mtr, err %d\n", err);

	return err;
}


static void free_eq_buf(struct hihtr_dev *hr_dev, struct hihtr_eq *eq)
{
	hihtr_mtr_destroy(hr_dev, &eq->mtr);
}


static int hihtr_v3_create_eq(struct hihtr_dev *hr_dev,
				 struct hihtr_eq *eq, u8 eq_cmd)
{
	struct hihtr_cmd_mailbox *mailbox;
	int ret;

	/* Allocate mailbox memory */
	mailbox = hihtr_alloc_cmd_mailbox(hr_dev);
	if (IS_ERR(mailbox))
		return PTR_ERR(mailbox);

	ret = alloc_eq_buf(hr_dev, eq);
	if (ret)
		goto free_cmd_mbox;

	ret = config_eqc(hr_dev, eq, mailbox->buf);
	if (ret)
		goto err_cmd_mbox;

	ret = hihtr_cmq_mbox(hr_dev, mailbox->dma, 0, eq_cmd, eq->eqn);
	if (ret) {
		dev_err(hr_dev->dev, "[mailbox cmd] create eqc failed.\n");
		goto err_cmd_mbox;
	}

	hihtr_free_cmd_mailbox(hr_dev, mailbox);

	return 0;

err_cmd_mbox:
	free_eq_buf(hr_dev, eq);

free_cmd_mbox:
	hihtr_free_cmd_mailbox(hr_dev, mailbox);

	return ret;
}

static void hihtr_irq_work_handle(struct work_struct *work)
{
	struct hihtr_work *irq_work =
				container_of(work, struct hihtr_work, work);
	struct ib_device *ibdev = &irq_work->hr_dev->ib_dev;

	switch (irq_work->event_type) {
	case HIHTR_EVENT_TYPE_PATH_MIG:
		ibdev_info(ibdev, "Path migrated succeeded.\n");
		break;
	case HIHTR_EVENT_TYPE_PATH_MIG_FAILED:
		ibdev_warn(ibdev, "Path migration failed.\n");
		break;
	case HIHTR_EVENT_TYPE_COMM_EST:
		break;
	case HIHTR_EVENT_TYPE_SQ_DRAINED:
		ibdev_warn(ibdev, "Send queue drained.\n");
		break;
	case HIHTR_EVENT_TYPE_WQ_CATAS_ERROR:
		ibdev_err(ibdev, "Local work queue 0x%x catast error, sub_event type is: %d\n",
			  irq_work->queue_num, irq_work->sub_type);
		break;
	case HIHTR_EVENT_TYPE_INV_REQ_LOCAL_WQ_ERROR:
		ibdev_err(ibdev, "Invalid request local work queue 0x%x error.\n",
			  irq_work->queue_num);
		break;
	case HIHTR_EVENT_TYPE_LOCAL_WQ_ACCESS_ERROR:
		ibdev_err(ibdev, "Local access violation work queue 0x%x error, sub_event type is: %d\n",
			  irq_work->queue_num, irq_work->sub_type);
		break;
	case HIHTR_EVENT_TYPE_SRQ_LIMIT_REACH:
		ibdev_warn(ibdev, "SRQ limit reach.\n");
		break;
	case HIHTR_EVENT_TYPE_SRQ_LAST_WQE_REACH:
		ibdev_warn(ibdev, "SRQ last wqe reach.\n");
		break;
	case HIHTR_EVENT_TYPE_SRQ_CATAS_ERROR:
		ibdev_err(ibdev, "SRQ catas error.\n");
		break;
	case HIHTR_EVENT_TYPE_CQ_ACCESS_ERROR:
		ibdev_err(ibdev, "CQ 0x%x access err.\n", irq_work->queue_num);
		break;
	case HIHTR_EVENT_TYPE_CQ_OVERFLOW:
		ibdev_warn(ibdev, "CQ 0x%x overflow\n", irq_work->queue_num);
		break;
	case HIHTR_EVENT_TYPE_DB_OVERFLOW:
		ibdev_warn(ibdev, "DB overflow.\n");
		break;
	case HIHTR_EVENT_TYPE_FLR:
		ibdev_warn(ibdev, "Function level reset.\n");
		break;
	case HIHTR_EVENT_TYPE_XRCD_VIOLATION:
		ibdev_err(ibdev, "xrc domain violation error.\n");
		break;
	case HIHTR_EVENT_TYPE_INVALID_XRCETH:
		ibdev_err(ibdev, "invalid xrceth error.\n");
		break;
	default:
		break;
	}

	kfree(irq_work);
}

static void hihtr_v3_init_irq_work(struct hihtr_dev *hr_dev,
				      struct hihtr_eq *eq, u32 queue_num)
{
	struct hihtr_work *irq_work;

	irq_work = kzalloc(sizeof(struct hihtr_work), GFP_ATOMIC);
	if (!irq_work)
		return;

	INIT_WORK(&(irq_work->work), hihtr_irq_work_handle);
	irq_work->hr_dev = hr_dev;
	irq_work->event_type = eq->event_type;
	irq_work->sub_type = eq->sub_type;
	irq_work->queue_num = queue_num;
	queue_work(hr_dev->irq_workq, &(irq_work->work));
}

static void update_eq_db(struct hihtr_eq *eq)
{
	struct hihtr_dev *hr_dev = eq->hr_dev;
	struct hihtr_v3_db eq_db = {0};

	if (eq->type_flag == HIHTR_AEQ) {
		hr_reg_write(&eq_db, DB_EQ_CMD,
			     eq->arm_st == HIHTR_V3_EQ_ALWAYS_ARMED ?
			     HIHTR_EQ_DB_CMD_AEQ :
			     HIHTR_EQ_DB_CMD_AEQ_ARMED);
	} else {
		hr_reg_write(&eq_db, DB_EQ_EQN, eq->eqn);

		hr_reg_write(&eq_db, DB_EQ_CMD,
			     eq->arm_st == HIHTR_V3_EQ_ALWAYS_ARMED ?
			     HIHTR_EQ_DB_CMD_CEQ :
			     HIHTR_EQ_DB_CMD_CEQ_ARMED);
	}
	hr_reg_write(&eq_db, DB_SRV, HIHTR_V3_DB_EQ_SRV);
	hr_reg_write(&eq_db, DB_EQ_CI, eq->cons_index);

	wmb();
	hihtr_write64(hr_dev, (__le32 *)&eq_db, eq->db_reg);
}

static struct hihtr_ceqe *next_ceqe_sw_v3(struct hihtr_eq *eq)
{
	struct hihtr_ceqe *ceqe;

	ceqe = hihtr_buf_offset(eq->mtr.kmem,
				   (eq->cons_index & (eq->entries - 1)) *
				   eq->eqe_size);

	return (hr_reg_read(ceqe, CEQE_OWNER) ^
		!!(eq->cons_index & eq->entries)) ? ceqe : NULL;
}

static irqreturn_t hihtr_v3_ceq_int(struct hihtr_dev *hr_dev,
				       struct hihtr_eq *eq)
{
	struct hihtr_ceqe *ceqe = next_ceqe_sw_v3(eq);
	irqreturn_t ceqe_found = IRQ_NONE;
	u32 cqn;

	while (ceqe) {
		/* Make sure we read CEQ entry after we have checked the
		 * ownership bit
		 */
		dma_rmb();

		cqn = hr_reg_read(ceqe, CEQE_CQN);

		hihtr_cq_completion(hr_dev, cqn);

		++eq->cons_index;
		ceqe_found = IRQ_HANDLED;

		ceqe = next_ceqe_sw_v3(eq);
	}

	update_eq_db(eq);

	return IRQ_RETVAL(ceqe_found);
}

static struct hihtr_aeqe *next_aeqe_sw_v3(struct hihtr_eq *eq)
{
	struct hihtr_aeqe *aeqe;

	aeqe = hihtr_buf_offset(eq->mtr.kmem,
				   (eq->cons_index & (eq->entries - 1)) *
				   eq->eqe_size);

	return (hr_reg_read(aeqe, AEQE_OWNER) ^
		!!(eq->cons_index & eq->entries)) ? aeqe : NULL;
}

static irqreturn_t hihtr_v3_aeq_int(struct hihtr_dev *hr_dev,
				       struct hihtr_eq *eq)
{
	struct device *dev = hr_dev->dev;
	struct hihtr_aeqe *aeqe = next_aeqe_sw_v3(eq);
	irqreturn_t aeqe_found = IRQ_NONE;
	int event_type;
	u32 queue_num;
	int sub_type;

	while (aeqe) {
		/* Make sure we read AEQ entry after we have checked the
		 * ownership bit
		 */
		dma_rmb();

		event_type = hr_reg_read(aeqe, AEQE_EVENT_TYPE);
		sub_type = hr_reg_read(aeqe, AEQE_SUB_TYPE);
		queue_num = hr_reg_read(aeqe, AEQE_EVENT_QUEUE_NUM);

		switch (event_type) {
		case HIHTR_EVENT_TYPE_PATH_MIG:
		case HIHTR_EVENT_TYPE_PATH_MIG_FAILED:
		case HIHTR_EVENT_TYPE_COMM_EST:
		case HIHTR_EVENT_TYPE_SQ_DRAINED:
		case HIHTR_EVENT_TYPE_WQ_CATAS_ERROR:
		case HIHTR_EVENT_TYPE_SRQ_LAST_WQE_REACH:
		case HIHTR_EVENT_TYPE_INV_REQ_LOCAL_WQ_ERROR:
		case HIHTR_EVENT_TYPE_LOCAL_WQ_ACCESS_ERROR:
		case HIHTR_EVENT_TYPE_XRCD_VIOLATION:
		case HIHTR_EVENT_TYPE_INVALID_XRCETH:
			hihtr_qp_event(hr_dev, queue_num, event_type);
			break;
		case HIHTR_EVENT_TYPE_SRQ_LIMIT_REACH:
		case HIHTR_EVENT_TYPE_SRQ_CATAS_ERROR:
			hihtr_srq_event(hr_dev, queue_num, event_type);
			break;
		case HIHTR_EVENT_TYPE_CQ_ACCESS_ERROR:
		case HIHTR_EVENT_TYPE_CQ_OVERFLOW:
			hihtr_cq_event(hr_dev, queue_num, event_type);
			break;
		case HIHTR_EVENT_TYPE_MB: // TODO 确认是否需要处理
		case HIHTR_EVENT_TYPE_DB_OVERFLOW:
		case HIHTR_EVENT_TYPE_FLR:
			break;
		default:
			dev_err(dev, "Unhandled event %d on EQ %d at idx %u.\n",
				event_type, eq->eqn, eq->cons_index);
			break;
		}

		eq->event_type = event_type;
		eq->sub_type = sub_type;
		++eq->cons_index;
		aeqe_found = IRQ_HANDLED;

		hihtr_v3_init_irq_work(hr_dev, eq, queue_num);

		aeqe = next_aeqe_sw_v3(eq);
	}

	update_eq_db(eq);

	return IRQ_RETVAL(aeqe_found);
}

static irqreturn_t hihtr_v3_msix_interrupt_eq(int irq, void *eq_ptr)
{
	struct hihtr_eq *eq = eq_ptr;
	struct hihtr_dev *hr_dev = eq->hr_dev;
	irqreturn_t int_work;

	if (eq->type_flag == HIHTR_CEQ)
		/* Completion event interrupt */
		int_work = hihtr_v3_ceq_int(hr_dev, eq);
	else
		/* Asychronous event interrupt */
		int_work = hihtr_v3_aeq_int(hr_dev, eq);

	return IRQ_RETVAL(int_work);
}


static int __hihtr_request_irq(struct hihtr_dev *hr_dev, int irq_num,
				  int ceq_num, int aeq_num)
{
	struct hihtr_eq_table *eq_table = &hr_dev->eq_table;
	struct hihtr_v3_priv *priv = hr_dev->priv;
	int i, j;
	int ret, err;

	for (i = 0; i < irq_num; i++) {
		hr_dev->irq_names[i] = kzalloc(HIHTR_INT_NAME_LEN,
					       GFP_KERNEL);
		if (!hr_dev->irq_names[i]) {
			ret = -ENOMEM;
			goto err_kzalloc_failed;
		}
	}

	/* irq contains: AEQ + CEQ */
	for (j = 0; j < aeq_num; j++) {
		err = snprintf((char *)hr_dev->irq_names[j], HIHTR_INT_NAME_LEN,
			 "hihtr-aeq-%d", j);
		if (err <= 0) {
			dev_err(hr_dev->dev, "snprintf HIHTR_INT_NAME_LEN failed, err : %d\n", err);
			goto err_kzalloc_failed;
		}
	}

	for (j = aeq_num; j < irq_num; j++) {
		err = snprintf((char *)hr_dev->irq_names[j], HIHTR_INT_NAME_LEN,
			 "hihtr-ceq-%d", j - aeq_num);
		if (err <= 0) {
			dev_err(hr_dev->dev, "snprintf HIHTR_INT_NAME_LEN failed, err : %d\n", err);
			goto err_kzalloc_failed;
		}
	}

	for (j = 0; j < irq_num; j++) {
		ret = request_irq(eq_table->eq[j].irq, hihtr_v3_msix_interrupt_eq, 0,
			hr_dev->irq_names[j], &eq_table->eq[j]);
		dev_info(hr_dev->dev, "%s os_irq %d, eqn %d \n", hr_dev->irq_names[j],
			eq_table->eq[j].irq, eq_table->eq[j].eqn);
		if (ret) {
			dev_err(hr_dev->dev, "loop %d, ret %d Request irq error!\n", j,
				ret);
			goto err_request_failed;
		}
	}

	return 0;

err_request_failed:
	for (j -= 1; j >= 0; j--) {
		free_irq(eq_table->eq[j].irq, &eq_table->eq[j]);
		hinic5_free_irq(priv->lld_dev->hwdev, SERVICE_T_HIHTR, eq_table->eq[j].irq);
	}

err_kzalloc_failed:
	for (i -= 1; i >= 0; i--)
		kfree(hr_dev->irq_names[i]);

	return ret;
}

static int fmea_ram_ecc_query(struct hihtr_dev *hr_dev,
			       struct fmea_ram_ecc *ecc_info)
{
	struct hihtr_cmq_desc desc;
	struct hihtr_cmq_req *req = (struct hihtr_cmq_req *)desc.data;
	int ret;

	hihtr_cmq_setup_basic_desc(&desc, HIHTR_QUERY_RAM_ECC, true);
	ret = hihtr5_cmq_send(hr_dev, &desc, 1);
	if (ret)
		return ret;

	ecc_info->is_ecc_err = hr_reg_read(req, QUERY_RAM_ECC_1BIT_ERR);
	ecc_info->res_type = hr_reg_read(req, QUERY_RAM_ECC_RES_TYPE);
	ecc_info->index = hr_reg_read(req, QUERY_RAM_ECC_TAG);

	return 0;
}

static int fmea_recover_gmv(struct hihtr_dev *hr_dev, u32 idx)
{
	struct hihtr_cmq_desc desc;
	struct hihtr_cmq_req *req = (struct hihtr_cmq_req *)desc.data;
	u32 addr_upper;
	u32 addr_low;
	int ret;

	hihtr_cmq_setup_basic_desc(&desc, HIHTR_OPC_CFG_GMV_BT, true);
	hr_reg_write(req, CFG_GMV_BT_IDX, idx);

	ret = hihtr5_cmq_send(hr_dev, &desc, 1);
	if (ret) {
		dev_err(hr_dev->dev,
			"failed to execute cmd to read gmv, ret = %d.\n", ret);
		return ret;
	}

	addr_low =  hr_reg_read(req, CFG_GMV_BT_BA_L);
	addr_upper = hr_reg_read(req, CFG_GMV_BT_BA_H);

	hihtr_cmq_setup_basic_desc(&desc, HIHTR_OPC_CFG_GMV_BT, false);
	hr_reg_write(req, CFG_GMV_BT_BA_L, addr_low);
	hr_reg_write(req, CFG_GMV_BT_BA_H, addr_upper);
	hr_reg_write(req, CFG_GMV_BT_IDX, idx);

	return hihtr5_cmq_send(hr_dev, &desc, 1);
}

static u64 fmea_get_ram_res_addr(u32 res_type, __le64 *data)
{
	if (res_type == ECC_RESOURCE_QPC_TIMER ||
	    res_type == ECC_RESOURCE_CQC_TIMER ||
	    res_type == ECC_RESOURCE_SCCC)
		return le64_to_cpu(*data);

	return le64_to_cpu(*data) << PAGE_SHIFT;
}

static int fmea_recover_others(struct hihtr_dev *hr_dev, u32 res_type,
			       u32 index)
{
	u8 write_bt0_op = fmea_ram_res[res_type].write_bt0_op;
	u8 read_bt0_op = fmea_ram_res[res_type].read_bt0_op;
	struct hihtr_cmd_mailbox *mailbox;
	u64 addr;
	int ret;

	mailbox = hihtr_alloc_cmd_mailbox(hr_dev);
	if (IS_ERR(mailbox))
		return PTR_ERR(mailbox);

	ret = hihtr_cmq_mbox(hr_dev, 0, mailbox->dma, read_bt0_op, index);
	if (ret) {
		dev_err(hr_dev->dev,
			"failed to execute cmd to read fmea ram, ret = %d.\n",
			ret);
		goto out;
	}

	addr = fmea_get_ram_res_addr(res_type, mailbox->buf);

	ret = hihtr_cmq_mbox(hr_dev, addr, 0, write_bt0_op, index);
	if (ret)
		dev_err(hr_dev->dev,
			"failed to execute cmd to write fmea ram, ret = %d.\n",
			ret);

out:
	hihtr_free_cmd_mailbox(hr_dev, mailbox);
	return ret;
}

static void fmea_ram_ecc_recover(struct hihtr_dev *hr_dev,
				 struct fmea_ram_ecc *ecc_info)
{
	u32 res_type = ecc_info->res_type;
	u32 index = ecc_info->index;
	int ret;

	BUILD_BUG_ON(ARRAY_SIZE(fmea_ram_res) != ECC_RESOURCE_COUNT);

	if (res_type >= ECC_RESOURCE_COUNT) {
		dev_err(hr_dev->dev, "unsupported fmea ram ecc type %u.\n",
			res_type);
		return;
	}

	if (res_type == ECC_RESOURCE_GMV)
		ret = fmea_recover_gmv(hr_dev, index);
	else
		ret = fmea_recover_others(hr_dev, res_type, index);
	if (ret)
		dev_err(hr_dev->dev,
			"failed to recover %s, index = %u, ret = %d.\n",
			fmea_ram_res[res_type].name, index, ret);
}

static void fmea_ram_ecc_work(struct work_struct *ecc_work)
{
    struct hihtr_dev *hr_dev =
		container_of(ecc_work, struct hihtr_dev, ecc_work);
	struct fmea_ram_ecc ecc_info = {};

	if (fmea_ram_ecc_query(hr_dev, &ecc_info)) {
		dev_err(hr_dev->dev, "failed to query fmea ram ecc.\n");
		return;
	}

	if (!ecc_info.is_ecc_err) {
		dev_err(hr_dev->dev, "there is no fmea ram ecc err found.\n");
		return;
	}

	fmea_ram_ecc_recover(hr_dev, &ecc_info);
}

static int hihtr_v3_int_mask_enable(struct hihtr_dev *hr_dev,
					int eq_num, u32 enable_flag)
{
	struct hihtr_cmq_desc desc;
	struct hihtr_cmq_req *req = (struct hihtr_cmq_req *)desc.data;

	hihtr_cmq_setup_basic_desc(&desc, HIHTR_OPT_SET_EQ_INT_MASK, false);
	req->data[0] = eq_num;
	req->data[1] = enable_flag;
	return hihtr5_cmq_send(hr_dev, &desc, 1);
}

int hihtr_v3_cfg_irq_idx(struct hihtr_dev *hr_dev, u16 eq_idx, u16 irq_idx, u16 eq_num)
{
	int ret;
	struct hihtr_v3_priv *priv = hr_dev->priv;
	roce_set_irq_idx_cmd_s irq_idx_cmd_info;
	u16 out_size = (u16)sizeof(irq_idx_cmd_info);
	irq_idx_cmd_info.func_id = hinic5_global_func_id(priv->lld_dev->hwdev); // TODO: 适配 vf
	irq_idx_cmd_info.eq_idx = eq_idx;
	irq_idx_cmd_info.irq_idx = irq_idx;
	irq_idx_cmd_info.irq_cnt_per_func = eq_num;
	ret = hihtr_v3_msg_to_mgmt_sync(priv->lld_dev->hwdev, ROCE_MPU_CMD_CFG_IRQ_IDX, &irq_idx_cmd_info,
	    sizeof(irq_idx_cmd_info), &irq_idx_cmd_info, &out_size);
	if (ret) {
		dev_err(hr_dev->dev, "failed to cfg irq offset eq_idx %d, irq_idx %d.\n", eq_idx, irq_idx);
	}
	return ret;
}

int hihtr_v3_create_aeq(struct hihtr_dev *hr_dev, struct irq_info *irq_array, int eq_num)
{
	// 1872 only support one aeq per vf/pf
	struct device *dev = hr_dev->dev;
	struct hihtr_eq_table *eq_table = &hr_dev->eq_table;
	struct hihtr_eq *eq;
	u8 eq_cmd;
	int ret;

	eq = &eq_table->eq[0];
	eq->hr_dev = hr_dev;
	eq->eqn = 0;
	eq_cmd = HIHTR_CMD_CREATE_AEQC;
	eq->type_flag = HIHTR_AEQ;
	eq->entries = hr_dev->caps.aeqe_depth;
	eq->eqe_size = hr_dev->caps.aeqe_size;
	eq->irq = hr_dev->irq[0];
	eq->eq_max_cnt = HIHTR_AEQ_DEFAULT_BURST_NUM;
	eq->eq_period = HIHTR_AEQ_DEFAULT_INTERVAL;
	ret = hihtr_v3_create_eq(hr_dev, eq, eq_cmd);
	if (ret) {
		dev_err(dev, "failed to create eq.\n");
		goto err_create_eq_fail;
	}
	// 配置cpi的中断映射关系
	ret = hihtr_v3_cfg_irq_idx(hr_dev, 0, irq_array[0].msix_entry_idx, eq_num);
	if (ret) {
		dev_err(dev, "failed to cfg irq.\n");
		goto err_create_eq_fail;
	}
	return 0;
err_create_eq_fail:
	free_eq_buf(hr_dev, &eq_table->eq[0]);
	return -1;
}

int hihtr_v3_create_ceq(struct hihtr_dev *hr_dev, struct irq_info *irq_array, int eq_num, int aeq_num)
{
	struct device *dev = hr_dev->dev;
	struct hihtr_eq_table *eq_table = &hr_dev->eq_table;
	struct hihtr_eq *eq;
	u8 eq_cmd;
	int i;
	int ret;

	for (i = 0; i < (eq_num - aeq_num); i++) {
		eq = &eq_table->eq[i + aeq_num]; // ceq table is behind aeq table
		eq->hr_dev = hr_dev;
		eq->eqn = i; // ceqn start from 0
		eq_cmd = HIHTR_CMD_CREATE_CEQC;
		eq->type_flag = HIHTR_CEQ;
		eq->entries = hr_dev->caps.ceqe_depth;
		eq->eqe_size = hr_dev->caps.ceqe_size;
		eq->irq = hr_dev->irq[i + aeq_num]; // ceq irq is behind aeq irq
		eq->eq_max_cnt = HIHTR_CEQ_DEFAULT_BURST_NUM;
		eq->eq_period = HIHTR_CEQ_DEFAULT_INTERVAL;

		ret = hihtr_v3_create_eq(hr_dev, eq, eq_cmd);
		if (ret) {
			dev_err(dev, "failed to create eq.\n");
			goto err_create_eq_fail;
		}
		// 配置cpi的中断映射关系, eq->eqn + aeq_num is used for cpi to config interrupt merge matrix
		ret = hihtr_v3_cfg_irq_idx(hr_dev, eq->eqn + aeq_num, irq_array[i + aeq_num].msix_entry_idx, eq_num);
		if (ret) {
			dev_err(dev, "failed to cfg irq.\n");
			goto err_create_eq_fail;
		}
	}
	return 0;

err_create_eq_fail:
	for (i -= 1; i >= 0; i--)
		free_eq_buf(hr_dev, &eq_table->eq[i + aeq_num]);
	return -1;
}

int hihtr_v3_init_eq_table(struct hihtr_dev *hr_dev)
{
	struct hihtr_eq_table *eq_table = &hr_dev->eq_table;
	struct hihtr_v3_priv *priv = hr_dev->priv;
	struct device *dev = hr_dev->dev;
	struct irq_info irq_array[HIHTR_MAX_IRQ_NUM];
	struct interrupt_info info = {0};
	int ceq_num;
	int aeq_num;
	int eq_num;
	u16 resp_num_irq;
	int ret, i;

	ceq_num = hr_dev->caps.num_comp_vectors;
	aeq_num = hr_dev->caps.num_aeq_vectors;
	eq_num = ceq_num + aeq_num;
	if (eq_num <= 0) {
		dev_info(dev, "no need to alloc eq table\r\n");
		return 0;
	}
	dev_info(dev, "eq init ceq %d, aeq %d\r\n", ceq_num, aeq_num);
	ret = hinic5_alloc_irqs(priv->lld_dev->hwdev, SERVICE_T_HIHTR, eq_num,
		irq_array, &resp_num_irq);
	// no enough irq for current func to use
	if ((ret != 0) || (resp_num_irq != eq_num)) {
		dev_err(dev, "eq alloc irq cnt %d, resp irq cnt %d, ret %d\r\n", eq_num,
			resp_num_irq, ret);
		return -1;
	}

	for (i = 0; i < resp_num_irq; i++) {
		hr_dev->irq[i] = irq_array[i].irq_id;
		dev_info(dev, "eq int %d, irq %d, idx %d\r\n", i, hr_dev->irq[i],
			irq_array[i].msix_entry_idx);
	}

	eq_table->eq = kcalloc(eq_num, sizeof(*eq_table->eq), GFP_KERNEL);
	if (!eq_table->eq)
		return -ENOMEM;

	/* create aeq */
	ret = hihtr_v3_create_aeq(hr_dev, irq_array, eq_num);
	if (ret) { // eqtable source has been cleared when create failed
		goto err_create_eq_fail;
	}
	/* create ceq */
	ret = hihtr_v3_create_ceq(hr_dev, irq_array, eq_num, aeq_num);
	if (ret) { // eqtable source has been cleared when create failed
		goto err_create_eq_fail;
	}

	INIT_WORK(&hr_dev->ecc_work, fmea_ram_ecc_work);

	hr_dev->irq_workq = alloc_ordered_workqueue("hihtr_irq_workq", 0);
	if (!hr_dev->irq_workq) {
		dev_err(dev, "failed to create irq workqueue.\n");
		ret = -ENOMEM;
		goto err_create_eq_fail;
	}

	ret = __hihtr_request_irq(hr_dev, eq_num, ceq_num, aeq_num);
	if (ret) {
		dev_err(dev, "failed to request irq.\n");
		goto err_request_irq_fail;
	}

	/* enable irq */
	ret = hihtr_v3_int_mask_enable(hr_dev, eq_num, EQ_ENABLE);
	if (ret) {
		dev_err(dev, "failed to set int mask.\n");
		goto err_request_irq_fail;
	}
	// 配置msix 对应idx状态使能
	for (i = 0; i < resp_num_irq; i++) {
		hinic5_set_msix_state(priv->lld_dev->hwdev, irq_array[i].msix_entry_idx, HINIC5_MSIX_ENABLE);
	}
	// 配置中断聚合为0, lli相关配置都为全零
	info.lli_set = 1;
	info.interrupt_coalesc_set = 1;
	hinic5_set_interrupt_cfg(priv->lld_dev->hwdev, info, HINIC5_CHANNEL_HIHTR);
	return 0;

err_request_irq_fail:
	destroy_workqueue(hr_dev->irq_workq);
	for (i = 0; i < eq_num; i++)
		free_eq_buf(hr_dev, &eq_table->eq[i]);

err_create_eq_fail:
	kfree(eq_table->eq);

	return ret;
}

static void hihtr_v3_destroy_eqc(struct hihtr_dev *hr_dev, u32 eqn)
{
	struct device *dev = hr_dev->dev;
	int ret;
	u8 cmd;

	if (eqn < hr_dev->caps.num_aeq_vectors) {
		cmd = HIHTR_CMD_DESTROY_AEQC;
	} else {
		cmd = HIHTR_CMD_DESTROY_CEQC;
		eqn -= hr_dev->caps.num_aeq_vectors; // ceq的eqn也是从0开始分配
	}

	ret = hihtr_cmq_mbox(hr_dev, 0, 0, cmd, (eqn & HIHTR_V3_EQN_M));
	if (ret)
		dev_err(dev, "[mailbox cmd] destroy eqc(%u) failed.\n", eqn);
}

static void __hihtr_free_irq(struct hihtr_dev *hr_dev)
{
	struct hihtr_v3_priv *priv = hr_dev->priv;
	int eq_num;
	int i;
	eq_num = hr_dev->caps.num_comp_vectors + hr_dev->caps.num_aeq_vectors;
	for (i = 0; i < eq_num; i++) {
		dev_info(hr_dev->dev, "free %s, irq %d\r\n", hr_dev->irq_names[i], hr_dev->eq_table.eq[i].irq);
		free_irq(hr_dev->eq_table.eq[i].irq, &hr_dev->eq_table.eq[i]);
		hinic5_free_irq(priv->lld_dev->hwdev, SERVICE_T_HIHTR, hr_dev->eq_table.eq[i].irq);
	}

	for (i = 0; i < eq_num; i++)
		kfree(hr_dev->irq_names[i]);
}

void hihtr_v3_cleanup_eq_table(struct hihtr_dev *hr_dev)
{
	struct hihtr_eq_table *eq_table = &hr_dev->eq_table;
	int eq_num;
	int i;

	eq_num = hr_dev->caps.num_comp_vectors + hr_dev->caps.num_aeq_vectors;

	/* Disable irq */
	if (hihtr_v3_int_mask_enable(hr_dev, eq_num, EQ_DISABLE)) {
		dev_err(hr_dev->dev, "failed to set int mask.\n");
	}

	__hihtr_free_irq(hr_dev);
	destroy_workqueue(hr_dev->irq_workq);

	for (i = 0; i < eq_num; i++) {
		hihtr_v3_destroy_eqc(hr_dev, i);

		free_eq_buf(hr_dev, &eq_table->eq[i]);
	}

	kfree(eq_table->eq);
}

int hihtr_v3_modify_eq(struct hihtr_dev *hr_dev, struct hihtr_eq *eq,
			  u16 eq_count, u16 eq_period, u16 type)
{
	struct hihtr_eq_context *eqc;
	struct hihtr_eq_context *eqc_mask;
	struct hihtr_cmd_mailbox *mailbox;
	int ret;

	mailbox = hihtr_alloc_cmd_mailbox(hr_dev);
	if (IS_ERR(mailbox))
		return PTR_ERR(mailbox);

	eqc = mailbox->buf;
	eqc_mask = (struct hihtr_eq_context *)mailbox->buf + 1;

	memset(eqc_mask, 0xff, sizeof(*eqc_mask));

	if (type == HIHTR_EQ_MAXCNT_MASK) {
		hr_reg_write(eqc, EQC_EQ_MAX_CNT, eq_count);
		hr_reg_write(eqc_mask, EQC_EQ_MAX_CNT, 0);
	} else if (type == HIHTR_EQ_PERIOD_MASK) {
		hr_reg_write(eqc, EQC_EQ_PERIOD, eq_period);
		hr_reg_write(eqc_mask, EQC_EQ_PERIOD, 0);
	}

	ret = hihtr_cmq_mbox(hr_dev, mailbox->dma, 0,
				HIHTR_CMD_MODIFY_CEQC, eq->eqn);
	hihtr_free_cmd_mailbox(hr_dev, mailbox);
	if (ret)
		dev_err(hr_dev->dev, "MODIFY EQ Failed to cmd mailbox.\n");

	return ret;
}

int hihtr_v3_query_aeqc(struct hihtr_dev *hr_dev, void *buffer)
{
	struct hihtr_aeq_context *context;
	struct hihtr_cmd_mailbox *mailbox;
	int ret;

	mailbox = hihtr_alloc_cmd_mailbox(hr_dev);
	if (IS_ERR(mailbox))
		return PTR_ERR(mailbox);

	context = mailbox->buf;
    ret = hihtr_cmq_mbox(hr_dev, 0, mailbox->dma, HIHTR_CMD_QUERY_AEQC, 0);
    if (ret) {
        ibdev_err(&hr_dev->ib_dev, "failed to process cmd of querying AEQC, ret = %d.\n", ret);
        goto out;
    }
    memcpy(buffer, context, sizeof(*context));

out:
	hihtr_free_cmd_mailbox(hr_dev, mailbox);
	return ret;
}