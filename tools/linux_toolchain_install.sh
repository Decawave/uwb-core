#!/bin/bash -x

# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#  http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

TOOLCHAIN_PATH=$HOME/TOOLCHAIN

GCC_URL=https://developer.arm.com/-/media/Files/downloads/gnu-rm/8-2018q4/gcc-arm-none-eabi-8-2018-q4-major-linux.tar.bz2
GCC_BASE=gcc-arm-none-eabi-8-2018-q4-major

mkdir -p $TOOLCHAIN_PATH
mkdir ~/bin

if [ ! -s ${TOOLCHAIN_PATH}/$GCC_BASE/bin/arm-none-eabi-gcc ]; then
    wget -O ${TOOLCHAIN_PATH}/${GCC_BASE}.tar.bz2 $GCC_URL
    tar xfj ${TOOLCHAIN_PATH}/${GCC_BASE}.tar.bz2 -C $TOOLCHAIN_PATH
    ${TOOLCHAIN_PATH}/$GCC_BASE/bin/arm-none-eabi-gcc --version
fi

for i in ${TOOLCHAIN_PATH}/${GCC_BASE}/bin/arm-none-eabi-* ; do
    rm -f  ~/bin/${i##*/}
    ln -s $i ~/bin/${i##*/}
done

GCC_URL=https://developer.arm.com/-/media/Files/downloads/gnu-a/8.3-2019.03/binrel/gcc-arm-8.3-2019.03-x86_64-aarch64-linux-gnu.tar.xz
GCC_BASE=gcc-arm-8.3-2019.03-x86_64-aarch64-linux-gnu


if [ ! -s ${TOOLCHAIN_PATH}/$GCC_BASE/bin/aarch64-linux-gnu-gcc ]; then
    wget -O ${TOOLCHAIN_PATH}/${GCC_BASE}.tar.xz $GCC_URL
    tar xf ${TOOLCHAIN_PATH}/${GCC_BASE}.tar.xz -C $TOOLCHAIN_PATH
    ${TOOLCHAIN_PATH}/$GCC_BASE/bin/aarch64-linux-gnu-gcc --version
fi

for i in ${TOOLCHAIN_PATH}/${GCC_BASE}/bin/aarch64-linux-gnu-* ; do
    rm -f  ~/bin/${i##*/}
    ln -s $i ~/bin/${i##*/}
done

