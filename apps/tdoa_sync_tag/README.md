## UWB Transport test module

This module can act as a sender and receiver for uwb transport messages.
Note that all sent messages are sent to 0xffff - broadcast currently.

### Act as ccp-master

```
# Android specific start
adb shell
su
cd data/local/tmp
# Hikey specific end
insmod uwb-hal.ko
insmod uwbcore.ko
insmod uwb_tp_test.ko
echo 0x0 > /sys/kernel/uwbcore/uwbcfg/frame_filter
echo 1 > /sys/kernel/uwbcore/uwbcfg/commit
echo 0 > /sys/kernel/uwbcore/uwbccp/role; echo 1 > /sys/kernel/uwbcore/uwbccp/enable
echo -e "Insert test-data to send here\n" > /dev/uwbtp
```

### Act as ccp-slave

```
# Android specific start
adb shell
su
cd data/local/tmp
# Hikey specific end
insmod uwb-hal.ko
insmod uwbcore.ko
insmod uwb_tp_test.ko
echo 0x0 > /sys/kernel/uwbcore/uwbcfg/frame_filter
echo 1 > /sys/kernel/uwbcore/uwbcfg/commit
echo 1 > /sys/kernel/uwbcore/uwbccp/role; echo 1 > /sys/kernel/uwbcore/uwbccp/enable
echo -e "Insert test-data to send here\n" > /dev/uwbtp
```

