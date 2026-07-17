/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2018. All rights reserved.
 * Description: Huawei safe print function define
 *
 * Huawei(R) HiNIC PCI Express Network Controller UEFI Driver.
 * This driver is only usable in the EFI Pre-boot execution environment.
 * All rights reserved.
 * Author	 :
 * Create	 : 2018/12/10
 */

#ifndef HW_SAFE_MEM_OP_WRAPPER_H
#define HW_SAFE_MEM_OP_WRAPPER_H

EFI_STATUS MemCpyS(
    void *dest,
    UINTN destMax,
    const void *src,
    UINTN count);

EFI_STATUS MemSetS(
    void *dest,
    UINTN destMax,
    UINT8 c,
    UINTN count);

EFI_STATUS MemMoveS(
    void *dest,
    UINTN destMax,
    const void *src,
    UINTN count);

#endif
