# uwb-core for as a standlone library

[![Build Status](https://travis-ci.com/Decawave/uwb-core.svg?token=Qc1ARRCEWyUvYoAtFTkY&branch=master)](https://travis-ci.com/Decawave/uwb-core)


## Overview
This uwb-core repo contains three coexisting build environment newt,kbuild can camke. The Mynewt and newt are the main stream environment for the uwb-core developemnt and is used to generate the default syscfg for the other environment. Kbuild is the user to build the linux kernel modules for Lunix/Android. CThe Cmake environment is used to generate a standalone library for porting to other operating system. The uwb-core contains a Decawave Porting Layer as a shim to other OSs. All platform are buid from the same source tree. 

- See [Linux Kernel](README_kernel.md) for building uwb-core as a kernel module
- See [Mynewt OS](README.md) for building uwb-core using the newt build syste

## Install Toolschain
If you do not already have an arm-none-eabi toolchain on you system you can use the following script to download into $HOME/TOOLCHAIN and install the necessary symbolic links in $HOME/bin.

```
./linux_toolschain_install.sh 
 
```

To generate a standalone library for cortex-m3 for example; (See Makefile.cmake for a list of other targets).

```
make -f Makefile.cmake cortex-m3
make -C build_cortex-m3
cpack -C build_cortex-m3
```



