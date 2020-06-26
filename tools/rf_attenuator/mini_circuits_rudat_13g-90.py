#!/usr/bin/python
import usb.core
import usb.util
import sys
import time
import numpy as np

class _Getch:
    """Gets a single character from standard input.  Does not echo to the
screen."""
    def __init__(self):
        try:
            self.impl = _GetchWindows()
        except ImportError:
            self.impl = _GetchUnix()

    def __call__(self): return self.impl()


class _GetchUnix:
    def __init__(self):
        import tty, sys

    def __call__(self):
        import sys, tty, termios
        fd = sys.stdin.fileno()
        old_settings = termios.tcgetattr(fd)
        try:
            tty.setraw(sys.stdin.fileno())
            ch = sys.stdin.read(1)
        finally:
            termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
        return ch


class _GetchWindows:
    def __init__(self):
        import msvcrt

    def __call__(self):
        import msvcrt
        return msvcrt.getch()


getch = _Getch()

ATTN=90


#find our device
dev = usb.core.find(idVendor=0x20ce, idProduct=0x0023)
if dev is None:
    raise ValueError('Device not found')

for configuration in dev:
    for interface in configuration:
        ifnum = interface.bInterfaceNumber
        if not dev.is_kernel_driver_active(ifnum):
            continue
        try:
            dev.detach_kernel_driver(ifnum)
        except usb.core.USBError, e:
            pass

#set the active configuration. with no args we use first config.
dev.set_configuration()
SerialN=""
ModelN=""
Fw=""

dev.write(1,"*:SN?")
sn=dev.read(0x81,64)
i=1
while (sn[i]<255 and sn[i]>0):
    SerialN=SerialN+chr(sn[i])
    i=i+1
dev.write(1,"*:MN?")
mn=dev.read(0x81,64)
i=1

while (mn[i]<255 and mn[i]>0):
    ModelN=ModelN+chr(mn[i])
    i=i+1
dev.write(1,"*:FIRMWARE?")

sn=dev.read(0x81,64)
i=1
while (sn[i]<255 and sn[i]>0):
    Fw=Fw+chr(sn[i])
    i=i+1

print ("model", ModelN)
print ("serial", SerialN)
print ("fw", Fw)
dev.write(1,"*:ATT?") # return all channels attenuation
resp=dev.read(0x81,64)
i=1
AttResp=""
while (resp[i]<255 and resp[i]>0):
    AttResp=AttResp+chr(resp[i])
    i=i+1
print ("ATT?", AttResp)

print ("'+' increase attenuation by 0.25dB")
print ("'-' decrease attenuation by 0.25dB")
print ("'q' to quit")
while True:
    keyb = getch()
    if (keyb == 'q'): sys.exit(0)
    if (keyb == '+'): ATTN+=0.25
    if (keyb == '-'): ATTN-=0.25
    dev.write(1,"*:CHAN:1:SETATT:{:.2f};".format(ATTN))
    resp=dev.read(0x81,64)
    i=1
    AttResp=""
    while (resp[i]<255 and resp[i]>0):
        AttResp=AttResp+chr(resp[i])
        i=i+1
    print ("ch1->{:.2f}".format(ATTN), AttResp)

    dev.write(1,"*:ATT?") # return all channels attenuation
    resp=dev.read(0x81,64)
    i=1
    AttResp=""
    while (resp[i]<255 and resp[i]>0):
        AttResp=AttResp+chr(resp[i])
        i=i+1
    print ("ATT?", AttResp)


