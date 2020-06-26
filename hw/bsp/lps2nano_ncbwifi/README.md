# Lps2nano + NCB Wifi from Loligoelectronics

Local Positioning System in a very small form factor. Indoor navigation system with 10 cm precision.
Higher precision can be achieved using on-board sensors for inertial navigation. 

- nRF52 as MCU, i.e. ARM Cortex M4F.
- USB micro
- BLE
- DWM1000 for UWB.
- ST lsm6dsl 6 axis gyro and accelerometer
- ST lis2mdl 3 axis compass
- ST lps22hb barometer


The source files are located in the src/ directory.

Header files are located in include/ 

pkg.yml contains the base definition of the package.

To erase the default flash image that shipped with the lps2mini board.
```
$ JLinkExe -device nRF52 -speed 4000 -if SWD
J-Link>erase
J-Link>exit
$ 
```

```
newt target create lps2nano_ncbwifi_boot
newt target set lps2nano_ncbwifi_boot app=@apache-mynewt-core/apps/boot
newt target set lps2nano_ncbwifi_boot bsp=@decawave-uwb-core/hw/bsp/lps2nano_ncbwifi
newt target set lps2nano_ncbwifi_boot build_profile=optimized
newt build lps2nano_ncbwifi_boot
newt create-image lps2nano_ncbwifi_boot 1.0.0
newt load lps2nano_ncbwifi_boot
```


```
newt target create lps2nano_ncbwifi_tag
newt target set lps2nano_ncbwifi_tag app=apps/twr_tag
newt target set lps2nano_ncbwifi_tag bsp=@decawave-uwb-core/hw/bsp/lps2nano_ncbwifi
newt target set lps2nano_ncbwifi_tag build_profile=debug
newt run lps2nano_ncbwifi_tag 0
```

Version of the tag code that shows IMU data on the console every 5s

```
newt target create lps2nano_ncbwifi_tag_imu
newt target set lps2nano_ncbwifi_tag_imu app=apps/twr_tag_imu
newt target set lps2nano_ncbwifi_tag_imu bsp=@decawave-uwb-core/hw/bsp/lps2nano_ncbwifi
newt target set lps2nano_ncbwifi_tag_imu build_profile=debug
newt run lps2nano_ncbwifi_tag_imu 0
```


```
newt target create lps2nano_ncbwifi_node
newt target set lps2nano_ncbwifi_node app=apps/twr_node
newt target set lps2nano_ncbwifi_node bsp=@decawave-uwb-core/hw/bsp/lps2nano_ncbwifi
newt target set lps2nano_ncbwifi_node build_profile=debug
newt run lps2nano_ncbwifi_node 0
```
