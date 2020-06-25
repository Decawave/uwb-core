![Decawave logo](https://github.com/Decawave/uwb-core/raw/master/doxy/Decawave.png)

# uwb-core for MyNewt OS

[![Build Status](https://travis-ci.com/Decawave/uwb-core.svg?token=Qc1ARRCEWyUvYoAtFTkY&branch=master)](https://travis-ci.com/Decawave/uwb-core)

- See [Linux Kernel](README_kernel.md) for building uwb-core as a kernel module
- See [Other RTOS](README_cmake.md) for building uwb-core as a standalone library

## Overview

The distribution <https://github.com/decawave/uwb-core> contains the device driver model for the Decawave Impulse Radio-Ultra Wideband (IR-UWB) transceiver(s) within the Mynewt-OS. The driver includes hardware abstraction layers (HAL), media access control (MAC) layer, Ranging Services (RNG) and light weight IP (lwip) stacks. The uwb-core driver and Mynewt-OS combine to create a hardware and architecture agnostic platform for IoT Location Based Services (LBS). This augmented with the newtmgt management tools creates a compelling environment for large-scale deployment of LBS. 
 
## Under-the-hood

The uwb-core driver implements the MAC layers and exports a MAC extension interface for additional services, this MAC interface is defined in the struct uwb_mac_interface found in (../master/hw/driver/uwb/include/uwb.h)


### Ranging Services (RNG)

Ranging services binds to the MAC interface--this interface exposes callbacks to various events within the ranging process. The driver currently support the following ranging profiles;

### Default Config:

| Config  | Description          |  Value  |
| ------------- |:-------------:| -----:|
| PRF  | Pulse Repetition Frequency   |  64MHz  |
| PLEN      | Preamble length         | 128  |
| NPHR      | Number of symbols       | 16  |
| SDF     | start of frame deliminator length  |  8 |
| DataRate     |Data Rate       | 6.8Mbps |

### RNG profile:
| profile       | Description          | Benchmark  |
| ------------- |:-------------:| -----:|
| twr_ss        | Single Sided Two Way Ranging      | 1110us    |
| twr_ds        | Double Sided Two Way Ranging      |  2420us   |

### NRNG profile:

| profile       | Description  | Benchmark  |
| ------------- |:-------------:| -----:|
| nrng_ss | n TWR_SS ranges with 2*n+2 messages  | 1860us for n=4, 2133us for n=6|

## How To Build

See the companion repo https://github.com/decawave/uwb-apps for building instructions. Recall that the above driver is a dependent repo and will be built from within that parent project. 

### Preparation if using mynewt 1.7

```
# After running newt upgrade or newt install we need to patch mynewt
cd repos/apache-mynewt-core
git apply ../decawave-uwb-core/patches/apache-mynewt-core/mynewt_1_7_0*.patch
```


