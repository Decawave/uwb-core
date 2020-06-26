```
/sys/kernel/debug/uwbcore/
├── dw1000_cli0                   # debug interactions with dw1000 module
│   ├── bt                        # - read to view backtrace of interrupts + spi transactions
│   ├── dump                      # - read to dump registers and semaphore status
│   ├── ev                        # - read event counters
│   └── ibt                       # - read for backtrace of interrupts
└── dw3000_cli0                   # debug interactions with dw3000 module
    ├── bt                        # - read to view backtrace of interrupts + spi transactions
    ├── dump                      # - read to dump registers and semaphore status
    ├── ev                        # - read event counters
    └── ibt                       # - read for backtrace of interrupts

/sys/kernel/uwbcore/
├── dw1000_cli0                   # debug interactions with dw1000 module
│   ├── addr                      # - read/write current uid / short 16-bit address.
│   ├── cw                        # - activate continous wave on current channel
│   ├── device_id                 # - read device-id, should be 0xDECA130
│   ├── euid                      # - read/write current euid / long 64-bit address.
│   ├── ev                        # - reset/enable event counters
│   ├── gpioN                     # - read/write gpio N [0-8]
│   ├── gpioN_dir                 # - set/get direction of gpio N [0-8]
│   └── gpioN_mode                # - set/get mode of gpio N [0-8]
├── dw3000_cli0                   # debug interactions with dw3000 module
│   ├── addr                      # - read/write current uid / short 16-bit address.
│   ├── antmux                    # - control internal antenna mux
│   ├── cw                        # - activate continous wave on current channel
│   ├── device_id                 # - read device-id, should be 0xDECA0300 ... 0xDECA0311 etc
│   ├── euid                      # - read/write current euid / long 64-bit address.
│   ├── ev                        # - reset/enable event counters
│   ├── gpioN                     # - read/write gpio N [0-8]
│   ├── gpioN_dir                 # - set/get direction of gpio N [0-8]
│   └── gpioN_mode                # - set/get mode of gpio N [0-8]
├── stats                         #  Statistic counters, read only
│   ├── ccp                       #  Clock calibration packet stats
│   │   ├── irq_latency
│   │   ├── listen
│   │   ├── master_cnt
│   │   ├── reset
│   │   ├── rx_complete
│   │   ├── rx_relayed
│   │   ├── rx_timeout
│   │   ├── rx_unsolicited
│   │   ├── sem_timeout
│   │   ├── send
│   │   ├── slave_cnt
│   │   ├── tx_complete
│   │   ├── tx_relay_error
│   │   ├── tx_relay_ok
│   │   ├── txrx_error
│   │   ├── tx_start_error
│   │   └── wcs_resets
│   ├── mac                       #  MAC-Layer stats
│   │   ├── DFR_cnt
│   │   ├── LDE_err
│   │   ├── ROV_err
│   │   ├── RTO_cnt
│   │   ├── rx_bytes
│   │   ├── RX_err
│   │   ├── TFG_cnt
│   │   ├── TXBUF_err
│   │   └── tx_bytes
│   ├── rng                       #  Ranging stats, toplevel
│   │   ├── complete_cb
│   │   ├── reset
│   │   ├── rng_listen
│   │   ├── rng_request
│   │   ├── rx_complete
│   │   ├── rx_error
│   │   ├── rx_other_frame
│   │   ├── rx_timeout
│   │   ├── rx_unsolicited
│   │   ├── superframe_reset
│   │   ├── tx_complete
│   │   └── tx_error
│   ├── stat
│   │   └── num_registered
│   ├── twr_ds                    #  Double Sided TWR protocol stack stats
│   │   ├── complete
│   │   └── start_tx_error
│   ├── twr_ds_ext                #  Double Sided TWR protocol stack with Ext data payload stats
│   │   ├── complete
│   │   └── tx_error
│   ├── twr_ss                    #  Single Sided TWR protocol stack stats
│   │   ├── complete
│   │   └── tx_error
│   ├── twr_ss_ack                #  Single Sided TWR with inserted ACK protocol stack stats
│   │   ├── ack_rx_err
│   │   ├── ack_seq_err
│   │   ├── ack_tx_err
│   │   ├── complete
│   │   └── tx_error
│   └── twr_ss_ext                #  Single Sided TWR protocol stack with Ext data payload stats
│       ├── complete
│       └── tx_error
├── uwbccp0                     # Clock Calibration Beacon module
│   ├── enable
│   ├── period                  # Period in DUT (65535*uus)
│   └── role                    # Role [0=Master, 1=Slave, 2=Relaying Slave]
├── uwbcfg                      # UWB Config, read / write of respective parameter. Only active after write to commit
│   ├── channel                 # Channel [1,2,3,4,5,7,9]
│   ├── commit                  # Write anything to this file to make the new configuration active
│   ├── datarate                # Datarate [110k, 850k, 6m8]
│   ├── ext_clkdly              # External clock delay on 38.4Mhz
│   ├── frame_filter            # Bitfield controlling the automatic frame filter
│   ├── prf                     # PRF, [16,64]. Only used on dw1000. DW3000 resolves prf from rx/txcode
│   ├── role                    # Bitfield setting the role (not yet used in kernel)
│   ├── rx_antdly               # Antenna RX delay in dtu, 0x4500 as default
│   ├── rx_ant_separation       # Antenna separation in meters
│   ├── rx_paclen               # Length of PAC [??]
│   ├── rx_pdoa_mode            # Pdoa mode [0,1,3]
│   ├── rx_phrmode              # Phr mode [???]
│   ├── rx_pream_cidx           # RX Preamble Code Index
│   ├── rx_sfdtype              # SFD Type used, [0,1]
│   ├── rx_sts_len              # STS preamble length
│   ├── rx_sts_mode             # STS Mode [0,1,1sdc]
│   ├── tx_antdly               # Antenna TX delay in dtu, 0x4500 as default
│   ├── tx_pream_cidx           # TX Preamble Code Index
│   ├── tx_pream_len            # Ipatov Preamble Length
│   ├── txrf_power_coarse       # Tx power, coarse [0,3,6,...,18]
│   ├── txrf_power_fine         # Tx power, fine [0,1,...,63]
│   └── xtal_trim
└── uwbrng0                     # Ranging services
    ├── listen                  # Write timeout in us to start listening, 0 = listen until a package is received.
    ├── twr_ds                  # Write short address of target to range to using Double Sided TWR
    ├── twr_ds_ext              #   -     "      -                                Double Sided TWR with Ext data passed
    ├── twr_ss                  #   -     "      -                                Single Sided TWR
    ├── twr_ss_ack              #   -     "      -                                Single Sided TWR with inserted Ack
    └── twr_ss_ext              #   -     "      -                                Single Sided TWR with Ext data passed
```

```
/dev/uwbhal0            # Hal direct spi interface to uwb-module
/dev/uwbrng0            # Range results output here as json sentances
/dev/uwbccp0            # Range results output here as json sentances
/dev/uwb_lstnr0         # When inserted, the uwb_listener module with output any received uwb packages here as json sentances.
```
