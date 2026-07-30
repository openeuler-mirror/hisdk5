#!/bin/bash
#----------------------------------------------------------------------------
# Purpose:
# Copyright Huawei Technologies Co., Ltd. 2026. All rights reserved.
#----------------------------------------------------------------------------

# 读取 os-release 文件
os_info=$(cat /etc/os-release)

unamer=$(uname -r)

# 提取 NAME 和 VERSION 字段
os_name=$(echo "$os_info" | grep -oP 'NAME=\K"[^"]*"')
os_version=$(echo "$os_info" | grep -oP 'VERSION=\K"[^"]*"')

# 去除双引号
os_name=${os_name//\"/}
os_version=${os_version//\"/}

# 名称映射表
case "$os_name" in
    *Anolis*)
        os_short="Anolis"
        ;;
    *BigCloud*)
        os_short="BCLinux"
        os_version=$(echo "$os_info" | grep -oP 'VERSION_ID=\K"[^"]*"' | tr -d '"')
        ;;
    *CentOS*)
      os_short="CentOS"
      centosrelease=$(cat /etc/centos-release)
      os_version=$(echo "$centosrelease" | awk '{print $4}')
      bigversion=$(echo "$os_version" | awk -F'.' '{print $1}')
      subversion=$(echo "$os_version" | awk -F'.' '{print $2}')
      os_version="$bigversion.$subversion"
      ;;
    *Debian*)
        # 字节OS特殊展示,类似Debian-ByteD6.6
        if echo "$unamer" | grep -q "business"; then
            os_short="Debian-ByteD"
            os_version=$(echo "$unamer" | cut -d. -f1-2)
        else
            os_short="Debian"
            # 去除第一个空格及之后的字符串
            os_version=${os_version%% *}
        fi
        ;;
    *EulerOS*)
        #EulerOSV2R12
        os_short="EulerOS"
        eulervversion=$(echo "$os_version" | awk '{print $1}')
        eulervversion=V${eulervversion//.0/}
        eulerrversion=$(echo "$os_version" | awk '{print $2}')
        eulerrversion=${eulerrversion//(SP/}
        eulerrversion=${eulerrversion//x86_64/}
        eulerrversion=${eulerrversion//aarch64/}
        eulerrversion=R${eulerrversion//)/}
        os_version="$eulervversion$eulerrversion"
        ;;
    *Kylin*Advanced*Server*)
        os_short="Kylin"
        os_version=${os_version%% *}
        productinfo=$(cat /etc/.productinfo)
        if echo "$productinfo" | grep -q "Halberd"; then
          os_version="$os_version"SP3-2403
        elif echo "$productinfo" | grep -q "Lance"; then
          os_version="$os_version"SP3
        elif echo "$productinfo" | grep -q "Sword"; then
            os_version="$os_version"SP2
        elif echo "$productinfo" | grep -q "Tercel"; then
            os_version="$os_version"SP1
        fi
        ;;
    *KylinSec*)
        os_short="KylinSec"
        if echo "$unamer" | grep -q "5.10.0-60"; then
          os_version=3.5.1
        elif echo "$unamer" | grep -q "5.10.0-153"; then
          os_version=3.5.2
        elif echo "$unamer" | grep -q "5.10.0-216"; then
          os_version=3.5.3
        fi
        if [ "$os_version" -eq 6 ]; then
            os_version=V"$os_version"
        fi
        ;;
    *openEuler*)
        os_short="openEuler"
        spversion=$(echo "$os_version" | awk -F'[-)]' '{print $2}')
        os_version="${os_version%% *}$spversion"
        ;;
    *Red*Hat*)
        os_short="RedHat"
        # 去除第一个空格及之后的字符串
        os_version=${os_version%% *}
        ;;
    *Rocky*)
        os_short="Rocky"
        # 去除第一个空格及之后的字符串
        os_version=${os_version%% *}
        ;;
    *Ubuntu*)
         # Ubuntu22.04
         os_short="Ubuntu"
         # 去除第一个空格及之后的字符串
         os_version=${os_version%% *}
         ;;
    *UnionTech*|*UOS*)
        os_short="UOS"
        # UOS20的版本非常多
        if [ "$os_version" -eq 20 ] && [ -f /etc/product-info ]; then
            uoseversion=$(cat /etc/product-info | sed 's/[()]//g' | tr ' ' '\n' | awk 'length==5 && /e$/ {print; exit}')
            os_version=V"$os_version-$uoseversion"
        else
            os_version=V"$os_version"
        fi
        ;;
    *VesselOS*)
        os_short="VesselOS"
        # 去除第一个空格及之后的字符串
        os_version=${os_version%% *}
        ;;
    *SLES*|*SUSE*)
        os_short="SUSE"
        os_version=${os_version//-/}
        ;;
    *CULinux*)
        os_short="CUlinux"
        # 去除第一个空格及之后的字符串
        os_version=${os_version%% *}
        ;;
    *LinxOS*)
        os_short="LinxOS"
        # 去除第一个空格及之后的字符串
        os_version=${os_version// /-}
        ;;
    *ctyunos*|*CTyunOS*)
        os_short="CTyunOS"
        ctyunosrelease=$(cat /etc/ctyunos-release)
        os_version=$(echo "$ctyunosrelease" | awk '{print $3}')
        ;;
    *FusionOS*)
        os_short="FusionOS"
        os_version=$(echo "$os_info" | grep -oP 'VERSION_ID=\K"[^"]*"' | tr -d '"')
        ;;
esac

mlnxofed=""
if command -v ofed_info &> /dev/null; then
    # 如果存在，执行 ofed_info -s 并获取输出
    ofedinfo=$(ofed_info -s)
    mlnxofed=-MLNX-OFED$(echo "$ofedinfo" | awk -F'[-]' '{print $3}')
fi

echo "${os_short}${os_version}${mlnxofed}"
finalOSName=${os_short}${os_version}${mlnxofed}
export finalOSName