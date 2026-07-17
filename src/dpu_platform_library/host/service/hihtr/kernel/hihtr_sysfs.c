// SPDX-License-Identifier: GPL-2.0 OR Linux-OpenIB
/*
 * Copyright (c) 2023 Hisilicon Limited.
 */
#include "hihtr_hw_v3.h"
#include "hihtr_device.h"

void hihtr_add_dwqe_pid(struct hihtr_dev *hr_dev, pid_t c_pid)
{
    struct pid_node *node, *existing;

    spin_lock(&hr_dev->dwqe_pid_lock);
    list_for_each_entry(existing, &hr_dev->dwqe_pid_list, node) {
        if (existing->pid == c_pid) {
            spin_unlock(&hr_dev->dwqe_pid_lock);
            return;
        }
    }

    node = kmalloc(sizeof(*node), GFP_ATOMIC);
    if (!node) {
        spin_unlock(&hr_dev->dwqe_pid_lock);
        return;
    }

    node->pid = c_pid;
    list_add_tail(&node->node, &hr_dev->dwqe_pid_list);
    spin_unlock(&hr_dev->dwqe_pid_lock);
}

void hihtr_remove_dwqe_pid(struct hihtr_dev *hr_dev, pid_t c_pid)
{
    struct pid_node *pos, *delete_node = NULL;

    spin_lock(&hr_dev->dwqe_pid_lock);
    list_for_each_entry(pos, &hr_dev->dwqe_pid_list, node) {
        if (pos->pid == c_pid) {
            list_del(&pos->node);
            delete_node = pos;
            break;
        }
    }
    spin_unlock(&hr_dev->dwqe_pid_lock);

    if (delete_node) {
        kfree(delete_node);
    }
}

static ssize_t dwqe_pids_show(struct device *dev,
                              struct device_attribute *attr, char *buf)
{

    struct hihtr_dev *hr_dev = container_of(dev, struct hihtr_dev, ib_dev.dev);
    struct pid_node *node;
    int len = 0;

    spin_lock(&hr_dev->dwqe_pid_lock);
    list_for_each_entry(node, &hr_dev->dwqe_pid_list, node) {
		if (len >= PAGE_SIZE) break;
        len += scnprintf(buf + len, PAGE_SIZE - len, "%d\n", node->pid);
    }
    spin_unlock(&hr_dev->dwqe_pid_lock);

    return len;
}

static ssize_t coalesce_maxcnt_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	struct hihtr_dev *hr_dev =
		container_of(dev, struct hihtr_dev, ib_dev.dev);
	struct hihtr_eq *eq = hr_dev->eq_table.eq;

	return scnprintf(buf, PAGE_SIZE, "%u\n", eq->eq_max_cnt);
}

static ssize_t coalesce_maxcnt_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	struct hihtr_dev *hr_dev =
		container_of(dev, struct hihtr_dev, ib_dev.dev);
	struct hihtr_eq *eq;
	u32 maxcnt;
	int ceq_num;
	int ret;
	int i;

	ceq_num = hr_dev->caps.num_comp_vectors;

	ret = kstrtou32(buf, HIHTR_CONVERSION_NUM, &maxcnt);
	if (ret) {
		dev_err(dev,
			"Input params of irq coalesce maxcnt format unmatch\n");
		return -EINVAL;
	}

	if (maxcnt >= hr_dev->caps.ceqe_depth) {
		dev_err(dev, "maxcnt must be less than %d!\n",
			hr_dev->caps.ceqe_depth);
		return -EINVAL;
	}

	for (i = 0; i < ceq_num; i++) {
		eq = &hr_dev->eq_table.eq[i];
		eq->eq_max_cnt = maxcnt;
		ret = hr_dev->hw->modify_eq(hr_dev, eq, eq->eq_max_cnt, 0,
					    HIHTR_EQ_MAXCNT_MASK);
		if (ret) {
			dev_err(dev, "eqc modify failed, eq_num=%d\n", eq->eqn);
			return -EBUSY;
		}
	}

	return count;
}

static ssize_t coalesce_period_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	struct hihtr_dev *hr_dev =
		container_of(dev, struct hihtr_dev, ib_dev.dev);
	struct hihtr_eq *eq = hr_dev->eq_table.eq;

	return scnprintf(buf, PAGE_SIZE, "%u\n", eq->eq_period);
}

static ssize_t coalesce_period_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	struct hihtr_dev *hr_dev =
		container_of(dev, struct hihtr_dev, ib_dev.dev);
	struct hihtr_eq *eq;
	u32 int_period;
	int ceq_num;
	int i;
	int ret;

	ceq_num = hr_dev->caps.num_comp_vectors;

	ret = kstrtou32(buf, HIHTR_CONVERSION_NUM, &int_period);
	if (ret) {
		dev_err(dev,
			"Input params of irq coalesce period format unmatch\n");
		return -EINVAL;
	}

	if (int_period > HIHTR_CEQ_MAX_INTERVAL) {
		dev_err(dev, "int_period must be less than 2^16!\n");
		return -EINVAL;
	}

	for (i = 0; i < ceq_num; i++) {
		eq = &hr_dev->eq_table.eq[i];
		eq->eq_period = int_period;
		ret = hr_dev->hw->modify_eq(hr_dev, eq, 0, eq->eq_period,
					    HIHTR_EQ_PERIOD_MASK);
		if (ret) {
			dev_err(dev, "eqc modify failed, eq_num=%d\n", eq->eqn);
			return -EBUSY;
		}
	}

	return count;
}

static ssize_t max_qp_show(struct device *dev,
			   struct device_attribute *attr, char *buf)
{
	struct hihtr_dev *hr_dev = container_of(dev, struct hihtr_dev, ib_dev.dev);

	return scnprintf(buf, PAGE_SIZE, "%u\n", hr_dev->caps.num_qps);
}

static ssize_t used_qp_show(struct device *dev,
			    struct device_attribute *attr, char *buf)
{
	struct hihtr_dev *hr_dev = container_of(dev, struct hihtr_dev, ib_dev.dev);
	u32 used_qp = hr_dev->qp_cnt.alloc_qp_cnt - hr_dev->qp_cnt.del_qp_cnt;

	return scnprintf(buf, PAGE_SIZE, "%u\n", used_qp);
}

static ssize_t cnp_tx_period_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct hihtr_dev *hr_dev = container_of(dev, struct hihtr_dev, ib_dev.dev);
	struct hihtr_v3_priv *priv;
	struct hinic5_lld_dev *lld_dev;
	u32 tx_period;
	int ret;

	priv = hr_dev->priv;
	if (!priv) {
		dev_err(dev, "priv of hr_dev is NULL\n");
		return -EINVAL;
	}
	lld_dev = priv->lld_dev;
	if (!lld_dev) {
		dev_err(dev, "lld_dev of hr_dev is NULL\n");
		return -EINVAL;
	}

	if (!roce_func_is_ppf(lld_dev->hwdev)) {
		dev_err(dev, "invalid pf, not ppf\n");
		return -EOPNOTSUPP;
	}

	ret = hr_dev->hw->query_cnp_tx_period(hr_dev, &tx_period);
	if (ret) {
		return ret;
	}

	return scnprintf(buf, PAGE_SIZE, "%u\n", tx_period);
}

static ssize_t cnp_tx_period_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	struct hihtr_dev *hr_dev = container_of(dev, struct hihtr_dev, ib_dev.dev);
	struct hihtr_v3_priv *priv;
	struct hinic5_lld_dev *lld_dev;
	u32 tx_period;
	int ret;

	priv = hr_dev->priv;
	if (!priv) {
		dev_err(dev, "priv of hr_dev is NULL\n");
		return -EINVAL;
	}
	lld_dev = priv->lld_dev;
	if (!lld_dev) {
		dev_err(dev, "lld_dev of hr_dev is NULL\n");
		return -EINVAL;
	}

	if (!roce_func_is_ppf(lld_dev->hwdev)) {
		dev_err(dev, "invalid pf, not ppf\n");
		return -EOPNOTSUPP;
	}

	ret = kstrtou32(buf, HIHTR_CONVERSION_NUM, &tx_period);
	if (ret) {
		dev_err(dev, "invalid cnp_tx_period format\n");
		return -EINVAL;
	}

	if (tx_period > 0xffff) {
		dev_err(dev, "cnp_tx_period must be in range 0x0~0xffff\n");
		return -EINVAL;
	}

	ret = hr_dev->hw->modify_cnp_tx_period(hr_dev, tx_period);
	if (ret) {
		return ret;
	}

	return count;
}

static ssize_t err_cnt_show(struct device *dev,
                            struct device_attribute *attr, char *buf)
{
    struct hihtr_dev *hr_dev = container_of(dev, struct hihtr_dev, ib_dev.dev);
    int len = 0;
	hihtr_history_cqe_error_count *u_err_count = NULL;
	hihtr_history_cqe_error_count *k_err_count = NULL;
	hihtr_history_event_error_count *event_count = NULL;

	if (hr_dev->user_count_page == NULL || hr_dev->user_count_buf == NULL || hr_dev->kernel_count_buf == NULL) {
		return len;
	}

	u_err_count = hr_dev->user_count_buf;
	k_err_count = &(hr_dev->kernel_count_buf->cqe_error_count);
	event_count = &(hr_dev->kernel_count_buf->event_error_count);

    len += scnprintf(buf + len, PAGE_SIZE - len, "Warning Count\n");

    len += scnprintf(buf + len, PAGE_SIZE - len, "  RQ PSN Error Count:        0x%llx\n",
		u_err_count->hihtr_rq_psn_error_count + k_err_count->hihtr_rq_psn_error_count);
    len += scnprintf(buf + len, PAGE_SIZE - len, "  RQ NAK Error Count:        0x%llx\n",
		u_err_count->hihtr_rq_nak_error_count + k_err_count->hihtr_rq_nak_error_count);
    len += scnprintf(buf + len, PAGE_SIZE - len, "  RQ RNR Error Count:        0x%llx\n",
		u_err_count->hihtr_rq_rnr_error_count + k_err_count->hihtr_rq_rnr_error_count);

    len += scnprintf(buf + len, PAGE_SIZE - len, "Event Port Error:                0x%llx\n", event_count->hihtr_event_port_err);
    len += scnprintf(buf + len, PAGE_SIZE - len, "Event CQ Error:                  0x%llx\n", event_count->hihtr_event_cq_err);
    len += scnprintf(buf + len, PAGE_SIZE - len, "Event QP Fatal Error:            0x%llx\n", event_count->hihtr_event_qp_fatal);
    len += scnprintf(buf + len, PAGE_SIZE - len, "Event QP Req Error:              0x%llx\n", event_count->hihtr_event_qp_req_err);
    len += scnprintf(buf + len, PAGE_SIZE - len, "Event QP Access Error:           0x%llx\n", event_count->hihtr_event_qp_access_err);
    len += scnprintf(buf + len, PAGE_SIZE - len, "Event QP Last WQE Reached:       0x%llx\n", event_count->hihtr_event_qp_last_wqe_reached);
    len += scnprintf(buf + len, PAGE_SIZE - len, "Event SRQ Error:                 0x%llx\n", event_count->hihtr_event_srq_err);
    len += scnprintf(buf + len, PAGE_SIZE - len, "Event SRQ Limit Reached:         0x%llx\n", event_count->hihtr_event_srq_limit_reached);
	return len;
}

static DEVICE_ATTR_RW(coalesce_maxcnt);
static DEVICE_ATTR_RW(coalesce_period);
static DEVICE_ATTR_RO(max_qp);
static DEVICE_ATTR_RO(used_qp);
static DEVICE_ATTR_RO(dwqe_pids);
static DEVICE_ATTR_RW(cnp_tx_period);
static DEVICE_ATTR_RO(err_cnt);

static struct device_attribute *hihtr_hw_attrs_list[] = {
	&dev_attr_coalesce_maxcnt,
	&dev_attr_coalesce_period,
	&dev_attr_max_qp,
	&dev_attr_used_qp,
	&dev_attr_dwqe_pids,
	&dev_attr_cnp_tx_period,
	&dev_attr_err_cnt,
};

int hihtr_register_sysfs(struct hihtr_dev *hr_dev)
{
	int ret;
	int i;

	for (i = 0; i < ARRAY_SIZE(hihtr_hw_attrs_list); i++) {
		ret = device_create_file(&hr_dev->ib_dev.dev, hihtr_hw_attrs_list[i]);
		if (ret) {
			dev_err(hr_dev->dev, "register_sysfs failed!\n");
			goto out;
		}
	}
	INIT_LIST_HEAD(&hr_dev->dwqe_pid_list);
    spin_lock_init(&hr_dev->dwqe_pid_lock);
	return ret;
out:
	for (i--; i >= 0; i--)
		device_remove_file(&hr_dev->ib_dev.dev, hihtr_hw_attrs_list[i]);
	return ret;
}

void hihtr_unregister_sysfs(struct hihtr_dev *hr_dev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(hihtr_hw_attrs_list); i++) {
		if (hihtr_hw_attrs_list[i]) {
			device_remove_file(&hr_dev->ib_dev.dev, hihtr_hw_attrs_list[i]);
		}
	}

	return;
}