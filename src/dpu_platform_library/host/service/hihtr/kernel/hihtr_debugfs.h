/* SPDX-License-Identifier: GPL-2.0+ */
// Copyright (c) 2016-2017 Hisilicon Limited.

#ifndef __HIHTR_DEBUGFS_H
#define __HIHTR_DEBUGFS_H

#define DEBUGFS_SEQFILE_MODE  0400

void hihtr_init_debugfs(void);
void hihtr_cleanup_debugfs(void);
void hihtr_register_debugfs(struct hihtr_dev *hr_dev);
void hihtr_unregister_debugfs(struct hihtr_dev *hr_dev);
void hihtr_register_uctx_debugfs(struct hihtr_dev *hr_dev,
				    struct hihtr_ucontext *uctx);
void hihtr_unregister_uctx_debugfs(struct hihtr_dev *hr_dev,
				      struct hihtr_ucontext *uctx);
struct hihtr_dca_debugfs *create_dca_debugfs(struct hihtr_dev *hr_dev, struct dentry *parent);
void destroy_dca_debugfs(struct hihtr_dca_debugfs *dca_dbgfs);
#endif
