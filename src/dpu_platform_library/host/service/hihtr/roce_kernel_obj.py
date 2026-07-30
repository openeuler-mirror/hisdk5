#!/usr/bin/python
# -*- coding: UTF-8 -*-
# Copyright Huawei Technologies Co., Ltd. 2024-2025. All rights reserved.

import os
import argparse
from roce_common import *

CONF = argparse.Namespace()

def parse_args():
    global CONF
    _parser = argparse.ArgumentParser(prog="roce-build-tool",
        description="entrance to build roce kernel device driver and userspace dirver")
    _parser.add_argument("--job", choices=['all', 'kernel', 'userspace'], help="specify what to build", default='all')
    _parser.add_argument("-p", choices=['fpga', 'emu', 'null', 'NULL', 'none'], help='specify the type of hardware', default='fpga')
    CONF = _parser.parse_args()
    print("debug: CONF = ", vars(CONF))
    if CONF.p in ['null', 'NULL', 'none']:
        print("debug: make p = fpga, if -p is set to be null")
        CONF.p = 'fpga'
    os.environ['p'] = CONF.p
    return CONF

def build_sdk_symvers():
    os.chdir(root_build_dir)
    ret = build_run_cmd("sh build_main.sh driver nic")
    if ret != 0:
        build_fail_print("1872 RocE build SDK symvers fail!")
    os.chdir(cur_dir)
    build_succ_print("1872 RocE build SDK symvers succ!")

def build_roce_kernel_obj():
    build_sdk_symvers()
    os.chdir(cur_dir)
    if os.path.exists(build_ko_dir):
        shutil.rmtree(build_ko_dir)
    os.makedirs(build_ko_dir)
    os.chdir(knl_drv_src_dir)

    ret = build_run_cmd("make CONFIG_INFINIBAND_HIHTR=m -C /lib/modules/$(uname -r)/build M=$(pwd) modules -j 16")
    if ret != 0:
        build_fail_print("1872 RocE build driver kernel object fail!")
    build_succ_print("cp -rf {}/*.ko {}".format(knl_drv_src_dir, build_ko_dir))
    build_run_cmd("cp -rf {}/*.ko {}".format(knl_drv_src_dir, build_ko_dir))

    os.chdir(cur_dir)
    build_succ_print("1872 RocE build driver kernel object succ!")

def build_roce_usespace_lib():
    userspace_script = os.path.join(cur_dir, "user_lib.sh")
    ret = build_run_cmd("bash {}".format(userspace_script))
    if ret != 0:
        build_fail_print("1872 RoCE build userspace lib fail!")
    build_succ_print("1872 RocE build userspace lib succ!")

def build_roce_kcompat_generator():
    roce_kcompat_generator_script = os.path.join(cur_dir, "roce-kcompat-generator.sh")
    roce_kcompat_generator_h = os.path.join(cur_dir, "roce_kcompat_gen.h")
    mlnx_rpm = subprocess.run(["rpm -qa | grep -E 'rdma-core.*mlnx.*'"], capture_output=True, text=True, shell=True)

    # 使用示例
    if (shutil.which('ofed_info') is not None) or (mlnx_rpm.returncode == 0 and mlnx_rpm.stdout.strip()):
        print("存在mlnx或doca")
        ret = build_run_cmd("bash {} ofed_src".format(roce_kcompat_generator_script))
    else:
        print("mlnx及doca不存在")
        ret = build_run_cmd("bash {}".format(roce_kcompat_generator_script))
    if ret != 0:
        build_fail_print("1872 RoCE build kcompat generator fail!")
    build_succ_print("1872 RocE build kcompat generator succ!")
    build_run_cmd("cat -n {}".format(roce_kcompat_generator_h))

def pack_kernel_ko():
    _path = ensure_pack_dir()
    _cmd = "cp -rf %s/*.ko %s" % (kernel_output_dir, _path)
    ret = build_run_cmd(_cmd)
    if ret != 0:
        build_fail_print("failed to pack kernel ko, cmd is [ %s ]" % _cmd)
    build_succ_print("succeed to pack kernel ko")

def pack_userspace_lib():
    _path = ensure_pack_dir()
    _cmd = "cp -rf %s/* %s" % (userspace_output_dir, _path)
    ret = build_run_cmd(_cmd)
    if ret != 0:
        build_fail_print("failed to pack userpace lib, cmd is [ %s ]" % _cmd)
    build_succ_print("succeed to pack userspace lib")
    os.system(f"chmod 550 {_path}/libhihtr.so")

if __name__ == "__main__":
    parse_args()
    if CONF.job in ['all', 'kernel']:
        build_roce_kcompat_generator()
        build_roce_kernel_obj()
        pack_kernel_ko()
    if CONF.job in ['all', 'userspace']:
        build_roce_usespace_lib()
        pack_userspace_lib()

    print("roce build success !")
    _info = os.uname()
    print(_info)