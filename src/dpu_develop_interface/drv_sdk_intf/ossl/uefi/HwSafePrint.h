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

#ifndef HW_SAFE_PRINT_H
#define HW_SAFE_PRINT_H

UINTN AsciiSPrintS(
    CHAR8      *startOfBuffer,
    UINTN       bufferSize,
    CONST CHAR8 *formatString,
    ...);

UINTN UnicodeSPrintS(
    CHAR16      *startOfBuffer,
    UINTN        bufferSize,
    CONST CHAR16 *formatString,
    ...);

UINTN AsciiVSPrintS(
    CHAR8         *startOfBuffer,
    UINTN          bufferSize,
    CONST CHAR8   *formatString,
    VA_LIST        marker);

UINTN UnicodeVSPrintS(
    CHAR16         *startOfBuffer,
    UINTN           bufferSize,
    CONST CHAR16   *formatString,
    VA_LIST         marker);

#ifdef EDKII_SUPPORT
UINTN AsciiBSPrintS(
    CHAR8         *startOfBuffer,
    UINTN          bufferSize,
    CONST CHAR8   *formatString,
    BASE_LIST      marker);

UINTN UnicodeBSPrintS(
    CHAR16         *startOfBuffer,
    UINTN           bufferSize,
    CONST CHAR16   *formatString,
    BASE_LIST       marker);
#endif

#endif
