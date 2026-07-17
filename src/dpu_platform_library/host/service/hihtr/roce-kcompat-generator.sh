#!/bin/bash
# SPDX-License-Identifier: GPL-2.0 or Linux-OpenIB
# Copyright(c) 2025 Huawei Technologies Co., Ltd
set -e
set -Eeuo pipefail

# This file is used to automatically generate HAVE_ and NEED_ macros
# for the current or incoming kernel.
#
# It is primarily implemented by invoking the 'gen' function in the roce-kcompat-lib.sh file
# (you can refer to the main part of 'gen_xx' for details).
# The 'gen' tool can search for declarations of various types in kernel header files to generate macros.
# For example, search for a function in a specified file and check whether the given function exists.
# Please refer to the comments above the 'gen' function in roce-kcompat-lib.sh.

# End of intro.
# The implementation is in roce-kcompat-lib.sh, and below is an example of the 'gen' invocation.

export LC_ALL=C
SCRIPT_DIR="$(dirname "${BASH_SOURCE[0]}")"
ORIG_CWD="$(pwd)"
trap 'rc=$?; echo >&2 "$(realpath "$ORIG_CWD/${BASH_SOURCE[0]}"):$LINENO: failed with rc: $rc"' ERR
USE_OFED=""
PCIE_RO_ENABLE="OFF"
# shellcheck source=roce-kcompat-lib.sh
if [[ -f "$SCRIPT_DIR/roce-kcompat-lib.sh" ]]; then
	source "$SCRIPT_DIR/roce-kcompat-lib.sh"
elif [[ -f "$SCRIPT_DIR/../roce-kcompat-lib.sh" ]]; then
	source "$SCRIPT_DIR/../roce-kcompat-lib.sh"
else
	echo "Error: roce-kcompat-lib.sh not found."
	exit 1
fi
function ready_gen()
{
	BUILD_KERNEL=$(uname -r)
	if [ -e "/usr/src/kernels/linux-$BUILD_KERNEL/include/config" ]; then
		KSRC="/usr/src/kernels/linux-$BUILD_KERNEL/"
	elif [ -e "/usr/src/kernels/$BUILD_KERNEL/include/config" ]; then
		KSRC="/usr/src/kernels/$BUILD_KERNEL/"
	elif [ -e "/lib/modules/$BUILD_KERNEL/build/include/config" ]; then
		KSRC="/lib/modules/$BUILD_KERNEL/build/"
	fi

	if [ -z "$KSRC" ]; then
		BUILD_KERNEL=$(uname -r | sed 's/\([0-9]*\.[0-9]*\)\..*/\1/')
		if [ -e "/usr/src/kernels/linux-$BUILD_KERNEL/include/config" ]; then
			KSRC="/usr/src/kernels/linux-$BUILD_KERNEL/"
		elif [ -e "/usr/src/kernels/$BUILD_KERNEL/include/config" ]; then
			KSRC="/usr/src/kernels/$BUILD_KERNEL/"
		elif [ -e "/lib/modules/$BUILD_KERNEL/build/include/config" ]; then
			KSRC="/lib/modules/$BUILD_KERNEL/build/"
		fi
	fi

	if [ -z "$KSRC" ]; then
		echo "Invalid kernel src. Expected: ${BUILD_KERNEL}"
		KSOURCES=$(ls -w 1 /usr/src/kernels/ ; ls -w 1 /lib/modules/)
		echo "Found: ${KSOURCES}"
		exit 1
	fi

	export KSRC

	# Generate roce_kcompat_gen.h
	GSRC=$KSRC
	if [ ! -d "${KSRC}/include/rdma" ]; then
		echo "Detect other Debian/SLES include directory using KSRC..."
		# detect other SLES include directory using KSRC
		t=$(dirname $KSRC)/$(basename $KSRC)
		if [ -L "$t" ]; then
			# a symlink
			t=$(readlink -f $t)
		fi
		# SLES
		t=${t%-obj*}
		# Debian
		t=${t/-amd64/-common}
		t=${t/-arm64/-common}
		if [ -d "${t}/include/rdma" ]; then
			GSRC=$t
		fi
	fi

	echo "KSRC: $KSRC"
	echo "GSRC: $GSRC"
}
# DO NOT break gen calls below (via \), to make our compat code more grep-able,
# keep them also grouped, first by feature (like DEVLINK), then by .h filename
# finally, keep them sorted within a group (sort by flag name)

# handy line of DOC copy-pasted form roce-kcompat-lib.sh:
#   gen DEFINE if (KIND [METHOD of]) NAME [(matches|lacks) PATTERN|absent] in <list-of-files>

function gen-rdma-ib-umem() {
	um='include/rdma/ib_umem.h'
	kernel_um=${KSRC}/include/rdma/ib_umem.h
	gen HAVE_IB_UMEM_SG_APPEND if struct ib_umem matches 'sg_append_table' in "$kernel_um"
	gen NEED_RDMA_UMEM_BLOCK_ITER_NEXT if fun __rdma_umem_block_iter_next absent in "$um"
	gen HAVE_IB_UMEM_NUM_DMA_BLOCKS if fun ib_umem_num_dma_blocks in "$um"
	gen HAVE_RDMA_UMEM_FOR_EACH_DMA_BLOCK if macro rdma_umem_for_each_dma_block in "$um"

	gen IB_UMEM_GET_HAVE_IB_DEVICE if fun ib_umem_get matches 'struct ib_device \\*' in "$um"
	gen IB_UMEM_GET_HAVE_IB_UCONTEXT if fun ib_umem_get matches 'struct ib_ucontext \\*' in "$um"
	gen IB_UMEM_GET_HAVE_DMASYNC if fun ib_umem_get matches 'int dmasync' in "$um"

	gen IB_UMEM_GET_PEER if fun ib_umem_get_peer in "$um"
	gen IB_UMEM_GET_PEER_HAVE_IBDEVICE if fun ib_umem_get_peer matches 'struct ib_device \\*' in "$um"

	gen IB_UMEM_NEED_IB_UMEM_FIND_BEST_PGSZ if fun ib_umem_find_best_pgsz absent in "$um"
}

function gen-rdma-ib-verbs() {
	iv='include/rdma/ib_verbs.h'

	gen HIHTR_DRIVER_NOT_SUPPORT_XRC if struct ib_xrcd matches 'tgt_qp_mutex' in "$iv"
	gen IB_REG_DEV_HAVE_NAME if fun ib_register_device matches 'char \\*name' in "$iv"
	gen IB_REG_DEV_HAVE_PORT_CALLBACK if fun ib_register_device matches 'int \\(\\*port_callback' in "$iv"
	gen IB_REG_DEV_HAVE_DEVICE if fun ib_register_device matches 'struct device \\*' in "$iv"

	gen IB_DEV_HAVE_DEV_IMMUTABLE if struct ib_device matches 'dev_immutable' in "$iv"
	gen IB_DEV_HAVE_UVERBS_EX_CMD_MASK if struct ib_device matches 'uverbs_ex_cmd_mask' in "$iv"

	gen IB_DEV_ATTR_HAVE_MAX_SEND_SGE if struct ib_device_attr matches 'max_send_sge' in "$iv"
	gen IB_DEV_ATTR_HAVE_MAX_FMR if struct ib_device_attr matches 'max_fmr' in "$iv"

	gen IB_VERBS_HAVE_RDMA_GID_ATTR_NETWORK_TYPE if fun rdma_gid_attr_network_type in "$iv"

	gen IB_VERBS_HAVE__IB_ALLOC_DEVICE if fun _ib_alloc_device in "$iv"

	gen IB_MODIFY_QP_IS_OK_HAVE_RDMA_LINK_LAYER if fun ib_modify_qp_is_ok matches 'enum rdma_link_layer' in "$iv"

	gen IB_WR_OPCODE_HAVE_MR_INTEGRITY if enum ib_wr_opcode matches IB_WR_REG_MR_INTEGRITY in "$iv"
	gen IB_QP_CREATE_FLAGS_HAVE_INTEGRITY_EN if enum ib_qp_create_flags matches IB_QP_CREATE_INTEGRITY_EN in "$iv"

	gen IB_DEV_RDMA_RESTRACK_ROOT if struct ib_device matches 'rdma_restrack_root' in "$iv"


	gen RDMA_HAVE_IB_DEVICE_OPS if struct ib_device_ops in "$iv";
	# CentOS 8.1 x86 is special cause 4.18 kernel use 5.0~5.2 patch, which ib_device_ops does not have driver_id member
	# However, SP200 netcard should use driver_id, which is different to SP6XX netcard
	gen ROCE_IB_DEVICE_OPS_HAVE_MODULE if struct ib_device_ops matches 'struct[ \t]*module[ \t]*\\*' in "$iv";
	gen ROCE_IB_DEVICE_HAVE_MODULE if struct ib_device matches 'struct[ \t]*module[ \t]*\\*' in "$iv";

	gen ROCE_CREATE_CQ_INT_HAVE_BUNDLE if method create_cq of ib_device_ops matches fun_param 'struct uverbs_attr_bundle \\*' in "$iv"
	gen ROCE_CREATE_CQ_IBCQ_HAVE_UCONTEXT if method create_cq of ib_device_ops matches fun_param 'struct ib_ucontext \\*' in "$iv"
	gen ROCE_CREATE_CQ_IBCQ if method create_cq of ib_device_ops matches return_type 'struct ib_cq[ \t]*\\(\\*' in "$iv"
	gen ROCE_CREATE_CQ_INT if method create_cq of ib_device_ops matches return_type 'int[ \t]*\\(\\*' in "$iv"

	gen ROCE_DESTROY_CQ_VOID if method destroy_cq of ib_device_ops matches return_type 'void[ \t]*\\(\\*' in "$iv"
	gen ROCE_DESTROY_CQ_INT if method destroy_cq of ib_device_ops matches return_type 'int[ \t]*\\(\\*' in "$iv"
	gen ROCE_DESTROY_CQ_HAVE_UDATA if method destroy_cq of ib_device_ops matches fun_param 'struct ib_udata \\*' in "$iv"

	gen ROCE_CREATE_QP_INT if method create_qp of ib_device_ops matches return_type 'int[ \t]*\\(\\*' in "$iv"
	gen ROCE_CREATE_QP_IB_QP if method create_qp of ib_device_ops matches return_type 'struct ib_qp[ \t]*\\*\\(\\*' in "$iv"

	gen ROCE_DESTROY_QP_INT if method destroy_qp of ib_device_ops matches return_type 'int[ \t]*\\(\\*' in "$iv"
	gen ROCE_DESTROY_QP_INT_HAVE_UDATA if method destroy_qp of ib_device_ops matches fun_param 'struct ib_udata \\*' in "$iv"

	gen ROCE_POST_SEND_HAVE_CONST if method post_send of ib_device_ops matches fun_param 'const struct ib_send_wr \\*' in "$iv"

	gen ROCE_POST_RECV_HAVE_CONST if method post_recv of ib_device_ops matches fun_param 'const struct ib_recv_wr \\*' in "$iv"

	gen ROCE_CREATE_SRQ_INT if method create_srq of ib_device_ops matches return_type 'int[ \t]*\\(\\*' in "$iv"
	gen ROCE_CREATE_SRQ_IB_SRQ if method create_srq of ib_device_ops matches return_type 'struct ib_srq[ \t]*\\*\\(\\*' in "$iv"

	gen ROCE_DESTROY_SRQ_VOID if method destroy_srq of ib_device_ops matches return_type 'void[ \t]*\\(\\*' in "$iv"
	gen ROCE_DESTROY_SRQ_INT if method destroy_srq of ib_device_ops matches return_type 'int[ \t]*\\(\\*' in "$iv"
	gen ROCE_DESTROY_SRQ_HAVE_UDATA if method destroy_srq of ib_device_ops matches fun_param 'struct ib_udata \\*' in "$iv"

	gen ROCE_POST_SRQ_RECV_HAVE_CONST if method post_srq_recv of ib_device_ops matches fun_param 'const struct ib_recv_wr \\*' in "$iv"

	gen ROCE_DEREG_MR_INT if method dereg_mr of ib_device_ops matches return_type 'int[ \t]*\\(\\*' in "$iv"
	gen ROCE_DEREG_MR_INT_HAVE_UDATA if method dereg_mr of ib_device_ops matches fun_param 'struct ib_udata \\*' in "$iv"

	gen ROCE_ALLOC_UCONTEXT_INT if method alloc_ucontext of ib_device_ops matches 'int[ \t]*\\(\\*' in "$iv"
	gen ROCE_ALLOC_UCONTEXT_IB_UCONTEXT if method alloc_ucontext of ib_device_ops matches 'struct ib_ucontext[ \t]*\\*\\(\\*' in "$iv"

	gen ROCE_DEALLOC_UCONTEXT_INT if method dealloc_ucontext of ib_device_ops matches return_type 'int[ \t]*\\(\\*' in "$iv"
	gen ROCE_DEALLOC_UCONTEXT_VOID if method dealloc_ucontext of ib_device_ops matches return_type 'void[ \t]*\\(\\*' in "$iv"

	gen ROCE_ALLOC_PD_IB_PD if method alloc_pd of ib_device_ops matches 'struct ib_pd [ \t]*\\*\\(\\*' in "$iv"
	gen ROCE_ALLOC_PD_INT if method alloc_pd of ib_device_ops matches 'int[ \t]*\\(\\*' in "$iv"

	gen ROCE_DEALLOC_PD_VOID if method dealloc_pd of ib_device_ops matches return_type 'void[ \t]*\\(\\*' in "$iv"
	gen ROCE_DEALLOC_PD_INT if method dealloc_pd of ib_device_ops matches return_type 'int[ \t]*\\(\\*' in "$iv"
	gen ROCE_DEALLOC_PD_HAVE_UDATA if method dealloc_pd of ib_device_ops matches fun_param 'struct ib_udata \\*' in "$iv"

	gen ROCE_IB_DEVICE_OPS_HAVE_CREATE_USER_AH if method create_user_ah of ib_device_ops in "$iv"

	gen ROCE_CREATE_AH_IB_AH if method create_ah of ib_device_ops matches return_type 'struct ib_ah [ \t]*\\*\\(\\*' in "$iv"
	gen ROCE_CREATE_AH_INT if method create_ah of ib_device_ops matches return_type 'int[ \t]*\\(\\*' in "$iv"
	gen ROCE_CREATE_AH_HAVE_FLAGS if method create_ah of ib_device_ops matches fun_param 'u32 flags' in "$iv"

	gen ROCE_DESTROY_AH_HAVE_FLAGS if method destroy_ah of ib_device_ops matches fun_param 'u32 flags' in "$iv"
	gen ROCE_DESTROY_AH_VOID if method destroy_ah of ib_device_ops matches return_type 'void[ \t]*\\(\\*' in "$iv"

	gen ROCE_ALLOC_XRCD_IB_XRCD if method alloc_xrcd of ib_device_ops matches return_type 'struct ib_xrcd [ \t]*\\*\\(\\*' in "$iv"
	gen ROCE_ALLOC_XRCD_INT if method alloc_xrcd of ib_device_ops matches 'int[ \t]*\\(\\*' in "$iv"
	gen ROCE_ALLOC_XRCD_HAVE_IB_UCONTEXT if method alloc_xrcd of ib_device_ops matches fun_param 'struct ib_ucontext \\*' in "$iv"

	gen ROCE_DEALLOC_XRCD_VOID if method dealloc_xrcd of ib_device_ops matches return_type 'void[ \t]*\\*\\(\\*' in "$iv"
	gen ROCE_DEALLOC_XRCD_INT if method dealloc_xrcd of ib_device_ops matches return_type 'int[ \t]*\\(\\*' in "$iv"
	gen ROCE_DEALLOC_XRCD_HAVE_IB_UDATA if method dealloc_xrcd of ib_device_ops matches fun_param 'struct ib_udata \\*' in "$iv"

	gen ROCE_ALLOC_MR_HAVE_UDATA if method alloc_mr of ib_device_ops matches fun_param 'struct ib_udata \\*' in "$iv"

	gen ROCE_ALLOC_MW_IB_MW if method alloc_mw of ib_device_ops matches 'struct ib_mw[ \t]*\\*\\(\\*' in "$iv"
	gen ROCE_ALLOC_MW_INT if method alloc_mw of ib_device_ops matches 'int[ \t]*\\(\\*' in "$iv"

	gen HAVE_ALL_ROCE_FUNCS_U32_PORT if method query_port of ib_device_ops matches fun_param 'u32 port_num' in "$iv"
	gen HAVE_ALL_ROCE_FUNCS_U8_PORT if method query_port of ib_device_ops matches fun_param 'u8 port_num' in "$iv"

	gen ROCE_IB_ADD_GID_HAVE_GID if method add_gid of ib_device_ops matches fun_param 'const union ib_gid \\*' in "$iv"

	gen ROCE_HAVE_IB_QP_OBJ if struct ib_device_ops matches 'DECLARE_RDMA_OBJ_SIZE\\(ib_qp\\)' in "$iv"
	gen ROCE_HAVE_IB_AH_OBJ if struct ib_device_ops matches 'DECLARE_RDMA_OBJ_SIZE\\(ib_ah\\)' in "$iv"
	gen ROCE_HAVE_IB_CQ_OBJ if struct ib_device_ops matches 'DECLARE_RDMA_OBJ_SIZE\\(ib_cq\\)' in "$iv"
	gen ROCE_HAVE_IB_PD_OBJ if struct ib_device_ops matches 'DECLARE_RDMA_OBJ_SIZE\\(ib_pd\\)' in "$iv"
	gen ROCE_HAVE_IB_SRQ_OBJ if struct ib_device_ops matches 'DECLARE_RDMA_OBJ_SIZE\\(ib_srq\\)' in "$iv"
	gen ROCE_HAVE_UCONTEXT_OBJ if struct ib_device_ops matches 'DECLARE_RDMA_OBJ_SIZE\\(ib_ucontext\\)' in "$iv"
	gen ROCE_HAVE_IB_XRCD_OBJ if struct ib_device_ops matches 'DECLARE_RDMA_OBJ_SIZE\\(ib_xrcd\\)' in "$iv"
	gen ROCE_HAVE_IB_MW_OBJ if struct ib_device_ops matches 'DECLARE_RDMA_OBJ_SIZE\\(ib_mw\\)' in "$iv"

	gen HAVE_IB_DEVICE_LOCAL_DMA_LKEY if enum ib_device_cap_flags matches IB_DEVICE_LOCAL_DMA_LKEY in "$iv"

	gen IB_ATOMIC_MR_HAVE_CONST if struct kobj_type matches 'const struct ib_send_wr \\*' in "$iv"

	gen ROCE_HAVE_UVERBS_NO_DRIVER_ID_BINDING if struct ib_device_ops matches 'uverbs_no_driver_id_binding' in "$iv"
	gen ROCE_HAVE_QUERY_UCONTEXT if struct ib_device_ops matches 'query_ucontext' in "$iv"
	gen ROCE_HAVE_MMAP_FREE if struct ib_device_ops matches 'mmap_free' in "$iv"
	gen ROCE_HAVE_INIT_PORT if struct ib_device_ops matches 'init_port' in "$iv"
	gen ROCE_HAVE_FILL_RES_MR_ENTRY if method fill_res_mr_entry of ib_device_ops in "$iv"
	gen ROCE_HAVE_FILL_RES_SRQ_ENTRY if method fill_res_srq_entry of ib_device_ops in "$iv"
	gen ROCE_HAVE_FILL_RES_MR_ENTRY_RAW if method fill_res_mr_entry_raw of ib_device_ops in "$iv"
	gen ROCE_HAVE_FILL_RES_CQ_ENTRY_RAW if method fill_res_cq_entry_raw of ib_device_ops in "$iv"
	gen ROCE_HAVE_FILL_RES_QP_ENTRY if method fill_res_qp_entry of ib_device_ops in "$iv"

	gen IB_CHECK_MR_ACCESS_HAVE_2PARAM if fun ib_check_mr_access countparam 2 in "$iv"
	gen REREG_USR_MR_RETURN_IB_MR if method rereg_user_mr of ib_device_ops matches return_type 'struct ib_mr[ \t]*\\*\\(\\*' in "$iv"
	gen IB_VERBS_HAVE_RDMA_STAT_DESC if struct rdma_stat_desc in "$iv"
	gen IB_VERBS_NEED_RDMA_USER_MMAP_IO if fun rdma_user_mmap_io absent in "$iv"
	gen IB_VERBS_HAVE_RDMA_NETDEV_GET_PARAMS if struct ib_device matches 'rdma_netdev_get_params' in "$iv"
	gen IB_DEVICE_HAVE_DRIVER_DEF if struct ib_device matches 'driver_def' in "$iv"

	#(LINUX_VERSION_CODE < KERNEL_VERSION(5, 0, 0))
	gen ROCE_IB_DEVICE_CREATE_AH_HAVE_FLAGS if method create_ah of ib_device matches 'u32 flags' in "$iv"
	gen ROCE_IB_DEVICE_DESTROY_AH_HAVE_FLAGS if method destroy_ah of ib_device matches fun_param 'u32 flags' in "$iv"
	gen ROCE_IB_DEVICE_ALLOC_XRCD_HAVE_IB_UCONTEXT if method alloc_xrcd of ib_device matches fun_param 'struct ib_ucontext \\*' in "$iv"
	gen ROCE_POST_SEND_HAVE_CONST if method post_send of ib_device matches fun_param 'const struct ib_send_wr \\*' in "$iv"
	gen ROCE_POST_RECV_HAVE_CONST if method post_recv of ib_device matches fun_param 'const struct ib_recv_wr \\*' in "$iv"
	gen ROCE_POST_SRQ_RECV_HAVE_CONST if method post_srq_recv of ib_device matches fun_param 'const struct ib_recv_wr \\*' in "$iv"
	gen ROCE_IB_ADD_GID_HAVE_GID if method add_gid of ib_device matches fun_param 'const union ib_gid \\*' in "$iv"
	gen ROCE_CREATE_SRQ_IB_SRQ if method create_srq of ib_device matches fun_param 'struct ib_srq \\*' in "$iv"
	gen RDMA_USER_MMAP_IO_HAVE_ENTRY if fun rdma_user_mmap_io countparam 6 in "$iv"
	gen IB_DEVICE_HAVE_IB_PORT_DATA if struct ib_device matches 'struct ib_port_data' in "$iv"
	gen IB_VERBS_HAVE_IB_DEVICE_SET_NETDEV if fun ib_device_set_netdev in "$iv"
	gen IB_VERBS_NEED_IBDEV_LOG if fun ibdev_err absent in "$iv"
	gen IB_VERBS_NEED_IBDEV_RATELIMITED_LOG if macro ibdev_err_ratelimited absent in "$iv"
	gen IB_VERBS_NEED_IB_PORT_PHYS_STATE if enum ib_port_phys_state absent in "$iv"
	gen IB_VERBS_NEED_IB_BLOCK_ITER if struct ib_block_iter absent in "$iv"
	gen IB_VERBS_NEED_RDMA_USER_MMAP_ENTRY if struct rdma_user_mmap_entry absent in "$iv"
	gen IB_VERBS_NEED_RDMA_USER_MMAP_ENTRY_INSERT_RANGE if fun rdma_user_mmap_entry_insert_range absent in "$iv"
	gen IB_VERBS_NEED_RDMA_USER_MMAP_ENTRY_INSERT_EXACT if fun rdma_user_mmap_entry_insert_exact absent in "$iv"
	gen IB_VERBS_NEED_VALID_PORT_MIN if macro IB_ROCE_UDP_ENCAP_VALID_PORT_MIN absent in "$iv"
	gen IB_VERBS_NEED_VALID_PORT_MAX if macro IB_ROCE_UDP_ENCAP_VALID_PORT_MAX absent in "$iv"
	gen IB_VERBS_NEED_RDMA_FLOW_LABEL_TO_UDP_SPORT if fun rdma_flow_label_to_udp_sport absent in "$iv"
	gen IB_VERBS_NEED_RDMA_GET_UDP_SPORT if fun rdma_get_udp_sport absent in "$iv"
	gen ROCE_CREATE_QP_IB_QP if method create_qp of ib_device matches return_type 'struct ib_qp[ \t]*\\*[ \t]*\\(\\*' in "$iv"
	gen IB_VERBS_HAVE_EVENT_HANDLER_RWSEM if struct ib_device matches 'event_handler_rwsem' in "$iv"
	gen IB_VERBS_HAVE_EVENT_HANDLER_LOCK if struct ib_device matches 'event_handler_lock' in "$iv"
	gen IB_UCONTEXT_HAVE_MMAP_XA if struct ib_ucontext matches 'mmap_xa' in "$iv"
}

function gen-rdma-ib-addr() {
	ia='include/rdma/ib_addr.h'

	gen RDMA_RESOLVE_IP_HAVE_GID_ATTR if fun rdma_resolve_ip matches 'bool resolve_by_gid_attr' in "$ia"
	gen RDMA_RESOLVE_IP_HAVE_CLIENT if fun rdma_resolve_ip matches 'struct rdma_addr_client \\*' in "$ia"
}

function gen-rdma-ib-cache() {
	ic='include/rdma/ib_cache.h'

	gen IB_CACHE_HAVE_RDMA_QUERY_GID if fun rdma_query_gid in "$ic"
	gen IB_CACHE_HAVE_IB_GET_CACHED_GID if fun ib_get_cached_gid in "$ic"
	gen IB_CACHE_NEED_RDMA_READ_GID_L2_FIELDS if fun rdma_read_gid_l2_fields absent in "$ic"
}

function gen-rdma-uverbs-ioctl() {
	ui='include/rdma/uverbs_ioctl.h'

	gen UVERBS_IOCTL_HAVE_RDMA_UDATA_TO_DRV_CONTEXT if macro rdma_udata_to_drv_context in "$ui"
	gen UVERBS_ATTR_BUNDLE_HAVE_DRIVER_UDATA if struct uverbs_attr_bundle matches 'driver_udata' in "$ui"
	gen UVERBS_IOCTL_NEED_UAPI_DEFINITION if struct uapi_definition absent in "$ui"
	gen UVERBS_IOCTL_NEED_UAPI_DEF_CHAIN if macro UAPI_DEF_CHAIN absent in "$ui"
	gen UVERBS_IOCTL_NEED_UAPI_DEF_CHAIN_OBJ_TREE_NAMED if macro UAPI_DEF_CHAIN_OBJ_TREE_NAMED absent in "$ui"
	gen UVERBS_IOCTL_NEED_UAPI_DEF_IS_OBJ_SUPPORTED if macro UAPI_DEF_IS_OBJ_SUPPORTED absent in "$ui"
	gen UVERBS_IOCTL_NEED_UAPI_DEFINITION_SCOPE if enum uapi_definition_scope absent in "$ui"
	gen UVERBS_METHOD_DEF_HANDLER_HAVE_UFILE if struct uverbs_method_def matches 'struct ib_uverbs_file' in "$ui"
}

function gen-rdma-restrack() {
	restrack_h='include/rdma/restrack.h'

	gen HAVE_RDMA_RESTRACK_ROOT if struct rdma_restrack_root in "$restrack_h"
}

function gen-rdma-io() {
	io='include/asm-generic/io.h'

	gen IO_MACRO_HAVE_MMIOWB if macro mmiowb in "$io"
}

function gen-rdma-ib-user-verbs() {
	iuv='include/uapi/rdma/ib_user_verbs.h'

	gen IB_UVERBS_WR_OP_HAVE_LOCAL_INV if enum ib_uverbs_wr_opcode matches 'IB_UVERBS_WR_LOCAL_INV' in "$iuv"
}

function gen-rdma-device() {
	device=${KSRC}/include/linux/device.h
	gen DEVNODE_NEED_CONST_DEV if method devnode of device_type matches fun_param '(const|RH_KABI_CONST) struct device \\*dev' in "$device"
}

function gen-rdma-class() {
	dc=${KSRC}/include/linux/device/class.h
	set +e
	# Starting from standard kernel version 5.6, the definition of class_create has been moved to class.h.
	if grep -q "class_create" "$device"; then
		gen CLASS_CREATE_HAVE_OWNER if macro class_create matches 'owner' in "$device"
	else
		gen CLASS_CREATE_HAVE_OWNER if macro class_create matches 'owner' in "$dc"
	fi
	set -e
}

function gen-rdma-genetlink() {
	gn=${KSRC}/include/net/genetlink.h

	gen HAVE_GENL_OPS_VALIDATE if struct genl_ops matches 'validate' in "$gn"
}

function gen-rdma-xarray() {
	xarray=${KSRC}/include/linux/xarray.h

	gen RDMA_HAVE_XARRAY if fun xa_load in "$xarray"
}

function gen-rdma-mmu-notifier() {
	# mlnx ofed defines HAVE_MMU_INTERVAL_NOTIFIER based on the kernel mmu_notifier.h
	mmu_notifier=${KSRC}/include/linux/mmu_notifier.h

	gen HAVE_MMU_INTERVAL_NOTIFIER if struct mmu_interval_notifier in "$mmu_notifier"
}

function gen-rdma-core-device() {
	coredev='drivers/infiniband/core/device.c'

	gen COREDEV_HAVE_SETUP_DMA_DEV if fun setup_dma_device in "$coredev"
}

function gen-rdma-inetdevice() {
	# mlnx ofed defines INETDEV_HAVE_FOR_IFA based on the kernel inetdevice.h
	inetdev=${KSRC}/include/linux/inetdevice.h
	ndh='include/linux/netdevice.h'

	gen INETDEV_HAVE_FOR_IFA if macro for_ifa in "$inetdev"
	# exception centos7.9 x86 os
	# (Un)registration functions for the notifiers that takes 'struct netdev_notifier_info *' as parameter in register_netdevice_notifier_rh
	if [[ -z "$USE_OFED" ]] && [[ -z "${MLNX_OFED_DIR:-}" ]]; then
		gen HAVE_REGISTER_NETDEVICE_NOTIFIER_RH if fun register_netdevice_notifier_rh in "$ndh"
	fi
}

function gen-rdma-kobjtype() {
	kobject=${KSRC}/include/linux/kobject.h

	gen KBOJ_TYPE_HAVE_ATTRS_GROUP if struct kobj_type matches 'const struct attribute_group \\*' in "$kobject"
}

function gen-rdma-random() {
	random='include/linux/random.h'

	gen RANDOM_NEED_GET_RANDOM_U32 if fun get_random_u32 absent in "$random"
}

function gen-rdma-mm() {
	mm='include/linux/mm.h'

	gen MM_HAVE_VM_INSERT_PAGES if fun vm_insert_pages in "$mm"
}

function gen-rdma-count-zero() {
	# kernel file must use KSRC cause mlnx-ofed will change compile path
	count_zero=${KSRC}/include/linux/count_zeros.h

	gen LINUX_HAVE_COUNT_TRAILING_ZEROS if fun count_trailing_zeros in "$count_zero"
}

function gen-rdma-idr() {
	idr=${KSRC}/include/linux/idr.h

	gen IDR_NEED_IDA_ALLOC_RANGE if fun ida_alloc_range absent in "$idr"
}

function gen-rdma-log2() {
	linux_log2=${KSRC}/include/linux/log2.h
	log2=include/linux/log2.h

	gen LOG2_NEED_BITS_PER if macro bits_per absent in "$linux_log2" "$log2"
}

function gen-rdma-ib-user-ioctl-verbs() {
	ib_user_ioctl_verbs='include/uapi/rdma/ib_user_ioctl_verbs.h'

	gen IOCTL_VERBS_NEED_IB_UVERBS_GID_TYPE if enum ib_uverbs_gid_type absent in "$ib_user_ioctl_verbs"
}

function gen-rdma-bitops() {
	bitops=${KSRC}/include/linux/bitops.h

	gen BITOPS_NEED_BITS_TO_BYTES if macro BITS_TO_BYTES absent in "$bitops"
}

function gen-rdma-compiler-types() {
	complier_types='include/linux/compiler_types.h'

	gen ATTRIBUTES_HAVE_FALLTHROUGH if macro fallthrough in "$complier_types"
}

function gen-rdma-bitmap() {
	bitmap=${KSRC}/include/linux/bitmap.h

	gen BITMAP_NEED_BITMAP_ZALLOC if fun bitmap_zalloc absent in "$bitmap"
}

function gen-rdma-list() {
	list=${KSRC}/include/linux/list.h

	gen LIST_NEED_LIST_DEL_INIT_CAREFUL if fun list_del_init_careful absent in "$list"
}

function gen-rdma-sysfs() {
	sysfs='include/linux/sysfs.h'

	gen SYSFS_NEED_SYSFS_EMIT if fun sysfs_emit absent in "$sysfs"
}

function gen-rdma-uverbs-types() {
	uverbs_types='include/rdma/uverbs_types.h'

	gen IDR_TYPE_HAVE_UVERBS_ATTR_BUNDLE if method destroy_object of uverbs_obj_idr_type matches fun_param 'uverbs_attr_bundle' in "$uverbs_types"
}

function gen-rdma-others() {
	u_rdma_netlink=${KSRC}/include/uapi/rdma/rdma_netlink.h

	gen HAVE_RDMA_NLDEV_ATTR_RES_RAW if enum rdma_nldev_attr matches 'RDMA_NLDEV_ATTR_RES_RAW' in "$u_rdma_netlink"
}

function gen-rdma-ro-option() {
	gen HIHTR_RO_ENABLE if string "$PCIE_RO_ENABLE" equals "ON"
}

function gen-rdma() {
	# Distinguish the mlnx and native rdma-core packages
	if [ "$USE_OFED" == "ofed_src" ]; then
		ARCH=$(uname -m)
		if [ -d "/usr/src/ofa_kernel/${ARCH}/${BUILD_KERNEL}/" ]; then
			cd /usr/src/ofa_kernel/${ARCH}/${BUILD_KERNEL}/
		else
			cd /usr/src/ofa_kernel/default/
		fi
	else
		if [[ -n "${MLNX_OFED_DIR:-}" && -d "$MLNX_OFED_DIR" ]]; then
			cd $MLNX_OFED_DIR
		else
			cd .
		fi
	fi

	gen-rdma-ib-umem
	gen-rdma-ib-verbs
	gen-rdma-ib-addr
	gen-rdma-ib-cache
	gen-rdma-uverbs-ioctl
	gen-rdma-restrack
	gen-rdma-io
	gen-rdma-ib-user-verbs
	gen-rdma-mmu-notifier
	gen-rdma-core-device
	gen-rdma-inetdevice
	gen-rdma-device
	gen-rdma-class
	gen-rdma-genetlink
	gen-rdma-xarray
	gen-rdma-kobjtype
	gen-rdma-random
	gen-rdma-mm
	gen-rdma-count-zero
	gen-rdma-idr
	gen-rdma-log2
	gen-rdma-ib-user-ioctl-verbs
	gen-rdma-bitops
	gen-rdma-compiler-types
	gen-rdma-bitmap
	gen-rdma-list
	gen-rdma-sysfs
	gen-rdma-uverbs-types
	gen-rdma-ro-option

    cd - > /dev/null
}

# all the generations, extracted from main() to keep normal code and various
# prep separated
function gen-all() {
	gen-rdma
}

function gen_roce_kcompat() {
	ready_gen
	OUT=$PWD/roce_kcompat_gen.h KSRC=$GSRC QUIET_COMPAT=1
	# check if caller (like our makefile) wants to redirect output to file
	OUT_DEFINE=_ROCE_KCOMPAT_GEN_H_
	if [ -n "${OUT-}" ]; then
		OUT_DEFINE="$(basename $OUT .h)"
		OUT_DEFINE="_${OUT_DEFINE^^}_H_"

		# in case OUT exists, we don't want to overwrite it, instead
		# write to a temporary copy.
		if [ -s "${OUT}" ]; then
			TMP_OUT="$(mktemp "${OUT}.XXX")"
			trap "rm -f '${TMP_OUT}'" EXIT

			REAL_OUT="${OUT}"
			OUT="${TMP_OUT}"
		fi

		exec > "$OUT"
		# all stdout goes to OUT since now
		echo "/* uname=$(uname -r) Autogenerated for KSRC=${KSRC-} via $(basename "$0") */"
	fi
	if [ -d "${KSRC-}" ]; then
		cd "${KSRC}"
	fi

	# check if KSRC was ok/if we are in proper place to look for headers
	if [ ! -e include/linux/kernel.h ]; then
		echo >&2 "seems that there are no kernel includes placed in KSRC=${KSRC}
			pwd=$(pwd); ls -l:"
		ls -l >&2
		exit 8
	fi

	echo "#ifndef $OUT_DEFINE"
	echo "#define $OUT_DEFINE"
	gen-all
	echo "#endif /* $OUT_DEFINE */"

	if [ -n "${OUT-}" ]; then
		cd "$ORIG_CWD"

		# Compare and see if anything changed. This avoids updating
		# mtime of the file.
		if [ -n "${REAL_OUT-}" ]; then
			if cmp --silent "${REAL_OUT}" "${TMP_OUT}"; then
				# exit now, skipping print of the output since
				# there were no changes. the trap should
				# cleanup TMP_OUT
				exit 0
			fi

			mv -f "${TMP_OUT}" "${REAL_OUT}"
			OUT="${REAL_OUT}"
		fi

		# dump output, will be visible in CI
		if [ -n "${JUST_UNIT_TESTING-}${QUIET_COMPAT-}" ]; then
			return
		fi
		cat -n "$OUT" >&2
	fi
}

for arg in "$@"; do
	case "$arg" in
		ofed_src)
			USE_OFED="ofed_src"
			;;
		relaxed_order)
			PCIE_RO_ENABLE="ON"
			;;
		*)
			echo "Warning: unknown option $arg"
			;;
	esac
done
gen_roce_kcompat
if [ $? -ne 0 ]; then
	echo "Failed to generate $PWD/roce_kcompat_gen.h"
	exit 1
fi