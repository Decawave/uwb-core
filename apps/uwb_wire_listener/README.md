## Listening via Wireshark

```
# Use the following commands in project's directory:

make -C /lib/modules/$(uname -r)/build M=$(pwd) modules CONFIG_UWB_DW1000=y CONFIG_UWB_DW3000=y CONFIG_UWB_CORE=m CONFIG_UWB_HAL=m CONFIG_UWB_WIRE_LISTENER=y CONFIG_UWB_LISTENER=y

sudo insmod porting/dpl/kernel/uwb_hal/uwb-hal.ko 
sudo insmod uwbcore.ko
sudo insmod apps/uwb_wire_listener/uwb_wire_listener.ko


# Get data from uwb_wire_lstnr into .cap file:

cd /dev/
cat uwb_wire_lstnr > /home/pi/log.cap


# On your host machine tail the file from pi and open it in wireshark:

ssh pi@192.168.9.200 tail -f /home/pi/log.cap | wireshark -k -i -


# Then get root shell and type in the following commands: (if your pi is one of the ranging devices)

echo 0xF > /sys/kernel/uwbcore/uwbcfg/frame_filter
echo 0x1234 > /sys/kernel/uwbcore/dw1000_cli/addr
while (( 1 ));do echo 0 > /sys/kernel/uwbcore/uwbrng0/listen;sleep 0.100;done


# If you want to sniff traffic between two devices: (no need for setting address)

echo 0x0 > /sys/kernel/uwbcore/uwbcfg/frame_filter
echo 0 > /sys/kernel/uwb_wire_listener0/listen

```
