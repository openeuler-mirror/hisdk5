#!/bin/bash
# SPDX-License-Identifier: GPL-2.0
# Copyright(c) 2025 Huawei Technologies Co., Ltd

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

set -e

echo "curr bash script file name: $(basename "${BASH_SOURCE[0]}")"
export LC_ALL=C
SCRIPT_DIR="$(dirname "${BASH_SOURCE[0]}")"
ORIG_CWD="$(pwd)"
trap 'rc=$?; echo >&2 "$(realpath "$ORIG_CWD/${BASH_SOURCE[0]}"):$LINENO: failed with rc: $rc"' ERR

# shellcheck source=kcompat-lib.sh
CURR_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
KCOMPAT_LIB="${CURR_DIR}/../kcompat-lib.sh"

if [ -f "$KCOMPAT_LIB" ]; then
    echo "$KCOMPAT_LIB file is exist"
else
    echo "$KCOMPAT_LIB file is not exist"
fi
source "$KCOMPAT_LIB"

ARCH=$(uname -m)
IS_ARM=""
if [ "$ARCH" == aarch64 ]; then
	IS_ARM=1
fi

function ready_gen()
{
	KERN_VER=$(uname -r)
    KSRC=""

	if [ -e /usr/src/kernels/linux-"$KERN_VER"/include/config ]; then
		KSRC="/usr/src/kernels/linux-"$KERN_VER"/"
	elif [ -e /usr/src/kernels/"$KERN_VER"/include/config ]; then
		KSRC="/usr/src/kernels/"$KERN_VER"/"
	elif [ -e /lib/modules/"$KERN_VER"/build/include/config ]; then
		KSRC="/lib/modules/"$KERN_VER"/build/"
	fi

	if [ -z "$KSRC" ]; then
		KERN_VER=$(uname -r | sed 's/\([0-9]*\.[0-9]*\)\..*/\1/')
		if [ -e /usr/src/kernels/linux-"$KERN_VER"/include/config ]; then
			KSRC="/usr/src/kernels/linux-"$KERN_VER"/"
		elif [ -e /usr/src/kernels/"$KERN_VER"/include/config ]; then
			KSRC="/usr/src/kernels/"$KERN_VER"/"
		elif [ -e /lib/modules/"$KERN_VER"/build/include/config ]; then
			KSRC="/lib/modules/"$KERN_VER"/build/"
		fi
	fi

	if [ -z "$KSRC" ]; then
		echo "Invalid kernel src. Expected: ${KERN_VER}"
		echo "Please check if kernel-devel(for other OS) or linux-headers(for debian) installed"
		KSOURCES=$(ls -w 1 /usr/src/kernels/ ; ls -w 1 /lib/modules/)
		echo "Found: ${KSOURCES}"
		exit 1
	fi
	KOBJ=$KSRC

	if [ ! -d "${KSRC}"/include/linux ]; then
	echo "Detect other Debian/SLES include directory using KSRC..."
	# detect other SLES include directory using KSRC
	t=$(dirname "$KSRC")/$(basename "$KSRC")
	if [ -L "$t" ]; then
		# a symlink
		t=$(readlink -f $t)
	fi
	# SLES
	t=${t%-obj*}
	# LinxOS
	t=${t/-linx-security-arm64/-common-linx-security}
	# Debian
	t=${t/-amd64/-common}
	t=${t/-arm64/-common}
	if [ -d "${t}"/include/linux ]; then
		KSRC=$t
	fi
	fi
    echo "sdk-kcompat-generator.sh -> KERN_VER: ${KERN_VER}"
    echo "sdk-kcompat-generator.sh -> KSRC: ${KSRC}"
	export KSRC KOBJ
}

# 解决在linux 内核头文件中未定义的语法说明
#eg. gen DEFINE if [fun|enum|struct|macro|typedef|symbol] NAME (absent) in <list-of-files>
# 解决在linux 内核头文件中有多种格式定义的语法说明
# matches PATTERN：定义中存在这个字符pattern. lacks PATTERN：定义中存在这个字符pattern: 定义中没有这个pattern
#eg. gen DEFINE if [fun|enum|struct|macro|typedef|symbol] NAME [matches|lacks] PATTERN in <list-of-files>
function gen_pci() {
	pci_h='include/linux/pci.h'
	pdc_h='include/linux/pci-dma-compat.h'

	gen NEED_PCI_DOMAIN_NR if fun pci_domain_nr absent in "$pci_h"
	gen NEED_DEFINE_PCI_DMA_COMPAT if macro PCI_DMA_BIDIRECTIONAL absent in "$pdc_h"
	gen HAVE_PCIE_RESET_DONE if struct pci_error_handlers matches reset_done in "$pci_h"
	gen NEED_PCI_SRIOV_GET_TOTALVFS if fun pci_sriov_get_totalvfs absent in "$pci_h"
}

function gen_sysfs() {
	sysfs_h='include/linux/sysfs.h'

	gen NEED_SYSFS_EMIT if fun sysfs_emit absent in "$sysfs_h"
}

function gen_ptp_clock_kernel() {
	ptp_clock_kernel_h='include/linux/ptp_clock_kernel.h'

	gen NEED_PTP_ADJUST_BY_SCALED_PPM if fun adjust_by_scaled_ppm absent in "$ptp_clock_kernel_h"
}

function gen_math64() {
	math64_h='include/linux/math64.h'

	gen NEED_MATH64_MUL_U64_U64_DIV_U64 if fun mul_u64_u64_div_u64 absent in "$math64_h"
}

function gen_timer() {
	timer_h='include/linux/timer.h'

	gen HAVE_TIMER_SETUP if fun timer_setup in "$timer_h"
}

function gen_cpumask() {
	cpumask_h='include/linux/cpumask.h'

	gen NEED_CPUMASK_LOCAL_SPREAD if fun cpumask_local_spread absent in "$cpumask_h"
}

function gen_etherdevice() {
	etherdevice_h='include/linux/etherdevice.h'

	gen NEED_ETH_ZERO_ADDR if fun eth_zero_addr absent in "$etherdevice_h"
}

function gen_devlink() {
	devlink_h='include/net/devlink.h'
	gen HAVE_DEVLINK_H if macro _NET_DEVLINK_H_ in "$devlink_h"

	gen HAVE_DEVLINK_FLASH_UPDATE_METHOD if struct devlink_ops matches flash_update in "$devlink_h"
	gen HAVE_DEVLINK_FLASH_UPDATE_BEGIN_END_NOTIFY if fun devlink_flash_update_begin_notify in "$devlink_h"
	gen HAVE_DEVLINK_FLASH_UPDATE_PARAMS_FW if struct devlink_flash_update_params matches 'struct firmware \\*fw' in "$devlink_h"
	gen HAVE_DEVLINK_FLASH_UPDATE_PARAMS_FILE_NAME if struct devlink_flash_update_params matches file_name in "$devlink_h"
	gen HAVE_DEVLINK_OPS_FLASH_UPDATE_HAVE_PARAMS if method flash_update of devlink_ops matches devlink_flash_update_params in "$devlink_h"

	gen HAVE_DEVLINK_PARAMS_PUBLISH if fun devlink_params_publish in "$devlink_h"
	gen HAVE_DEVLINK_PARAMS_UNPUBLISH if fun devlink_params_unpublish in "$devlink_h"

	gen HAVE_DEVLINK_ALLOC_SET_DEV if fun devlink_alloc matches 'struct device' in "$devlink_h"
	gen HAVE_DEVLINK_ALLOC if fun devlink_alloc in "$devlink_h"

	gen HAVE_DEVLINK_REGISTER if fun devlink_register in "$devlink_h"
	gen HAVE_DEVLINK_REGISTER_SET_DEV if fun devlink_register matches 'struct device' in "$devlink_h"
	gen HAVE_DEVLINK_REGISTER_HAVE_RET if fun devlink_register matches 'int devlink_register' in "$devlink_h"

	gen HAVE_DEVLINK_PARAM_SET_EXTACK if method set of devlink_param matches 'struct netlink_ext_ack' in "$devlink_h"
}

function gen_uaccess() {
	uaccess_h='include/linux/uaccess.h'
	asm_generic_uaccess_h='include/asm-generic/uaccess.h'

	gen NEED_FORCE_UACCESS_BEGIN if fun force_uaccess_begin absent in "$uaccess_h"
	gen NEED_FORCE_UACCESS_END if fun force_uaccess_end absent in "$uaccess_h"
	gen NEED_GET_FS if macro get_fs absent in "$asm_generic_uaccess_h"
	gen NEED_SET_FS if fun set_fs absent in "$asm_generic_uaccess_h"
}

function gen_aer() {
	aer_h='include/linux/aer.h'

	gen NEED_PCI_ENABLE_PCIE_ERROR_REPORTING if fun pci_enable_pcie_error_reporting absent in "$aer_h"
	gen NEED_PCI_DISABLE_PCIE_ERROR_REPORTING if fun pci_disable_pcie_error_reporting absent in "$aer_h"
}

function gen_gnss() {
	class_h='include/linux/device/class.h'
	device_h='include/linux/device.h'
	class2_h='include/linux/class.h'

	gen HAVE_DEVNODE_CONST_DEV if method devnode of class matches '(const|RH_KABI_CONST) struct device' in "$class_h" "$device_h" "$class2_h"

	NEED_CLASS_CREATE=0
	if check fun class_create matches 'owner' in "$class_h" "$device_h" "$class2_h" ||
	check macro class_create matches 'owner' in "$class_h" "$device_h" "$class2_h" ; then
		NEED_CLASS_CREATE=1
	fi
	gen HAVE_CLASS_CREATE_OWNER if string "$NEED_CLASS_CREATE" equals 1

}

# all the generations, extracted from main() to keep normal code and various
# prep separated
function gen_all() {
	gen_pci
	gen_sysfs
	gen_ptp_clock_kernel
	gen_math64
	gen_timer
	gen_cpumask
	gen_etherdevice
	gen_devlink
	gen_uaccess
	gen_aer
	gen_gnss
}

function gen_sdk_kcompat() {
	ready_gen
	local out=$1
	if ! [ -d "${KSRC-}" ]; then
		echo >&2 "env KSRC=${KSRC-} does not exist or is not a directory"
		exit 11
	fi

	# check if caller (like our makefile) wants to redirect output to file
	if [ -n "${out-}" ]; then

		# in case out exists, we don't want to overwrite it, instead
		# write to a temporary copy.
		if [ -s "${out}" ]; then
			TMP_OUT="$(mktemp "${out}.XXX")"
			trap "rm -f '${TMP_OUT}'" EXIT

			REAL_OUT="${out}"
			out="${TMP_OUT}"
		fi

		exec 3>&1
		exec > "$out"
		# all stdout goes to out since now
		echo "/* uname=$(uname -r) Autogenerated for KSRC=${KSRC-} via $(basename "$0") */"
	fi

	cd "${KSRC}"


	# check if KSRC was ok/if we are in proper place to look for headers
	if [ ! -e include/linux/kernel.h ]; then
		echo >&2 "seems that there are no kernel includes placed in KSRC=${KSRC}
			pwd=$(pwd); ls -l:"
		ls -l >&2
		exit 8
	fi

	echo "#ifndef SDK_KCOMPAT_H"
	echo "#define SDK_KCOMPAT_H"

	set +x
	gen_all
	set -x

	echo "#endif /* SDK_KCOMPAT_H */"
	exec >&3
	if [ -n "${out-}" ]; then
		cd "$ORIG_CWD"

		# Compare and see if anything changed. This avoids updating
		# mtime of the file.
		if [ -n "${REAL_OUT-}" ]; then
			if cmp --silent "${REAL_OUT}" "${TMP_OUT}"; then
				# exit now, skipping print of the output since
				# there were no changes. the trap should
				# cleanup TMP_OUT
		        return 0
			fi

			mv -f "${TMP_OUT}" "${REAL_OUT}"
			out="${REAL_OUT}"
		fi
	fi
    cat -n "$out" >&2
}

if [ "$1" = "--gen_sdk_kcompat" ]; then
    gen_sdk_kcompat "$2"
fi