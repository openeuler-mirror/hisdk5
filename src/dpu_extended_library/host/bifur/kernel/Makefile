# =============================================================================
# hibifur5.ko - Kernel Module Makefile
# =============================================================================

# 当前Makefile所在目录的绝对路径
CUR_MK_DIR_PATH := $(dir $(abspath $(lastword ${MAKEFILE_LIST})))
# =============================================================================
# 编译选项与头文件路径
# =============================================================================
HI1823_TRUNK_DIR := ${CUR_MK_DIR_PATH}/../../../../../
HI1823_BUILD_DIR := ${HI1823_TRUNK_DIR}/build
EXPORT_SYMBOL := true

#HUAWEI SEC
ccflags-y += -I$(HI1823_TRUNK_DIR)/platform/huawei_secure_c/include

ccflags-y += -I$(HI1823_TRUNK_DIR)/src/dpu_develop_interface/fw_msg_intf/public
ccflags-y += -I$(HI1823_TRUNK_DIR)/src/dpu_develop_interface/fw_msg_intf/cfg_mgmt
ccflags-y += -I$(HI1823_TRUNK_DIR)/src/dpu_develop_interface/fw_msg_intf/cqm
ccflags-y += -I$(HI1823_TRUNK_DIR)/src/dpu_develop_interface/fw_msg_intf/bond
ccflags-y += -I$(HI1823_TRUNK_DIR)/src/dpu_develop_interface/fw_msg_intf/mpu
ccflags-y += -I$(HI1823_TRUNK_DIR)/src/dpu_develop_interface/fw_base_intf/hw
ccflags-y += -I$(HI1823_TRUNK_DIR)/src/dpu_develop_interface/fw_base_intf/hw/hi1872v100
ccflags-y += -I$(HI1823_TRUNK_DIR)/src/dpu_platform_library/host/sdk/knldk/crm
ccflags-y += -I$(HI1823_TRUNK_DIR)/src/dpu_develop_interface/drv_sdk_intf/hisdk
ccflags-y += -I$(HI1823_TRUNK_DIR)/src/dpu_platform_library/host/sdk/ossl/linux/kernel
ccflags-y += -I$(HI1823_TRUNK_DIR)/src/dpu_platform_library/host/sdk/ossl/include
ccflags-y += -I$(HI1823_TRUNK_DIR)/src/dpu_platform_library/host/include/sdk/knldk
ccflags-y += -I$(HI1823_TRUNK_DIR)/src/dpu_develop_interface/drv_sdk_intf/ossl
ccflags-y += -I$(HI1823_TRUNK_DIR)/src/dpu_develop_interface/fw_msg_intf/mag
ccflags-y += -I$(HI1823_TRUNK_DIR)/src/dpu_develop_interface/drv_srvc_intf
ccflags-y += -I$(HI1823_TRUNK_DIR)/src/dpu_platform_library/host/service/nic/linux/netdev
ccflags-y += -I$(HI1823_TRUNK_DIR)/src/dpu_platform_library/host/service/nic/linux/ioctl
ccflags-y += -I$(HI1823_TRUNK_DIR)/src/dpu_platform_library/host/service/nic/linux/macsec
ccflags-y += -I$(HI1823_TRUNK_DIR)/src/dpu_develop_interface/fw_msg_intf/nic
ccflags-y += -I$(HI1823_TRUNK_DIR)/src/dpu_develop_interface/fw_msg_intf/public
ccflags-y += -I$(HI1823_TRUNK_DIR)/src/dpu_develop_interface/drv_srvc_intf/nic
ccflags-y += -I$(HI1823_TRUNK_DIR)/src/dpu_platform_library/host/service/nic/linux/ctrl
ccflags-y += -I$(HI1823_TRUNK_DIR)/src/dpu_platform_library/host/service/include
ccflags-y += -I$(HI1823_TRUNK_DIR)/src/dpu_platform_library/host/include/cfm/bond
ccflags-y += -I$(HI1823_TRUNK_DIR)/src/dpu_platform_library/include/nic
ccflags-y += -I$(HI1823_TRUNK_DIR)/src/dpu_platform_library/host/service/nic/include
ccflags-y += -I$(HI1823_TRUNK_DIR)/src/dpu_platform_library/host/service/nic/linux/nicio
ccflags-y += -I$(HI1823_TRUNK_DIR)/src/dpu_platform_library/include/mag
ccflags-y += -I$(HI1823_TRUNK_DIR)/src/dpu_develop_interface/fw_msg_intf/macsec
ccflags-y += -I$(HI1823_TRUNK_DIR)/src/dpu_platform_library/include/drv_tool_msg
ccflags-y += -I$(HI1823_TRUNK_DIR)/src/dpu_platform_library/host/service/nic/comm

ccflags-y += -I${HI1823_TRUNK_DIR}/src/dpu_extended_library/host/bifur/kernel
ccflags-y += -I${HI1823_TRUNK_DIR}/src/dpu_extended_library/common_header/nic
ccflags-y += -I${HI1823_TRUNK_DIR}/src/dpu_extended_library/host/bifur/kernel/include
ccflags-y += -I${HI1823_TRUNK_DIR}/src/dpu_extended_library/host/bifur/kernel/187x_qpool_adapt

# 宏定义
ccflags-y += -DHIUDK_SDK -DHIUDK_ULD
 
# 编译安全与警告选项
# bifur兼容Kylin V10SP3 2403, 环境默认未开启stack-protector, 驱动加载报错; 单独处理该os, 环境里未开启stack-protector则不添加-fstack-protector选项
KERNEL_RELEASE := $(shell uname -r)
ifeq ($(KERNEL_RELEASE), 4.19.90-89.11.v2401.ky10.aarch64)
STACK_PROTECT := $(shell \
grep -q "^CONFIG_STACKPROTECTOR=y" \
$(KERNEL_DIR)/.config && echo y || echo n)
ifeq ($(STACK_PROTECT),y)
ccflags-y += -fstack-protector
$(info enable -fstack-protector)
else
$(info disable -fstack-protector)
endif
else
ccflags-y += -fstack-protector
endif
ccflags-y += -Werror
ccflags-y += -Wall

EXTRA_CFLAGS += -Wno-implicit-fallthrough

# =============================================================================
# 符号导出支持（用于依赖其他模块的符号）
# =============================================================================
NIC_SYMVERS := $(HI1823_TRUNK_DIR)/ci/lib/NIC_Module.symvers
SDK_SYMVERS := $(HI1823_TRUNK_DIR)/ci/lib/SDK_Module.symvers

# 检查依赖文件是否存在，如果不存在报错
$(foreach f,$(NIC_SYMVERS) $(SDK_SYMVERS),\
  $(if $(wildcard $(f)), , $(error "Required symvers file $(f) not found, build dependency module first")))

KBUILD_EXTRA_SYMBOLS := $(NIC_SYMVERS) $(SDK_SYMVERS)

MLNX_OFA_KERNEL_INSTALLED := $(shell rpm -qa | grep -E 'mlnx-ofa_kernel' 2>/dev/null)
ORIGN_LINUXINCLUDE := $(LINUXINCLUDE)
ifneq ($(MLNX_OFA_KERNEL_INSTALLED),)
    KBUILD_EXTRA_SYMBOLS += /usr/src/ofa_kernel/default/Module.symvers
	LINUXINCLUDE := -I/usr/src/ofa_kernel/default/include/ -I/usr/src/ofa_kernel/default/include/uapi $(ORIGN_LINUXINCLUDE)
endif

# =============================================================================
# 包含内核模块构建主 Makefile
# =============================================================================
SECURE_C_PATH = ../../../../../platform/huawei_secure_c/src
obj-m += hibifur5.o
hibifur5-objs := bifur_main.o \
            bifur_vf_mgr.o \
            bifur_pfile.o \
            bifur_event.o \
            bifur_cdev.o \
            bifur_qpool_main.o \
            ./187x_qpool_adapt/187x_qpool_ops.o \
            $(SECURE_C_PATH)/securecutil.o \
            $(SECURE_C_PATH)/secureinput_a.o \
            $(SECURE_C_PATH)/secureprintoutput_a.o \
            $(SECURE_C_PATH)/memset_s.o \
            $(SECURE_C_PATH)/memcpy_s.o \
            $(SECURE_C_PATH)/strncpy_s.o \
            $(SECURE_C_PATH)/strncat_s.o \
            $(SECURE_C_PATH)/sprintf_s.o \
            $(SECURE_C_PATH)/vsprintf_s.o \
            $(SECURE_C_PATH)/vsnprintf_s.o \
            $(SECURE_C_PATH)/snprintf_s.o \
            $(SECURE_C_PATH)/vsscanf_s.o \
            $(SECURE_C_PATH)/sscanf_s.o \
            $(SECURE_C_PATH)/memmove_s.o \
            $(SECURE_C_PATH)/strcpy_s.o \
            $(SECURE_C_PATH)/strcat_s.o \
            $(SECURE_C_PATH)/strtok_s.o \

include $(HI1823_BUILD_DIR)/host/linux/Makefile.ko