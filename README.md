![Decawave logo](https://github.com/Decawave/uwb-core/raw/master/doxy/Decawave.png)

# uwb-core for MyNewt OS

[![Build Status](https://travis-ci.org/Decawave/uwb-core.svg?branch=master)](https://travis-ci.org/Decawave/uwb-core)

- See [Linux Kernel](README_kernel.md) for building uwb-core as a kernel module
- See [Other RTOS](README_cmake.md) for building uwb-core as a standalone library

## Overview

The distribution <https://github.com/decawave/uwb-core> contains the device driver model for the Decawave Impulse Radio-Ultra Wideband (IR-UWB) transceiver(s) within the Mynewt-OS.
The driver includes hardware abstraction layers (HAL), media access control (MAC) layer, Ranging Services (RNG). The uwb-core driver and Mynewt-OS combine to create a hardware and
architecture agnostic platform for IoT Location Based Services (LBS). This augmented with the newtmgt management tools creates a compelling environment for large-scale deployment of LBS.

## Under-the-hood

The uwb-core driver implements the MAC layers and exports a MAC extension interface for additional services, this MAC interface is defined in the struct uwb_mac_interface found in (../master/hw/driver/uwb/include/uwb.h)

## How To Build for mynewt

See the companion repo [UWB-Apps](https://github.com/Decawave/uwb-apps) for building instructions.

### Preparation if using mynewt 1.7

```
# After running newt upgrade or newt install we need to patch mynewt
cd repos/apache-mynewt-core
git apply ../decawave-uwb-core/patches/apache-mynewt-core/mynewt_1_7_0*.patch
```



