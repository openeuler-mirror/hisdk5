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

#include <linux/gfp.h>
#include <linux/dmapool.h>
#include "hihtr_common.h"
#include "hihtr_device.h"
#include "hihtr_cmd.h"

#define CMD_MAX_NUM 32
int hihtr_cmd_init(struct hihtr_dev *hr_dev)
{
	sema_init(&hr_dev->cmd.poll_sem, 1);
	hr_dev->cmd.use_events = 0;
	hr_dev->cmd.max_cmds = CMD_MAX_NUM;
	hr_dev->cmd.pool = dma_pool_create("hihtr_cmd", hr_dev->dev,
					   HIHTR_MAILBOX_SIZE,
					   HIHTR_MAILBOX_SIZE, 0);
	if (!hr_dev->cmd.pool)
		return -ENOMEM;

	return 0;
}

void hihtr_cmd_cleanup(struct hihtr_dev *hr_dev)
{
	dma_pool_destroy(hr_dev->cmd.pool);
}

struct hihtr_cmd_mailbox *
hihtr_alloc_cmd_mailbox(struct hihtr_dev *hr_dev)
{
	struct hihtr_cmd_mailbox *mailbox;

	mailbox = kmalloc(sizeof(*mailbox), GFP_KERNEL);
	if (!mailbox)
		return ERR_PTR(-ENOMEM);

	mailbox->buf =
		dma_pool_alloc(hr_dev->cmd.pool, GFP_KERNEL, &mailbox->dma);
	if (!mailbox->buf) {
		kfree(mailbox);
		return ERR_PTR(-ENOMEM);
	}

	return mailbox;
}

void hihtr_free_cmd_mailbox(struct hihtr_dev *hr_dev,
			       struct hihtr_cmd_mailbox *mailbox)
{
	if (!mailbox)
		return;

	dma_pool_free(hr_dev->cmd.pool, mailbox->buf, mailbox->dma);
	kfree(mailbox);
}
