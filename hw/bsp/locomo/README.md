# UbiTraq LOCOMO 

LOCOMO is ultra low-power UWB position module developed by UbiTraq. 

LOCOMO2 -- UT100-TE1

welcome to our website:

	http://www.ubitraq.com/html/index.html

## Overview

- nRF52832 MCU with BLE
- DW1000 chip for UWB.
- PA for more range distance

1. To erase the default flash image that shipped with the LOCOMO.

```no-highlight
$ JLinkExe -device nRF52 -speed 4000 -if SWD
J-Link>erase
J-Link>exit
$ 
```

2. Build the new bootloader applicaiton for the LOCOMO target.

(executed from the mynewt-dw1000-app directory).

```no-highlight

newt target create locomo_boot
newt target set locomo_boot app=@apache-mynewt-core/apps/boot
newt target set locomo_boot bsp=@mynewt-dw1000-core/hw/bsp/locomo
newt target set locomo_boot build_profile=optimized 
newt build locomo_boot
newt create-image locomo_boot 1.0.0
newt load locomo_boot

```

3. On the first locomo-dev board build the Two-Way-Ranging (twr_tag) applicaitons for the LOCOMO module. The run command compiled the project and loads the image on the target platform.

(executed from the mynewt-dw1000-app directory).

```no-highlight

newt target create twr_tag
newt target set twr_tag app=apps/twr_tag
newt target set twr_tag bsp=@mynewt-dw1000-core/hw/bsp/locomo
newt target set twr_tag build_profile=debug 
newt run twr_tag 0

```

4. On a second locomo-dev build the master side of the Two-Way-Ranging (twr_node) applicaitons as follows. 

(executed from the mynewt-dw1000-app directory).

```no-highlight

newt target create twr_node 
newt target set twr_node app=apps/twr_node
newt target set twr_node bsp=@mynewt-dw1000-core/hw/bsp/locomo
newt target set twr_node build_profile=debug 
newt run twr_node 0

```

5. Both examples are configured to use the Segger RTT console interface. This is covered within the mynewt tutorials/Tooling/SeggarRTT (https://mynewt.apache.org/latest/os/tutorials/segger_rtt/). To launch the console simply telnet localhost 19021. Note at time of writing the newt tools does not support multiple connect locomo-dev devices. So it is recomended that you connect twr_tag and twr_node examples to different computers or at least the twr_tag to an external battery. If all going well you should see the twr_node example stream range information on the console. 

(executed from the mynewt-dw1000-app directory).

```no-highlight

telnet localhost 19021

```
