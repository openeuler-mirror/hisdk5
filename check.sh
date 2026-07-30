#!/bin/bash
#----------------------------------------------------------------------------
# Purpose:
# Copyright Huawei Technologies Co., Ltd. 2026. All rights reserved.
#----------------------------------------------------------------------------

# lossing dependencies
missing_deps=()
#rpmbuild command for rpm-build dependency
BASIC_TOOLS=(gcc make strip pkg-config)
BASIC_RPM_TOOLS=(rpmbuild)


# cmake : used when rdma-core need to be compiled
RDMA_CORE_COMPILE_DEPENDENCY=(cmake)
# if user want to compile roce, rdma-core must be installed
# Otherwise, we cannot determine which symbol set to use for compiling the kernel-mode RoCE driver.
ROCE_COMPILE_DEPENDENCY=(rdma-core)
ROCE_RDMA_CORE_DEVEL_DEPENDENCY=(rdma-core-devel)
# if user installed OFED, below dependencies will not be checked
# 以下这些包都可能包含在rdma-core-devel里面，所以单独搞一个数组
ROCE_MAY_NEED_DEPENDENCY=(libibverbs infiniband-diags libibverbs-devel)

OFED_SRC_NAME="$found_package"
MLNX_RPM_VERSION=""
RDMA_CORE_PATH=""
REQUIRED_GCC_VERSION="4.8.5"
TOP_DIR=$(pwd)
check_run=true

rdma_core_version=""
rdma_core_pattern='rdma-core-\K[0-9.]+'
rdma_core_mlnx_pattern='rdma-core-\K[0-9]+mlnx[0-9]+'

pkg_format="rpm"
if  [ -f /etc/debian_version ]; then
    pkg_format="deb"
    # debian系 没有rdma-core-devel
    ROCE_RDMA_CORE_DEVEL_DEPENDENCY=(libibverbs-dev)
    ROCE_MAY_NEED_DEPENDENCY=()
fi


check_help()
{
    echo -e "\nA scripts to compile driver of nic, bifur or roce, all parameters are the following:\n"
    echo -e "-d      driver to be installed(nic, bifur or roce)"
    echo -e "-h      provide detailed command guidance information"
    echo -e "-o      version of ofed, only used by roce(recommended M5_1、M5_4、M5_8、M24_10)"
    echo -e "-r      version of rdma-core, only used by roce(recommended R41_0、R22_4)"
    echo -e "Other cases show the help information of this script.\n"
    exit 0
}

# Main check function
check_deps() {
    local d_param=""
    local o_param=""
    local r_param=""
    local o_flag=false
    local r_flag=false
    local u_flag=false
    
    # Parse command line arguments
    OPTIND=1
    # 检查是否直接传入 help 参数
    if [ $# -eq 1 ] && ([ "$1" = "help" ] || [ "$1" = "-h" ] || [ "$1" = "--help" ]); then
        check_help
        exit 0
    fi

    while getopts "d:o:r:k:t:i:hucl" opt; do
        case $opt in
            d)
                d_param="$OPTARG"
                ;;
            o)
                if [ -n "$OPTARG" ]; then
                    o_flag=true
                    # The -o option is used to specify the mlnx_ofed version for compiling the kernel-mode and user-mode drivers.
                    # When the source package contains multiple.so files, this option is used to specify the mlnx_ofed version.
                    # M<1-99>_<1-99> or rdma-core_<0.0-99.9>
                    pattern="^M[1-9][0-9]?_[1-9][0-9]?$|^mlnx-ofed-[0-9]{1,2}\.[0-9]{1,2}$"

                    # check version
                    if [[ $OPTARG =~ $pattern ]]; then
                        o_param="$OPTARG"
                        if ! command -v ofed_info 2>/dev/null;then
                            echo "=========================================="
                            echo -e "Error: When MLNX_OFED version is specified, MLNX_OFED MUST be installed."
                            echo "=========================================="
                            exit 1
                        fi
                    else
                        echo "=========================================="
                        echo -e "The specified mlnx-ofed version does not meet the requirements."
                        echo -e "Or install MLNX-OFED and try to install drivers once again"
                        echo "=========================================="
                        return 1
                    fi
                fi
                ;;
            r)
                if [ -n "$OPTARG" ]; then
                    r_flag=true
                    pattern="^R[1-9][0-9]?_[0-9][0-9]?$|^rdma-core_[0-9]{1,2}\.[0-9]{1,2}$"

                    # check version
                    if [[ $OPTARG =~ $pattern ]]; then
                        r_param="$OPTARG"
                    else
                        echo "=========================================="
                        echo -e "The specified rdma-core version does not meet the pattern."
                        echo -e "please input R41_0 or rdma-core_41.0"
                        echo "=========================================="
                        return 1
                    fi
                fi
                ;;
            h) 
                check_help
                return 1
                ;;
            k|t|c|l|i)
                if [ "$check_run" = true ]; then
                    echo "=========================================="
                    echo "Unsupport this option."
                    check_help
                    echo "=========================================="
                    return 1
                fi
                ;;
            *)
                echo "=========================================="
                echo "Unsupport this option."
                check_help
                echo "=========================================="
                return 1
                ;;
        esac
    done
        # If -d doesn't contain roce but -o is specified
    if [[ "$o_flag" = true && "$d_param" != *"roce"* ]]; then
        echo "unsupport: -o parameter is not supported without roce in -d"
        exit 1
    fi
    if [[ "$r_flag" = true && "$d_param" != *"roce"* ]]; then
        echo "unsupport: -r parameter is not supported without roce in -d"
        exit 1
    fi
    
    # Check basic dependencies
    check_basic_deps
    
    # TOTEST: compile with rdma-core uninstalled, will the roce user-mode driver can be used?
    # 强制客户安装rdma-core，否则后续编译了也没办法使用，或者无法判断去如何编译内核态
    # 暂未提供交叉编译的选择（在A主机上编译后不安装但要在B主机上使用）
    # Check if -d parameter contains roce
    if [[ "$d_param" == *"roce"* ]]; then
        # Check RDMA core dependencies
        check_rdma_core_deps
        check_rdma_core_path
        # found rdma-core src, need to check if make is ok
        check_rdma_core_src_deps
    fi

    # If any dependencies are missing, prompt user to install
    if [ ${#missing_deps[@]} -ne 0 ]; then
        echo "========================================"
        echo -e "Please install the following packages:"
        echo -e " ${missing_deps[*]}"
        echo "========================================"
        exit 1
    fi

    echo "========================================"
    echo "Check dependencies success."
    echo "========================================"
}

check_env_compile_tool()
{
    local comp_tool=${1}
    if ! command -v ${comp_tool} &>/dev/null; then
        missing_deps+=(${comp_tool})
    elif [ "$comp_tool" == "gcc" ]; then
        local current_ver
        current_ver=$(gcc -dumpversion 2>/dev/null || gcc --version | head -n1 | awk '{print $3}')
        if ! check_version "$current_ver" "$REQUIRED_GCC_VERSION"; then
            echo "========================================"
            echo "GCC version must be later than $REQUIRED_GCC_VERSION."
            echo "========================================"
            exit 1
        fi
    fi
    echo "Info: ${comp_tool} check ok" >&2
}

check_rdma_core_src_deps()
{
    for comp_tool in ${RDMA_CORE_COMPILE_DEPENDENCY[*]}; do
        if ! command -v ${comp_tool} &>/dev/null; then
            missing_deps+=(${comp_tool})
        else
            echo "Info: ${comp_tool} check ok"
        fi
    done
}

check_basic_tools()
{
    for comp_tool in ${BASIC_TOOLS[*]}; do
        check_env_compile_tool ${comp_tool}
    done
}

check_version() {
    local current_ver=$1
    local required_ver=$2
    
    # Convert version numbers to arrays
    IFS='.' read -ra current_arr <<< "$current_ver"
    IFS='.' read -ra required_arr <<< "$required_ver"
    
    # Get the maximum number of segments between the two versions
    local max_length=$((${#current_arr[@]} > ${#required_arr[@]} ? ${#current_arr[@]} : ${#required_arr[@]}))
    
    # Compare each segment
    for ((i=0; i<max_length; i++)); do
        # Use 0 if segment doesn't exist
        local current_num=${current_arr[i]:-0}
        local required_num=${required_arr[i]:-0}
        
        # Remove leading zeros (e.g., 08 becomes 8)
        current_num=$((10#${current_num}))
        required_num=$((10#${required_num}))
        
        if ((current_num > required_num)); then
            return 0  # Current version meets requirement
        elif ((current_num < required_num)); then
            return 1  # Current version does not meet requirement
        fi
    done
    
    return 1  # Versions are equal, so requirement is met
}

check_kernel_devel()
{
    local KERN_VER=$(uname -r)
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

	if [ -z "$KSRC" ]; then
		echo "Invalid kernel src. Expected: ${KERN_VER}. Please check if kernel-devel(for other OS) or linux-headers(for debian) installed"
		KSOURCES=`ls -w 1 /usr/src/kernels/ ; ls -w 1 /lib/modules/`
		echo "Found: ${KSOURCES}"
        missing_deps+=(kernel-devel)
	fi
}

check_if_installed()
{
    local pkgname=$1
    if [[ "$pkg_format" == "rpm" ]] && rpm -qa | grep "$pkgname" >/dev/null 2>&1; then
        return 0
    elif [[ "$pkg_format" == "deb" ]] && dpkg -l | grep "$pkgname" >/dev/null 2>&1; then
        return 0
    else
        return 1
    fi
}

# 当没有安装rdma-core 或者安装了 rdma-core（非mlnx）但没有安装 rdma-core-devel 的时候
check_ROCE_MAY_NEED_DEPENDENCY()
{
    for dep in "${ROCE_MAY_NEED_DEPENDENCY[@]}"; do
        if ! check_if_installed $dep; then
            missing_deps+=("$dep")
        else
            echo "Info: ${dep} check ok"
        fi
    done
}

check_ROCE_RDMA_CORE_DEVEL_DEPENDENCY()
{
    for dep in "${ROCE_RDMA_CORE_DEVEL_DEPENDENCY[@]}"; do
        if ! check_if_installed $dep; then
            missing_deps+=("$dep")
            check_ROCE_MAY_NEED_DEPENDENCY
        else
            echo "Info: ${dep} check ok"
        fi
    done
}

check_ROCE_COMPILE_DEPENDENCY()
{
    for dep in "${ROCE_COMPILE_DEPENDENCY[@]}"; do
        if ! check_if_installed $dep; then
            missing_deps+=("$dep")
            check_ROCE_RDMA_CORE_DEVEL_DEPENDENCY
        else
            echo "Info: ${dep} check ok"
            # 装了rdma-core且不是mlnx，就要检查rdma-core-devel
            if ! check_if_installed "rdma-core.*mlnx"; then
                check_ROCE_RDMA_CORE_DEVEL_DEPENDENCY
            fi
        fi
    done
}


# Function to check basic compilation dependencies
check_basic_deps() {
    echo "Checking basic compilation dependencies..."

    # Check kernel-devel
    check_kernel_devel

    check_basic_tools
}

# Function to check RDMA core dependencies
check_rdma_core_deps() {
    echo "Checking RDMA core dependencies..."
    check_ROCE_COMPILE_DEPENDENCY
}

# 获取安装的rdma-core版本，这里获取的是完整的版本
# 举例 rdma-core-2410mlnx54-1.2410068.aarch64 获取到的rdma-core版本是2410mlnx54
# 举例 rdma-core-41.0 获取到的版本是 41.0
get_rdma_core_version() {
    if [[ ! -z "$o_param" ]]; then
        local cleaned_version="${o_param//M/}"
        cleaned_version="${cleaned_version//_/}"
        cleaned_version="${cleaned_version//mlnx-ofed/}"
        rdma_core_version="${cleaned_version}mlnx.*"
    elif [[ ! -z "$r_param" ]]; then
        local cleaned_version="${r_param//R/}"
        cleaned_version="${cleaned_version//rdma-core_/}"
        cleaned_version="${cleaned_version//_/.}"
        rdma_core_version=$cleaned_version
    elif check_if_installed "rdma-core.*mlnx";then
        if [[ "$pkg_format" == "rpm" ]];then
            rdma_core_version=$(echo "$(rpm -q rdma-core)" | grep -oP "$rdma_core_mlnx_pattern" )
        else
            rdma_core_version=$(echo "$(dpkg -l | grep rdma-core)" | awk '{print $3}' | awk -F'-' '{print $1}')
        fi
    elif check_if_installed "rdma-core"; then
        if [[ "$pkg_format" == "rpm" ]];then
            rdma_core_version=$(echo "$(rpm -q rdma-core)" | grep -oP "$rdma_core_pattern")
        else
            rdma_core_version=$(dpkg -l | grep "rdma-core" | awk '{print $3}' | awk -F'-' '{print$1}')
        fi
    fi
}

# Function to check RDMA core path
check_rdma_core_path() {
    echo "Checking RDMA core path..."

    local path_found=false
    
    # Extract version from rdma-core package name
    local version=""
    local ofed_version=""
    local package_type=""
    local found_paths=()

     if [[ "$o_flag" = true ]]; then
        # if -o is true, mlnx-ofed must be installed, cause ofa-kernel should be compiled into hiroce3.ko
        package_type="mlnx"

        version=$(echo "$o_param" | tr -cd '0-9')mlnx
        ofed_version=$(echo $(ofed_info -s) | awk -F'-' '{print $3}')
        # ofed_version=$(ofed_info -s 2>/dev/null | grep -oP 'OFED-internal-\K[0-9.]+' || echo "")

        local expected_paths=(
            "./rdma-core-${version}*/"
            "./rdma-core/rdma-core-${version}*/"
        )
        local source_files=(
            "rdma-core-${version}*.tar.gz"
            "rdma-core-${version}*.tgz"
            "rdma-core-${version}*.zip"
            "MLNX_OFED_SRC-${ofed_version}*.tgz"
            "MLNX_OFED_SRC-${ofed_version}*.tar.gz"
        )
        get_rdma_core_version
        MLNX_RPM_VERSION="$rdma_core_version"
    elif [[ "$r_flag" = true ]]; then
        package_type="standard"
        if [[ "$r_param" =~ ^R[1-9][0-9]?_[0-9][0-9]?$ ]]; then
            version=$(echo "$r_param" | sed 's/R//; s/_/./')
        elif [[ "$r_param" =~ ^rdma-core_[0-9]{1,2}\.[0-9]{1,2}$ ]]; then
            version=$(echo "$r_param" | sed 's/rdma-core_//')
        fi
        local expected_paths=(
            "./rdma-core-${version}*/"
            "./rdma-core/rdma-core-${version}*/"
        )
        local source_files=(
            "rdma-core-${version}*.tar.gz"
            "rdma-core-${version}*.zip"
        )
    elif check_if_installed "rdma-core"; then
        echo "=========================== check_if_installed true ============================="
        if check_if_installed "rdma-core.*mlnx"; then
            # MLNX version
            package_type="mlnx"
            get_rdma_core_version
            MLNX_RPM_VERSION="$rdma_core_version"

            # Get OFED version
            ofed_version=$(ofed_info -s 2>/dev/null | grep -oP 'OFED-internal-\K[0-9.]+' || echo "")
            local expected_paths=(
                "./rdma-core-${rdma_core_version}/"
                "./rdma-core/rdma-core-${rdma_core_version}/"
            )
        
            local source_files=(
                "rdma-core-${rdma_core_version}.tar.gz"
                "rdma-core-${rdma_core_version}.tgz"
                "rdma-core-${rdma_core_version}.zip"
                "MLNX_OFED_SRC-${ofed_version}*.tgz"
        )
        else
            # Standard version
            package_type="standard"
            get_rdma_core_version
        
            local expected_paths=(
                "./rdma-core/"
            )
        
            local source_files=(
                "rdma-core-${rdma_core_version}.tar.gz"
                "rdma-core-${rdma_core_version}.zip"
            )
        fi
    else
        local expected_paths=(
            "./rdma-core/"
        )
        local source_files=(
            "rdma-core-*.tar.gz"
            "rdma-core-*.tgz"
            "rdma-core-*.zip"
            "MLNX_OFED_SRC-*.tgz"
        )

    fi

    for path in "${expected_paths[@]}"; do
        for actual_path in $path; do
            if [ -d "$actual_path" ] && [ -d "$actual_path/build/include" ]; then # 存在并且编译OK
                echo "Found valid RDMA core path: $actual_path"
                path_found=true
                found_paths+=($actual_path)
            fi
        done
    done
    
    if [ "$path_found" != true ]; then
        check_source_packages "${source_files[@]}"
    elif [ ${#found_paths[@]} -gt 1 ]; then
        echo "========================================"
        echo "Found multiple valid RDMA core paths:"
        for path in "${found_paths[@]}"; do
            echo "  $path"
        done
        echo "Buildtools cannot specify a unique version."
        echo "========================================"
        check_source_packages "${source_files[@]}"
    else
        RDMA_CORE_PATH="${found_paths[0]}"
        local dir_name=$(basename "${found_paths[0]}")
        if [[ "$dir_name" =~ ([0-9]+)mlnx ]]; then
            MLNX_RPM_VERSION="${BASH_REMATCH[1]}"
        fi
    fi
}

# Function to check source packages
check_source_packages() {
    local source_files="$@"
    local package_found=false
    local found_packages=()
    
    # Check if any of the source files exist in current directory
    for pattern in $source_files; do
        for file in ../$pattern; do
            if [ -e "$file" ]; then
                found_packages+=("$file")
                echo "Found source package: $file"
                package_found=true
            fi
        done
    done

    if [ "$package_found" = true ]; then
        if [ ${#found_packages[@]} -eq 1 ]; then
            OFED_SRC_NAME="${found_packages[0]}"
        else 
            echo "========================================"
            echo "SRC cannot specify a unique version."
            echo "========================================"
            exit 1
        fi
    else
        # Prompt user to download the appropriate source package
        echo "========================================"
        echo "Rdma-core or MLXN_OFED source code package not found."
        echo "Please download the corresponding rdma-core source package the directory($(dirname $TOP_DIR)):"
        if [ "$package_type" = "mlnx" ]; then
            local ofed_version=$(ofed_info -s 2>/dev/null | grep -oP 'OFED-internal-\K[0-9.-]+' || echo "unknown")
            echo "rdma-core-${version}.tar.gz or MLNX_OFED_SRC-${ofed_version}.tgz"
        else
            echo "rdma-core-${version}.tar.gz"
        fi
        echo "========================================"
        exit 1
    fi
}

# Main script execution
if [[ -n "${BASH_SOURCE[0]}" && "${BASH_SOURCE[0]}" == "${0}" ]] || \
   [[ -z "${BASH_SOURCE[0]}" && "$0" == "$(basename "$0")" && -f "$0" ]]; then
    check_deps "$@"
else
    check_run=false
fi