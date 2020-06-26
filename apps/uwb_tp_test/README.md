## UWB Transport test module

This module can act as a sender and receiver for uwb transport messages.
Note that all sent messages are sent to 0xffff - broadcast currently.

### Act as ccp-master and video stream endpoint

```
# Android specific start
adb shell
su
cd data/local/tmp
# Hikey specific end
insmod uwb-hal.ko
insmod uwbcore.ko
insmod uwb_tp_test.ko
echo 0xF > /sys/kernel/uwbcore/uwbcfg/frame_filter
echo 1 > /sys/kernel/uwbcore/uwbcfg/commit
# Below only needed if using CCP
# echo 0 > /sys/kernel/uwbcore/uwbccp/role; echo 1 > /sys/kernel/uwbcore/uwbccp/enable

# Display incoming video stream on framebuffer
mplayer -vo fbdev -vf scale=800:480 -cache 8192 -cache-min 2 /dev/uwbtp0
```

### Act as ccp-slave and video stream sender

```
# Android specific start
adb shell
su
cd data/local/tmp
# Hikey specific end
insmod uwb-hal.ko
insmod uwbcore.ko
insmod uwb_tp_test.ko
# Below only needed if using CCP
#echo 0x0 > /sys/kernel/uwbcore/uwbcfg/frame_filter
#echo 1 > /sys/kernel/uwbcore/uwbcfg/commit
#echo 1 > /sys/kernel/uwbcore/uwbccp/role; echo 1 > /sys/kernel/uwbcore/uwbccp/enable
# End if CCP
# Set destination address
echo 0x1234 > /sys/kernel/uwbtp0/dest_uid
echo -e "Insert test-data to send here\n" > /dev/uwbtp0

# Download youtube stream and send to uwbtp char device
youtube-dl -f 135 -o - https://www.youtube.com/watch?v=xY96v0OIcK4 >/dev/uwbtp0
# Send local file to be displayed
dd if=stream_file.bin of=/dev/uwbtp0
```

