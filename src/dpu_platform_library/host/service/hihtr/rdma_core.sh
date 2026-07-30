#!/usr/bin/env bash
# Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
set -e

echo "start to build rdma-core dependency"
export EXTRA_CMAKE_FLAGS='-DCMAKE_C_FLAGS=-Wno-format-overflow'

get_kernel_version_code() {
    local major=$1
    local minor=$2
    local patch=$3
    local version_code=$((major * 65536 + minor * 256 + patch))
    echo $version_code
}
# 获取当前 Linux 内核版本
current_kernel_version=$(uname -r)
echo "kernel version:$current_kernel_version"
current_major=$(echo $current_kernel_version | awk -F. '{print $1}')
current_minor=$(echo $current_kernel_version | awk -F. '{print $2}')
current_patch=$(echo $current_kernel_version | awk -F. '{print $3}' | awk -F- '{print $1}')
current_kernel_version_code=$(get_kernel_version_code $current_major $current_minor $current_patch)
target_kernel_version_code=$(get_kernel_version_code 6 6 0)

hihtr_abi_path="${WORKSPACE}/ChipSolution/src/dpu_platform_library/host/service/hihtr/abi/hihtr-abi.h"
target_hihtr_abi_path="${WORKSPACE}/ChipSolution/rdma-core/kernel-headers/rdma/hihtr-abi.h"
rdma_core_cmakefile="${WORKSPACE}/ChipSolution/rdma-core/kernel-headers/CMakeLists.txt"

cp -f "$hihtr_abi_path" "$target_hihtr_abi_path"

if [ ! -f "$rdma_core_cmakefile" ]; then
  echo "Error: $rdma_core_cmakefile not found!"
  exit 1
fi

sed -i '/publish_internal_headers(rdma/,/)/ s/)/rdma\/hihtr-abi.h\n  )/' "$rdma_core_cmakefile"
sed -i '/rdma_kernel_provider_abi(/,/)/ s/)/rdma\/hihtr-abi.h\n  )/' "$rdma_core_cmakefile"

cd ${WORKSPACE}/ChipSolution/rdma-core/
bash build.sh
echo "end to build rdma-core dependency"