# NIC Makefile
EXPORT_SYMBOL := true

#HUAWEI SEC
EXTRA_CFLAGS += -DSECUREC_EXPORT_KERNEL_SYMBOL=0

#SDK
EXTRA_CFLAGS += -I$(HI1823_TRUNK_DIR)/src/dpu_develop_interface
EXTRA_CFLAGS += -I$(HI1823_TRUNK_DIR)/src/dpu_develop_interface/drv_srvc_intf
EXTRA_CFLAGS += -I$(HI1823_TRUNK_DIR)/src/dpu_develop_interface/drv_sdk_intf/ossl
EXTRA_CFLAGS += -I$(HI1823_TRUNK_DIR)/src/dpu_develop_interface/fw_msg_intf/public
EXTRA_CFLAGS += -I$(HI1823_TRUNK_DIR)/src/dpu_develop_interface/fw_msg_intf/cfg_mgmt
EXTRA_CFLAGS += -I$(HI1823_TRUNK_DIR)/src/dpu_develop_interface/fw_msg_intf/mpu
EXTRA_CFLAGS += -I$(HI1823_TRUNK_DIR)/src/dpu_platform_library/host/include/sdk/knldk
EXTRA_CFLAGS += -I$(HI1823_TRUNK_DIR)/src/dpu_develop_interface/drv_sdk_intf/hisdk
EXTRA_CFLAGS += -I$(HI1823_TRUNK_DIR)/src/dpu_platform_library/host/sdk/knldk/lld
#NIC
EXTRA_CFLAGS += -I$(HI1823_TRUNK_DIR)/src/dpu_platform_library/include/drv_tool_msg
EXTRA_CFLAGS += -I$(HI1823_TRUNK_DIR)/src/dpu_platform_library/host/service/nic/linux/1588
EXTRA_CFLAGS += -I$(HI1823_TRUNK_DIR)/src/dpu_platform_library/host/service/nic/linux/ctrl
EXTRA_CFLAGS += -I$(HI1823_TRUNK_DIR)/src/dpu_platform_library/host/service/nic/linux/ethtool
EXTRA_CFLAGS += -I$(HI1823_TRUNK_DIR)/src/dpu_platform_library/host/service/nic/linux/ioctl
EXTRA_CFLAGS += -I$(HI1823_TRUNK_DIR)/src/dpu_platform_library/host/service/nic/linux/netdev
EXTRA_CFLAGS += -I$(HI1823_TRUNK_DIR)/src/dpu_platform_library/host/service/nic/linux/nicio
EXTRA_CFLAGS += -I$(HI1823_TRUNK_DIR)/src/dpu_platform_library/host/service/nic/include
EXTRA_CFLAGS += -I$(HI1823_TRUNK_DIR)/src/dpu_platform_library/host/service/include
EXTRA_CFLAGS += -I$(HI1823_TRUNK_DIR)/src/common/nic
EXTRA_CFLAGS += -I$(HI1823_TRUNK_DIR)/src/dpu_platform_library/host/service/nic/comm
EXTRA_CFLAGS += -I$(HI1823_TRUNK_DIR)/src/dpu_develop_interface/fw_msg_intf/nic
EXTRA_CFLAGS += -I$(HI1823_TRUNK_DIR)/src/dpu_develop_interface/fw_msg_intf/bond
EXTRA_CFLAGS += -I$(HI1823_TRUNK_DIR)/src/dpu_develop_interface/fw_msg_intf/mag
EXTRA_CFLAGS += -I$(HI1823_TRUNK_DIR)/src/dpu_develop_interface/fw_msg_intf/cfm
EXTRA_CFLAGS += -I$(HI1823_TRUNK_DIR)/src/dpu_develop_interface/drv_srvc_intf/nic
#BOND
EXTRA_CFLAGS += -I$(HI1823_TRUNK_DIR)/src/dpu_platform_library/include/drv_fw_msg/cfm
EXTRA_CFLAGS += -I$(HI1823_TRUNK_DIR)/src/dpu_platform_library/host/include/cfm/bond
#MPU
EXTRA_CFLAGS += -I$(HI1823_TRUNK_DIR)/src/dpu_platform_library/include/drv_fw_msg/mpu

#MACsec
EXTRA_CFLAGS += -I$(HI1823_TRUNK_DIR)/src/dpu_develop_interface/fw_msg_intf/macsec
EXTRA_CFLAGS += -I$(HI1823_TRUNK_DIR)/src/dpu_platform_library/host/service/nic/linux/macsec

EXTRA_CFLAGS += -Wno-implicit-fallthrough
EXTRA_CFLAGS += -Wframe-larger-than=2048

ifndef HISDK5_SYMVERS
HISDK5_SYMVERS = ${HI1823_TRUNK_DIR}/ci/lib/SDK_Module.symvers
endif
KBUILD_EXTRA_SYMBOLS += $(HISDK5_SYMVERS)

EXTRA_CFLAGS += -Werror -Wno-error=missing-prototypes -Wno-missing-braces

ifndef __TIME_STR__
SYS_TIME=$(shell date +%Y-%m-%d_%H:%M:%S)
ccflags-y += -D __TIME_STR__=\"$(SYS_TIME)\"
endif
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

obj-m += hinic5.o
hinic5-objs := \
			./1588/hinic5_ptp.o \
			./ctrl/hinic5_main.o \
			./ctrl/hinic5_irq.o \
			./ethtool/hinic5_rss.o \
			./ethtool/hinic5_ntuple.o \
			./ethtool/hinic5_ethtool.o \
			./ethtool/hinic5_ethtool_coalesce.o \
			./ethtool/hinic5_ethtool_lb_test.o \
			./ethtool/hinic5_ethtool_priv_flags.o \
			./ethtool/hinic5_ethtool_port_stats.o \
			./ethtool/hinic5_ethtool_link_stats.o \
			./ethtool/hinic5_tc.o \
			./ioctl/hinic5_dcb.o \
			./ioctl/hinic5_dbg.o \
			./netdev/hinic5_netdev_ops.o \
			./netdev/hinic5_filter.o \
			./nicio/hinic5_tx.o \
			./nicio/hinic5_rx.o \
			./nicio/hinic5_xdp.o \
			./macsec/hinic5_macsec_dfx.o \
			./macsec/hinic5_macsec_main.o \
			./macsec/hinic5_macsec_mgmt.o \
			./macsec/hinic5_macsec_nictool.o \
			./macsec/hinic5_macsec_protocol.o \
			./macsec/hinic5_macsec_service.o \
			../comm/hinic5_nic_cfg.o \
			../comm/hinic5_mag_cfg.o \
			../comm/hinic5_nic_cfg_vf.o \
			../comm/hinic5_rss_cfg.o \
			../comm/hinic5_nic_event.o \
			../comm/hinic5_nic_io.o \
			../comm/hinic5_nic_dbg.o \
			../comm/hinic5_cmdq_adapt.o \
			../comm/182x_cmdq_adapt/182x_cmdq_ops.o \
			../comm/187x_cmdq_adapt/187x_cmdq_ops.o \
			../../../cfm/bond/hinic5_bond_event.o \
			../../../cfm/bond/hinic5_bond.o \

# Driver Extension
ifneq ($(DPU_HINIC5_DRV_EXTEND_MK),)
$(info Using hinic5  extension $(DPU_HINIC5_DRV_EXTEND_MK))
# Dirctory of NIC makefile
export DPU_DRV_MK_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
include $(DPU_HINIC5_DRV_EXTEND_MK)
hinic5-objs += $(DPU_DRV_HINIC5_EXTEND_OBJS)
endif

all:build_info default

build_info:
	mkdir -p build
	@echo "CURDIR=$(CURDIR)" > build/build_src.txt
	@for obj in $(hinic5-objs); do \
		echo $$(realpath $$obj) >> build/build_src.txt; \
	done

KERNEL_VER := $(shell uname -r 2>/dev/null)
KERNEL_DIR := /lib/modules/$(KERNEL_VER)/build

default:
	$(MAKE) -C $(KERNEL_DIR) M=$(shell pwd) -W modules

clean:
	rm -rf *.o *.ko *.order .*.cmd *.mod.* .H* .tm* .tmp_versions Module.symvers *.ko.unsigned null