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
		echo "Invalid kernel src. Expected: ${KERN_VER}. Please check if kernel-devel(for other OS) or linux-headers(for debian) installed"
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
    echo "nic-kcompat-generator.sh -> KERN_VER: ${KERN_VER}"
    echo "nic-kcompat-generator.sh -> KSRC: ${KSRC}"
	export KSRC KOBJ
}

function gen_devres() {
	dvh='include/linux/device.h'
	ddrh='include/linux/device/devres.h'
	# gen NEED_DEVM_KASPRINTF if fun devm_kasprintf absent in "$dvh" "$ddrh"
	# gen NEED_DEVM_KCALLOC if fun devm_kcalloc absent in "$dvh" "$ddrh" -- delete
	# gen NEED_DEVM_KFREE if fun devm_kfree absent in "$dvh" "$ddrh"
	# gen NEED_DEVM_KMEMDUP if fun devm_kmemdup absent in "$dvh" "$ddrh"
	# gen NEED_DEVM_KSTRDUP if fun devm_kstrdup absent in "$dvh" "$ddrh"
	# gen NEED_DEVM_KVASPRINTF if fun devm_kvasprintf absent in "$dvh" "$ddrh"
	# gen NEED_DEVM_KZALLOC if fun devm_kzalloc absent in "$dvh" "$ddrh"
}

function gen_dma() {
	dma='include/linux/dma-mapping.h'
	da='include/linux/dma-attrs.h'
	# gen HAVE_DMA_SET_MASK if fun dma_set_mask in "$dma"
	# gen NEED_DMA_ZALLOC_COHERENT if fun dma_zalloc_coherent absent in "$dma"
	# gen NEED_DMA_SET_MASK_AND_COHERENT if fun dma_set_mask_and_coherent absent in "$dma"
	# gen HAVE_STRUCT_DMA_ATTRS if struct dma_attrs in "$da"
	HAVE_DMA_ATTR_WRITE_BARRIER=0
	if check enum dma_attr matches DMA_ATTR_WRITE_BARRIER in "$da" ||
	check macro DMA_ATTR_WRITE_BARRIER in "$dma"; then
		HAVE_DMA_ATTR_WRITE_BARRIER=1
	fi
	# gen HAVE_DMA_ATTR_WRITE_BARRIER if string "$HAVE_DMA_ATTR_WRITE_BARRIER" equals 1
}

function gen_ethtool() {
	eth='include/linux/ethtool.h'
	ueth='include/uapi/linux/ethtool.h'
	gen HAVE_ETHTOOL_COALESCE_EXTACK if method get_coalesce of ethtool_ops matches 'struct kernel_ethtool_coalesce \\*' in "$eth"
	gen HAVE_ETHTOOL_EXTENDED_RINGPARAMS if method get_ringparam of ethtool_ops matches 'struct kernel_ethtool_ringparam \\*' in "$eth"
	gen HAVE_RXFH_HASHFUNC if method get_rxfh of ethtool_ops matches hfunc in "$eth"
	# gen HAVE_RXFH_PARAM if method get_rxfh of ethtool_ops matches 'struct ethtool_rxfh_param \\*' in "$eth"
	# gen HAVE_ETHTOOL_GET_RXNFC_U32_RULELOCS if method get_rxnfc of ethtool_ops matches 'u32 \\*rule_locs' in "$eth"

	gen HAVE_ETHTOOL_GET_RXNFC_VOID_RULELOCS if method get_rxnfc of ethtool_ops matches 'void \\*rule_locs' in "$eth"
	gen HAVE_ETHTOOL_GET_RXFH_INDIR_SIZE if method get_rxfh_indir_size of ethtool_ops in "$eth"
	gen HAVE_ETHTOOL_GET_MODULE_EEPROM_BY_PAGE if method get_module_eeprom_by_page of ethtool_ops in "$eth"
	gen HAVE_ETHTOOL_RXFH_INDIR_STRUCT_RXFH_INDIR if method set_rxfh_indir of ethtool_ops matches 'struct ethtool_rxfh_indir \\*' in "$eth"
	gen SUPPORTED_COALESCE_PARAMS if struct ethtool_ops matches supported_coalesce_params in "$eth"
	gen HAVE_ETHTOOL_SET_PHYS_ID if method set_phys_id of ethtool_ops in "$eth"
	gen HAVE_ETHTOOL_OPS_EXT if struct ethtool_ops_ext in "$eth"
	gen NEED_DEFINE_SPEED_20000 if macro SPEED_20000 absent in "$eth"
	gen NEED_DEFINE_SPEED_25000 if macro SPEED_25000 absent in "$eth"
	gen NEED_DEFINE_SPEED_40000 if macro SPEED_40000 absent in "$eth"
	gen NEED_DEFINE_SPEED_100000 if macro SPEED_100000 absent in "$eth"
	gen NEED_DEFINE_ETH_MODULE_SFF_8636 if macro ETH_MODULE_SFF_8636 absent in "$ueth"
	gen NEED_DEFINE_ETH_MODULE_SFF_8636_LEN if macro ETH_MODULE_SFF_8636_LEN absent in "$ueth"
	gen NEED_DEFINE_ETH_MODULE_SFF_8436 if macro ETH_MODULE_SFF_8436 absent in "$ueth"
	gen NEED_DEFINE_ETH_MODULE_SFF_8436_LEN if macro ETH_MODULE_SFF_8436_LEN absent in "$ueth"
	gen HAVE_ETHTOOL_GLINKSETTINGS if macro ETHTOOL_GLINKSETTINGS in "$ueth"
	gen NEED_ENUM_ETHTOOL_LINK_MODE_25000baseCR_Full_BIT if enum ethtool_link_mode_bit_indices lacks ETHTOOL_LINK_MODE_25000baseCR_Full_BIT in "$ueth"
	gen NEED_ENUM_ETHTOOL_LINK_MODE_25000baseKR_Full_BIT if enum ethtool_link_mode_bit_indices lacks ETHTOOL_LINK_MODE_25000baseKR_Full_BIT in "$ueth"
	gen NEED_ENUM_ETHTOOL_LINK_MODE_100000baseKR4_Full_BIT if enum ethtool_link_mode_bit_indices lacks ETHTOOL_LINK_MODE_100000baseKR4_Full_BIT in "$ueth"
	gen NEED_ENUM_ETHTOOL_LINK_MODE_100000baseCR4_Full_BIT if enum ethtool_link_mode_bit_indices lacks ETHTOOL_LINK_MODE_100000baseCR4_Full_BIT in "$ueth"

	HAVE_NEW_ETHTOOL_LINK_SETTINGS_ONLY=0
	if check method get_settings of ethtool_ops absent in "$eth" &&
	check method get_link_ksettings of ethtool_ops in "$eth"; then
		HAVE_NEW_ETHTOOL_LINK_SETTINGS_ONLY=1
	fi
	gen HAVE_NEW_ETHTOOL_LINK_SETTINGS_ONLY if string "$HAVE_NEW_ETHTOOL_LINK_SETTINGS_ONLY" equals 1

	gen HAVE_KERNEL_ETHTOOL_TS_INFO if struct kernel_ethtool_ts_info in "$eth"
	gen HAVE_ETHTOOL_RXFH_PARAM if struct ethtool_rxfh_param in "$eth"
	gen NEED_ETHTOOL_COALESCE_USECS_LOW_HIGH if macro ETHTOOL_COALESCE_USECS_LOW_HIGH absent in "$eth"
	gen NEED_ETHTOOL_COALESCE_MAX_FRAMES_LOW_HIGH if macro ETHTOOL_COALESCE_MAX_FRAMES_LOW_HIGH absent in "$eth"
	gen NEED_ETHTOOL_COALESCE_PKT_RATE_RX_USECS if macro ETHTOOL_COALESCE_PKT_RATE_RX_USECS absent in "$eth"
}

function gen_filter() {
	fh='include/linux/filter.h'
	gen HAVE_NETDEV_PROG_XDP_WARN_ACTION if fun bpf_warn_invalid_xdp_action matches 'struct net_device \\*' in "$fh"
	# gen NEED_DEFINE_XDP_DO_FLUSH_MAP if macro xdp_do_flush_map absent in "$fh"
	gen HAVE_XDP_DO_FLUSH_MAP if fun xdp_do_flush_map in "$fh"
}

function gen_netdevice() {
	ndh='include/linux/netdevice.h'
	nddh='include/linux/net_debug.h'
	gen NEED_NETIF_NAPI_ADD_NO_WEIGHT if fun netif_napi_add matches 'int weight' in "$ndh"
	gen HAVE_NETIF_NAPI_NO_WEIGHT if fun netif_napi_add lacks 'int weight' in "$ndh"
	gen HAVE_NETIF_NAPI_ADD_WEIGHT if fun netif_napi_add_weight in "$ndh"
	gen HAVE_NDO_TX_TIMEOUT_TXQ if method ndo_tx_timeout of net_device_ops matches txqueue in "$ndh"
	gen HAVE_NDO_XDP if method ndo_xdp of net_device_ops in "$ndh"
	gen HAVE_NDO_BPF if method ndo_bpf of net_device_ops in "$ndh"
	gen HAVE_XDP_XDP_QUERY_PROG if enum xdp_netdev_command matches XDP_QUERY_PROG in "$ndh"
	gen HAVE_BPF_XDP_QUERY_PROG if enum bpf_netdev_command matches XDP_QUERY_PROG in "$ndh"
	gen HAVE_NDO_GET_STATS64 if method ndo_get_stats64 of net_device_ops in "$ndh"
	gen HAVE_NDO_ETH_IOCTL if method ndo_eth_ioctl of net_device_ops in "$ndh"

	gen HAVE_VOID_NDO_GET_STATS64 if method ndo_get_stats64 of net_device_ops matches 'void[ \t]*\\(\\*' in "$ndh"
	gen HAVE_NDO_SET_FEATURES if method ndo_set_features of net_device_ops in "$ndh"
	gen HAVE_NDO_SET_U32_FEATURES if method ndo_set_features of net_device_ops matches 'u32 features' in "$ndh"
	gen HAVE_NAPI_GRO_FLUSH_OLD if fun napi_gro_flush matches 'bool flush_old' in "$ndh"
	gen HAVE_NDO_UDP_TUNNEL_ADD if method ndo_udp_tunnel_add of net_device_ops in "$ndh"
	gen HAVE_VF_SPOOFCHK_CONFIGURE if method ndo_set_vf_spoofchk of net_device_ops in "$ndh"
	gen HAVE_NDO_SET_VF_TRUST if method ndo_set_vf_trust of net_device_ops in "$ndh"
	gen HAVE_NDO_SET_VF_MIN_MAX_TX_RATE if method ndo_set_vf_rate of net_device_ops in "$ndh"
	# gen HAVE_NETDEV_LEVEL_ONCE if macro netdev_warn_once in "$ndh" "$nddh"
	# gen HAVE_DEV_OPEN_EXTACK if fun dev_open matches extack in "$ndh"
	gen HAVE_NETDEVICE_MIN_MAX_MTU if struct net_device matches min_mtu in "$ndh"
	gen HAVE_NETDEV_STATS_IN_NETDEV if struct net_device matches 'struct net_device_stats[ \t]*stats' in "$ndh"
	gen HAVE_NETDEVICE_MACSEC_OPS if struct net_device matches 'const struct macsec_ops[ \t]*\\*macsec_ops' in "$ndh"
	# gen NEED_DEFINE_NAPI_POLL_WEIGHT if macro NAPI_POLL_WEIGHT absent in "$ndh"
	gen HAVE_NDO_SET_VF_LINK_STATE if method ndo_set_vf_link_state of net_device_ops in "$ndh"
	gen HAVE_NDO_SELECT_QUEUE_FALLBACK if method ndo_select_queue of net_device_ops matches fallback in "$ndh"
	gen HAVE_NDO_SELECT_QUEUE_SB_DEV if method ndo_select_queue of net_device_ops matches sb_dev in "$ndh"
	gen HAVE_NDO_SELECT_QUEUE_ACCEL if method ndo_select_queue of net_device_ops matches accel_priv in "$ndh"
	gen NEED_NETDEV_NOTIFIER_INFO_TO_DEV if fun netdev_notifier_info_to_dev absent in "$ndh"
	# exception centos7.9 x86 os
	# (Un)registration functions for the notifiers that takes 'struct netdev_notifier_info *' as parameter in register_netdevice_notifier_rh
	# gen HAVE_REGISTER_NETDEVICE_NOTIFIER_RH if fun register_netdevice_notifier_rh in "$ndh"
	gen HAVE_NET_DEVICE_OPS_EXT if struct net_device_ops_ext in "$ndh"
	gen HAVE_NET_DEVICE_OPS_EXTENDED if struct net_device_ops_extended in "$ndh"
	gen HAVE_NET_DEV_OPS_EXT_NDO_SET_VF_VLAN if method ndo_set_vf_vlan of net_device_ops_extended in "$ndh"
	gen HAVE_NET_DEV_OPS_EXT_NDO_CHANGE_MTU if method ndo_change_mtu of net_device_ops_extended in "$ndh"
	gen HAVE_NETDEVICE_EXTENDED_MIN_MAX_MTU if struct net_device_extended matches min_mtu in "$ndh"
	gen NEED_NETDEV_PHYS_ITEM_ID if struct netdev_phys_item_id absent in "$ndh"
	gen NEED_DEFINE_NETDEV_RSS_KEY_LEN if macro NETDEV_RSS_KEY_LEN absent in "$ndh"
	gen NEED_NAPI_SCHEDULE_IRQOFF if fun napi_schedule_irqoff absent in "$ndh"
	# gen NEED_DEFINE_NETDEV_HW_ADDR_LIST_FOR_EACH if macro netdev_hw_addr_list_for_each absent in "$ndh"
	# gen HAVE_NDO_XDP_XMIT if method ndo_xdp_xmit of net_device_ops in "$ndh"
	gen HAVE_NETDEV_CHANGEUPPER if enum netdev_cmd matches NETDEV_CHANGEUPPER in "$ndh"
	gen HAVE_NETDEV_CHANGEUPPER if macro NETDEV_CHANGEUPPER in "$ndh"
}

function gen_pci() {
	pcih='include/linux/pci.h'
	pdch='include/linux/pci-dma-compat.h'
	# gen NEED_DEFINE_PCI_POOL_ALLOC if macro pci_pool_alloc absent in "$pcih"
	# gen NEED_DEFINE_PCI_POOL_FREE if macro pci_pool_free absent in "$pcih"
	# gen NEED_DEFINE_PCI_POOL if macro pci_pool absent in "$pcih"
	# gen NEED_DEFINE_PCI_DMA_BIDIRECTIONAL if macro PCI_DMA_BIDIRECTIONAL absent in "$pcih" "$pdch"
	# gen NEED_PCI_SRIOV_GET_TOTALVFS if fun pci_sriov_get_totalvfs absent in "$pcih"
	# gen NEED_DEFINE_PCI_DEVID if macro PCI_DEVID absent in "$pcih"
	# gen NEED_PCI_ENABLE_MSIX_RANGE if fun pci_enable_msix_range absent in "$pcih"
	# gen HAVE_SRIOV_CONFIGURE if method sriov_configure of pci_driver in "$pcih"
	# gen HAVE_PCI_DEV_FLAGS_ASSIGNED if enum pci_dev_flags matches PCI_DEV_FLAGS_ASSIGNED in "$pcih"
	# gen NEED_PCI_VFS_ASSIGNED if fun pci_vfs_assigned absent in "$pcih"
}

function gen_limits() {
	lih='include/linux/limits.h'
	kh='include/linux/kernel.h'
	gen NEED_DEFINE_U16_MAX if macro U16_MAX absent in "$lih" "$kh"
	gen NEED_DEFINE_U32_MAX if macro U32_MAX absent in "$lih" "$kh"
}

function gen_skbuff() {
	skbh='include/linux/skbuff.h'
	gen HAVE_SKB_RECV_DATAGRAM_NOBLOCK if fun skb_recv_datagram matches 'int noblock' in "$skbh"
	# gen NEED_PLT_HASH_TYPES if enum pkt_hash_types absent in "$skbh"
	gen NEED_SKB_SET_HASH if fun skb_set_hash absent in "$skbh"
	gen HAVE_SK_BUFF_ENCAPSULATION if struct sk_buff matches encapsulation in "$skbh"
	gen NEED_SKB_FRAG_OFF_ADD if fun skb_frag_off_add absent in "$skbh"
	gen HAVE_TYPEDEF_SKB_FRAG_T_BIOVEC if typedef skb_frag_t matches bio_vec in "$skbh"
	gen HAVE_SKB_L4_RXHASH if struct sk_buff matches l4_rxhash in "$skbh"
	# gen NEED_DEFINE_DEV_ALLOC_PAGES if macro dev_alloc_pages absent in "$skbh"
	# gen NEED_DEFINE_DEV_ALLOC_PAGE if macro dev_alloc_page absent in "$skbh"
	gen HAVE_SKBUFF_CSUM_LEVEL if struct sk_buff matches csum_level in "$skbh"
	# gen NEED___SKB_PUT_DATA if fun __skb_put_data absent in "$skbh"
}

function gen_list() {
    lh='include/linux/list.h'
	gen NEED_DEFINE_LIST_FIRST_ENTRY_OR_NULL if macro list_first_entry_or_null absent in "$lh"
	gen NEED_DEFINE_LIST_NEXT_ENTRY if macro list_next_entry absent in "$lh"
	gen NEED_DEFINE_LIST_PREV_ENTRY if macro list_prev_entry absent in "$lh"
}

function gen_ether() {
	edh='include/linux/etherdevice.h'
	ieh='include/uapi/linux/if_ether.h'
	gen HAVE_ETH_GET_HEADLEN_NET_DEVICE_ARG if fun eth_get_headlen matches 'struct net_device \\*' in "$edh"
	gen HAVE_ETH_HW_ADDR_SET if fun eth_hw_addr_set in "$edh"
	# gen HAVE_ETH_MAC_ADDR if fun eth_mac_addr in "$edh"
	gen NEED_ETHER_ADDR_COPY if fun ether_addr_copy absent in "$edh"
	gen HAVE_ETH_GET_HEADLEN_FUNC if fun eth_get_headlen in "$edh"
	gen NEED_ETH_P_8021AD if macro ETH_P_8021AD absent in "$edh"
	# gen NEED_ETHER_ZERO_ADDR if fun eth_zero_addr absent in "$edh"
}

function gen_mm() {
	mm='include/linux/mm.h'
	mmt='include/linux/mm_types.h'
	gen HAVE_VM_FLAGS_SET if fun vm_flags_set in include/linux/mm.h
	# gen HAVE_GET_USER_PAGES_GUP_FLAGS if fun get_user_pages matches gup_flags in "$mm"
	# gen HAVE_GET_USER_PAGES_VMAS if fun get_user_pages matches vmas in "$mm"
	# gen HAVE_GET_USER_PAGES_LONGTERM if fun get_user_pages_longterm in "$mm"
	# gen HAVE_PINNED_VM_TYPE_ATOMIC64_T if struct mm_struct matches 'atomic64_t[ \t]*pinned_vm;' in "$mmt"
	# gen HAVE_GET_USER_PAGES_8_PARAMS if fun get_user_pages countparam 8 in "$mm"
}

function gen_mmap_lock() {
	mml='include/linux/mmap_lock.h'
	# gen HAVE_MMAP_WRITE_LOCK if fun mmap_write_lock in "$mml"
}

function gen_time() {
	tih='include/linux/time.h'
	utih='include/uapi/linux/time.h'
	tp32h='include/linux/timekeeping32.h'
	# gen NEED_DO_GETTIMEOFDAY if fun do_gettimeofday absent in "$tih" "$tp32h"

	NEED_STRUCT_TIMEVAL=0
	if grep -q "^#ifndef __KERNEL__" "${KSRC%/}/$tih" || grep -q "^#ifndef __KERNEL__" "${KSRC%/}/$utih"; then
		NEED_STRUCT_TIMEVAL=1
	fi
	# gen NEED_STRUCT_TIMEVAL if string "$NEED_STRUCT_TIMEVAL" equals 1
}

function gen_kobject() {
    obh='include/linux/kobject.h'
    gen HAVE_KOBJ_TYPE_DEFAULT_GROUPS if struct kobj_type matches 'const struct attribute_group \\*' in "$obh"
	gen HAVE_KOBJ_TYPE_DEFAULT_ATTRS if struct kobj_type matches 'struct attribute \\*' in "$obh"
}

function gen_other() {
	pciaerh='include/linux/aer.h'
	ush='include/linux/u64_stats_sync.h'
	gfp='include/linux/gfp.h'
	# gen HAVE_PCI_ENABLE_PCIE_ERROR_REPORTING if fun pci_enable_pcie_error_reporting in "$pciaerh"
	# gen HAVE_PCI_DISABLE_PCIE_ERROR_REPORTING if fun pci_disable_pcie_error_reporting in "$pciaerh"
	# gen NEED_PCI_CLEANUP_AER_UNCORRECT_ERROR_STATUS if fun pci_cleanup_aer_uncorrect_error_status absent in "$pciaerh"
	# gen HAVE_TIMER_SETUP if fun timer_setup in include/linux/timer.h
	# gen NEED_TIMER_SETUP if fun timer_setup absent in include/linux/timer.h
	gen NEED_PDE_DATA if fun PDE_DATA absent in include/linux/proc_fs.h
	gen HAVE_PDE_DATA_LOWERCASE if fun pde_data in include/linux/proc_fs.h
	# gen HAVE_PROC_OPS if struct proc_ops in include/linux/proc_fs.h
	# gen NEED_PROC_OPS if struct proc_ops absent in include/linux/proc_fs.h
	gen NEED_NETIF_F_SCTP_CRC if macro NETIF_F_SCTP_CRC absent in include/linux/netdev_features.h
	# gen NEED_DEFINE_NETIF_F_GSO_UDP_TUNNEL_CSUM if macro NETIF_F_GSO_UDP_TUNNEL_CSUM absent in include/linux/netdev_features.h
	gen HAVE_NETIF_F_RXHASH if macro NETIF_F_RXHASH in include/linux/netdev_features.h
	gen HAVE_SOCK_CREATE_KERN_NET if fun sock_create_kern matches 'struct net \\*' in include/linux/net.h
	gen HAVE_SK_DATE_READY_BYTES if method sk_data_ready of sock matches 'int bytes' in include/net/sock.h
	gen HAVE_UDP_TUNNEL_NIC_INFO if struct udp_tunnel_nic_info in include/net/udp_tunnel.h
	# gen HAVE_VLAN_FIND_DEV_DEEP_RCU if fun __vlan_find_dev_deep_rcu in include/linux/if_vlan.h
	gen NEED_DEFINE_SKB_VLAN_TAG_PRESENT if macro skb_vlan_tag_present absent in include/linux/if_vlan.h
	gen NEED___vlan_get_protocol if fun __vlan_get_protocol absent in include/linux/if_vlan.h
	gen NEED_DEFINE_FIELD_SIZEOF if macro FIELD_SIZEOF absent in include/linux/kernel.h
	# gen NEED_RTC_TIME_TO_TM if fun rtc_time_to_tm absent in include/linux/rtc.h
	gen NEED_STRLCPY if fun strlcpy absent in include/linux/string.h
	# gen NEED_CPUMASK_LOCAL_SPREAD if fun cpumask_local_spread absent in include/linux/cpumask.h
	# gen HAVE_GENL_OPS_FIELD_VALIDATE if struct genl_ops matches validate in include/net/genetlink.h
	# gen NEED_DEFINE_GET_DS if macro get_ds absent in include/asm-generic/uaccess.h
	# gen NEED_BITMAP_ZALLOC if fun bitmap_zalloc absent in include/linux/bitmap.h
	# gen HAVE_MACRO_VM_FAULT_T if typedef vm_fault_t in include/linux/mm_types.h
	# gen NEED_CSUM_REPLACE_BY_DIFF if fun csum_replace_by_diff absent in include/net/checksum.h
	# gen NEED_DEFINE_KFREE_RCU if macro kfree_rcu absent in include/linux/rcupdate.h
	# gen NEED_DEFINE_U64_STATS_INIT if macro u64_stats_init absent in include/linux/u64_stats_sync.h
	# gen NEED_DEFINE_BIT_ULL if macro BIT_ULL absent in include/linux/bitops.h include/linux/bits.h
	gen NEED_DEFINE_DMA_RMB if macro dma_rmb absent in include/asm-generic/barrier.h
	gen HAVE_XDP_SUPPORT if macro __LINUX_NET_XDP_H__ in include/net/xdp.h
	# gen HAVE_XDP_FRAME_SZ if struct xdp_buff matches frame_sz in include/net/xdp.h
	gen HAVE_XDP_DATA_META if fun xdp_set_data_meta_invalid in include/net/xdp.h
	gen HAVE_XDP_RXQ_INFO_REG_NAPI_ID if fun xdp_rxq_info_reg matches napi_id in include/net/xdp.h
	# only declared in some EulerOS version, like EulerV2R12, only used in SDI scenarios
	# gen HAVE_KALLSYMS_LOOKUP_NAME_WRAP if fun kallsyms_lookup_name_wrap in include/linux/kallsyms.h
	# gen NEED_KREF_READ if fun kref_read absent in include/linux/kref.h
	# gen HAVE_BPF_TRACE_H if macro __LINUX_BPF_TRACE_H__ in include/linux/bpf_trace.h
	gen HAVE_NETDEV_XDP_ACT_NDO_XMIT if enum netdev_xdp_act matches NETDEV_XDP_ACT_NDO_XMIT in include/uapi/linux/netdev.h
	NEED_KMALLOC_ARRAY=0
	if check fun kmalloc_array absent in include/linux/slab.h &&
    check macro kmalloc_array absent in include/linux/slab.h; then
		NEED_KMALLOC_ARRAY=1
	fi
	# gen NEED_KMALLOC_ARRAY if string "$NEED_KMALLOC_ARRAY" equals 1
	gen HAVE_PAGE_POOL_SUPPORT if fun alloc_pages_bulk_array_node in "$gfp"

	ppl1='include/net/page_pool.h'
    ppl2='include/net/page_pool/types.h'
    ppl3='include/net/page_pool/helpers.h'
	
	if [[ -e "$ppl1" ]]; then
		gen HAVE_PAGE_POOL_OLD if fun page_pool_alloc_frag in "$ppl1"
	fi
	if [[ -e "$ppl2" && -e "$ppl3" ]]; then
		gen HAVE_PAGE_POOL_NEW if fun page_pool_alloc_frag in "$ppl2" "$ppl3"
	fi

	gen HAVE_PP_FLAG_PAGE_FRAG if macro PP_FLAG_PAGE_FRAG in "$ppl1" "$ppl2" "$ppl3"
	gen HAVE_FLOW_ACTION_PRIORITY if enum flow_action_id matches FLOW_ACTION_PRIORITY in include/net/flow_offload.h

	tinfo='arch/arm64/include/asm/thread_info.h'
	processor='arch/x86/include/asm/processor.h'
	uaccess='include/linux/uaccess.h'
	gen HAVE_MM_SEGMENT_T if typedef mm_segment_t in "$tinfo" "$processor" "$uaccess"
}

function gen_symbol() {
	module_symvers='Module.symvers'

	# only used in SDI scenarios, some special OS which kernel version < 5.7 but have not
	# exported function kallsyms_lookup_name
	HAVE_KALLSYMS_LOOKUP_NAME_EXPORTED=0
	if grep -q "kallsyms_lookup_name" "${KSRC%/}/$module_symvers"; then
		HAVE_KALLSYMS_LOOKUP_NAME_EXPORTED=1
	fi
	# gen HAVE_KALLSYMS_LOOKUP_NAME_EXPORTED if string "$HAVE_KALLSYMS_LOOKUP_NAME_EXPORTED" equals 1
}

# all the generations, extracted from main() to keep normal code and various
# prep separated
function gen_all() {
	gen_netdevice
	# code above is covered by unit_tests/test_gold.sh
	if [ -n "${JUST_UNIT_TESTING-}" ]; then
		return
	fi
	gen_devres
	gen_dma
	gen_ethtool
	gen_filter
	gen_pci
	gen_limits
	gen_skbuff
	gen_list
	gen_ether
	gen_mm
	gen_mmap_lock
	# gen_time
	gen_other
	#gen_symbol
    gen_kobject
}

function gen_nic_kcompat() {
	ready_gen
	local out=$1
	if ! [ -d "${KSRC-}" ]; then
		echo >&2 "env KSRC=${KSRC-} does not exist or is not a directory"
		exit 11
	fi

	# Assume KOBJ is the same as KSRC if not set.
	if [ -z "${KOBJ-}" ]; then
		KOBJ="${KSRC-}"
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

	echo "#ifndef NIC_KCOMPAT_H"
	echo "#define NIC_KCOMPAT_H"

	set +x
	gen_all
	set -x

	echo "#endif /* NIC_KCOMPAT_H */"
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

if [ "$1" = "--gen_nic_kcompat" ]; then
    gen_nic_kcompat "$2"
fi