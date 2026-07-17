# SDK Makefile
EXPORT_SYMBOL := true

# Version information
GIT_COMMIT_ID := $(shell cd $(M) && git rev-parse --short=8 HEAD 2>/dev/null || echo "unknown")
EXTRA_CFLAGS += -DGIT_COMMIT_ID=\"$(GIT_COMMIT_ID)\"

# HUAWEI SEC
EXTRA_CFLAGS += -DSECUREC_EXPORT_KERNEL_SYMBOL=0

# HWSDK INNER
HWSDK_SRC_PATH = $(HI1823_TRUNK_DIR)/src/dpu_platform_library/host/sdk/knldk/
EXTRA_CFLAGS += -I$(HWSDK_SRC_PATH)/cqm
EXTRA_CFLAGS += -I$(HWSDK_SRC_PATH)/crm
EXTRA_CFLAGS += -I$(HWSDK_SRC_PATH)/lld
EXTRA_CFLAGS += -I$(HWSDK_SRC_PATH)/hwif
EXTRA_CFLAGS += -I$(HWSDK_SRC_PATH)/mt
EXTRA_CFLAGS += -I$(HWSDK_SRC_PATH)/include

# HWSDK HEADER
EXTRA_CFLAGS += -I$(HI1823_TRUNK_DIR)/src/dpu_platform_library/include
EXTRA_CFLAGS += -I$(HI1823_TRUNK_DIR)/src/dpu_platform_library/include/drv_tool_msg
EXTRA_CFLAGS += -I$(HI1823_TRUNK_DIR)/src/dpu_platform_library/include/drv_fw_msg/mpu
EXTRA_CFLAGS += -I$(HI1823_TRUNK_DIR)/src/dpu_platform_library/host/include/sdk/knldk
EXTRA_CFLAGS += -I$(HI1823_TRUNK_DIR)/src/dpu_platform_library/host/include/cfm/fast_msg
EXTRA_CFLAGS += -I$(HI1823_TRUNK_DIR)/src/dpu_develop_interface
EXTRA_CFLAGS += -I$(HI1823_TRUNK_DIR)/src/dpu_develop_interface/drv_sdk_intf/hisdk
EXTRA_CFLAGS += -I$(HI1823_TRUNK_DIR)/src/dpu_develop_interface/drv_sdk_intf/ossl

# DBGTOOL
EXTRA_CFLAGS += -I$(HI1823_TRUNK_DIR)/src/dpu_develop_interface/fw_msg_intf/public
EXTRA_CFLAGS += -I$(HI1823_TRUNK_DIR)/src/dpu_develop_interface/fw_msg_intf/mag
EXTRA_CFLAGS += -I$(HI1823_TRUNK_DIR)/src/dpu_develop_interface/fw_msg_intf/mpu
EXTRA_CFLAGS += -I$(HI1823_TRUNK_DIR)/src/dpu_develop_interface/fw_msg_intf/cqm
EXTRA_CFLAGS += -I$(HI1823_TRUNK_DIR)/src/dpu_develop_interface/fw_msg_intf/bond
EXTRA_CFLAGS += -I$(HI1823_TRUNK_DIR)/src/dpu_develop_interface/fw_msg_intf/cfm
EXTRA_CFLAGS += -I$(HI1823_TRUNK_DIR)/src/dpu_develop_interface/fw_msg_intf/cfg_mgmt
EXTRA_CFLAGS += -I$(HI1823_TRUNK_DIR)/src/tools/micro_log

EXTRA_CFLAGS += -Wframe-larger-than=2048
EXTRA_CFLAGS += -Wno-implicit-fallthrough -Wno-error=deprecated-declarations

ifeq ($(CONFIG_SP_DEVICE), y)
EXTRA_CFLAGS += -DCONFIG_SP_VID_DID
endif

EXTRA_CFLAGS += -Werror

obj-m += hisdk5.o
hisdk5-objs := \
			../vram/vram_common.o \
			../vram/hinic5_vram.o \
			../../../cfm/fast_msg/hinic5_fast_msg_init.o \
			../../../cfm/fast_msg/hinic5_fast_msg.o \
			../crm/hinic5_hwdev.o \
			../crm/hinic5_hw_cfg.o \
			../crm/hinic5_hw_comm.o \
			../crm/hinic5_prof_adap.o \
			hinic5_sriov.o \
			hinic5_lld.o \
			hinic5_bus.o \
			hinic5_pcie.o \
			hinic5_sysfs.o \
			hinic5_dev_mgmt.o \
			../hwif/hinic5_common.o \
			../hwif/hinic5_hwif.o \
			../hwif/hinic5_wq.o \
			../hwif/hinic5_cmdq.o \
			../hwif/hinic5_enhance_cmdq.o \
			../hwif/hinic5_eqs.o \
			../hwif/hinic5_mbox.o \
			../hwif/hinic5_mgmt.o \
			../hwif/hinic5_api_cmd.o \
			../hwif/hinic5_hw_api.o \
			../hwif/hinic5_sml_lt.o \
			../../../../../../src/tools/micro_log/micro_log_comm.o \
			../../../../../../src/tools/micro_log/micro_log_procfs_cmd.o \
			../../../../../../src/tools/micro_log/micro_log_index.o \
			../../../../../../src/tools/micro_log/hinic5_micro_log.o \
			../mt/hinic5_fw_update.o \
			../mt/hinic5_hw_mt.o \
			../mt/hinic5_nictool.o \
			../mt/hinic5_non_ptp.o \
			../mt/hinic5_devlink.o \
			../../ossl/linux/kernel/ossl_knl_linux.o \
			../cqm/cqm_bat_cla.o \
			../cqm/cqm_bitmap_table.o \
			../cqm/cqm_object_intern.o \
			../cqm/cqm_bloomfilter.o \
			../cqm/cqm_cmd.o \
			../cqm/cqm_db.o \
			../cqm/cqm_fast_msg.o \
			../cqm/cqm_object.o \
			../cqm/cqm_main.o \
			../cqm/cqm_cmdq_adapt.o \
			../cqm/cqm_182x_cmdq_adapt/cqm_182x_cmdq_ops.o \
			../cqm/cqm_187x_cmdq_adapt/cqm_187x_cmdq_ops.o

# sdk_attack support
ifneq ($(CONFIG_HINIC5_SDK_DEBUG),)
EXTRA_CFLAGS += -D__CQM_DEBUG__ -DCONFIG_HINIC5_SDK_DEBUG -DCONFIG_HINIC5_LOCK_DEBUG
hisdk5-objs += ../mt/hinic5_sdk_attack.o
endif

# UB release support
ifeq ($(CONFIG_UB), y)
hisdk5-objs += hinic5_ubus.o hinic5_ubus_sriov.o
EXTRA_CFLAGS += -D__UBUS_DRIVER__ -DUB_SUPPORT_ENTITY -DUB_SUPPORT_B177
endif

# UB devel support
ifeq ($(CONFIG_UBUS_DEVICE), y)
KBUILD_EXTRA_SYMBOLS += $(UBUS_UBC_BUILD_DIR)/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(UMMU_CORE_BUILD_DIR)/Module.symvers
EXTRA_CFLAGS += -D__UBUS_DRIVER__
hisdk5-objs += hinic5_ubus.o
hisdk5-objs += hinic5_ubus_sriov.o
ifeq ($(UB_BUILD_B173), y)
EXTRA_CFLAGS += -DUB_SUPPORT_ENTITY
endif
ifeq ($(UB_BUILD_B177), y)
EXTRA_CFLAGS += -DUB_SUPPORT_B177
endif
endif

# Driver Extension
ifneq ($(DPU_HISDK5_DRV_EXTEND_MK),)
$(info Using hisdk5 extension $(DPU_HISDK5_DRV_EXTEND_MK))
export DPU_DRV_MK_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
include $(DPU_HISDK5_DRV_EXTEND_MK)
hisdk5-objs += $(DPU_DRV_HISDK5_EXTEND_OBJS)
endif

all:build_info default

build_info:
	mkdir -p build
	@echo "CURDIR=$(CURDIR)" > build/build_src.txt
	@for obj in $(hisdk5-objs); do \
		echo $$(realpath $$obj) >> build/build_src.txt; \
	done

GLOBAL_VERSION=$(shell cat $(HI1823_TRUNK_DIR)/src/GLOBAL_VERSION_NEW | grep driver | awk -F ':' '{print $$2}')
ccflags-y += -DGLOBAL_VERSION_STR=\"$(GLOBAL_VERSION)\"

$(warning cflags, $(ccflags-y))
V ?= 0

ifeq ($(HI1823_RELEASE_TYPE), LLT)
	ccflags-y += -D_LLT_TEST_
else
	ccflags-y += -DHW_CONVERT_ENDIAN
endif

ccflags-y += -D__LINUX__

include $(HI1823_TRUNK_DIR)/build/host/linux/Makefile.ko