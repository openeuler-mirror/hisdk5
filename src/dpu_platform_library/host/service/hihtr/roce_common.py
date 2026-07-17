#!/usr/bin/python
# -*- coding: UTF-8 -*-
# Copyright Huawei Technologies Co., Ltd. 2024-2025. All rights reserved.

import re
import os
import sys
import shutil
import argparse
import time
import glob
import platform
import copy
import logging
import subprocess

cur_file = os.path.abspath(__file__)
cur_dir = os.path.dirname(cur_file)
build_ko_dir = os.path.join(cur_dir, "build")
knl_drv_src_dir = os.path.join(cur_dir, "kernel")
root_src_dir = os.path.join(cur_dir, "../../../../../")
root_build_dir = os.path.join(root_src_dir, "build")
rpm_build_dir = os.path.join(cur_dir, "rpmbuild")

cie_output_dir = os.path.join(cur_dir, "../../../../../bin/driver/linux")
pack_type = "roce"
kernel_output_dir = os.path.join(cur_dir, 'build')
userspace_output_dir = os.path.join(cur_dir, "build_userspace_scripts/output")

def build_succ_print(outstr):
    print("\033[0;32m{}\033[0m".format(outstr))

def build_fail_print(outstr, terminate=False):
    print("\033[0;31m{}\033[0m".format(outstr))
    sys.exit(-1)

def copy_file(srcfile, dstpath, dst_fname = None):
    if not os.path.isfile(srcfile):
        logging.error(f"{srcfile} not exist!")
        return 1
    else:
        fpath, fname = os.path.split(srcfile)
        if not os.path.exists(dstpath):
            os.makedirs(dstpath)
        if dst_fname:
            shutil.copy(srcfile, f"{dstpath}/{dst_fname}")
        else:
            shutil.copy(srcfile, f"{dstpath}/{fname}")

    return 0

def copy_file_with_end(srcpath, dstpath, suffix):
    if not os.path.exists(srcpath):
        logging.error(f"{srcpath} not exist!")
        return 1
    else:
        if not os.path.exists(dstpath):
            os.makedirs(dstpath)
        for file_name in os.listdir(srcpath):
            if file_name.endswith(suffix):
                ret = copy_file(f"{srcpath}/{file_name}", dstpath)
                if ret:
                    return ret

    return 0

def copy_dir(source_dir, target_dir):
    if not os.path.exists(target_dir):
        os.makedirs(target_dir)
    for root, dirs, files in os.walk(source_dir):
        target_root = os.path.join(target_dir, os.path.relpath(root, source_dir))
        for file in files:
            source_file = os.path.join(root, file)
            target_file = os.path.join(target_root, file)
            shutil.copy(source_file, target_file)

def delete_directory(directory):
    for root, dirs, files in os.walk(directory):
        for file in files:
            os.remove(os.path.join(root, file))
        for dir in dirs:
            shutil.rmtree(os.path.join(root, dir))

def build_run_cmd(cmd):
    ret, output = subprocess.getstatusoutput(cmd)
    print(output)
    return ret

def run_shell_cmd(cmd, output=True):
    rslt = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    if rslt.returncode:
        logging.error(f"exec cmd fail:{' '.join(cmd)}. status = [{rslt.returncode}]. output:[{rslt.stdout}]")
        return 1
    if output:
        logging.info(f"output = [{rslt.stdout}]")
    return 0

def get_pack_dirname():
    _info = os.uname()
    _l = _info[2].split('-')
    _k_version = _l[0]
    _os_arch = '-'.join(_l[1].split('.')[-2:])
    return _k_version + "-" + _os_arch

def ensure_pack_dir():
    _minfo = get_pack_dirname()
    if not os.path.isdir(cie_output_dir):
        build_run_cmd("ls -l %s" % root_src_dir)
        build_fail_print("error: no such directory (%s)" % cie_output_dir)
    _path = os.path.join(cie_output_dir, pack_type, _minfo)
    os.makedirs(_path, mode=0o755, exist_ok=True)
    return _path