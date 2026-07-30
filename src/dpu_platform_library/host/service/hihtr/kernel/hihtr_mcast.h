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

#ifndef _HIHTR_MCAST_H
#define _HIHTR_MCAST_H

#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/string.h>
#include <linux/jhash.h>

#include "comm_defs.h"
#include "hinic5_lld.h"
#include "hinic5_hw.h"

#include "hihtr_device.h"
#include "hihtr_cmdq.h"


#define DEFAULT_GID_IDX 32

enum mgid_op_type {
	MGID_DEL = 0,
	MGID_ADD
};

enum gid_attr_find_mask {
	GID_ATTR_FIND_MASK_GID      = 1UL << 0,
	GID_ATTR_FIND_MASK_NETDEV	= 1UL << 1,
	GID_ATTR_FIND_MASK_DEFAULT	= 1UL << 2,
	GID_ATTR_FIND_MASK_GID_TYPE	= 1UL << 3,
};

enum gid_table_entry_state {
    GID_TABLE_ENTRY_INVALID		= 1,
	GID_TABLE_ENTRY_VALID		= 2,
	GID_TABLE_ENTRY_PENDING_DEL	= 3,
};

struct hihtr_gid_ndev_storage {
    struct rcu_head rcu_head;
    struct net_device *ndev;
};

struct hihtr_gid_table_entry {
    struct kref			kref;
    struct work_struct		del_work;
    struct ib_gid_attr		attr;
    void				*context;
    struct hihtr_gid_ndev_storage	*ndev_storage;
    enum gid_table_entry_state	state;
};

struct hihtr_gid_table {
	int				sz;
	struct mutex			lock;
	rwlock_t			rwlock;
	struct hihtr_gid_table_entry	**data_vec;
	u32				default_gid_indices;
};

int hihtr_v3_attach_mcast(struct ib_qp *ibqp, union ib_gid *gid, u16 lid);
int hihtr_v3_detach_mcast(struct ib_qp *ibqp, union ib_gid *gid, u16 lid);

#endif /* HIHTR_MCAST_H */