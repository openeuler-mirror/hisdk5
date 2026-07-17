if("${BUILD_VERSION}" MATCHES "ub_ascend")
    set(UMMU_CORE_BUILD_DIR ${UBUS_BUILD_DIR}/kernel/ummu-core-v1)
elseif("${PRODUCT}" STREQUAL "ascend910D" OR "${PRODUCT}" STREQUAL "ascend910Desl")
    set(UBUS_DIR ${TOP_DIR}/drivers/ubus)
    set(KDIR ${KERNEL_WORK_DIR}/../linux-4.19)
    set(UMMU_CORE_BUILD_DIR ${UBUS_DIR}/kernel/ummu-core-v1)
else()
    set(UMMU_CORE_BUILD_DIR ${UBUS_BUILD_DIR}/kernel/ummu-core)
endif()

set(HISDK5_BUILD_DIR ${BUILD_CACHE_TOP_DIR}/cmake/ChipSolution/src/dpu_platform_library/host/sdk/knldk/lld)

# =============================== 使用KCompat自动化工具时临时适配1650的NIC驱动编译(TODO)===============================
message(STATUS "KDIR = ${KDIR}")
set(HI1823_TRUNK_DIR ${TOP_DIR}/ChipSolution)
set(NIC_KCOMPAT_GENERATOR_PATH "${HI1823_TRUNK_DIR}/build/host/linux/nic/nic-kcompat-generator.sh")
set(NIC_KCOMPAT_PATH "${HI1823_TRUNK_DIR}/src/dpu_develop_interface/drv_sdk_intf/ossl/nic_kcompat.h")

if("${KDIR}" MATCHES "2403_SP2")
    message(STATUS "KNL_HEADER_TYPE: UB1650")
    set(KERN_VER "NULL")
    set(KSRC "${KDIR}/../../../../open_source/2403_SP2")
    message(STATUS "KSRC = ${KSRC}")

    if(EXISTS ${NIC_KCOMPAT_GENERATOR_PATH})
        message(STATUS "${NIC_KCOMPAT_GENERATOR_PATH} file is exist!")
    endif()

    string(RANDOM LENGTH 4 RAND_NUM)
    set(NIC_KCOMPAT_GENERATOR_PATH_TMP "${HI1823_TRUNK_DIR}/build/host/linux/nic/nic-kcompat-generator_${RAND_NUM}.sh")

    if(EXISTS ${NIC_KCOMPAT_GENERATOR_PATH_TMP})
        file(REMOVE ${NIC_KCOMPAT_GENERATOR_PATH_TMP})
    endif()

    file(COPY_FILE "${NIC_KCOMPAT_GENERATOR_PATH}" "${NIC_KCOMPAT_GENERATOR_PATH_TMP}")
    if(EXISTS ${NIC_KCOMPAT_GENERATOR_PATH_TMP})
        message(STATUS  "${NIC_KCOMPAT_GENERATOR_PATH_TMP} file copy succeed")
    else()
        message(STATUS  "${NIC_KCOMPAT_GENERATOR_PATH_TMP} file copy failed")
    endif()

    file(READ ${NIC_KCOMPAT_GENERATOR_PATH_TMP} FILE_CONTENTS)
    string(REPLACE "KERN_VER=\$(uname -r)" "KERN_VER=${KERN_VER}" FILE_CONTENTS "${FILE_CONTENTS}")
    string(REPLACE "KSRC=\"\"" "KSRC=\"${KSRC}\"" FILE_CONTENTS "${FILE_CONTENTS}")
    file(WRITE ${NIC_KCOMPAT_GENERATOR_PATH_TMP} "${FILE_CONTENTS}")

    execute_process(
        COMMAND bash -c "source ${NIC_KCOMPAT_GENERATOR_PATH_TMP} && gen_nic_kcompat ${NIC_KCOMPAT_PATH}"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        RESULT_VARIABLE RESULT
    )
    if(EXISTS ${NIC_KCOMPAT_GENERATOR_PATH_TMP})
        file(REMOVE ${NIC_KCOMPAT_GENERATOR_PATH_TMP})
    endif()
else()
    message(STATUS "KNL_HEADER_TYPE: DEFAULT")
    execute_process(
        COMMAND bash -c "source ${NIC_KCOMPAT_GENERATOR_PATH} && gen_nic_kcompat ${NIC_KCOMPAT_PATH}"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        RESULT_VARIABLE RESULT
    )
endif()

if(EXISTS ${NIC_KCOMPAT_PATH})
    message(STATUS "${NIC_KCOMPAT_PATH} file is exist!")
else()
    message(STATUS "${NIC_KCOMPAT_PATH} file is not exist!")
endif()
# =====================================================================================================

add_custom_target(hinic5_ko
    COMMENT echo "build ${CMAKE_CURRENT_SOURCE_DIR} start."
    COMMAND cd ${TOP_DIR}/ChipSolution && git apply build/host/linux/sdk/patch_code/knl6_6_compile.patch && cd -
    COMMAND cp -f ${CMAKE_CURRENT_SOURCE_DIR}/Makefile ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${MAKE} -j64 -C ${KDIR} M=${CMAKE_CURRENT_BINARY_DIR} src=${CMAKE_CURRENT_SOURCE_DIR} HI1823_TRUNK_DIR=${TOP_DIR}/ChipSolution HI1823_BUILD_DIR=${TOP_DIR}/ChipSolution HI1823_OS_RELEASE=openEuler24.03 HISDK5_SYMVERS=${HISDK5_BUILD_DIR}/Module.symvers
    COMMAND cp -f *.ko ${CMAKE_INSTALL_PREFIX}/ko
    COMMAND cd ${TOP_DIR}/ChipSolution && git apply --reverse build/host/linux/sdk/patch_code/knl6_6_compile.patch && cd -
    DEPENDS kernel
)

add_dependencies(hinic5_ko hisdk5_ko)