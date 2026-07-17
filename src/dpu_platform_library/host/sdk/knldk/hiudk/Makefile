# NIC Makefile
EXPORT_SYMBOL := true
EXTRA_CFLAGS += -DSECUREC_EXPORT_KERNEL_SYMBOL=0
#UDK
EXTRA_CFLAGS += -I$(HI1823_TRUNK_DIR)/src/dpu_develop_interface
EXTRA_CFLAGS += -I$(HI1823_TRUNK_DIR)/src/dpu_develop_interface/fw_msg_intf/public
EXTRA_CFLAGS += -I$(HI1823_TRUNK_DIR)/src/dpu_develop_interface/fw_msg_intf/mag
EXTRA_CFLAGS += -I$(HI1823_TRUNK_DIR)/src/dpu_develop_interface/fw_msg_intf/cqm
EXTRA_CFLAGS += -I$(HI1823_TRUNK_DIR)/src/dpu_develop_interface/fw_msg_intf/bond
EXTRA_CFLAGS += -I$(HI1823_TRUNK_DIR)/src/dpu_develop_interface/fw_msg_intf/cfg_mgmt
EXTRA_CFLAGS += -I$(HI1823_TRUNK_DIR)/src/dpu_develop_interface/drv_sdk_intf/ossl
EXTRA_CFLAGS += -I$(HI1823_TRUNK_DIR)/src/dpu_develop_interface/drv_sdk_intf/hisdk
EXTRA_CFLAGS += -I$(HI1823_TRUNK_DIR)/src/dpu_develop_interface/fw_msg_intf/mpu

EXTRA_CFLAGS += -Werror
EXTRA_CFLAGS += -Wno-implicit-fallthrough

obj-m += hiudk5.o
hiudk5-objs := hiudk_lld.o \

all:build_info default

build_info:
	mkdir -p build
	@echo "CURDIR=$(CURDIR)" > build/build_src.txt
	@for obj in $(hiudk5-objs); do \
		echo $$(realpath $$obj) >> build/build_src.txt; \
	done

include $(HI1823_BUILD_DIR)/host/linux/Makefile.ko