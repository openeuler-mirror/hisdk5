/*
 * @file sml_table_struct_dict_def.h
 * Copyright (c) Huawei Technologies Co., Ltd. 2026-2026. All rights reserved.
 * @brief SML Table 结构体元数据注册宏定义
 *
 * 本文件定义了用于将结构体信息注入二进制符号表的宏接口。
 * 开启 DEFINE_TABLE_STRUCT_ENABLE 宏后，调用 DEFINE_TABLE_STRUCT_VAR
 * 将在链接后的二进制文件中生成特定前缀的符号，供 sml_table_struct_dict_gen.py 脚本解析。
 */
#ifndef SML_TABLE_STRUCT_DICT_DEF_H
#define SML_TABLE_STRUCT_DICT_DEF_H

#ifdef DEFINE_TABLE_STRUCT_ENABLE
#define DEFINE_TABLE_STRUCT_VAR_DIRECT(struct_name, table_name, entry_id, entry_num)                    \
    volatile struct_name sml_table_struct_var_prefix__##struct_name##__##entry_id##__##entry_num;       \
    volatile const char sml_table_struct_table_name_var_prefix__##struct_name[] = #table_name

#define DEFINE_TABLE_STRUCT_VAR(struct_name, table_name, entry_id, entry_num)                           \
    DEFINE_TABLE_STRUCT_VAR_DIRECT(struct_name, table_name, entry_id, entry_num)
#else
#define DEFINE_TABLE_STRUCT_VAR(struct_name, table_name, entry_id, entry_num)
#endif

#endif
