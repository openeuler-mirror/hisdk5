/* SPDX-License-Identifier: (GPL-2.0 OR BSD-2-Clause) */
/*
 * Copyright (c) 2017 Hisilicon Limited.
 * Copyright (c) 2007, 2008 Mellanox Technologies. All rights reserved.
 */

#include <rdma/ib_umem.h>
#include <rdma/uverbs_ioctl.h>
#include "hihtr_device.h"

int hihtr_db_map_user(struct hihtr_ucontext *context, unsigned long virt,
			 struct hihtr_db *db, struct ib_udata *udata)
{
	unsigned long page_addr = virt & PAGE_MASK;
	struct hihtr_user_db_page *page;
	unsigned int offset;
	int ret = 0;

	mutex_lock(&context->page_mutex);

	list_for_each_entry(page, &context->page_list, list)
		if (page->user_virt == page_addr)
			goto found;

	page = kmalloc(sizeof(*page), GFP_KERNEL);
	if (!page) {
		ret = -ENOMEM;
		goto out;
	}

	refcount_set(&page->refcount, 1);
	page->user_virt = page_addr;
	page->umem = hihtr_ib_umem_get(&context->ibucontext, page_addr, PAGE_SIZE, 0, udata);
	if (IS_ERR(page->umem)) {
		ret = PTR_ERR(page->umem);
		kfree(page);
		goto out;
	}

	list_add(&page->list, &context->page_list);

found:
	offset = virt - page_addr;
	db->dma = hihtr_umem_get_dma(page->umem, offset);
	db->virt_addr = hihtr_umem_get_virt(page->umem, offset);
	db->u.user_page = page;
	refcount_inc(&page->refcount);

out:
	mutex_unlock(&context->page_mutex);

	return ret;
}

void hihtr_db_unmap_user(struct hihtr_ucontext *context,
			    struct hihtr_db *db)
{
	mutex_lock(&context->page_mutex);

	refcount_dec(&db->u.user_page->refcount);
	if (refcount_dec_if_one(&db->u.user_page->refcount)) {
		list_del(&db->u.user_page->list);
		ib_umem_release(db->u.user_page->umem);
		kfree(db->u.user_page);
	}

	mutex_unlock(&context->page_mutex);
}

static struct hihtr_db_pgdir *hihtr_alloc_db_pgdir(
					struct device *dma_device)
{
	struct hihtr_db_pgdir *pgdir;

	pgdir = kzalloc(sizeof(*pgdir), GFP_KERNEL);
	if (!pgdir)
		return NULL;

	bitmap_fill(pgdir->order1,
		    HIHTR_DB_PER_PAGE / HIHTR_DB_TYPE_COUNT);
	pgdir->bits[0] = pgdir->order0;
	pgdir->bits[1] = pgdir->order1;
	pgdir->page = dma_alloc_coherent(dma_device, PAGE_SIZE,
					 &pgdir->db_dma, GFP_KERNEL);
	if (!pgdir->page) {
		kfree(pgdir);
		return NULL;
	}

	return pgdir;
}

static int hihtr_alloc_db_from_pgdir(struct hihtr_db_pgdir *pgdir,
					struct hihtr_db *db, int order)
{
	unsigned long o;
	unsigned long i;

	for (o = order; o <= 1; ++o) {
		i = find_first_bit(pgdir->bits[o], HIHTR_DB_PER_PAGE >> o);
		if (i < HIHTR_DB_PER_PAGE >> o)
			goto found;
	}

	return -ENOMEM;

found:
	clear_bit(i, pgdir->bits[o]);

	i <<= o;

	if (o > order)
		set_bit(i ^ 1, pgdir->bits[order]);

	db->u.pgdir	= pgdir;
	db->index	= i;
	db->db_record	= pgdir->page + db->index;
	db->dma		= pgdir->db_dma  + db->index * HIHTR_DB_UNIT_SIZE;
	db->order	= order;

	return 0;
}

int hihtr_alloc_db(struct hihtr_dev *hr_dev, struct hihtr_db *db,
		      int order)
{
	struct hihtr_db_pgdir *pgdir;
	int ret = 0;

	mutex_lock(&hr_dev->pgdir_mutex);

	list_for_each_entry(pgdir, &hr_dev->pgdir_list, list)
		if (!hihtr_alloc_db_from_pgdir(pgdir, db, order))
			goto out;

	pgdir = hihtr_alloc_db_pgdir(hr_dev->dev);
	if (!pgdir) {
		ret = -ENOMEM;
		goto out;
	}

	list_add(&pgdir->list, &hr_dev->pgdir_list);

	/* This should never fail -- we just allocated an empty page: */
	WARN_ON(hihtr_alloc_db_from_pgdir(pgdir, db, order));

out:
	mutex_unlock(&hr_dev->pgdir_mutex);

	return ret;
}

void hihtr_free_db(struct hihtr_dev *hr_dev, struct hihtr_db *db)
{
	unsigned long o;
	unsigned long i;

	mutex_lock(&hr_dev->pgdir_mutex);

	o = db->order;
	i = db->index;

	if (db->order == 0 && test_bit(i ^ 1, db->u.pgdir->order0)) {
		clear_bit(i ^ 1, db->u.pgdir->order0);
		++o;
	}

	i >>= o;
	set_bit(i, db->u.pgdir->bits[o]);

	if (bitmap_full(db->u.pgdir->order1,
			HIHTR_DB_PER_PAGE / HIHTR_DB_TYPE_COUNT)) {
		dma_free_coherent(hr_dev->dev, PAGE_SIZE, db->u.pgdir->page,
				  db->u.pgdir->db_dma);
		list_del(&db->u.pgdir->list);
		kfree(db->u.pgdir);
	}

	mutex_unlock(&hr_dev->pgdir_mutex);
}
