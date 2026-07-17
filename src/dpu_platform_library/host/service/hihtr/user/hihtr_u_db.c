/*
 * Copyright (c) 2017 Hisilicon Limited.
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "roce_ucompat_gen.h"
#if defined(UTIL_BITMAP_HAVE_BITMAP_ALLOC1)
#include <util/bitmap.h>
#else
#include <ccan/bitmap.h>
#endif
#include "hihtr_u.h"
#include "hihtr_u_db.h"

/* the sw db length, on behalf of the qp/cq/srq length from left to right */
static const unsigned int db_size[] = {
	[HIHTR_QP_TYPE_DB] = 4,
	[HIHTR_CQ_TYPE_DB] = 4,
	[HIHTR_SRQ_TYPE_DB] = 4,
};

static struct hihtr_db_page *hihtr_add_db_page(
						struct hihtr_context *ctx,
						enum hihtr_db_type type)
{
	struct hihtr_db_page *page;
	int page_size;

	page_size = to_hr_dev(ctx->ibv_ctx.context.device)->page_size;
	page = calloc(1, sizeof(*page));
	if (!page)
		goto err_page;

	/* allocate bitmap space for sw db and init all bitmap to 1 */
	page->num_db = page_size / db_size[type];
	page->use_cnt = 0;
	page->bitmap = bitmap_alloc1(page->num_db);
	if (!page->bitmap)
		goto err_map;

	if (hihtr_alloc_buf(&(page->buf), page_size, page_size))
		goto err;

	/* add the set ctx->db_list */
	page->prev = NULL;
	page->next = ctx->db_list[type];
	ctx->db_list[type] = page;
	if (page->next)
		page->next->prev = page;

	return page;
err:
	free(page->bitmap);

err_map:
	free(page);

err_page:
	return NULL;
}

static void hihtr_clear_db_page(struct hihtr_db_page *page)
{
	free(page->bitmap);
	hihtr_free_buf(&(page->buf));
}

void *hihtr_alloc_db(struct hihtr_context *ctx,
			enum hihtr_db_type type)
{
	struct hihtr_db_page *page;
	void *db = NULL;
	uint32_t npos;

	pthread_mutex_lock((pthread_mutex_t *)&ctx->db_list_mutex);

	for (page = ctx->db_list[type]; page != NULL; page = page->next)
		if (page->use_cnt < page->num_db)
			goto found;

	page = hihtr_add_db_page(ctx, type);
	if (!page)
		goto out;

found:
	++page->use_cnt;

	npos = bitmap_find_first_bit(page->bitmap, 0, page->num_db);
	bitmap_clear_bit(page->bitmap, npos);
	db = page->buf.buf + npos * db_size[type];

out:
	pthread_mutex_unlock((pthread_mutex_t *)&ctx->db_list_mutex);

	return db;
}

void hihtr_free_db(struct hihtr_context *ctx, unsigned int *db,
		      enum hihtr_db_type type)
{
	struct hihtr_db_page *page;
	uint32_t npos;
	uint32_t page_size;

	pthread_mutex_lock((pthread_mutex_t *)&ctx->db_list_mutex);

	page_size = to_hr_dev(ctx->ibv_ctx.context.device)->page_size;
	for (page = ctx->db_list[type]; page != NULL; page = page->next)
		if (((uintptr_t)db & (~((uintptr_t)page_size - 1))) ==
						(uintptr_t)(page->buf.buf))
			goto found;

	goto out;

found:
	--page->use_cnt;
	if (!page->use_cnt) {
		if (page->prev)
			page->prev->next = page->next;
		else
			ctx->db_list[type] = page->next;

		if (page->next)
			page->next->prev = page->prev;

		hihtr_clear_db_page(page);
		free(page);

		goto out;
	}

	npos = ((uintptr_t)db - (uintptr_t)page->buf.buf) / db_size[type];
	bitmap_set_bit(page->bitmap, npos);

out:
	pthread_mutex_unlock((pthread_mutex_t *)&ctx->db_list_mutex);
}
