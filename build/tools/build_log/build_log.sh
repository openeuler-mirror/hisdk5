#!/bin/bash
# Perform hot backups of Oracle databases.
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
set -e
set +e
CUR_DIR=$PWD
SOURCE_DIR=$1
TARGET_DIR=$2
CMTARGET=$3
INFO_SUFFIX=$4

if [ ! -z "$INFO_SUFFIX" ]; then
    BUILD_INFO_FILE="$TARGET_DIR/build_info_$INFO_SUFFIX.txt"
else
    BUILD_INFO_FILE=$TARGET_DIR/build_info.txt
fi

cd $SOURCE_DIR
# Source
if [ -e "build_src.txt" ]; then
    sed -i "s/ /\n/g" build_src.txt
    count=0
    for line in $(cat build_src.txt)
    do
        let count++
    done
    if [ -e "$BUILD_INFO_FILE" ];then
        echo "" >> $BUILD_INFO_FILE
        echo "Source $count" >> $BUILD_INFO_FILE
    else
        echo "Source $count" > $BUILD_INFO_FILE
    fi
    cat build_src.txt >> $BUILD_INFO_FILE
else
    echo "Source 0" > $BUILD_INFO_FILE
fi

# OpenSource
if [ -e "build_opensrc.txt" ]; then
    sed -i "s/ /\n/g" build_opensrc.txt
    count=0
    for line in $(cat build_opensrc.txt)
    do
        let count++
    done
    echo "OpenSource $count" >> $BUILD_INFO_FILE
    cat build_opensrc.txt >> $BUILD_INFO_FILE
else
    echo "OpenSource 0" >> $BUILD_INFO_FILE
fi

# ThirdParty
if [ -e "build_third_party.txt" ]; then
    sed -i "s/ /\n/g" build_third_party.txt
    count=0
    for line in $(cat build_third_party.txt)
    do
        let count++
    done
    echo "ThirdParty $count" >> $BUILD_INFO_FILE
    cat build_third_party.txt >> $BUILD_INFO_FILE
else
    echo "ThirdParty 0" >> $BUILD_INFO_FILE
fi

# Tool
if [ -e "build_tool.txt" ]; then
    sed -i "s/ /\n/g" build_tool.txt
    count=0
    for line in $(cat build_tool.txt)
    do
        let count++
    done
    echo "Tool $count" >> $BUILD_INFO_FILE
    cat build_tool.txt >> $BUILD_INFO_FILE
else
    echo "Tool 0" >> $BUILD_INFO_FILE
fi

# Target
count=0
target_suffix=".bin\|.ko\|.so\|hinicadm"
for line in $(ls $TARGET_DIR | grep $target_suffix)
do
    let count++
done
echo "RealTarget $count" >> $BUILD_INFO_FILE
ls $TARGET_DIR | grep $target_suffix >> $BUILD_INFO_FILE

# CM Target
count=1
echo "CMTarget $count" >> $BUILD_INFO_FILE
echo $CMTARGET >> $BUILD_INFO_FILE


