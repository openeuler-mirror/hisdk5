#!/usr/bin/env bash
# Perform Compilation of roce lib.
# Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
set -e

echo "start to compile roce userspace library"

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
rdma_core_depend_dir=$cur_dir/depends/rdma-core
# 默认编译任务是跑在流水线环境下的，但是我们有本地测试的需求，因此添加自动判断逻辑; 否则就需要频繁的修改脚本
pipeline=y
export EXTRA_CMAKE_FLAGS='-DCMAKE_C_FLAGS=-Wno-format-overflow'

if [[ $# -eq 1 ]]; then
    pipeline=n
    root_dir=$(realpath $1)
    rdma_core_dir=${root_dir}/rdma-core
    if [[ ! -d "${rdma_core_dir}" ]]; then
        echo "no such directory: ${rdma_core_dir}"
        echo "error: the directory you specify is not found, please check!"
        exit 1
    fi
else
    if [[ -d "${rdma_core_depend_dir}/providers" ]] ; then
        pipeline=n
        # 本地环境下，依赖放置的路径
        rdma_core_dir=${cur_dir}/depends/rdma-core
    fi
fi

if [[ ! -d "${rdma_core_dir}" ]];then
    echo "no such directory: ${rdma_core_dir}"
    echo "error: must have dependency, please check!"
    exit 1
fi

# 执行用户态生成宏的脚本
bash $cur_dir/roce-ucompat-generator.sh
if [ $? -eq 0 ]; then
    echo "roce-ucompat-generator.sh 执行成功"
    cat -n "$cur_dir/build_userspace_scripts/output/roce_ucompat_gen.h"
else
    echo "roce-ucompat-generator.sh 执行失败"
    exit 1
fi

cmake_build_dir=$cur_dir/build_userspace_scripts/build
mkdir -p $cmake_build_dir

rdma_core_build_dir=/tmp/rdma-core
[[ ! -z "$rdma_core_build_dir" ]] && rm -rf $rdma_core_build_dir
# 因为目录层次太深，导致全路径长度非常长，会导致编译错误
ln -s ${rdma_core_dir} $rdma_core_build_dir

# 需要打包的文件，放置到output_dir
output_dir=$cur_dir/build_userspace_scripts/output
mkdir -p $output_dir

if [[ "${pipeline}" == "y" ]];then
    cd $rdma_core_build_dir
    bash $rdma_core_build_dir/build.sh
    cd $cur_dir

    cd $cmake_build_dir
    cmake -DROOT_PROJECT_DIR=$root_dir ..
    make hihtr
    cd $cur_dir
else
    rm -rf ${rdma_core_dir}/providers/hns
    ln -s ${cur_dir}/user ${rdma_core_dir}/providers/hns
    cd $rdma_core_build_dir
    bash $rdma_core_build_dir/build.sh
    cd $cur_dir
fi

if [[ "${pipeline}" == "y" ]];then
    cp $cmake_build_dir/libhihtr.so $output_dir
    chmod 755 $output_dir/libhihtr.so
    #不同的rdma-core版本，对应的version不一样，在编译的时候直接确定对应的.so名称
    ROCE_LIB_VER=$(grep "IBVERBS_PABI_VERSION " "${root_dir}/rdma-core/CMakeLists.txt" | awk -F '"' '{print $2}')
    cp $cmake_build_dir/libhihtr.so $output_dir/libhihtr-rdmav$ROCE_LIB_VER.so
    chmod 755 $output_dir/libhihtr-rdmav$ROCE_LIB_VER.so
    cp $rdma_core_build_dir/build/lib/libibverbs.so $output_dir
    tar -czf $output_dir/rdma-build.tar.gz $rdma_core_build_dir/build/bin $rdma_core_build_dir/build/lib $rdma_core_build_dir/build/include
else
    if [ -e "$rdma_core_build_dir/build/lib/libhns-rdmav34.so"];then
        cp $rdma_core_build_dir/build/lib/libhns-rdmav34.so $output_dir/libhihtr.so
        chmod 755 $output_dir/libhihtr.so
    else
        cp $rdma_core_build_dir/build/lib/libhns-rdmav22.so $output_dir/libhihtr.so
        chmod 755 $output_dir/libhihtr.so
    fi
    cp $rdma_core_build_dir/build/lib/libibverbs.so $output_dir/libibverbs.so
    tar -czf $output_dir/rdma-build.tar.gz $rdma_core_build_dir/build/bin $rdma_core_build_dir/build/lib $rdma_core_build_dir/build/include
fi

echo "succeed to compile roce userspace library"