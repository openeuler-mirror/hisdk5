#!/bin/bash
#----------------------------------------------------------------------------
# Purpose:
# Copyright Huawei Technologies Co., Ltd. 2026. All rights reserved.
#----------------------------------------------------------------------------
set -e
TOP_DIR=$(pwd)
CMD=""
OFED_VER=""
KERNEL_VER="$(uname -r)"
KERNEL_VER_PATH="/usr/lib/modules"
COMPILE_CHECK_FLAG="true"
LOG_FILE="${TOP_DIR}/log.txt"
LOG_FLAG=""
HUAWEI_SECURE_CODE_DIR="${TOP_DIR}/platform/huawei_secure_c"
ERR_FLAG=0
build_run=true

source "${TOP_DIR}"/get_osname.sh
source "$TOP_DIR"/check.sh

build_help()
{
    echo -e "\nA scripts to compile driver of nic, bifur or roce, all parameters are the following:\n"
    echo -e "-c      check compile dependencies"
    echo -e "-d      driver to be installed(nic, bifur or roce)"
    echo -e "-h      provide detailed command guidance information"
    echo -e "-k      version of kernel, which is in /usr/lib/modules/"
    echo -e "-l      record log to ./log.txt"
    echo -e "-o      version of ofed, only used by roce(recommended M5_1、M5_4、M5_8、M24_10)"
    echo -e "-r      version of rdma-core, only used by roce(recommended R41_0、R22_4)"
    echo -e "-t      type of compile OS, only for specific scenarios"
    echo -e "Other cases show the help information of this script.\n"
    exit 0
}

# Direct compilation of rdma-core source package
build_rdma_core_direct() {
    local tar_file="$1"
    local target_dir="$2"
    local original_dir=$(pwd)

    echo "Processing direct rdma-core source package..."

    # Get source directory name (remove extension)
    local src_dir_name=$(basename "$tar_file")
    src_dir_name=${src_dir_name%.tar.gz}
    src_dir_name=${src_dir_name%.tgz}
    src_dir_name=${src_dir_name%.zip}

    # Clean previous build
    rm -rf "$target_dir"
    # rm -rf "$target_dir/$src_dir_name"

    # Extract source code based on file type
    echo "Extracting $tar_file to $target_dir"

    case "$tar_file" in
        *.tar.gz|*.tgz)
            tar -xzf "$tar_file"
            ;;
        *.zip)
            unzip -q "$tar_file"
            ;;
        *)
            echo "Error: Unsupported file format: $tar_file"
            return 1
            ;;
    esac
    mv $src_dir_name $target_dir

    if [ $? -ne 0 ]; then
        echo "Error: Failed to extract $tar_file"
        return 1
    fi

    # Enter source directory and compile
    cd "$target_dir"
    echo "Entering directory: $(pwd)"
    RDMA_CORE_PATH=$(pwd)

    # Check if build.sh exists
    if [ ! -f "build.sh" ]; then
        echo "Error: build.sh script not found"
        cd "$original_dir"
        return 1
    fi

    hihtr_abi_path="${TOP_DIR}/src/dpu_platform_library/host/service/hihtr/abi/hihtr-abi.h"
    target_hihtr_abi_path="${TOP_DIR}/rdma-core/kernel-headers/rdma/hihtr-abi.h"
    rdma_core_cmakefile="${TOP_DIR}/rdma-core/kernel-headers/CMakeLists.txt"

    if [ ! -f "$hihtr_abi_path" ]; then
        echo "Error: $hihtr_abi_path not found!"
        return 1
    fi

    cp -f "$hihtr_abi_path" "$target_hihtr_abi_path"

    if [ ! -f "$rdma_core_cmakefile" ]; then
        echo "Error: $rdma_core_cmakefile not found!"
        return 1
    fi

    sed -i '/publish_internal_headers(rdma/,/)/ s/)/rdma\/hihtr-abi.h\n  )/' "$rdma_core_cmakefile"
    sed -i '/rdma_kernel_provider_abi(/,/)/ s/)/rdma\/hihtr-abi.h\n  )/' "$rdma_core_cmakefile"

    # Add execute permission to build.sh
    chmod +x build.sh

    # Run build script
    echo "Starting rdma-core compilation..."
    ./build.sh

    local build_result=$?
    cd "$original_dir"

    if [ $build_result -eq 0 ]; then
        echo "rdma-core compilation successful"
    else
        echo "Error: rdma-core compilation failed"
        return 1
    fi
}

# Extract and compile rdma-core from MLNX OFED source package
build_rdma_core_from_mlnx_rpm() {
    local tar_file="$1"
    local target_dir="$2"
    local temp_dir=$(mktemp -d)
    local original_dir=${TOP_DIR}

    echo "Extracting rdma-core from MLNX OFED source package..."

    # Create RPM build directory structure
    mkdir -p ~/rpmbuild/{BUILD,RPMS,SOURCES,SPECS,SRPMS}

    # Extract MLNX OFED source package to temporary directory
    echo "Extracting MLNX OFED source package..."
    tar -xzf "$tar_file" -C "$temp_dir"

    if [ $? -ne 0 ]; then
        echo "Error: Failed to extract MLNX OFED source package"
        rm -rf "$temp_dir"
        return 1
    fi

    # Find rdma-core src.rpm file
    local src_rpm=$(find "$temp_dir" -name "rdma-core-*.src.rpm" | head -1)

    if [ -z "$src_rpm" ]; then
        echo "Error: Could not find rdma-core src.rpm in MLNX OFED package"
        rm -rf "$temp_dir"
        return 1
    fi

    echo "Found src.rpm: $src_rpm"

    # Install src.rpm
    echo "Installing src.rpm..."
    rpm -ivh "$src_rpm"

    if [ $? -ne 0 ]; then
        echo "Error: Failed to install src.rpm"
        rm -rf "$temp_dir"
        return 1
    fi

    # Find rdma-core tar.gz file in SOURCES directory
    local rdma_core_tar=$(find ~/rpmbuild/SOURCES -regex ".*/rdma-core-.*${MLNX_RPM_VERSION}.*\.\(tar\.gz\|tgz\)" | head -1)

    if [ -z "$rdma_core_tar" ]; then
        echo "Error: Could not find rdma-core tar.gz in ~/rpmbuild/SOURCES"
        rm -rf "$temp_dir"
        return 1
    fi

    echo "Found rdma-core source package: $rdma_core_tar"

    # Compile using the found source package
    build_rdma_core_direct "$rdma_core_tar" "$target_dir"

    local build_result=$?

    # Clean up temporary directory
    rm -rf "$temp_dir"

    return $build_result
}

build_ofed_src()
{
    local buildtools_dir="."
    local rdma_core_dir="$buildtools_dir/rdma-core"

    echo "=== Starting OFED source code compilation ==="

    # Create build directory
    if [ ! -d "$rdma_core_dir" ]; then
        mkdir -p "$rdma_core_dir"
    fi

    if [[ "$OFED_SRC_NAME" == *"MLNX"* && "$OFED_SRC_NAME" != *"rdma-core"* ]]; then
        build_rdma_core_from_mlnx_rpm "${OFED_SRC_NAME}" "$rdma_core_dir"
    else
        build_rdma_core_direct "${OFED_SRC_NAME}" "$rdma_core_dir"
    fi

    echo "=== OFED source code compilation completed ==="
}


change_Makefile()
{
    sed -i "s/KERNEL_VER.*:=.*/KERNEL_VER:=${KERNEL_VER}/g" ./Makefile
    sed -i "s/KERNEL_VER.*:=.*/KERNEL_VER:=${KERNEL_VER}/g" ${TOP_DIR}/build/host/linux/Makefile.ko
    sed -i "s/KERN_VER.*:=.*/KERN_VER:=${KERNEL_VER}/g" ${TOP_DIR}/build/host/linux/sdk/sdk-kcompat-generator.sh
    sed -i "s/KERN_VER.*:=.*/KERN_VER:=${KERNEL_VER}/g" ${TOP_DIR}/build/host/linux/nic/nic-kcompat-generator.sh
    sed -i "s/BUILD_KERNEL.*:=.*/BUILD_KERNEL:=${KERNEL_VER}/g" ${TOP_DIR}/src/dpu_platform_library/host/service/hihtr/roce-kcompat-generator.sh

}


change_spec()
{
    for file in ./*
    do
        num=$(grep -n "%define ker_ver" $file |awk -F ':' '{print $1}')
        sed -i "${num}s/uname -r/echo %(uname -r)/" $file
    done

    sed -i "s/%(uname -r)/${KERNEL_VER}/g" ./*.spec
}

export_stdarg()
{
    # When compiling on an OS with kernel version 5.15 or later,
    # the compilation error occurs because the linux/stdarg.h header
    # file was introduced starting with kernel, which conflicts
    # with the GCC header file.

    # find kernel src path
    local KERN_VER=$(uname -r)
    local KSRC
    if [ -e /usr/src/kernels/linux-$KERN_VER/include/config ]; then
        KSRC="/usr/src/kernels/linux-$KERN_VER/"
    elif [ -e /usr/src/kernels/$KERN_VER/include/config ]; then
        KSRC="/usr/src/kernels/$KERN_VER/"
    elif [ -e /lib/modules/$KERN_VER/build/include/config ]; then
        KSRC="/lib/modules/$KERN_VER/build/"
    fi

    if [ -z "$KSRC" ]; then
        KERN_VER=`uname -r | sed 's/\([0-9]*\.[0-9]*\)\..*/\1/'`
        if [ -e /usr/src/kernels/linux-$KERN_VER/include/config ]; then
            KSRC="/usr/src/kernels/linux-$KERN_VER/"
        elif [ -e /usr/src/kernels/$KERN_VER/include/config ]; then
            KSRC="/usr/src/kernels/$KERN_VER/"
        elif [ -e /lib/modules/$KERN_VER/build/include/config ]; then
            KSRC="/lib/modules/$KERN_VER/build/"
        fi
    fi

    if find $KSRC -name "stdarg.h" 2>/dev/null | grep -q "linux/stdarg.h"; then
        export C_INCLUDE_PATH=$(dirname "$(find / -name "stdarg.h"| grep gcc | grep include)"):$C_INCLUDE_PATH
        echo "C_INCLUDE_PATH path set to: $C_INCLUDE_PATH"
    else
        echo "Header file linux/stdarg.h does not exist. C_INCLUDE_PATH path no need to set."
    fi
}

build_sdk()
{
    cd ${TOP_DIR}/src/dpu_platform_library/host/sdk/knldk/lld
    change_Makefile
    make clean
    eval make -s -j 16 ${LOG_FLAG}
    mv hisdk5.ko hisdk5.ko.tmp
    ERR_FLAG=0
    strip --strip-debug hisdk5.ko.tmp -o hisdk5.ko || ERR_FLAG=1
    if [ ${ERR_FLAG} -ne 0 ]; then
        cp hisdk5.ko.tmp ${TOP_DIR}/output/binary/hisdk5.ko
        echo "strip failed"
    else
        cp hisdk5.ko ${TOP_DIR}/output/binary
    fi
    cp Module.symvers ${TOP_DIR}/ci/lib/SDK_Module.symvers
    cp Module.symvers ${TOP_DIR}/output/SDK_Module.symvers
}

build_hinic()
{
    cd ${TOP_DIR}/src/dpu_platform_library/host/service/nic/linux
    change_Makefile
    make clean
    eval make -s -j 16 ${LOG_FLAG}
    mv hinic5.ko hinic5.ko.tmp
    strip --strip-debug hinic5.ko.tmp -o hinic5.ko || ERR_FLAG=1
    if [ ${ERR_FLAG} -ne 0 ]; then
        cp hinic5.ko.tmp ${TOP_DIR}/output/binary/hinic5.ko
        echo "strip failed"
    else
        cp hinic5.ko ${TOP_DIR}/output/binary
    fi
    cp Module.symvers ${TOP_DIR}/ci/lib/NIC_Module.symvers
    cp Module.symvers ${TOP_DIR}/output/NIC_Module.symvers
}

build_hibifur()
{
    cd ${TOP_DIR}/src/dpu_extended_library/host/bifur/kernel
    change_Makefile
    make clean
    eval make -s -j 16 ${LOG_FLAG}
    mv hibifur5.ko hibifur5.ko.tmp
    strip --strip-debug hibifur5.ko.tmp -o hibifur5.ko || ERR_FLAG=1
    if [ ${ERR_FLAG} -ne 0 ]; then
        cp hibifur5.ko.tmp ${TOP_DIR}/output/binary/hibifur5.ko
        echo "strip failed"
    else
        cp hibifur5.ko ${TOP_DIR}/output/binary
    fi
    cp Module.symvers ${TOP_DIR}/ci/lib/BIFUR_Module.symvers
    cp Module.symvers ${TOP_DIR}/output/BIFUR_Module.symvers
}

build_hiroce_kern()
{
    cd ${TOP_DIR}/src/dpu_platform_library/host/service/hihtr/
    if [ $1 == "OFED_SRC" ]; then
        bash roce-kcompat-generator.sh ofed_src
    else
        bash roce-kcompat-generator.sh
    fi
    cat roce_kcompat_gen.h

    cd ${TOP_DIR}/src/dpu_platform_library/host/service/hihtr/kernel
    change_Makefile
    # make clean
    eval make CONFIG_INFINIBAND_HIHTR=m -C /lib/modules/$(uname -r)/build M=$(pwd) modules -j 16 ${LOG_FLAG}

    mv hihtr5.ko hihtr5.ko.tmp
    strip --strip-debug hihtr5.ko.tmp -o hihtr5.ko || ERR_FLAG=1
    if [ ${ERR_FLAG} -ne 0 ]; then
        cp hihtr5.ko.tmp ${TOP_DIR}/output/binary/hihtr5.ko
        echo "strip failed"
    else
        cp hihtr5.ko ${TOP_DIR}/output/binary
    fi
}

function build_huawei_securec()
{
	cd $HUAWEI_SECURE_CODE_DIR/src
    make clean

    make -s -C $HUAWEI_SECURE_CODE_DIR/src
    make -s -C $HUAWEI_SECURE_CODE_DIR/src lib
}

build_hiroce_user()
{
    cd ${TOP_DIR}/src/dpu_platform_library/host/service/hihtr

    bash roce-ucompat-generator.sh
    cmake_build_dir=${TOP_DIR}/src/dpu_platform_library/host/service/hihtr/build_userspace_scripts/build
    mkdir -p $cmake_build_dir

    cd $cmake_build_dir
    cmake -DROOT_PROJECT_DIR=$TOP_DIR ..
    make hihtr

    cp $cmake_build_dir/libhihtr.so ${TOP_DIR}/output/binary

    if [ -f "${TOP_DIR}/rdma-core/CMakeLists.txt" ]; then
        ROCE_LIB_VER=$(grep "IBVERBS_PABI_VERSION " "${TOP_DIR}/rdma-core/CMakeLists.txt" | awk -F '"' '{print $2}')
    else
        ROCE_LIB_VER=34
    fi
    cp $cmake_build_dir/libhihtr.so ${TOP_DIR}/output/binary/libhihtr-rdmav$ROCE_LIB_VER.so


}

build_rpm_for_spec()
{
    local SPEC_FILE=$1
    if [[ $(arch) = "x86_64" ]]; then
        rpmbuild --define "_topdir ${TOP_DIR}/rpmbuild" -bb --quiet $SPEC_FILE
    else
        rpmbuild --define "_topdir ${TOP_DIR}/rpmbuild" -bb --quiet $SPEC_FILE --target=aarch64
    fi
}

build_rpm()
{
    set +x
    cd ${TOP_DIR}
    mkdir -pv -m 777 rpmbuild/{BUILD,BUILDROOT,RPMS,SOURCES,SPECS,SRPMS}
    cp output/binary/*.ko rpmbuild/SOURCES
    if [[ "$CMD" == *"roce"* ]]; then
        cp output/binary/*.so rpmbuild/SOURCES
    fi
    cp src/GLOBAL_VERSION_NEW rpmbuild/SOURCES
    cp build/host/linux/sdk/sdk_rpm_build/*.conf rpmbuild/SOURCES
    cp build/host/linux/nic/nic_rpm_build/*.conf rpmbuild/SOURCES
    cp src/dpu_platform_library/host/service/hihtr/hihtr5_rpm_build/*.conf  rpmbuild/SOURCES
    cp src/dpu_platform_library/host/service/hihtr/hihtr5_rpm_build/hihtr5  rpmbuild/SOURCES
    cp build/host/linux/bifur/bifur_rpm_build/*.conf rpmbuild/SOURCES
    cp build/host/linux/sdk/sdk_rpm_build/*.spec rpmbuild/SPECS
    cp build/host/linux/nic/nic_rpm_build/*.spec rpmbuild/SPECS
    cp src/dpu_platform_library/host/service/hihtr/hihtr5_rpm_build/*.spec rpmbuild/SPECS
    cp build/host/linux/bifur/bifur_rpm_build/*.spec rpmbuild/SPECS

    cd rpmbuild/SPECS
    change_spec

    build_rpm_for_spec hisdk5.spec
    build_rpm_for_spec hinic5.spec

    if [[ "$CMD" == *"bifur"* ]]; then
        build_rpm_for_spec hibifur5.spec
    fi

    if [[ "$CMD" == *"roce"* ]]; then
        build_rpm_for_spec hihtr5.spec
    fi

    cp -f ${TOP_DIR}/rpmbuild/RPMS/$(arch)/*.rpm ${TOP_DIR}/output/rpmdebs/
    mv_file_to_folder
}

mv_file_to_folder()
{
    find "${TOP_DIR}/output/rpmdebs/" -maxdepth 1 -name "*.$pkg_format" -type f | while read -r file; do
        filename=$(basename "$file")
        case "$filename" in
            *hinic5*|*hisdk5*)
                mv "$file" "${TOP_DIR}/output/rpmdebs/nic/"
                ;;
            *hihtr5*)
                mv "$file" "${TOP_DIR}/output/rpmdebs/roce/"
                ;;
            *hibifur5*)
                mv "$file" "${TOP_DIR}/output/rpmdebs/bifur/"
                ;;
        esac
    done
}

build_deb()
{
    local global_ver=$(cat ${TOP_DIR}/src/GLOBAL_VERSION_NEW | grep driver | awk -F ':' '{print $2}')
    local ker_ver=$(uname -r | awk -F ".el" '{print $1}' | sed 's/-/_/g')
    local hihtr5_deb_build_path=${TOP_DIR}/debbuild/$(arch)
    local deb_ko_path=${hihtr5_deb_build_path}/lib/modules
    local deb_debian_path=${hihtr5_deb_build_path}/DEBIAN
    local modules_load_dir=${hihtr5_deb_build_path}/etc/modules-load.d
    local dracut_conf_dir=${hihtr5_deb_build_path}/etc/dracut-conf.d
    local libibverbs_dir=${hihtr5_deb_build_path}/etc/libibverbs.d

    if [ "$(arch)" == "x86_64" ]; then
        arch_name="amd64"
    else
        arch_name="arm64"
    fi

    #hisdk5
    rm -rf ${TOP_DIR}/debbuild/*
    mkdir -p ${deb_debian_path}
    mkdir -p ${deb_ko_path}/$(uname -r)/updates/hisdk5
    cp ${TOP_DIR}/output/binary/hiudk5.ko ${deb_ko_path}/$(uname -r)/updates/hisdk5/
    cp ${TOP_DIR}/output/binary/hisdk5.ko ${deb_ko_path}/$(uname -r)/updates/hisdk5/
    cp ${TOP_DIR}/build/host/linux/sdk/sdk_deb_build/${arch_name}/DEBIAN/* ${deb_debian_path}/
    sed -i "/^Version/c Version: ${global_ver}" ${deb_debian_path}/control
    mkdir -p $modules_load_dir
    cp ${TOP_DIR}/build/host/linux/udk/udk_rpm_build/hiudk5-modules.conf $modules_load_dir
    cp ${TOP_DIR}/build/host/linux/sdk/sdk_deb_build/hisdk5-modules.conf $modules_load_dir
    chmod 775 ${deb_debian_path}/*
    cd ${TOP_DIR}/debbuild/
    dpkg -b $(arch) hisdk5-${global_ver}-${ker_ver}.$(arch).deb
    cp ${TOP_DIR}/debbuild/*.deb ${TOP_DIR}/output/rpmdebs/

    #hinic5
    rm -rf ${TOP_DIR}/debbuild/*
    mkdir -p ${deb_debian_path}
    mkdir -p ${deb_ko_path}/$(uname -r)/updates/hinic5
    cp ${TOP_DIR}/output/binary/hinic5.ko ${deb_ko_path}/$(uname -r)/updates/hinic5/
    cp ${TOP_DIR}/build/host/linux/nic/nic_deb_build/${arch_name}/DEBIAN/* ${deb_debian_path}/
    sed -i "/^Version/c Version: ${global_ver}" ${deb_debian_path}/control
    mkdir -p $modules_load_dir
    cp ${TOP_DIR}/build/host/linux/nic/nic_deb_build/hinic5-modules.conf $modules_load_dir
    chmod 775 ${deb_debian_path}/*
    cd ${TOP_DIR}/debbuild/
    dpkg -b $(arch) hinic5-${global_ver}-${ker_ver}.$(arch).deb
    cp ${TOP_DIR}/debbuild/*.deb ${TOP_DIR}/output/rpmdebs/

    if [[ "$CMD" == *"bifur"* ]]; then
        #hibifur5
        rm -rf ${TOP_DIR}/debbuild/*
        mkdir -p ${deb_debian_path}
        mkdir -p ${deb_ko_path}/$(uname -r)/updates/hibifur5
        cp ${TOP_DIR}/output/binary/hibifur5.ko ${deb_ko_path}/$(uname -r)/updates/hibifur5/
        cp ${TOP_DIR}/build/host/linux/bifur/bifur_deb_build/$(arch)/DEBIAN/* ${deb_debian_path}/
        sed -i "/^Version/c Version: ${global_ver}" ${deb_debian_path}/control
        mkdir -p $modules_load_dir
        cp ${TOP_DIR}/build/host/linux/bifur/bifur_deb_build/hibifur5-modules.conf $modules_load_dir
        chmod 775 ${deb_debian_path}/*
        cd ${TOP_DIR}/debbuild/
        dpkg -b $(arch) hibifur5-${global_ver}-${ker_ver}.$(arch).deb
        cp ${TOP_DIR}/debbuild/*.deb ${TOP_DIR}/output/rpmdebs/
    fi

    if [[ "$CMD" == *"roce"* ]]; then
        #hihtr5
        rm -rf ${TOP_DIR}/debbuild/*
        mkdir -p ${deb_debian_path}
        mkdir -p ${deb_ko_path}/$(uname -r)/updates/hihtr5
        cp ${TOP_DIR}/output/binary/hihtr5.ko ${deb_ko_path}/$(uname -r)/updates/hihtr5/
        mkdir -p ${hihtr5_deb_build_path}/usr/lib/$(arch)-linux-gnu/
        cp ${TOP_DIR}/output/binary/*.so ${hihtr5_deb_build_path}/usr/lib/$(arch)-linux-gnu/
        cp ${TOP_DIR}/src/dpu_platform_library/host/service/hihtr/hihtr5_deb_build/$(arch)/DEBIAN/* ${deb_debian_path}/
        sed -i "/^Version/c Version: ${global_ver}" ${deb_debian_path}/control
        mkdir -p $modules_load_dir
        cp ${TOP_DIR}/src/dpu_platform_library/host/service/hihtr/hihtr5_deb_build/hihtr5-modules.conf $modules_load_dir

        mkdir -p $dracut_conf_dir
        touch $dracut_conf_dir/hihtr5.conf
        chmod 640 $dracut_conf_dir/hihtr5.conf

        mkdir -p $libibverbs_dir
        echo "driver hihtr" > $libibverbs_dir/hihtr5.driver
        chmod 644 $libibverbs_dir/hihtr5.driver

        chmod 775 ${deb_debian_path}/*
        cd ${TOP_DIR}/debbuild/
        dpkg -b $(arch) hihtr5-${global_ver}-${ker_ver}.$(arch).deb
        cp ${TOP_DIR}/debbuild/*.deb ${TOP_DIR}/output/rpmdebs/
    fi

    mv_file_to_folder
}

build_sdk_linux() {

    local version=$(basename "$TOP_DIR" | awk -F '-' '{print $3}')
    local arch=$(uname -m)

    local sdk_final_name="SP200-SDK-${version}-${finalOSName}-${arch}"

    local sdk_temp_dir="${TOP_DIR}/output/${sdk_final_name}"

    rm -rf "$sdk_temp_dir"
    mkdir -p "$sdk_temp_dir/$sdk_final_name"

    [ -d "${TOP_DIR}/binaryscripts" ] && find "${TOP_DIR}/binaryscripts" -name "*.sh" -exec cp -f {} "$sdk_temp_dir/$sdk_final_name/" \;

    [ -d "${TOP_DIR}/output/rpmdebs" ] && cp -rf "${TOP_DIR}/output/rpmdebs"/* "$sdk_temp_dir/$sdk_final_name/" 2>/dev/null

    [ -d "${TOP_DIR}/firmware" ] && cp -rf "${TOP_DIR}/firmware"/* "$sdk_temp_dir/$sdk_final_name/" 2>/dev/null

    case "$arch" in
        x86_64|aarch64)
            local tool_source_dir="${TOP_DIR}/tool/$arch"
            if [ -d "$tool_source_dir" ]; then
                mkdir -p "$sdk_temp_dir/$sdk_final_name/tool"
                find "$tool_source_dir" -name "hinicadm5*" -type f -exec cp -f {} "$sdk_temp_dir/$sdk_final_name/tool/" \;
                find "$TOP_DIR" -name "hinic5_log_collect.py" -type f -exec cp -f {} "$sdk_temp_dir/$sdk_final_name/tool/" \;
                find "$TOP_DIR" -name "hinic5_log_collect.sh" -type f -exec cp -f {} "$sdk_temp_dir/$sdk_final_name/tool/" \;
            fi
            ;;
    esac

    cd "$sdk_temp_dir"
    zip -r "${TOP_DIR}/${sdk_final_name}.zip" "$sdk_final_name"
    mv -f "${TOP_DIR}/${sdk_final_name}.zip" "${TOP_DIR}/output/"
}

build()
{
    echo "Building drivers..."
    local c_flag=false
    # Parse command line arguments
    OPTIND=1
    if [ $# -eq 1 ] && ([ "$1" = "help" ] || [ "$1" = "-h" ] || [ "$1" = "--help" ]); then
        build_help
        exit 0
    fi
    while getopts "d:o:r:k:t:i:hcl" opt; do
        case $opt in
            d)
            if [ -n "$OPTARG" ]; then
                CMD="$OPTARG"
            fi
            ;;
            o)
            if [ -n "$OPTARG" ]; then
                OFED_VER="$OPTARG"
            fi
            ;;
            r)
            ;;
            c)
                c_flag=true
            ;;
            l)
                LOG_FLAG="2>&1 | tee -a ${LOG_FILE}"
            ;;
            k)
            if [ -n "$OPTARG" ]; then
                if [ -d "${KERNEL_VER_PATH}/${OPTARG}/" ];then
                    KERNEL_VER=$OPTARG
                    echo "The kernel version is $OPTARG."
                else
                    echo "=========================================="
                    echo "The kernel version is not exist."
                    echo "=========================================="
                    return 1
                fi
            fi
            ;;
            t)
            if [ -n "$OPTARG" ]; then
                if [ "$OPTARG" == "Baidu_CentOS" ]; then
                    COMPILE_OS_TYPE=$OPTARG
                else
                    echo "the compile os type is not exist."
                fi
            fi
            ;;
            i)
            if [ "$build_run" = true ]; then
                echo "=========================================="
                echo "Unsupport this option."
                build_help
                echo "=========================================="
                return 1
            fi
            ;;
            h)
            build_help
            return 1
            ;;
            *)
            echo "=========================================="
            echo "Unsupport this option."
            build_help
            echo "=========================================="
            return 1
            ;;
        esac
    done

    echo "Running dependency checks..."
    check_deps "$@"
    if [ "$c_flag" = true ]; then
        exit 0
    fi

    if [ -n "${OFED_SRC_NAME:-}" ]; then
        build_ofed_src
    fi

    cd ${TOP_DIR}
    [ -d "output" ] && rm -rf output
    mkdir -p output/rpmdebs/nic
    mkdir -p output/binary

    [ -d "rpmbuild" ] && rm -rf rpmbuild

    [ -d "${TOP_DIR}/ci/lib/" ] && rm -rf ${TOP_DIR}/ci/lib/
    mkdir -p ${TOP_DIR}/ci/lib/

    set +x
    export_stdarg

    build_sdk
    if [[ "$CMD" == *"nic"* ]] || [[ "$CMD" == *"bifur"* ]] || [[ "$CMD" == *"roce"* ]]; then
        build_hinic
    fi

    if [[ "$CMD" == *"bifur"* ]]; then
        mkdir -p ${TOP_DIR}/output/rpmdebs/bifur
        build_hibifur
    fi
    if [[ "$CMD" == *"roce"* ]]; then
        mkdir -p ${TOP_DIR}/output/rpmdebs/roce
        if [ -z "$MLNX_RPM_VERSION" ]; then
            echo "Use the system ib_core."
            build_hiroce_kern "KER_SRC"
            build_hiroce_user "KER_SRC" $RDMA_CORE_PATH
        else
            echo "Use the MLNX OFED compatible with the system kernel."
            build_hiroce_kern "OFED_SRC"
            build_hiroce_user "OFED_SRC" $RDMA_CORE_PATH
        fi
    fi

    build_sdk_linux
    echo "=========================================="
    echo "Building drivers success."
    echo "=========================================="
}

if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    build "$@"
    exit $?
else
    build_run=false
fi