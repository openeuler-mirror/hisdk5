# SPDX-License-Identifier: GPL-2.0-only
#
# Makefile for the Hisilicon RoCE drivers.
#

HIHTR_CUR_DIR := $(realpath $(dir $(lastword $(MAKEFILE_LIST))))
DPU_ROOT_DIR := $(HIHTR_CUR_DIR)/../../../../../../
HI1872_BUILD_DIR = $(DPU_ROOT_DIR)/build

include $(HI1872_BUILD_DIR)/host/linux/Makefile.ko
ccflags-y := $(filter-out -DGLOBAL_VERSION_STR=% -D __TIME_STR__=%,$(ccflags-y))
# 记录编译时间
ifndef __TIME_STR__
SYS_TIME=$(shell date +%Y-%m-%d_%H:%M:%S)
ccflags-y += -D __TIME_STR__=\"$(SYS_TIME)\"
endif
ccflags-y += -DHW_CONVERT_ENDIAN -D__LINUX__
ccflags-y += -D__ROCE_DFX__ # 使能DFX编译

#获取driver版本号
GLOBAL_VERSION=$(shell cat $(DPU_ROOT_DIR)/src/GLOBAL_VERSION_NEW | grep driver | awk -F ':' '{print $$2}')
ccflags-y += -DGLOBAL_VERSION_STR=\"$(GLOBAL_VERSION)\"

EXTRA_CFLAGS += -I$(DPU_ROOT_DIR)/src/dpu_develop_interface/fw_msg_intf/public
EXTRA_CFLAGS += -I$(DPU_ROOT_DIR)/src/dpu_develop_interface
EXTRA_CFLAGS += -I$(DPU_ROOT_DIR)/src/dpu_develop_interface/fw_msg_intf/mpu
EXTRA_CFLAGS += -I$(DPU_ROOT_DIR)/src/dpu_develop_interface/drv_sdk_intf/hisdk
EXTRA_CFLAGS += -I$(DPU_ROOT_DIR)/src/dpu_develop_interface/drv_sdk_intf/ossl
EXTRA_CFLAGS += -I$(DPU_ROOT_DIR)/src/dpu_develop_interface/fw_msg_intf/nic
EXTRA_CFLAGS += -I$(DPU_ROOT_DIR)/src/dpu_develop_interface/drv_srvc_intf/nic
EXTRA_CFLAGS += -I$(DPU_ROOT_DIR)/src/dpu_develop_interface/fw_msg_intf/mag
EXTRA_CFLAGS += -I$(DPU_ROOT_DIR)/src/dpu_develop_interface/fw_msg_intf/rdma
EXTRA_CFLAGS += -I$(DPU_ROOT_DIR)/src/dpu_develop_interface/fw_msg_intf/rdma/hi1872v100
EXTRA_CFLAGS += -I$(DPU_ROOT_DIR)/src/dpu_develop_interface/fw_msg_intf/cfm
EXTRA_CFLAGS += -I$(DPU_ROOT_DIR)/src/dpu_platform_library/host/service/include
EXTRA_CFLAGS += -I$(DPU_ROOT_DIR)/src/dpu_platform_library/fw/npu/include/base
EXTRA_CFLAGS += -I$(DPU_ROOT_DIR)/src/dpu_platform_library/host/service/roce/linux/kernel/include
EXTRA_CFLAGS += -I$(DPU_ROOT_DIR)/platform/huawei_secure_c/include
EXTRA_CFLAGS += -I$(DPU_ROOT_DIR)/src/dpu_platform_library/host/include/sdk/knldk
EXTRA_CFLAGS += -I$(DPU_ROOT_DIR)/src/dpu_platform_library/include/drv_tool_msg
EXTRA_CFLAGS += -I$(DPU_ROOT_DIR)/src/dpu_platform_library/include/drv_fw_msg/hihtr
EXTRA_CFLAGS += -I$(DPU_ROOT_DIR)/src/dpu_platform_library/host/service/hihtr/abi
EXTRA_CFLAGS += -I$(DPU_ROOT_DIR)/src/dpu_platform_library/host/service/hihtr/
EXTRA_CFLAGS += -DSECUREC_EXPORT_KERNEL_SYMBOL=0
ifeq ($(CONFIG_UBUS_DEVICE), y)
EXTRA_CFLAGS += -D__UBUS_DRIVER__
else
KBUILD_EXTRA_SYMBOLS += $(DPU_ROOT_DIR)/ci/lib/SDK_Module.symvers
KBUILD_EXTRA_SYMBOLS += $(DPU_ROOT_DIR)/ci/lib/NIC_Module.symvers
endif

# 检查系统中是否存在 ofed_info 命令或 MLNX OFED 相关的 rdma-core 包
OFED_INFO := $(shell command -v ofed_info 2>/dev/null)
MLNX_OFED_INSTALLED := $(shell rpm -qa | grep -E 'rdma-core.*mlnx.*' 2>/dev/null)
# 保证mlnx-ofed的路径在系统路径之前
ORIGN_LINUXINCLUDE := $(LINUXINCLUDE)

# 如果检测到存在 ofed_info 或 MLNX OFED 相关的 rdma-core 包，则添加相应的路径
ifeq ($(or $(MLNX_OFED_INSTALLED),$(OFED_INFO)),)
# 无操作，Makefile中不能使用shell中的 || 逻辑
else
    KBUILD_EXTRA_SYMBOLS += /usr/src/ofa_kernel/default/Module.symvers
	LINUXINCLUDE := -I/usr/src/ofa_kernel/default/include/ -I/usr/src/ofa_kernel/default/include/uapi $(ORIGN_LINUXINCLUDE)
endif


hihtr-roce-objs := hihtr_main.o \
	hihtr_cmd.o \
	hihtr_cmdq.o \
	hihtr_pd.o \
	hihtr_ah.o \
	hihtr_hem.o \
	hihtr_mr.o \
	hihtr_qp.o \
	hihtr_cq.o \
	hihtr_alloc.o \
	hihtr_db.o \
	hihtr_srq.o \
	hihtr_dca.o \
	hihtr_debugfs.o \
	hihtr_sysfs.o \
	hihtr_restrack.o\
	hihtr_mcast.o \
	hihtr_compat.o

hihtr5-objs := hihtr_hw_v3.o \
	hihtr_hw_v3_qp.o \
	hihtr_hw_v3_cq.o \
	hihtr_hw_v3_post.o \
	hihtr_hw_v3_eq.o \
	hihtr_hw_v3_srq.o \
	dfx/hihtr_dfx_query.o \
	dfx/hihtr_dfx_bypass_set.o \
	dfx/hihtr_dfx_scc.o \
	dfx/hihtr_dfx_order.o \
	dfx/hihtr_dfx.o \
	$(hihtr-roce-objs)       \

obj-$(CONFIG_INFINIBAND_HIHTR) += hihtr5.o

$(info ============== RoCE KERNEL MAKEFILE INFO ==============)
$(info linux include: $(LINUXINCLUDE))
$(info =======================================================)
