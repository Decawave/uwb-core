
## Mynewt

A few config parameters control the CIR output on the console:

```
uwb/cir_size   <numbers of samples to load, 0-16 (CIR_MAX_SIZE is by default 16)>
uwb/cir_offs   <where the LDE shall be placed in CIR (0 - cir_size) >
uwb/rx_diag_en <to see cir for ipatov (only one implemented thus far) or with 0x40, i.e. 0x71>
```

For example on a nucleo platform, with the cir-package included:

```
config uwb/cir_size 8
config uwb/cir_offs 4
config rx_diag_en 0x71
config commit
config save    # optional, for saving in flash
```

This will setup to show the json sentance for each and every package received with 8 bins from
the accumulator (real and imaginary values) with the detected leading edge placed close to bin 4.

## Kernel module

Same config parameters apply but here they are located in sysfs:

```
echo 8 > /sys/kernel/uwbcore/uwbcfg/cir_size
echo 4 > /sys/kernel/uwbcore/uwbcfg/cir_offs
echo 0x71 > /sys/kernel/uwbcore/uwbcfg/rx_diag_en
echo 1 > /sys/kernel/uwbcore/uwbcfg/commit
```

And the output will be available in /dev/uwbcir as json sentances.
Of course provided the device is listening for packets. 
