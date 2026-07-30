/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2025 Huawei Technologies Co., Ltd */

#ifndef HINIC5_TYPESDEF_INNER_H
#define HINIC5_TYPESDEF_INNER_H

/* static methods testable */
#ifdef EXPORT_STATIC_SYMBOL
#define STATIC __attribute__((weak, noinline))
#define INLINE __attribute__((weak, noinline))
#else
#define STATIC static
#define INLINE inline
#endif

#ifndef GIT_COMMIT_ID
#define GIT_COMMIT_ID "unknown"
#endif

#endif /* HINIC5_TYPESDEF_INNER_H */