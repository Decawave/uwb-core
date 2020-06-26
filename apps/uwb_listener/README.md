# UWB Listener / Sniffer module

## Overview
Separate module using the UWBCore api

## Usage

```no-highlight
insmod uwb_listener.ko
echo 0 > /sys/kernel/uwb_listener/listen
cat /dev/uwb_listener

# or to broadcast UWB packets as UDP messages on port 8787 on local lan:
cat /dev/uwb_listener |socat - UDP-DATAGRAM:192.168.10.255:8787,broadcast
```
