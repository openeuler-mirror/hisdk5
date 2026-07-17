if("${BUILD_VERSION}" MATCHES "ub_ascend")
    set(UMMU_CORE_BUILD_DIR ${UBUS_BUILD_DIR}/kernel/ummu-core-v1)
elseif("${PRODUCT}" STREQUAL "ascend910D" OR "${PRODUCT}" STREQUAL "ascend910Desl")
    set(UBUS_DIR ${TOP_DIR}/drivers/ubus)
    set(KDIR ${KERNEL_WORK_DIR}/../linux-4.19)
    set(UMMU_CORE_BUILD_DIR ${UBUS_DIR}/kernel/ummu-core-v1)
else()
    set(UMMU_CORE_BUILD_DIR ${UBUS_BUILD_DIR}/kernel/ummu-core)
endif()

set(UBUS_UBC_BUILD_DIR ${UBUS_BUILD_DIR}/kernel/ubus)

if("${BUILD_VERSION}" MATCHES "b173")
    set(B173_VALUE y)
else()
    set(B173_VALUE n)
endif()

if("${BUILD_VERSION}" MATCHES "b177" OR "${BUILD_VERSION}" MATCHES "b180")
    set(B177_VALUE y)
    set(B173_VALUE y)
else()
    set(B177_VALUE n)
endif()

# =============================== 使用KCompat自动化工具时适配1650的SDK 驱动编译 ===============================
set(SDK_KCOMPAT_GENERATOR_PATH "${TOP_DIR}/ChipSolution/build/host/linux/sdk/sdk-kcompat-generator.sh")
set(SDK_KCOMPAT_PATH "${TOP_DIR}/ChipSolution/src/dpu_develop_interface/drv_sdk_intf/ossl/sdk_kcompat.h")
if("${KDIR}" MATCHES "2403_SP2")
    message(STATUS "KNL_HEADER_TYPE: UB1650")
    set(KERN_VER "NULL")
    set(KSRC "${KDIR}/../../../../open_source/2403_SP2")
    message(STATUS "KSRC = ${KSRC}")

    if(EXISTS ${SDK_KCOMPAT_GENERATOR_PATH})
        message(STATUS "${SDK_KCOMPAT_GENERATOR_PATH} file is exist!")
    endif()

    string(RANDOM LENGTH 4 RAND_NUM)
    set(SDK_KCOMPAT_GENERATOR_PATH_TMP "${TOP_DIR}/ChipSolution/build/host/linux/sdk/sdk-kcompat-generator_${RAND_NUM}.sh")

    if(EXISTS ${SDK_KCOMPAT_GENERATOR_PATH_TMP})
        file(REMOVE ${SDK_KCOMPAT_GENERATOR_PATH_TMP})
    endif()

    file(COPY_FILE "${SDK_KCOMPAT_GENERATOR_PATH}" "${SDK_KCOMPAT_GENERATOR_PATH_TMP}")
    if(EXISTS ${SDK_KCOMPAT_GENERATOR_PATH_TMP})
        message(STATUS  "${SDK_KCOMPAT_GENERATOR_PATH_TMP} file copy succeed")
    else()
        message(STATUS  "${SDK_KCOMPAT_GENERATOR_PATH_TMP} file copy failed")
    endif()

    file(READ ${SDK_KCOMPAT_GENERATOR_PATH_TMP} FILE_CONTENTS)
    string(REPLACE "KERN_VER=\$(uname -r)" "KERN_VER=${KERN_VER}" FILE_CONTENTS "${FILE_CONTENTS}")
    string(REPLACE "KSRC=\"\"" "KSRC=\"${KSRC}\"" FILE_CONTENTS "${FILE_CONTENTS}")
    file(WRITE ${SDK_KCOMPAT_GENERATOR_PATH_TMP} "${FILE_CONTENTS}")

    execute_process(
        COMMAND bash -c "source ${SDK_KCOMPAT_GENERATOR_PATH_TMP} && gen_sdk_kcompat ${SDK_KCOMPAT_PATH}"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        RESULT_VARIABLE RESULT
    )
    if(EXISTS ${SDK_KCOMPAT_GENERATOR_PATH_TMP})
        file(REMOVE ${SDK_KCOMPAT_GENERATOR_PATH_TMP})
    endif()
else()
    message(STATUS "KNL_HEADER_TYPE: DEFAULT")
    execute_process(
        COMMAND bash -c "source ${SDK_KCOMPAT_GENERATOR_PATH} && gen_sdk_kcompat ${SDK_KCOMPAT_PATH}"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        RESULT_VARIABLE RESULT
    )
endif()

if("${PRODUCT}" STREQUAL "ascend910D" OR "${PRODUCT}" STREQUAL "ascend910Desl")
    set(hisdk5_depends ubus)
    add_device_ko(LOCAL_MODULE hisdk5
        KO_SRC_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}
        MAKE_ARGS "IS_ASCEND=true"
        TARGETE_DPENDS "${hisdk5_depends}")
else()
add_custom_target(hisdk5_ko
    COMMENT echo "build ${CMAKE_CURRENT_SOURCE_DIR} start."
    COMMAND cd ${TOP_DIR}/ChipSolution && git apply build/host/linux/sdk/patch_code/knl6_6_compile.patch && cd -
    COMMAND cp -f ${CMAKE_CURRENT_SOURCE_DIR}/Makefile ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${MAKE} -j64 -C ${KDIR} M=${CMAKE_CURRENT_BINARY_DIR} src=${CMAKE_CURRENT_SOURCE_DIR} UBUS_UBC_BUILD_DIR=${UBUS_UBC_BUILD_DIR} UMMU_CORE_BUILD_DIR=${UMMU_CORE_BUILD_DIR} HI1823_TRUNK_DIR=${TOP_DIR}/ChipSolution HI1823_BUILD_DIR=${TOP_DIR}/ChipSolution CONFIG_UBUS_DEVICE=y HI1823_OS_TYPE=openEuler UB_BUILD_B173=${B173_VALUE} UB_BUILD_B177=${B177_VALUE}
    COMMAND cp -f *.ko ${CMAKE_INSTALL_PREFIX}/ko
    COMMAND cd ${TOP_DIR}/ChipSolution && git apply --reverse build/host/linux/sdk/patch_code/knl6_6_compile.patch && cd -
    DEPENDS kernel
)

add_dependencies(hisdk5_ko ubus_ko)
add_dependencies(hisdk5_ko ummu_core_ko)
endif()