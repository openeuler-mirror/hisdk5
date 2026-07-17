#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-only
# Copyright(c) 2025 Huawei Technologies Co., Ltd

set -e
# set -Eeuo pipefail

# This file is used to automatically generate HAVE_ and NEED_ macros 
# for the current or incoming kernel.
#
# It is primarily implemented by invoking the 'gen' function in the kcompat-lib.sh file 
# (you can refer to the main part of 'gen_xx' for details).
# The 'gen' tool can search for declarations of various types in kernel header files to generate macros.
# For example, search for a function in a specified file and check whether the given function exists.
# Please refer to the comments above the 'gen' function in kcompat-lib.sh.

# End of intro.
# The implementation is in kcompat-lib.sh, and below is an example of the 'gen' invocation.

export LC_ALL=C
# ROCE_USER_ORIG_CWD="$(pwd)"
# trap 'rc=$?; echo >&2 "$(realpath "$ROCE_USER_ORIG_CWD/${BASH_SOURCE[0]}"):$LINENO: failed with rc: $rc"' ERR

# shellcheck source=kcompat-lib.sh
KCOMPAT_LIB=$(find ./ -name roce-kcompat-lib.sh)
source "$KCOMPAT_LIB"

cur_dir=$(dirname $0)
cur_dir=$(realpath $cur_dir)

# 与CIE约定好的依赖路径
root_dir=$(realpath ${cur_dir}/../../../../../)
rdma_core_dir=${root_dir}/rdma-core

# 优先级顺序
#    A: $1 指定依赖路径  (本地编译)
#    B: depends/rdma-core  （本地编译）
#    C: cie配置的依赖 $(realpath ${cur_dir}/../..)
# 分支中未添加depends/rdma-core时，可以使用这种方式，用于本地测试

# 为了便于开发，可以在自己的仓库中添加依赖，并不推送到主代码仓
# rdma_core_depend_dir=$cur_dir/depends/rdma-core
# 默认编译任务是跑在流水线环境下的，但是我们有本地测试的需求，因此添加自动判断逻辑; 否则就需要频繁的修改脚本
# pipeline=y

# if [[ $# -eq 1 ]]; then
#     pipeline=n
#     root_dir=$(realpath $1)
#     rdma_core_dir=${root_dir}/rdma-core
#     if [[ ! -d "${rdma_core_dir}" ]]; then 
#         echo "no such directory: ${rdma_core_dir}"
#         echo "error: the directory you specify is not found, please check!"
#         exit 1
#     fi
# else
#     if [[ -d "${rdma_core_depend_dir}/providers" ]] ; then 
#         pipeline=n
#         # 本地环境下，依赖放置的路径
#         rdma_core_dir=${cur_dir}/depends/rdma-core
#     fi
# fi

if [[ ! -d "${rdma_core_dir}" ]];then
    echo "no such directory: ${rdma_core_dir}"
    echo "error: must have dependency, please check!"
    exit 1
fi

function gen_roce_user_util() {
    util="util/util.h"

    gen UTIL_NEED_BIT if macro BIT absent in "$util"
    gen UTIL_NEED_DIV_ROUND_UP if fun DIV_ROUND_UP absent in "$util"
    gen UTIL_NEED_ALIGN if fun align absent in "$util"
    gen UTIL_NEED_FIELD_GET if macro FIELD_GET absent in "$util"
    gen UTIL_NEED_GENMASK if macro GENMASK absent in "$util"
    gen UTIL_NEED_ROUNDUP_POW_OF_TWO if fun roundup_pow_of_two absent in "$util"
    gen UTIL_NEED_GET_RANDOM if fun get_random absent in "$util"
}

function gen_roce_user_libibverbs_driver() {
    verbs_driver="libibverbs/driver.h"

    gen REG_MR_HAVE_HCA_VA if method reg_mr of verbs_context_ops matches 'uint64_t hca_va' in "$verbs_driver"
    gen VERBS_DRIVER_NEED_VERBS_CQ if struct verbs_cq absent in "$verbs_driver"
    gen DEVICE_OPS_HAVE_FREE_CTX if method free_context of verbs_device_ops matches 'free_context' in "$verbs_driver"
    gen CONTEXT_OPS_HAVE_FREE_CTX if method free_context of verbs_context_ops matches 'free_context' in "$verbs_driver"
    gen VERBS_DRIVER_NEED_VERBS_LOG if macro verbs_log absent in "$verbs_driver"
    gen HAVE_IBV_CMD_QUERY_DEV_ANY if fun ibv_cmd_query_device_any in "$verbs_driver"
    gen CREATE_QP_EX2_HAVE_VQP_SZ if fun ibv_cmd_create_qp_ex2 matches 'int vqp_sz' in "$verbs_driver"
    gen NEED_IBV_GID_TYPE_SYSFS if enum ibv_gid_type_sysfs absent in "$verbs_driver"
    gen CREATE_SRQ_EX_HAVE_VQP_SZ if fun ibv_cmd_create_srq_ex matches 'int vsrq_sz' in "$verbs_driver"
    gen CREATE_CQ_EX_HAVE_VERBS_CQ if fun ibv_cmd_create_cq_ex matches 'struct verbs_cq' in "$verbs_driver"
    gen CREATE_CQ_EX_HAVE_CMD_FLAGS if fun ibv_cmd_create_cq_ex matches 'uint32_t cmd_flags' in "$verbs_driver"
    gen VERBS_OPS_HAVE_QUERY_DEVICE if method query_device of verbs_context_ops matches '\\(\\*query_device\\)' in "$verbs_driver"
    gen ALLOC_CONTEXT_HAVE_PRIVATE_DATA if method alloc_context of verbs_device_ops matches 'private_data' in "$verbs_driver"
    gen IBV_CMD_MODIFY_QP_EX_HAVE_CMD_CORE_SZ if fun ibv_cmd_modify_qp_ex matches 'size_t cmd_core_size' in "$verbs_driver"
    gen IBV_CMD_QUERY_DEVICE_EX_HAVE_CMD_CORE_SZ if fun ibv_cmd_query_device_ex matches 'size_t cmd_core_size' in "$verbs_driver"
    gen IBV_CMD_CREATE_QP_EX2_HAVE_CMD_CORE_SZ if fun ibv_cmd_create_qp_ex2 matches 'size_t cmd_core_size' in "$verbs_driver"
    gen HAVE_PROVIDER_DRIVER_IN_DRIVER if macro PROVIDER_DRIVER matches 'provider_name' in "$verbs_driver"
    gen HAVE_VERBS_DRIVER_ID if macro VERBS_DRIVER_ID in "$verbs_driver"
    gen IBV_CMD_GET_CONTEXT_HAVE_IBV_FD_ARR if fun ibv_cmd_get_context matches 'ibv_fd_arr' in "$verbs_driver"
}

function gen_roce_user_libibmad_mad() {
    mad="libibmad/mad.h"

    gen MAD_HAVE_ALIGN if macro ALIGN in "$mad"
}

function gen_roce_user_ccan_ilog() {
    ilog="ccan/ilog.h"

    gen ILOG_HAVE_ILOG32 if fun ilog32 in "$ilog"
}

function gen_roce_user_libibverbs_verbs() {
    verbs="libibverbs/verbs.h"

    gen VERBS_NEED_IBV_DATA_BUF if struct ibv_data_buf absent in "$verbs"
    gen VERBS_HAVE_IBV_QP_EX if struct ibv_qp_ex in "$verbs"
    gen HAVE_IBV_QP_INIT_ATTR_SEND_OPS_FLAGS if enum ibv_qp_init_attr_mask matches IBV_QP_INIT_ATTR_SEND_OPS_FLAGS in "$verbs"
    gen NEED_IB_GRH_FLOWLABEL_MASK if macro IB_GRH_FLOWLABEL_MASK absent in "$verbs"
    gen NEED_IBV_FLOW_LABEL_TO_UDP_SPORT if fun ibv_flow_label_to_udp_sport absent in "$verbs"
}

function gen_roce_user_util_bitmap() {
    bitmap="util/bitmap.h"

    gen UTIL_BITMAP_HAVE_BITMAP_ALLOC1 if fun bitmap_alloc1 in "$bitmap"
    gen BITMAP_HAVE_BITMAP_FIND_FIRST_BIT if fun bitmap_find_first_bit in "$bitmap"
}

# all the generations, extracted from main() to keep normal code and various
# prep separated
function gen_roce_user_all() {
    gen_roce_user_util
    gen_roce_user_libibverbs_driver
    gen_roce_user_libibmad_mad
    gen_roce_user_ccan_ilog
    gen_roce_user_libibverbs_verbs
    gen_roce_user_util_bitmap
}

function gen_roce_user_compat() {
    output_dir=$cur_dir/build_userspace_scripts/output
    mkdir -p $output_dir

    OUT=${output_dir}/roce_ucompat_gen.h

	# check if caller (like our makefile) wants to redirect output to file
	if [ -n "${OUT-}" ]; then

		# in case OUT exists, we don't want to overwrite it, instead
		# write to a temporary copy.
		if [ -s "${OUT}" ]; then
			TMP_OUT="$(mktemp "${OUT}.XXX")"
			trap "rm -f '${TMP_OUT}'" EXIT
 
			REAL_OUT="${OUT}"
			OUT="${TMP_OUT}"
		fi

		exec 3>&1
		exec > "$OUT"
 
        echo "/* uname=$(uname -r) Autogenerated for KSRC=${KSRC-} via $(basename "$0") */"
	fi

    cd "${rdma_core_dir}"

	echo "#ifndef _ROCE_UCOMPAT_H_"
	echo "#define _ROCE_UCOMPAT_H_"

    set +x
	gen_roce_user_all
    set -x

	echo "#endif /* _ROCE_UCOMPAT_H_ */"

    exec >&2
	if [ -n "${OUT-}" ]; then
		# Compare and see if anything changed. This avoids updating
		# mtime of the file.
		if [ -n "${REAL_OUT-}" ]; then
			if cmp --silent "${REAL_OUT}" "${TMP_OUT}"; then
				# exit now, skipping print of the output since
				# there were no changes. the trap should
				# cleanup TMP_OUT
                rm -f "${TMP_OUT}"
			    return 0
			fi

			mv -f "${TMP_OUT}" "${REAL_OUT}"
			OUT="${REAL_OUT}"
		fi

	fi
}

gen_roce_user_compat