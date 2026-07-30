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

#include <linux/pci.h>
#include "hihtr_device.h"

void hihtr_init_pd_table(struct hihtr_dev *hr_dev)
{
	struct hihtr_ida *pd_ida = &hr_dev->pd_ida;

	ida_init(&pd_ida->ida);
	pd_ida->max = hr_dev->caps.num_pds - 1;
	pd_ida->min = hr_dev->caps.reserved_pds;
}

#if defined(ROCE_ALLOC_PD_INT)
int hihtr_alloc_pd(struct ib_pd *ibpd, struct ib_udata *udata)
{
	struct ib_device *ib_dev = ibpd->device;
	struct hihtr_dev *hr_dev = to_hr_dev(ib_dev);
	struct hihtr_ida *pd_ida = &hr_dev->pd_ida;
	struct hihtr_pd *pd = to_hr_pd(ibpd);
	int ret = 0;
	int id;

	id = ida_alloc_range(&pd_ida->ida, pd_ida->min, pd_ida->max,
			     GFP_KERNEL);
	if (id < 0) {
		ibdev_err(ib_dev, "failed to alloc pd, id = %d.\n", id);
		return -ENOMEM;
	}
	pd->pdn = (unsigned long)id;
	if (udata) {
		struct hihtr_ib_alloc_pd_resp resp = {.pdn = pd->pdn};

		ret = ib_copy_to_udata(udata, &resp,
				       min(udata->outlen, sizeof(resp)));
		if (ret) {
			ida_free(&pd_ida->ida, id);
			ibdev_err(ib_dev, "failed to copy to udata, ret = %d\n", ret);
		}
	}
	return ret;
}
#else
struct ib_pd *hihtr_alloc_pd(struct ib_device *ib_dev, struct ib_ucontext *ibcontext, struct ib_udata *udata)
{
	struct hihtr_dev *hr_dev = to_hr_dev(ib_dev);
	struct hihtr_ida *pd_ida = &hr_dev->pd_ida;
	struct hihtr_pd *pd = (struct hihtr_pd *)kmalloc(sizeof *pd, GFP_KERNEL);
	int ret = 0;
	int id;

	if (pd == NULL) {
        ret = -ENOMEM;
        dev_err(hr_dev->dev, "failed to alloc memory for roce pd\n");
        goto err_out;
    }

	id = ida_alloc_range(&pd_ida->ida, pd_ida->min, pd_ida->max,
			     GFP_KERNEL);
	if (id < 0) {
		ret = -ENOMEM;
		dev_err(hr_dev->dev, "failed to alloc pd, id = %d.\n", id);
		goto err_alloc_pdn;
	}
	pd->pdn = (unsigned long)id;
	if (udata) {
		struct hihtr_ib_alloc_pd_resp resp = {.pdn = pd->pdn};
 
		ret = ib_copy_to_udata(udata, &resp,
				       min(udata->outlen, sizeof(resp)));
		if (ret) {
			ida_free(&pd_ida->ida, id);
			dev_err(hr_dev->dev, "failed to copy to udata, ret = %d\n", ret);
			goto err_alloc_pdn;
		}
	}
	return &pd->ibpd;

err_alloc_pdn:
	kfree(pd);
err_out:
	return (struct ib_pd *)ERR_PTR((long)ret);
}
#endif

#if defined(ROCE_DEALLOC_PD_VOID)
void hihtr_dealloc_pd(struct ib_pd *pd, struct ib_udata *udata)
#elif defined(ROCE_DEALLOC_PD_HAVE_UDATA)
int hihtr_dealloc_pd(struct ib_pd *pd, struct ib_udata *udata)
#else
int hihtr_dealloc_pd(struct ib_pd *pd)
#endif
{
	struct hihtr_dev *hr_dev = to_hr_dev(pd->device);

	ida_free(&hr_dev->pd_ida.ida, (int)to_hr_pd(pd)->pdn);

#if !defined(ROCE_DEALLOC_PD_VOID)
	return 0;
#else
	return;
#endif
}

int hihtr_uar_alloc(struct hihtr_dev *hr_dev, struct hihtr_uar *uar)
{
	struct hihtr_ida *uar_ida = &hr_dev->uar_ida;
	int id;

	/* Using bitmap to manager UAR index */
	id = ida_alloc_range(&uar_ida->ida, uar_ida->min, uar_ida->max,
			     GFP_KERNEL);
	if (id < 0) {
		ibdev_err(&hr_dev->ib_dev, "failed to alloc uar id(%d).\n", id);
		return -ENOMEM;
	}
	uar->logic_idx = (unsigned long)id;

	if (uar->logic_idx > 0 && hr_dev->caps.phy_num_uars > 1)
		uar->index = (uar->logic_idx - 1) %
			     (hr_dev->caps.phy_num_uars - 1) + 1;
	else
		uar->index = 0;

	return 0;
}

void hihtr_init_uar_table(struct hihtr_dev *hr_dev)
{
	struct hihtr_ida *uar_ida = &hr_dev->uar_ida;

	ida_init(&uar_ida->ida);
	uar_ida->max = hr_dev->caps.num_uars - 1;
	uar_ida->min = hr_dev->caps.reserved_uars;
}

static int hihtr_xrcdn_alloc(struct hihtr_dev *hr_dev, u32 *xrcdn)
{
	struct hihtr_ida *xrcd_ida = &hr_dev->xrcd_ida;
	int id;

	id = ida_alloc_range(&xrcd_ida->ida, xrcd_ida->min, xrcd_ida->max,
			     GFP_KERNEL);
	if (id < 0) {
		ibdev_err(&hr_dev->ib_dev, "failed to alloc xrcdn(%d).\n", id);
		return -ENOMEM;
	}
	*xrcdn = (u32)id;

	return 0;
}

void hihtr_init_xrcd_table(struct hihtr_dev *hr_dev)
{
	struct hihtr_ida *xrcd_ida = &hr_dev->xrcd_ida;

	ida_init(&xrcd_ida->ida);
	xrcd_ida->max = hr_dev->caps.num_xrcds - 1;
	xrcd_ida->min = hr_dev->caps.reserved_xrcds;
}

#if defined(ROCE_ALLOC_XRCD_INT)
int hihtr_alloc_xrcd(struct ib_xrcd *ib_xrcd, struct ib_udata *udata)
{
	struct hihtr_dev *hr_dev = to_hr_dev(ib_xrcd->device);
	struct hihtr_xrcd *xrcd = to_hr_xrcd(ib_xrcd);
	int ret;

	if (!(hr_dev->caps.flags & HIHTR_CAP_FLAG_XRC))
		return -EINVAL;

	ret = hihtr_xrcdn_alloc(hr_dev, &xrcd->xrcdn);
	if (ret)
		return ret;

	return 0;
}
#else
#if defined(ROCE_IB_DEVICE_ALLOC_XRCD_HAVE_IB_UCONTEXT) || defined(ROCE_ALLOC_XRCD_HAVE_IB_UCONTEXT)
struct ib_xrcd *hihtr_alloc_xrcd(struct ib_device *ibdev, struct ib_ucontext *ibcontext, struct ib_udata *udata)
#else
struct ib_xrcd *hihtr_alloc_xrcd(struct ib_device *ibdev, struct ib_udata *udata)
#endif
{
	struct hihtr_dev *hr_dev = to_hr_dev(ibdev);
	struct hihtr_xrcd *xrcd = (struct hihtr_xrcd *)kzalloc(sizeof(*xrcd), GFP_KERNEL);
	int ret;
 
	if (xrcd == NULL) {
		dev_err(hr_dev->dev, "failed to alloc xrcd memory.\n");
		return (struct ib_xrcd *)ERR_PTR((long)-ENOMEM);
	}
 
	if (!(hr_dev->caps.flags & HIHTR_CAP_FLAG_XRC)) {
		kfree(xrcd);
		return (struct ib_xrcd *)ERR_PTR(-EINVAL);
	}
		
 
	ret = hihtr_xrcdn_alloc(hr_dev, &xrcd->xrcdn);
	if (ret) {
		kfree(xrcd);
		return (struct ib_xrcd *)ERR_PTR((long)ret);
	}
 
	return &xrcd->ibxrcd;
}
#endif

#if defined(ROCE_ALLOC_XRCD_INT)
int hihtr_dealloc_xrcd(struct ib_xrcd *ib_xrcd, struct ib_udata *udata)
{
	struct hihtr_dev *hr_dev = to_hr_dev(ib_xrcd->device);
	u32 xrcdn = to_hr_xrcd(ib_xrcd)->xrcdn;

	ida_free(&hr_dev->xrcd_ida.ida, (int)xrcdn);

	return 0;
}
#else
#if defined(ROCE_DEALLOC_XRCD_HAVE_IB_UDATA)
int hihtr_dealloc_xrcd(struct ib_xrcd *ib_xrcd, struct ib_udata *udata)
#else
int hihtr_dealloc_xrcd(struct ib_xrcd *ib_xrcd)
#endif
{
	struct hihtr_dev *hr_dev = NULL;
	struct hihtr_xrcd *xrcd = NULL;
	u32 xrcdn;

	if (ib_xrcd == NULL) {
		pr_err("failed to dealloc xrcd memory, ib_xrcd is NULL.\n");
		return -EINVAL;
	}

	hr_dev = to_hr_dev(ib_xrcd->device);
	xrcd = to_hr_xrcd(ib_xrcd);
	xrcdn = xrcd->xrcdn;

	ida_free(&hr_dev->xrcd_ida.ida, (int)xrcdn);

	kfree(xrcd);

	return 0;
}
#endif