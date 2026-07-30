/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 * Description: macsec main
 * Create: 2025/11/05
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": [MACsec]" fmt

#include <net/rtnetlink.h>
#include <linux/module.h>
#include <linux/netdev_features.h>
#include <linux/netlink.h>
#include <linux/netdevice.h>
#include <linux/rculist.h>

#include "ossl_knl.h"
#include "hinic5_hw.h"
#include "hinic5_srv_nic.h"

#include "hinic5_macsec_dfx.h"
#include "hinic5_macsec_common.h"
#include "hinic5_macsec_dev.h"

// TODO 该参数适配支持4sa模式, 配置文件ready后删除
static u8 macsec_sc_mode = HIMACSEC_SC_MODE_FOUR_SA;
module_param(macsec_sc_mode, byte, 0444);
MODULE_PARM_DESC(macsec_sc_mode, "MACsec SC mode, 0: 2SA, 1: 4SA (default=1)");

struct macsec_port_res g_macsec_port_res[MACSEC_PORT_NUM] = {0};

#define HIMACSEC_DRV_DESC      "Huawei(R) Intelligent Network Interface Card, MACsec driver"
#define HIMACSEC_AUTHOR        "Huawei Technologies CO., Ltd"

struct himacsec_sc *get_g_macsec_port_res(u32 mode, u32 port_id)
{
    if (port_id >= MACSEC_PORT_NUM) {
        pr_err("MACsec param port_id(0x%x) invalid", port_id);
        return NULL;
    }
    if (mode == MACSEC_OUTBOUND) {
        return &g_macsec_port_res[port_id].enc_sc;
    } else {
        return &g_macsec_port_res[port_id].dec_sc;
    }
}

int himacsec_init_dev_spec(struct macsec_resource *macsec_res, struct hinic5_lld_dev *lld_dev, struct himacsec_spec *spec)
{
    macsec_res->spec.macsec_support = spec->macsec_support;
    macsec_res->spec.max_port = spec->max_port;

    if (macsec_sc_mode >= HIMACSEC_SC_MODE_MAX) {
        macsec_err(lld_dev->dev, "MACsec param macsec_sc_mode(0x%x) invalid", macsec_sc_mode);
        return -EINVAL;
    }

    macsec_res->spec.max_sa = (macsec_sc_mode + 1) << 0x1; // macsec_sc_mod=0: 2SA模式 macsec_sc_mode=1: 4SA模式
    macsec_res->spec.max_port_sc = spec->max_port_sc; // 端口SC数量：2SA模式-8个SC 4SA模式-4个SC

    return 0;
}

int himacsec_init_resource(struct hinic5_nic_dev *nic_dev, struct hinic5_lld_dev *lld_dev,
    struct himacsec_spec *spec, u64 feature_bitmap)
{
    struct macsec_resource *macsec_res = NULL;
    int ret;
    u16 func_id = hinic5_global_func_id(lld_dev->hwdev);
    u8 port = hinic5_physical_port_id(lld_dev->hwdev);

    macsec_res = kzalloc(sizeof(struct macsec_resource), GFP_KERNEL);
    if (macsec_res == NULL) {
        macsec_err(lld_dev->dev, "Alloc MACsec resource memory failed");
        return -ENOMEM;
    }

    /* 1、初始化 spec */
    ret = himacsec_init_dev_spec(macsec_res, lld_dev, spec);
    if (ret != 0) {
        kfree(macsec_res);
        return ret;
    }

    /* 3、私有数据回挂 */
    macsec_res->function_port = port;
    macsec_res->offload_dev_num = 0;
    macsec_res->himacsec_feature[0] = feature_bitmap;
    nic_dev->macsec_res = macsec_res; // 回挂到nic_dev

    macsec_info(lld_dev->dev, "Func 0x%x macsec capability: sc_mode(0:2sa, 1:4sa): %u, func_port: %u, feature: 0x%llx",
        func_id, macsec_sc_mode, port, feature_bitmap);

    return ret;
}

void himacsec_release_resource(struct hinic5_nic_dev *nic_dev)
{
    struct macsec_resource *macsec_res = NULL;

    if (nic_dev == NULL) {
        return;
    }

    macsec_res = nic_dev->macsec_res;
    if (macsec_res == NULL) {
        return;
    }

    // 2、清理 MACsec 资源
    kfree(macsec_res);
    nic_dev->macsec_res = NULL;
}

int macsec_init_offload(struct hinic5_nic_dev *nic_dev)
{
    u64 feature_bitmap = 0;
    int ret;
    struct himacsec_spec spec = {0};
    struct hinic5_lld_dev *lld_dev = NULL;
    u8 macsec_flag = MACSEC_GLOBAL_SWITCH_IS_DISABLE;

    if (nic_dev == NULL || nic_dev->lld_dev == NULL || nic_dev->netdev == NULL) {
        pr_err("lld device is NULL, MACsec init failed");
        return -ENODEV;
    }

    lld_dev = nic_dev->lld_dev;

    /* 1、确认MACsec使能情况 */
    if (!hinic5_support_macsec(lld_dev->hwdev)) {
        macsec_info(lld_dev->dev, "HW don't support macsec");
        return 0;
    }

    /* 2、特性协商 */
    if (himacsec_cmd_exec_get_feature_nego(lld_dev, &feature_bitmap, 1) != 0) {
        macsec_err(lld_dev->dev, "Feature negotiation failed");
        return -EBUSY;
    }

    /* 3、全局配置获取 */
    if (himacsec_cmd_exec_get_spec(lld_dev->hwdev, &spec) != 0) {
        macsec_err(lld_dev->dev, "Get chip spec failed");
        return -EBUSY;
    }

    /* 4、nic_dev中初始化MACsec私有数据 */
    ret = himacsec_init_resource(nic_dev, lld_dev, &spec, feature_bitmap);
    if (ret != 0) {
        macsec_err(lld_dev->dev, "Init MACsec private data failed, ret:%d", ret);
        return ret;
    }

    /* 5、内核协议栈 macsec 功能卸载初始化 */
    himacsec_offload_init(nic_dev);

    /* 6、使能 macsec，需要进行初始化操作（只需要一次） */
    ret = himacsec_cmd_exec_macsec_enable(lld_dev, MACSEC_CMD_SERVICE_OP_MACSEC_ENABLE, &macsec_flag);
    if (ret != 0) {
        macsec_err(lld_dev->dev, "Enable macsec failed, ret:%d", ret);
        goto enable_macsec_fail;
    }

    macsec_info(lld_dev->dev, "Macsec resource init successfully");
    return ret;

enable_macsec_fail:
    himacsec_offload_deinit(nic_dev);
    himacsec_release_resource(nic_dev);
    if (macsec_flag == MACSEC_GLOBAL_SWITCH_IS_DISABLE) {
        return 0;
    }
    return ret;
}

void macsec_cleanup_offload(struct hinic5_nic_dev *nic_dev)
{
    struct macsec_resource *macsec_res = NULL;
    struct hinic5_lld_dev *lld_dev = NULL;
    int ret;

    if (nic_dev == NULL || nic_dev->lld_dev == NULL || nic_dev->netdev == NULL) {
        pr_err("lld device is NULL, MACsec resource release failed");
        return;
    }

    lld_dev = nic_dev->lld_dev;
    macsec_res = nic_dev->macsec_res;

    if ((macsec_res == NULL) || (macsec_res->spec.macsec_support == 0)) {
        return;
    }

    ret = himacsec_cmd_exec_macsec_enable(lld_dev, MACSEC_CMD_SERVICE_OP_MACSEC_DISABLE, NULL);
    if (ret != 0) {
        macsec_err(lld_dev->dev, "Disable macsec failed, ret:%d", ret);
    }

    himacsec_offload_deinit(nic_dev);

    himacsec_release_resource(nic_dev);
    macsec_info(lld_dev->dev, "Macsec resource release successfully");
}