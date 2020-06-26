### Edit kernel/hikey-linaro/arch/arm64/boot/dts/hisilicon/hi3660-hikey960.dts to have the following:

```
cd $KERNEL_TOP_DIR/hikey-linaro
git apply $KERNEL_TOP_DIR/modules/uwbcore/porting/dpl/kernel/hikey960_dts.patch 
```

### Compile

```
ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- make -f Makefile.kernel

```


### Install on board:

```
adb push ./porting/dpl/kernel/uwb_hal/uwb-hal.ko /data/local/tmp/
adb shell
su
insmod /data/local/tmp/uwb-hal.ko
```