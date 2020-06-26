<!--
# Copyright (C) 2017-2018, Decawave Limited, All Rights Reserved
#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
#  KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
#
-->

# Decawave Porting Layer

## Overview

The Decawave Porting Layer (DPL) abstracts the OS dependencies from the mynewt-dw1000-core (this implementation was inspired by the Runtime.io Nimble porting example). DPL provides an API for OS services required by the driver. 

A standalone CMake build environment co-exists with the newt environment. The design intent is that much of the development will continue under the newt but with an option to build static libraries of the UWB Framework for POSIX,  Zephyr and FreeRTOS instantiations.
```
├── README.md
├── dpl                                                 # API implementation for individual OSs 
│   ├── CMakeLists.txt
│   └── src
│       ├── freertos                                    # API implementation for FreeRTOS, not validated      
│       │   ├── include
│       │   │   └── nimble
│       │   │       ├── nimble_npl_os.h
│       │   │       ├── nimble_port_freertos.h
│       │   │       └── npl_freertos.h
│       │   └── src
│       │       ├── nimble_port_freertos.c
│       │       └── npl_os_freertos.c
│       ├── linux                                      # API implementation for Linux/POSIX
│       │   ├── CMakeLists.txt
│       │   ├── include
│       │   │   └── linux
│       │   │       ├── dpl.h
│       │   │       ├── dpl_atomic.h
│       │   │       ├── dpl_callout.h
│       │   │       ├── dpl_error.h
│       │   │       ├── dpl_eventq.h
│       │   │       ├── dpl_os.h
│       │   │       ├── dpl_os_mutex.h
│       │   │       ├── dpl_os_types.h
│       │   │       ├── dpl_sem.h
│       │   │       ├── dpl_tasks.h
│       │   │       └── dpl_time.h
│       │   ├── src
│       │   │   ├── os_atomic.c
│       │   │   ├── os_callout.c
│       │   │   ├── os_eventq.cc
│       │   │   ├── os_mutex.c
│       │   │   ├── os_sem.c
│       │   │   ├── os_task.c
│       │   │   ├── os_time.c
│       │   │   └── wqueue.h
│       │   └── test
│       │       ├── Makefile
│       │       ├── test_dpl_callout.c
│       │       ├── test_dpl_eventq.c
│       │       ├── test_dpl_mempool.c
│       │       ├── test_dpl_sem.c
│       │       ├── test_dpl_task.c
│       │       └── test_util.h
│       └── mynewt                                      # API implementation for Mynewt, one-to-one binding
│           ├── include
│           │   └── mynewt
│           │       └── dpl.h
│           └── pkg.yml
├── dpl_hal                                             # HAL Layer, for timer services
│   ├── CMakeLists.txt
│   ├── include
│   │   └── hal
│   │       └── hal_timer.h
│   ├── pkg.yml
│   └── src
│       └── hal_timer.c
└── dpl_os                                              # Extra collateral to suppot additional services
    ├── CMakeLists.txt
    ├── include
    │   ├── log
    │   │   └── log.h
    │   ├── mem
    │   │   └── mem.h
    │   ├── modlog
    │   │   └── modlog.h
    │   ├── os
    │   │   ├── endian.h
    │   │   ├── os.h
    │   │   ├── os_dev.h
    │   │   ├── os_error.h
    │   │   ├── os_mbuf.h
    │   │   ├── os_mempool.h
    │   │   ├── os_mutex.h
    │   │   └── queue.h
    │   ├── stats
    │   │   └── stats.h
    │   ├── syscfg
    │   │   └── syscfg.h
    │   └── sysinit
    │       └── sysinit.h
    ├── pkg.yml
    └── src
        ├── endian.c
        ├── mem.c
        ├── os_mbuf.c
        ├── os_mempool.c
        └── os_msys_init.c

```


#Usage
```
make -f Makefile.cmake clean;make -f Makefile.cmake;make -C build_generic/

```

#Testing
```
./build_generic/porting/dpl/src/linux/dpl_callout
```
