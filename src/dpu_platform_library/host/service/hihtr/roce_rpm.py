#!/usr/bin/python
# -*- coding: UTF-8 -*-
# Copyright Huawei Technologies Co., Ltd. 2024-2025. All rights reserved.

import os
import sys
import platform
from roce_common import *

def collect_common_file():
    shutil.rmtree(rpm_build_dir, ignore_errors=True)
    cmd = ["mkdir", "-pv", "-m", "777", f"{rpm_build_dir}/BUILD", f"{rpm_build_dir}/BUILDROOT",
    f"{rpm_build_dir}/RPMS", f"{rpm_build_dir}/SOURCES", f"{rpm_build_dir}/SPECS",
    f"{rpm_build_dir}/SRPMS"]
    returncode = run_shell_cmd(cmd, output=True)
    if returncode:
        return returncode

    copy_file(f"{root_src_dir}/src/GLOBAL_VERSION_NEW", f"{rpm_build_dir}/SOURCES")

    src_build_dir = f"{cur_dir}/hihtr5_rpm_build"
    copy_file_with_end(src_build_dir, f"{rpm_build_dir}/SPECS", "spec")

    returncode = copy_dir(src_build_dir, f'{rpm_build_dir}/SOURCES')
    if returncode:
        return returncode

    copy_file_with_end(f"{userspace_output_dir}", f"{rpm_build_dir}/SOURCES", "so")
    signed_ko_dir = ensure_pack_dir()
    copy_file_with_end(signed_ko_dir, f"{rpm_build_dir}/SOURCES", "ko")
    copy_file_with_end(f"{userspace_output_dir}", f"{rpm_build_dir}/SOURCES", "gz")
    cmd = ["tar", "-zxvf" ,f"{userspace_output_dir}/rdma-build.tar.gz","-C", f"{userspace_output_dir}"]

    returncode = run_shell_cmd(cmd, output=True)
    if returncode:
        return returncode
    _path = f'{rpm_build_dir}/SOURCES'
    src_dir = f"{userspace_output_dir}/tmp/rdma-core/build/lib"
    for root, dirs, files in os.walk(src_dir):
        for file in files:
            src_file = os.path.join(root, file)
            dst_file = os.path.join(_path, file)
            shutil.copy(src_file, dst_file)

    src_dir = f"{rpm_build_dir}/SOURCES"
    for root, dirs, files in os.walk(src_dir):
        for file in files:
            src_file = os.path.join(root, file)
            print(src_file)
    directory = f"{userspace_output_dir}/tmp"
    delete_directory(directory)

    bin_pack_dir = ensure_pack_dir()
    bin_tar_gz_path = f"{bin_pack_dir}/rdma-build.tar.gz"
    bin_libibverbs_path = f"{bin_pack_dir}/libibverbs.so"
    if os.path.exists(bin_tar_gz_path):
        os.remove(bin_tar_gz_path)
        os.remove(bin_libibverbs_path)

    return 0

def collect_common_file_rdma():
    shutil.rmtree(rpm_build_dir, ignore_errors=True)
    cmd = ["mkdir", "-pv", "-m", "777", f"{rpm_build_dir}/BUILD", f"{rpm_build_dir}/BUILDROOT",
    f"{rpm_build_dir}/RPMS", f"{rpm_build_dir}/SOURCES", f"{rpm_build_dir}/SPECS",
    f"{rpm_build_dir}/SRPMS"]
    returncode = run_shell_cmd(cmd, output=True)
    if returncode:
        return returncode

    copy_file(f"{root_src_dir}/src/GLOBAL_VERSION_NEW", f"{rpm_build_dir}/SOURCES")

    src_build_dir = f"{cur_dir}/hihtr5_rpm_build"
    copy_file_with_end(src_build_dir, f"{rpm_build_dir}/SPECS", "spec")

    returncode = copy_dir(src_build_dir, f'{rpm_build_dir}/SOURCES')
    if returncode:
        return returncode

    copy_file_with_end(f"{userspace_output_dir}", f"{rpm_build_dir}/SOURCES", "gz")
    cmd = ["tar", "-zxvf" ,f"{userspace_output_dir}/rdma-build.tar.gz","-C", f"{userspace_output_dir}"]

    returncode = run_shell_cmd(cmd, output=True)
    if returncode:
        return returncode
    _path = f'{rpm_build_dir}/SOURCES'
    src_dir = f"{userspace_output_dir}/tmp/rdma-core/build/lib"
    for root, dirs, files in os.walk(src_dir):
        for file in files:
            src_file = os.path.join(root, file)
            dst_file = os.path.join(_path, file)
            shutil.copy(src_file, dst_file)

    src_dir = f"{rpm_build_dir}/SOURCES"
    for root, dirs, files in os.walk(src_dir):
        for file in files:
            src_file = os.path.join(root, file)
            print(src_file)
    file_path = f"{rpm_build_dir}/SOURCES/" + "libibverbs.so.1"
    os.remove(os.path.join(root, file_path))
    file_path = f"{rpm_build_dir}/SOURCES/" + "libibverbs.so"
    os.remove(os.path.join(root, file_path))
    file_path = f"{rpm_build_dir}/SOURCES/" + "libhns-rdmav34.so"
    os.remove(os.path.join(root, file_path))
    directory = f"{userspace_output_dir}/tmp"
    delete_directory(directory)

    bin_pack_dir = ensure_pack_dir()
    bin_tar_gz_path = f"{bin_pack_dir}/rdma-build.tar.gz"
    bin_libibverbs_path = f"{bin_pack_dir}/libibverbs.so"
    if os.path.exists(bin_tar_gz_path):
        os.remove(bin_tar_gz_path)
        os.remove(bin_libibverbs_path)

    return 0

def build_roce_rpm():
    os.makedirs(rpm_build_dir, exist_ok=True)
    ret = collect_common_file()
    if ret:
        return ret

    hi1872_os_arch = platform.machine()
    path = ensure_pack_dir()
    name = os.uname().release
    os.chdir(f"{rpm_build_dir}/SPECS")

    cmd = [
        "rpmbuild", "--define", f'_topdir {rpm_build_dir}','-bb', 'hihtr5.spec',
        "--define", f'KVERSION {name}'
    ]
    returncode = run_shell_cmd(cmd, output=True)
    if returncode:
        return returncode
    src_dir = f"{rpm_build_dir}/RPMS/"
    _path = ensure_pack_dir()
    for root, dirs, files in os.walk(src_dir):
        for file in files:
            if file.endswith('.rpm'):
                src_file = os.path.join(root, file)
                dst_file = os.path.join(_path, file)
                shutil.copy(src_file, dst_file)
    src_dir = f"{rpm_build_dir}/SOURCES/"
    for root, dirs, files in os.walk(src_dir):
        for file in files:
            src_file = os.path.join(root, file)
            print(src_file)

    return 0

def build_roce_deb():
    result = subprocess.run(["bash roce_deb.sh"], capture_output=True, text=True, shell=True)
    print("bash roce_deb.sh")
    print("返回值:", result.returncode)
    print("标准输出:", result.stdout)
    print("错误输出:", result.stderr)

    _path = ensure_pack_dir()
    _cmd = "cp -rf %s/*.deb %s" % (cur_dir, _path)
    ret = build_run_cmd(_cmd)
    if ret != 0:
        build_fail_print("failed to pack deb, cmd is [ %s ]" % _cmd)
    build_succ_print("succeed to pack deb")

    return 0

if __name__ == "__main__":
    if os.path.exists("/etc/debian_version"):
        ret = build_roce_deb()
    else:
        ret = build_roce_rpm()
    if (ret):
         print("roce build rpm or deb error ")
    print("roce build rpm or deb success !")
    _info = os.uname()
    print(_info)