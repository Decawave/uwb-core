## Prerequisite for HiKey960

### Setup environment
```
export WORKSPACE_TOP_DIR=${WORKSPACE_TOP_DIR:=$HOME/workspace}
export PLATFORM_TOP_DIR=${PLATFORM_TOP_DIR:=$WORKSPACE_TOP_DIR/platform}
export ANDROID_TOP_DIR=${ANDROID_TOP_DIR:=$PLATFORM_TOP_DIR}
export KERNEL_TOP_DIR=${KERNEL_TOP_DIR:=$WORKSPACE_TOP_DIR/kernel}
export BRANCH=mirror-aosp-android-hikey-linaro-4.14
export KERNEL_SRC=$KERNEL_TOP_DIR/out/$BRANCH/hikey-linaro
export KERNEL_DIST_DIR=$KERNEL_TOP_DIR/out/$BRANCH/dist
export KBUILD_OBJ=$KERNEL_TOP_DIR/out/$BRANCH/hikey-linaro
export UWBCORE_TOP_DIR=${UWBCORE_TOP_DIR:=$WORKSPACE_TOP_DIR/uwb-core}
```

### Download
```
mkdir -p ${WORKSPACE_TOP_DIR}/platform
cd $PLATFORM_TOP_DIR
repo init -u https://android.googlesource.com/platform/manifest
repo sync -j$(nproc)

mkdir -p ${WORKSPACE_TOP_DIR}/kernel
cd $KERNEL_TOP_DIR
repo init -u https://android.googlesource.com/kernel/manifest -b android-hikey-linaro-4.14
repo sync -j$(nproc)
```

## Update HDMI
```
cd $PLATFORM_TOP_DIR
wget https://dl.google.com/dl/android/aosp/hisilicon-hikey960-OPR-3c243263.tgz
tar -xf hisilicon-hikey960-OPR-3c243263.tgz
extract-hisilicon-hikey960.sh
```

### build platform
```
cd ${PLATFORM_TOP_DIR}
. ./build/envsetup.sh
lunch hikey960-userdebug
make -j$(nproc)
```

#### Flash baseimage
On Hikey960 Switch to fastboot mode, switch 3 to on
```
fastboot flash boot out/target/product/hikey960/boot.img
fastboot flash dts out/target/product/hikey960/dt.img
fastboot flash system out/target/product/hikey960/system.img
fastboot flash vendor out/target/product/hikey960/vendor.img
fastboot flash userdata out/target/product/hikey960/userdata.img
```
Switch off fast bootmode, switch 3 to off

#### Device Tree
Modify the device tree with the LSI HiKey Connector board configuration.

- Use porting/dpl/kernel/hikey960_dts.patch to update dts and dtsi files in ./arm64/boot/dts/hisilicon/

```
cd $KERNEL_TOP_DIR/hikey-linaro
git apply $KERNEL_TOP_DIR/modules/uwbcore/porting/dpl/kernel/hikey960_dts.patch

```

### Build Kernel
#### Enable UIO module in kernel

```
cd ${KERNEL_TOP_DIR}
echo 'CONFIG_UIO=y' >> $KERNEL_TOP_DIR/hikey-linaro/arch/configs/hikey960_defconfig
```
#### Switch to clang tool gain and build

```
cd ${KERNEL_TOP_DIR}
rm build.config
ln -s hikey-linaro/build.config.hikey960.clang build.config
```

#### Build

```
cd ${KERNEL_TOP_DIR}
build/build.sh
```

#### Copy the generated files over to platform

```
cp -v $KERNEL_DIST_DIR/hi3660-hikey960.dtb ${PLATFORM_TOP_DIR}/device/linaro/hikey-kernel/hi3660-hikey960.dtb-4.14
cp -v $KERNEL_DIST_DIR/Image.gz-dtb ${PLATFORM_TOP_DIR}/device/linaro/hikey-kernel/Image.gz-dtb-hikey960-4.14
```
#### Make boot image for platform

```
cd ${PLATFORM_TOP_DIR}
make bootimage -j$(nproc) TARGET_KERNEL_USE=4.14
${KERNEL_TOP_DIR}/hikey-linaro/mkdtimg -d ${PLATFORM_TOP_DIR}/device/linaro/hikey-kernel/hi3660-hikey960.dtb-4.14 -s 2048 -c -o $PLATFORM_TOP_DIR/out/target/product/hikey960/dt.img
```

#### Reflash boot and dt images
Note: Using the S3 switch on Hikey is a little more reliable than using adb reboot bootloader command
```
adb reboot bootloader
fastboot flash boot $PLATFORM_TOP_DIR/out/target/product/hikey960/boot.img
fastboot flash dts $PLATFORM_TOP_DIR/out/target/product/hikey960/dt.img
fastboot reboot
```

### dw1000-sensor module during booting
Below are the steps insert dw1000-sensor module during booting, below mentioned steps needs to be followed.

1) Add below mentioned line in device/linaro/hikey/hikey960/BoardConfig.mk to enable permissions to load module automatically.
- BOARD_KERNEL_CMDLINE += androidboot.selinux=permissive
2) Add below mentioned line in system/core/rootdir/init.rc in "on post-fs" section to insert module.
- insmod /data/dw1000-sensor.ko
3) Re-build and flash AOSP images (system.img,vendor.img).

4) Push dw1000-sensor.ko in /data folder using adb.
- sudo adb push dw1000-sensor.ko /data
5) Restart the Hikey960.

Note:  If SPI driver is modified then repeat steps (4), (5).

### As an Out-of-Tree Module:

```
cd ${WORKSPACE_TOP_DIR}
git clone git@github.com:Decawave/uwb-core.git
cd ${WORKSPACE_TOP_DIR}/uwb-core
ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- make -f Makefile.kernel

```

#### push modules
```
cd ${WORKSPACE_TOP_DIR}/uwb-core
adb wait-for-device
adb push ./porting/dpl/kernel/uwb_hal/uwb-hal.ko /data/local/tmp
adb push ./uwbcore.ko /data/local/tmp
adb push ./lib/uwb_listener/uwb_listener.ko /data/local/tmp
```
####  load module
```
adb wait-for-device
adb root
adb shell
insmod /data/local/tmp/uwb-hal.ko
insmod /data/local/tmp/uwbcore.ko
insmod /data/local/tmp/uwb_listener.ko
```

- See [Testing and sysfs interface](./porting/dpl/kernel/README.md)

#### Build and run TWR_ALOAH CLI
```
make -f Makefile.cmake arm64
make -C build_arm64
adb push $UWBCORE_TOP_DIR/build_arm64/apps/twr_aloha/TWR_ALOHA /data/local/tmp
adb shell
su
insmod /data/local/tmp/uwb-hal.ko
/data/local/tmp/TWR_ALOHA
```


#### Start CCP Service
```
adb shell
su
echo 1 > /sys/kernel/uwbcore/uwbccp/role
echo 1 > /sys/kernel/uwbcore/uwbccp/enable
cat /dev/uwbccp
```
