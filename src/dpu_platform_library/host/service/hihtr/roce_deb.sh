#!/usr/bin/env bash
# Copyright (c) Huawei Technologies Co., Ltd. 2026. All rights reserved.
set -e

function build_roce_deb(){
    # python script dir: src/dpu_platform_library/host/service/hihtr/
    # src/GLOBAL_VERSION_NEW
    # roce deb build path:
    # src/dpu_platform_library/host/service/hihtr/hihtr5_deb_build/arm64/
    # 获取当前工作目录（执行python命令时所在的目录）
    current_dir=$(pwd)
    workdir=$current_dir/hihtr5_deb_build 
    hihtr5_gids=$workdir/hihtr5
    kernel_release=$(uname -r)
    kernel_release_underline="${kernel_release//-/_}"
    arch=$(arch)
    driver_version=$(cat $current_dir/../../../../GLOBAL_VERSION_NEW |grep driver|awk -F: '{print $2}')

    distdir=$workdir/$arch
    control_file=${distdir}/DEBIAN/control
    modules_dir=${distdir}/lib/modules
    # no idea why there are two modules-load.d
    modules_load_dir=${distdir}/etc/modules-load.d
    dracut_conf_dir=${distdir}/etc/dracut-conf.d
    libibverbs_dir=${distdir}/etc/libibverbs.d

    ko_dir=${modules_dir}/${kernel_release}/updates/hihtr5
    lib_path=$distdir/usr/lib/$arch-linux-gnu
    libibverbs_path=$lib_path/libibverbs

    hihtr5_gids_path=$distdir/usr/local/bin

    # ko and so outputpath
    kernel_output_dir=$current_dir/build
    userspace_output_dir=$current_dir/build_userspace_scripts/output
    
    # 复制内核模块
    mkdir -p $ko_dir
    cp -rf $kernel_output_dir/*.ko $ko_dir/

    mkdir -p $lib_path
    mkdir -p $libibverbs_path
    cp -rf $userspace_output_dir/libhihtr.so $lib_path/
    cp -rf $userspace_output_dir/libhihtr.so $libibverbs_path/
    # 直接copy，而非使用软链接
    cp -rf $userspace_output_dir/libhihtr-rdmav*.so $lib_path/

    mkdir -p $dracut_conf_dir
    touch $dracut_conf_dir/hihtr5.conf

    mkdir -p $libibverbs_dir
    echo "driver hihtr" > $libibverbs_dir/hihtr5.driver

    # 更新control文件版本号
    sed -i "/^Version/c Version: ${driver_version}" ${control_file}

    # 创建模块加载配置
    mkdir -p $modules_load_dir
    cp $workdir/hihtr5-modules.conf $modules_load_dir/

    # hihtr5 gids
    mkdir -p $hihtr5_gids_path
    cp $hihtr5_gids $hihtr5_gids_path/
    
    sed -i 's/\r$//' $hihtr5_gids_path/hihtr5
    chmod -R 775 ${distdir}/
    chmod 755 $libibverbs_path/libhihtr.so
    chmod 755 $lib_path/libhihtr.so
    chmod 755 $lib_path/libhihtr-rdmav*.so
    chmod 644 $dracut_conf_dir/hihtr5.conf
    chmod 644 $libibverbs_dir/hihtr5.driver
    dpkg-deb --root-owner-group -b $distdir hihtr5-${driver_version}-${kernel_release_underline}.$arch.deb
}

build_roce_deb