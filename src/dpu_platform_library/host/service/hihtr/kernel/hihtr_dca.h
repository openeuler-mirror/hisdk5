/* SPDX-License-Identifier: GPL-2.0 OR Linux-OpenIB */
/*
 * Copyright (c) 2022 Hisilicon Limited. All rights reserved.
 */

#ifndef __HIHTR_DCA_H
#define __HIHTR_DCA_H

#include <rdma/uverbs_ioctl.h>

/* DCA page state (32 bit) */
struct hihtr_dca_page_state {
	u32 buf_id : 29; /* If zero, means page can be used by any buffer. */
	u32 lock : 1; /* @buf_id locked this page to prepare access. */
	u32 active : 1; /* @buf_id is accessing this page. */
	u32 head : 1; /* This page is the head in a continuous address range. */
};

extern const struct uapi_definition hihtr_dca_uapi_defs[];

#define HIHTR_DCA_INVALID_BUF_ID 0UL
#define HIHTR_DCA_INVALID_DCA_NUM ~0U

/*
 * buffer id(29b) = tag(7b) + owner(22b)
 * [28:22] tag  : indicate the QP config update times.
 * [21: 0] owner: indicate the QP to which the page belongs.
 */
#define HIHTR_DCA_ID_MASK GENMASK(28, 0)
#define HIHTR_DCA_TAG_MASK GENMASK(28, 22)
#define HIHTR_DCA_OWN_MASK GENMASK(21, 0)

#define HIHTR_DCA_BUF_ID_TO_TAG(buf_id) (((buf_id) & HIHTR_DCA_TAG_MASK) >> 22)
#define HIHTR_DCA_BUF_ID_TO_QPN(buf_id) ((buf_id) & HIHTR_DCA_OWN_MASK)
#define HIHTR_DCA_TO_BUF_ID(qpn, tag) (((qpn) & HIHTR_DCA_OWN_MASK) | \
					(((tag) << 22) & HIHTR_DCA_TAG_MASK))

#define DCA_UNIT_SIZE_MODE 0444
#define DCA_MAX_SIZE_MODE 0444
#define DCA_MIN_SIZE_MODE 0444
#define DCA_NUM_STATUS_REGIONS 2
struct hihtr_dca_attach_attr {
	u32 sq_offset;
	u32 sge_offset;
	u32 rq_offset;
};

struct hihtr_dca_attach_resp {
#define HIHTR_DCA_ATTACH_FLAGS_NEW_BUFFER BIT(0)
	u32 alloc_flags;
	u32 alloc_pages;
};

struct hihtr_dca_detach_attr {
	u32 sq_idx;
};

typedef int (*hihtr_dca_enum_callback)(struct hihtr_dca_page_state *, u32, void *);

void hihtr_init_dca(struct hihtr_dev *hr_dev);
void hihtr_cleanup_dca(struct hihtr_dev *hr_dev);

void hihtr_register_udca(struct hihtr_dev *hr_dev, int max_qps,
			    struct hihtr_ucontext *uctx);
void hihtr_unregister_udca(struct hihtr_dev *hr_dev,
			      struct hihtr_ucontext *uctx);

int hihtr_enable_dca(struct hihtr_dev *hr_dev, struct hihtr_qp *hr_qp,
			struct ib_udata *udata);
void hihtr_disable_dca(struct hihtr_dev *hr_dev,
			  struct hihtr_qp *hr_qp, struct ib_udata *udata);

int hihtr_dca_attach(struct hihtr_dev *hr_dev, struct hihtr_qp *hr_qp,
			struct hihtr_dca_attach_attr *attr);
void hihtr_dca_detach(struct hihtr_dev *hr_dev, struct hihtr_qp *hr_qp,
			 struct hihtr_dca_detach_attr *attr);
void hihtr_modify_dca(struct hihtr_dev *hr_dev, struct hihtr_qp *hr_qp,
			 struct ib_udata *udata);

void hihtr_enum_dca_pool(struct hihtr_dca_ctx *dca_ctx, void *param,
			    hihtr_dca_enum_callback cb);
#endif
