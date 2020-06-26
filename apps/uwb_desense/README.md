# Decawave Desense RF Test Example

## Overview

Application for performing the three roles of Desense testing:
1. Tag, responds to request for test packets
2. Agressor, continously sends packets to interfere
3. DUT - Device under test, instructs the tag to send data and collects statistics


## Load module

```
insmod <path to module>/uwb_desense.ko
```

## Run a test

### 1. Prepare Tag:

```
$ echo 1 > /sys/kernel/uwb_desense0/rx
$ cat /sys/kernel/uwb_desense0/rx
Listening on 0x1234
```

Note the address given, in this case 0x1234.

### 2. Start Aggressor:

```
# Set a different preamble to not be accepted as real packets by DUT:
$ echo 11 > /sys/kernel/uwbcore/uwbcfg/tx_pream_cidx
$ echo 1  > /sys/kernel/uwbcore/uwbcfg/commit

# Set the desired packet length (1 to 1022 bytes)
$ echo 32 > /sys/kernel/uwb_desense0/params/txon_packet_length

# Activate repeated TX with packet to packet delay of 1000000ns:
$ echo 1000000 > /sys/kernel/uwb_desense0/txon

# To list other config parameters and change them:
$ cat /sys/kernel/uwbcore/uwbcfg/*

# In order to make the config parameters active they have to be commited by
# writing to the "commit" file in sysfs.
```

To stop the aggressor:

```
$ echo 1 > /sys/kernel/uwb_desense0/txoff
```

### 3. On the DUT

Set the test parameters used:

```
$ cat /sys/kernel/uwb_desense0/params/*
ms_start_delay: 1000
ms_test_delay: 10
n_strong: 5
n_test: 100
strong_coarse_power: 9
strong_fine_power: 31
test_coarse_power: 3
test_fine_power: 9
txon_packet_length: 32

# Change the number of test packets to 1234:
$ echo 1234 > /sys/kernel/uwb_desense0/params/n_test
```

Start the test by sending a request to 0x1234:

```
$ echo 0x1234 > /sys/kernel/uwb_desense0/req
```

After the test has finished, view the statistics:

```
$ cat /sys/kernel/debug/uwb_desense/inst0/stats

# Event counters:
  RXPHE:       0  # rx PHR err
  RXFSL:       0  # rx sync loss
  RXFCG:     101  # rx CRC OK
  RXFCE:       0  # rx CRC Errors
  ARFE:        0  # addr filt err
  RXOVRR:      0  # rx overflow
  RXSTO:       1  # rx SFD TO
  RXPTO:       0  # pream search TO
  FWTO:       14  # rx frame wait TO
  TXFRS:       0  # tx frames sent

  PER:  0.000000
  RSSI: -79.737  dBm
  CLKO: 4.627842 ppm
```

Dump the individual package data:

```
$ cat /sys/kernel/debug/uwb_desense/inst0/data
{"idx": 0, "nth_pkt": 0, "rssi": -79.969, "clko_ppm": 4.665782}
{"idx": 1, "nth_pkt": 1, "rssi": -80.305, "clko_ppm": 4.683549}
{"idx": 2, "nth_pkt": 2, "rssi": -79.623, "clko_ppm": 4.739142}
{"idx": 3, "nth_pkt": 3, "rssi": -79.979, "clko_ppm": 4.688134}
{"idx": 4, "nth_pkt": 4, "rssi": -79.397, "clko_ppm": 4.697877}
...
```

## Manipulating the interal antenna mux

DW3000's antenna mux can be manipulated using the sysfs antmux api:

```
$ cat /sys/kernel/uwbcore/dw3000_cli0/antmux
# Antenna Mux examples:
- Force ant 1, pdoa start on ant 1:  0x1100
- Force ant 2, pdoa start on ant 2:  0x2102
- Force ant 1, no ant-sw in pdoa_md: 0x1101
- Force ant 2, no ant-sw in pdoa_md: 0x2103
- Default: 0x0000

# For example:
$ echo 0x1100 > /sys/kernel/uwbcore/dw3000_cli0/antmux
```

## Relevant source code

```
apps/desense/
├── pkg.yml
├── README.md
├── src
│   └── main.c
└── syscfg.yml
repos/decawave-uwb-core/lib/uwb_rf_tests/
└── desense
    ├── CMakeLists.txt
    ├── include
    │   └── desense
    │       └── desense.h
    ├── pkg.yml
    ├── src
    │   ├── desense.c
    │   ├── desense_cli.c
    │   ├── desense_debugfs.c
    │   └── desense_sysfs.c
    └── syscfg.yml
```