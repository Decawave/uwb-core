# uwb-core for Linux Kernel driver

- See [HiKey960 Prerequisite](README_hikey960.md) if building uwb-core module for HiKey960 development platform.
- See [Raspberry Pi Prerequisite](./porting/dpl/kernel/README.md) if building uwb-core module for Raspberry Pi.

The uwb-core support both the DW1000 and DW3X00 devices and detect the device at runtime. The uwb-core comprises of two modules

- ./uwbcore.ko
- ./porting/dpl/kernel/uwb_hal/uwb-hal.ko

## How To Build

### Preparing

```
make -f Makefile.cmake kernel-prereq
```

### As An In-Kernel Driver:
- Place contents in <kernel-src>/driver/misc/uwbcore
- Also include the specific drivers needed from separate repos (decawave-uwb-dw1000, decawave-uwb-dw3000-c0,...)
- Add `CONFIG_UWB_DW1000=y` to config.
- Add `CONFIG_UWB_DW3000=y` to config.
- Add `CONFIG_UWB_CORE=y` (or m) to config.
- Add `CONFIG_UWB_HAL=y` (or m) to config.
- Build as usual.

### As An Out-of-Tree Module:
- From repository root: `make KERNEL_SRC=<kernel-src> O=<out-dir> modules modules_install`

### In Android Kernel Project:
- Add to local manifest (i.e. .repo/local_manifests/uwbcore.xml):
  ```
  <?xml version="1.0" encoding="UTF-8"?>
  <manifest>
    <remote name="uwb-core" fetch="ssh://git@github.com" />
    <project path="modules/uwbcore" name="Decawave/uwb-core" remote="uwb-core" revision="master" />
  </manifest>
  ```
  - Add to local manifest (i.e. .repo/local_manifests/uwbcore-syscfg.xml):
  ```
  <?xml version="1.0" encoding="UTF-8"?>
  <manifest>
    <remote name="uwb-core-syscfg" fetch="ssh://git@github.com" />
    <project path="modules/uwbcore/bin" name="Decawave/uwb-core-syscfg" remote="uwb-core-syscfg" revision="master" />
  </manifest>
  ```

- Perform `repo sync`.
- Add `modules/uwbcore` to `EXT_MODULES` in build.config.
  ```
  echo EXT_MODULES="modules/uwbcore" >> build.config
  ```
- Build as usual.
