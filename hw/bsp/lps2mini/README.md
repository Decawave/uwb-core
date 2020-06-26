# Lps2Mini from Loligoelectronics

Local Positioning System in a very small form factor. Indoor navigation system with 10 cm precision. Higher precision can be achieved using on-board sensors for inertial navigation. 

- nRF52 as MCU, i.e. ARM Cortex M4F.
- USB micro
- BLE
- DWM1000 for UWB.
- Accelerometer/Gyro/Compass: MPU-9250.
- Altimeter: MS5611-01BA03.
- Single cell LiPo charger with power either from USB or a small solar panel.
- Outdoor range: Around 200-300m line of sight depending on your propagation environment.
- Indoor range: Around 30-50m depending on your walls.
- Size: 25×30 mm.


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
newt target create lps2mini_boot
newt target set lps2mini_boot app=@apache-mynewt-core/apps/boot
newt target set lps2mini_boot bsp=@mynewt-dw1000-core/hw/bsp/lps2mini
newt target set lps2mini_boot build_profile=optimized 
newt build lps2mini_boot
newt create-image lps2mini_boot 1.0.0
newt load lps2mini_boot
```


```
newt target create lps2mini_tag
newt target set lps2mini_tag app=apps/twr_tag
newt target set lps2mini_tag bsp=@mynewt-dw1000-core/hw/bsp/lps2mini
newt target set lps2mini_tag build_profile=debug
newt run lps2mini_tag 0
```


```
newt target create lps2mini_node
newt target set lps2mini_node app=apps/twr_node
newt target set lps2mini_node bsp=@mynewt-dw1000-core/hw/bsp/lps2mini
newt target set lps2mini_node build_profile=debug
newt run lps2mini_node 0
```