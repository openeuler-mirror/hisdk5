cmake_minimum_required(VERSION 3.15.1)
project(roce)

find_package(Git)
if(Git_FOUND)
    EXECUTE_PROCESS(COMMAND git rev-parse HEAD
                    TIMEOUT 1
                    OUTPUT_VARIABLE HIROCE5_DRV_COMPILE_FINGERPRINT
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                    )
else()
    set(HIROCE5_DRV_COMPILE_FINGERPRINT "Unknown")
endif()

set(TARGET_ARCH "${CMAKE_SYSTEM_PROCESSOR}")

if("${TARGET_ARCH}" MATCHES "x86_64|AMD64|amd64")
    set(IS_X86_64 TRUE)
endif()

include(CheckCCompilerFlag)
check_c_compiler_flag(-Wcast-function-type HAS_CAST_FUNCTION_TYPE)

# set(IBVERBS_PABI_VERSION "34")
# set(IBVERBS_PROVIDER_SUFFIX "-rdmav${IBVERBS_PABI_VERSION}.so")
# set(LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

if(IS_X86_64)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mavx512f -Wall -Wextra -Wno-sign-compare -Wno-unused-parameter -Wmissing-prototypes -Wmissing-declarations -Wwrite-strings -Wformat=2 -Wformat-nonliteral -Wnested-externs -Wshadow -Wstrict-prototypes -Wold-style-definition -Wredundant-decls -O2 -g -DNDEBUG -fPIC -std=gnu11 -fPIE -pie")
else()
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wno-sign-compare -Wno-unused-parameter -Wmissing-prototypes -Wmissing-declarations -Wwrite-strings -Wformat=2 -Wformat-nonliteral -Wnested-externs -Wshadow -Wstrict-prototypes -Wold-style-definition -Wredundant-decls -g -DNDEBUG -fPIC -std=gnu11 -O2 -fPIE -pie")
endif()

# 强制启用栈保护（Stack Protector）
# -fstack-protector-strong 从 GCC 4.9 开始支持
include(CheckCCompilerFlag)
check_c_compiler_flag("-fstack-protector-strong" COMPILER_HAS_STACK_PROTECTOR_STRONG)
if(COMPILER_HAS_STACK_PROTECTOR_STRONG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-protector-strong")
else()
    check_c_compiler_flag("-fstack-protector-all" COMPILER_HAS_STACK_PROTECTOR_ALL)
    if(COMPILER_HAS_STACK_PROTECTOR_ALL)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-protector-all")
    endif()
endif()

set(KERNEL_VER ${CMAKE_COMMAND} -E environment uname -r 2>/dev/null)
set(KERNEL_DIR "/lib/modules/${KERNEL_VER}/build")

execute_process(
    COMMAND grep -q "STACKPROTECTOR_NONE=y" "${KERNEL_DIR}/.config"
    RESULT_VARIABLE NO_STACK_PROTECT_RESULT
    ERROR_QUIET
)
execute_process(
    COMMAND grep -q "STACKPROTECTOR=y" "${KERNEL_DIR}/.config"
    RESULT_VARIABLE STACK_PROTECT_RESULT
    ERROR_QUIET
)
if(NOT NO_STACK_PROTECT_RESULT AND STACK_PROTECT_RESULT)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-protector-strong")
endif()

if(HAS_CAST_FUNCTION_TYPE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wcast-function-type")
endif()

add_link_options(-s -Wl,-z,relro,-z,now,-z,noexecstack)

set(CMAKE_SKIP_BUILD_RPATH TRUE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_SKIP_RPATH TRUE)
set(CMAKE_SKIP_INSTALL_RPATH TRUE)

# ROOT_PROJECT_DIR 应该是在其他编译工程中设置的变量，值是ChipSolution顶层目录
if (NOT DEFINED ROOT_PROJECT_DIR)
    set(ROOT_PROJECT_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../../../../../../)
    get_filename_component(ROOT_PROJECT_DIR ${ROOT_PROJECT_DIR} ABSOLUTE)
endif()
# 本地测试
# set(ROOT_PROJECT_DIR /home/z30063835)
include_directories("${ROOT_PROJECT_DIR}/rdma-core/build/include/")
include_directories("${ROOT_PROJECT_DIR}/src/dpu_platform_library/host/service/hihtr/shared/")
include_directories("${ROOT_PROJECT_DIR}/src/dpu_platform_library/host/service/hihtr/abi/")
include_directories("${ROOT_PROJECT_DIR}/src/dpu_platform_library/host/service/hihtr/build_userspace_scripts/output")

link_directories("${ROOT_PROJECT_DIR}/rdma-core/build/lib/")
link_directories("${ROOT_PROJECT_DIR}/rdma-core/build/ccan/")
link_directories("${ROOT_PROJECT_DIR}/rdma-core/build/util/")

aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/../user SRCLIST)

add_library(hihtr SHARED ${SRCLIST})

set_target_properties(hihtr PROPERTIES
    SOVERSION 1
    VERSION 1.0.42.0
    SKIP_BUILD_RPATH TRUE
    BUILD_WITH_INSTALL_RPATH FALSE
    INSTALL_RPATH ""
    INSTALL_RPATH_USE_LINK_PATH FALSE
)

target_link_libraries(hihtr PRIVATE libccan_pic.a librdma_util_pic.a libibverbs.so)
