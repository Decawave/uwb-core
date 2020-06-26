# Kernel modules for UWB-core

Device tree usage overview: [README_sysfs.md](README_sysfs.md)

## Device tree changes needed for hikey960
See porting/dpl/kernel/hikey960_dts.patch or porting/dpl/kernel/hikey960_dts_rstbuf.patch for details. The patch with _rstbuf in the name is for hikey-boards where there's a buffer / level-shifter on the RSTn pin. This is true for all dw1000 and some dwm3020 boards.

## Raspberry Pi

Prerequisites
```
apt-get install raspberrypi-kernel-headers device-tree-compiler
```

Build device tree overlay:
```
dtc -W no-unit_address_vs_reg -@ -I dts -O dtb -o uwbhal.dtbo rpi_uwbhal_overlay.dts
mv uwbhal.dtbo /boot/overlays/
echo "dtoverlay=uwbhal" >> /boot/config.txt # Or wherever your permanent config.txt resides
```

Build module natively on rapi:
```
KERNEL_SRC=/lib/modules/$(uname -r)/build ARCH=arm make -f Makefile.kernel modules
```

### Command lines for tests (Hikey)

```
adb wait-for-device;adb shell
# For dwm1000
su root /data/local/tmp/TWR_TAG

# in separate shell, for viewing dmesg
$HOME/dev/android/out/soong/host/linux-x86/bin/adb shell su root dmesg -w
```

#### Act as Listening Node
```
su
ls /sys/class/uwbhal/uwbhal/device/ -l
echo performance > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
echo 1 > /sys/devices/system/cpu/cpu0/cpuidle/state0/disable
echo 1 > /sys/devices/system/cpu/cpu0/cpuidle/state1/disable
cd data/local/tmp
insmod uwb-hal.ko
insmod uwbcore.ko
echo 0x1234 > /sys/kernel/uwbcore/dw3000_cli0/addr
echo 0xF > /sys/kernel/uwbcore/uwbcfg/frame_filter
echo 1 > /sys/kernel/uwbcore/uwbcfg/commit
while (( 1 ));do echo 0 > /sys/kernel/uwbcore/uwbrng/listen;sleep 0.001;done
```

#### Act as Ranging tag
```
su
ls /sys/class/uwbhal/uwbhal/device/ -l
echo performance > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
echo 1 > /sys/devices/system/cpu/cpu0/cpuidle/state0/disable
echo 1 > /sys/devices/system/cpu/cpu0/cpuidle/state1/disable
cd data/local/tmp
insmod uwb-hal.ko
insmod uwbcore.ko
while (( 1 ));do echo 0x1234 > /sys/kernel/uwbcore/uwbrng/twr_ss_ack;sleep 0.01;done
```

#### Various dbg commands
```
cat /sys/kernel/uwbcore/dw3000_cli0/dump
cat /sys/kernel/uwbcore/stats/twr_ss_ack/*
cat /dev/uwbrng
echo 0x1234 > /sys/kernel/uwbcore/uwbrng/twr_ss_ack

echo 64 > /sys/kernel/uwbcore/uwbcfg/tx_pream_len
echo 64 > /sys/kernel/uwbcore/uwbcfg/commit
cat /sys/kernel/uwbcore/uwbcfg/tx_pream_len
cat /sys/kernel/uwbcore/uwbcfg/*
cat /sys/kernel/uwbcore/stat/*
ls /sys/kernel/uwbcore/uwbcfg/*|while read name;do echo $name;cat $name;done
```
