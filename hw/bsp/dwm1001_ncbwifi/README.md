# DWM1001 + NCB Wifi from Loligoelectronics

Local Positioning System in a very small form factor. Indoor navigation system with 10 cm precision.
Higher precision can be achieved using on-board sensors for inertial navigation. 

- nRF52 as MCU, i.e. ARM Cortex M4F.
- USB micro
- BLE
- DW1000 for UWB.
- LIS2DH12TR motion sensor


The source files are located in the src/ directory.

Header files are located in include/ 

pkg.yml contains the base definition of the package.

To erase the default flash image:

```
$ JLinkExe -device nRF52 -speed 4000 -if SWD
J-Link>erase
J-Link>exit
$ 
```

or shorter:

```
nrfjprog -f NRF52 -e
```

```
newt target create dwm1001_ncbwifi_boot
newt target set dwm1001_ncbwifi_boot app=@apache-mynewt-core/apps/boot
newt target set dwm1001_ncbwifi_boot bsp=@decawave-uwb-core/hw/bsp/dwm1001_ncbwifi
newt target set dwm1001_ncbwifi_boot build_profile=optimized
newt build dwm1001_ncbwifi_boot
newt create-image dwm1001_ncbwifi_boot 1.0.0
newt load dwm1001_ncbwifi_boot
```


```
newt target create dwm1001_ncbwifi_tdoatag
newt target set dwm1001_ncbwifi_tdoatag app=apps/tdoa_tag
newt target set dwm1001_ncbwifi_tdoatag bsp=@decawave-uwb-core/hw/bsp/dwm1001_ncbwifi
newt target set dwm1001_ncbwifi_tdoatag build_profile=debug
newt run dwm1001_ncbwifi_tdoatag 0
```

```
newt target create dwm1001_ncbwifi_listener
newt target set dwm1001_ncbwifi_listener app=apps/listener
newt target set dwm1001_ncbwifi_listener bsp=@decawave-uwb-core/hw/bsp/dwm1001_ncbwifi
newt target set dwm1001_ncbwifi_listener build_profile=debug
newt run dwm1001_ncbwifi_listener 0
```


```
newt target create dwm1001_ncbwifi_node
newt target set dwm1001_ncbwifi_node app=apps/twr_node
newt target set dwm1001_ncbwifi_node bsp=@decawave-uwb-core/hw/bsp/dwm1001_ncbwifi
newt target set dwm1001_ncbwifi_node build_profile=debug
newt run dwm1001_ncbwifi_node 0
```
