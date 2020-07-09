/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef __UWB_H__
#define __UWB_H__

#include <dpl/dpl.h>
#include <dpl/dpl_types.h>
#include <os/os_dev.h>

#ifdef __KERNEL__
#include <linux/module.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

//! Extension ids for services.
typedef enum uwb_extension_id {
    UWBEXT_CCP=1,                            //!< Clock Calibration Packet
    UWBEXT_WCS,                              //!< Wireless Clock Synchronization services
    UWBEXT_TDMA,                             //!< TDMA services
    UWBEXT_RNG,                              //!< Ranging
    UWBEXT_RNG_SS,                           //!< Ranging
    UWBEXT_RNG_SS_EXT,                       //!< Ranging
    UWBEXT_RNG_SS_ACK,                       //!< Ranging
    UWBEXT_RNG_DS,                           //!< Ranging
    UWBEXT_RNG_DS_EXT,                       //!< Ranging
    UWBEXT_RANGE,                            //!< Ranging
    UWBEXT_NRNG,                             //!< Nrng
    UWBEXT_NRNG_SS,                          //!< Nrng
    UWBEXT_NRNG_SS_EXT,                      //!< Nrng
    UWBEXT_NRNG_DS,                          //!< Nrng
    UWBEXT_NRNG_DS_EXT,                      //!< Nrng
    UWBEXT_LWIP,                             //!< LWIP
    UWBEXT_PAN,                              //!< Personal area network
    UWBEXT_PROVISION,                        //!< Provisioning
    UWBEXT_NMGR_UWB,                         //!< UWB transport layer
    UWBEXT_NMGR_CMD,                         //!< UWB command support
    UWBEXT_CIR,                              //!< Channel impulse response
    UWBEXT_OT = 0x30,                        //!< Openthread
    UWBEXT_RTDOA = 0x40,                     //!< RTDoA
    UWBEXT_RTDOA_BH,                         //!< RTDoA Backhaul
    UWBEXT_SURVEY = 0x50,                    //!< Survey
    UWBEXT_TRANSPORT = 0x60,                 //!< Data Transport layer
    UWBEXT_LISTENER = 0x70,                  //!< Listener / Sniffer
    UWBEXT_RF_DESENSE = 0x80,                //!< RF Desense test
    UWBEXT_APP0 = 1024,
    UWBEXT_APP1,
    UWBEXT_APP2
} uwb_extension_id_t;

//! Device Roles
#define UWB_ROLE_CCP_MASTER   (0x0001)  //!< Act as Clock Master for the network
#define UWB_ROLE_PAN_MASTER   (0x0002)  //!< Act as Pan Master handing out slots and addresses
#define UWB_ROLE_ANCHOR       (0x0004)  //!< Act as an Anchor, a non-mobile unit


//! IDs for blocking/non-blocking mode .
typedef enum _uwb_dev_modes_t{
    UWB_BLOCKING,                    //!< Blocking mode
    UWB_NONBLOCKING                  //!< Non-blocking mode
}uwb_dev_modes_t;

//! Structure of UWB device capabilities.
struct uwb_dev_capabilities {
    union {
        struct {
            uint16_t sts:1;                   //!< STS sequence capable
            uint16_t single_receiver_pdoa:1;  //!< Single receiver pdoa possible
        };
        uint16_t val;
    };
};

//! Structure of UWB device status.
struct uwb_dev_status {
    uint32_t selfmalloc:1;            //!< Internal flag for memory garbage collection
    uint32_t initialized:1;           //!< Instance allocated
    uint32_t start_tx_error:1;        //!< Start transmit error
    uint32_t start_rx_error:1;        //!< Start receive error
    uint32_t tx_frame_error:1;        //!< Transmit frame error
    uint32_t txbuf_error:1;           //!< Tx buffer error
    uint32_t rx_error:1;              //!< Receive error
    uint32_t rx_timeout_error:1;      //!< Receive timeout error

    uint32_t rx_autoframefilt_rej:1;  //!< Automatic Frame Filter rejection
    uint32_t rx_prej:1;               //!< Rx preamble rejection
    uint32_t lde_error:1;             //!< Error in Leading Edge Detection
    uint32_t spi_error:1;             //!< SPI error
    uint32_t LDE_enabled:1;           //!< Load LDE microcode on wake up
    uint32_t LDO_enabled:1;           //!< Load the LDO tune value on wake up
    uint32_t autoack_triggered:1;     //!< Autoack event triggered
    uint32_t sleep_enabled:1;         //!< Indicates sleep_enabled bit is set

    uint32_t sleeping:1;              //!< Indicates sleeping state
    uint32_t sem_force_released:1;    //!< Semaphore was released in forcetrxoff
    uint32_t overrun_error:1;         //!< Dblbuffer overrun detected
    uint32_t rx_restarted:1;          //!< RX restarted since last received packet
    uint32_t ext_sync:1;              //!< External sync successful
    uint32_t spi_r_error:1;           //!< SPI CRC failed on read
    uint32_t spi_w_error:1;           //!< SPI CRC failed on read
    uint32_t spi_fifo_error:1;        //!< SPI FIFO overflow or underflow

    uint32_t cmd_error:1;             //!< Fast CMD error, cmd was ignored
    uint32_t aes_error:1;             //!< AES-DMA error, AES auth err, or DMA conflict
    uint32_t sem_error:1;             //!< Semaphore error
    uint32_t mtx_error:1;             //!< Mutex error
    uint32_t sts_pream_error:1;       //!< STS Preamble error
    uint32_t sts_ts_error:1;          //!< STS Timestamp doesn't match Ipatov Timestamp
    uint32_t dblbuff_current:2;       //!< Current access dblbuffer (0=off, 1=A, 2=B)
};

//! Clock recovery calculation types
typedef enum {
    UWB_CR_CARRIER_INTEGRATOR,        //!< Clock ratio from Carrier integrator
    UWB_CR_RXTTCKO                    //!< Clock ratio from Receiver time tracking offset
} uwb_cr_types_t;

/** Parent structure for rx diagnostics. Used to store values for later
 * processing if needed in a device agnostic way.
 */
struct uwb_dev_rxdiag {
    uint16_t rxd_len;                 //!< length of rxdiag structure
    uint16_t enabled;                 //!< Features enabled by rxdiag structure
    uint16_t valid;                   //!< Features valid in rxdiag structure
};

#define UWB_RXDIAG_COMB_TPDOA    (0x0001)  //!< tdoa + pdoa
#define UWB_RXDIAG_COMB_XTALO    (0x0002)  //!< xtaloffset etc
#define UWB_RXDIAG_IPATOV_RXTIME (0x0010)  //!< Ipatov phase of arrival and rxtime
#define UWB_RXDIAG_IPATOV        (0x0020)  //!< Ipatov statistics
#define UWB_RXDIAG_IPATOV_CIR    (0x0040)  //!< Ipatov CIR output
#define UWB_RXDIAG_STS_RXTIME    (0x0100)  //!< STS phase of arrival and rxtime
#define UWB_RXDIAG_STS           (0x0200)  //!< STS statistics
#define UWB_RXDIAG_STS_CIR       (0x0400)  //!< STS CIR output
#define UWB_RXDIAG_STS2_RXTIME   (0x1000)  //!< STS2 phase of arrival and rxtime
#define UWB_RXDIAG_STS2          (0x2000)  //!< STS2 statistics
#define UWB_RXDIAG_STS2_CIR      (0x4000)  //!< STS2 CIR output

//! physical attributes per IEEE802.15.4-2011 standard, Table 101
struct uwb_phy_attributes {
    dpl_float32_t Tpsym;       //!< Preamble symbols duration (usec) for MPRF
    dpl_float32_t Tbsym;       //!< Baserate symbols duration (usec)
    dpl_float32_t Tdsym;       //!< Datarate symbols duration (usec)
    uint8_t nsfd;              //!< Number of symbols in start of frame delimiter
    uint16_t nsync;            //!< Number of symbols in preamble sequence
    uint16_t nstssync;         //!< Number of symbols in STS preamble sequence
    uint8_t nphr;              //!< Number of symbols in phy header
    uint8_t phr_rate:1;        //!< Rate of phr symbols (0=base, 1=data)
};

//! Event Counters
struct uwb_dev_evcnt {
    union {
        uint32_t event_count0;
        struct {
            uint16_t count_rxphe;   //!< Count of RX PHR Errors
            uint16_t count_rxfsl;   //!< Count of RX Frame sync loss events
        } ev0s __attribute__((__packed__, aligned(1)));
    };
    union {
        uint32_t event_count1;
        struct {
            uint16_t count_rxfcg;   //!< Count of RX Frame CRC Good events
            uint16_t count_rxfce;   //!< Count of RX Frame CRC Error events
        } ev1s __attribute__((__packed__, aligned(1)));
    };
    union {
        uint32_t event_count2;
        struct {
            uint16_t count_arfe;    //!< Count of Address filter errors
            uint16_t count_rxovrr;  //!< Count of Receive overflow events
        } ev2s __attribute__((__packed__, aligned(1)));
    };
    union {
        uint32_t event_count3;
        struct {
            uint16_t count_rxsto;   //!< Count of RX SFD Timeouts
            uint16_t count_rxpto;   //!< Count of preamble search timeouts
        } ev3s __attribute__((__packed__, aligned(1)));
    };
    union {
        uint32_t event_count4;
        struct {
            uint16_t count_fwto;    //!< Count of Receive frame wait timeouts
            uint16_t count_txfrs;   //!< Count of Transmit data frames sent
        } ev4s __attribute__((__packed__, aligned(1)));
    };
    union {
        uint32_t event_count5;
        struct {
            uint16_t count_hpwarn;   //!< Count of Half Period Warnings
            uint16_t count_spicrc;   //!< Count of SPI CRC Errors
        } ev5s __attribute__((__packed__, aligned(1)));
        /* DW1000 has tpwarn instead of spicrc */
        struct {
            uint16_t count_hpwarn;   //!< Count of Half Period Warnings
            uint16_t count_tpwarn;   //!< Count of Tx power-up Warnings
        } ev5s_1000 __attribute__((__packed__, aligned(1)));
    };
    union {
        uint32_t event_count6;
        struct {
            uint16_t count_notused;  //!< Not used
            uint16_t count_rxprej;   //!< Count of Preamble rejections in the receiver
        } ev6s __attribute__((__packed__, aligned(1)));
    };
    union {
        uint32_t event_count7;
        struct {
            uint16_t count_cperr;   //!< Count of STS quality errors. Note this may increment in units > 1 as multiple srcs contribute.
            uint16_t count_vwarn;   //!< Count of Vwarn flag events
        } ev7s __attribute__((__packed__, aligned(1)));
    };
};

/* Forward declaration */
struct uwb_dev;

//! Structure of extension callbacks structure common for mac layer.
struct uwb_mac_interface {
    struct {
        uint16_t initialized:1;           //!< Instance initialized
    } status;
    uint16_t id;                          //!< Identifier
    void *inst_ptr;                       //!< Pointer to instance owning this interface
    bool (* tx_begins_cb)   (struct uwb_dev *, struct uwb_mac_interface *);    //!< Transmit frame begins callback
    bool (* tx_complete_cb) (struct uwb_dev *, struct uwb_mac_interface *);    //!< Transmit complete callback
    bool (* rx_complete_cb) (struct uwb_dev *, struct uwb_mac_interface *);    //!< Receive complete callback
    bool (* cir_complete_cb)(struct uwb_dev *, struct uwb_mac_interface *);    //!< CIR complete callback, prior to RXEN
    bool (* rx_timeout_cb)  (struct uwb_dev *, struct uwb_mac_interface *);    //!< Receive timeout callback
    bool (* rx_error_cb)    (struct uwb_dev *, struct uwb_mac_interface *);    //!< Receive error callback
    bool (* tx_error_cb)    (struct uwb_dev *, struct uwb_mac_interface *);    //!< Transmit error callback
    bool (* reset_cb)       (struct uwb_dev *, struct uwb_mac_interface *);    //!< Reset interface callback
    bool (* final_cb)       (struct uwb_dev *, struct uwb_mac_interface *);    //!< Final frame preperation interface callback
    bool (* complete_cb)    (struct uwb_dev *, struct uwb_mac_interface *);    //!< Completion event interface callback
    bool (* sleep_cb)       (struct uwb_dev *, struct uwb_mac_interface *);    //!< Wakeup event interface callback
    bool (* superframe_cb)  (struct uwb_dev *, struct uwb_mac_interface *);    //!< Marks the start of a UWB_CCP superframe
    SLIST_ENTRY(uwb_mac_interface) next;                                           //!< Next callback in the list
};

//! Receiver configuration parameters.
struct uwb_dev_rx_config {
    uint8_t pacLength;                      //!< Acquisition Chunk Size DWT_PAC8..DWT_PAC64 (Relates to RX preamble length)
    uint8_t preambleCodeIndex;              //!< RX preamble code
    uint8_t sfdType;                        //!< 0 to use standard 8 symbol SFD, 1 to use non-standard 8 symbol, 2 for non-standard 16 symbol SFD and 3 for 4z 8 symbol SDF type
    uint8_t phrMode;                        //!< PHR mode {0x0 - standard DWT_PHRMODE_STD, 0x3 - extended frames DWT_PHRMODE_EXT}
    uint8_t phrRate;                        //!< PHR rate {0x0 - standard DWT_PHRRATE_STD, 0x1 - at datarate DWT_PHRRATE_DTA}
    uint16_t sfdTimeout;                    //!< SFD timeout value (in symbols) (preamble length + 1 + SFD length - PAC size).
    uint8_t stsMode;                        //!< STS mode (no sts, STS before PHR or STS after data)
    uint8_t stsLength;                      //!< STS length (the allowed values are from 7 to 255, 7 corresponds to 64 length and 254 is 2040 which is a max value)
    uint8_t pdoaMode;                       //!< PDOA mode
    uint8_t timeToRxStable;                 //!< Time until the Receiver i stable, (us)
    uint16_t frameFilter;                   //!< Frame filter config
    uint8_t xtalTrim;                       //!< Crystal trim config
#if MYNEWT_VAL(CIR_ENABLED)
    uint16_t cirSize;
    uint16_t cirOffset;
#endif
};

//! UWB transmitter configuration parameters.
struct uwb_dev_tx_config {
    uint8_t preambleCodeIndex;              //!< TX preamble code
    uint8_t preambleLength;                 //!< DWT_PLEN_64..DWT_PLEN_4096
};

//! UWB transmitter power configuration parameters.
struct uwb_dev_txrf_config {
    uint8_t   PGdly;
    union _power {
        struct _smart{
            uint8_t BOOSTNORM;      //!< PWR_TX_DATA_PWR
            uint8_t BOOSTP500;      //!< PWR_TX_PHR_PWR
            uint8_t BOOSTP250;      //!< PWR_TX_SHR_PWR
            uint8_t BOOSTP125;      //!< TODO
        };
        struct _manual {
            uint8_t _NA1;           //!< TODO
            uint8_t TXPOWPHR;       //!< TODO
            uint8_t TXPOWSD;        //!< TODO
            uint8_t _NA4;           //!< TODO
        };
        uint32_t power;             //!< TODO
    };
};

//! UWB device configuration parameters.
struct uwb_dev_config{
    uint8_t channel;                        //!< channel number {1, 2, 3, 4, 5, 7, 9}
    uint8_t dataRate;                       //!< Data Rate {DWT_BR_110K, DWT_BR_850K or DWT_BR_6M8}
    uint8_t prf;                            //!< Pulse Repetition Frequency {DWT_PRF_16M or DWT_PRF_64M}
    struct uwb_dev_rx_config rx;            //!< UWB receiver configuration parameters.
    struct uwb_dev_tx_config tx;            //!< UWB transmitter configuration parameters.
    struct uwb_dev_txrf_config txrf;        //!< UWB transmitter power configuration parameters.
    uint32_t spicrc_r_enable:1;             //!< Enables crc8 on spi-read transfers
    uint32_t spicrc_w_enable:1;             //!< Enables crc8 on spi-write transfers
    uint32_t autoack_enabled:1;             //!< Enables automatic acknowledgement
    uint32_t autoack_delay_enabled:1;       //!< Enables automatic acknowledgement feature with delay
    uint32_t dblbuffon_enabled:1;           //!< Enables double buffer
    uint32_t trxoff_enable:1;               //!< Enables forced TRXOFF in start_tx and start_tx interface
    uint32_t rxdiag_enable:1;               //!< Enables receive diagnostics parameters
    uint32_t rxttcko_enable:1;              //!< Enables reading of time tracking integrator (used in dblbuffer only as carrier integrator isn't available)
    uint32_t rxauto_enable:1;               //!< Enables auto receive parameter
    uint32_t bias_correction_enable:1;      //!< Enables bias correction ploynomial
    uint32_t LDE_enable:1;                  //!< Enables LDE
    uint32_t LDO_enable:1;                  //!< Enables LDO
    uint32_t wakeup_rx_enable:1;            //!< Enables wakeup_rx_enable
    uint32_t sleep_enable:1;                //!< Enables sleep_enable bit
    uint32_t cir_enable:1;                  //!< Enables reading CIR as default
    uint32_t cir_pdoa_slave:1;              //!< This instance is acting as a pdoa slave
    uint32_t blocking_spi_transfers:1;      //!< Disables non-blocking reads and writes
};

//! TX fctrl extended parameters
struct uwb_fctrl_ext {
    uint8_t dataRate;
    uint8_t preambleLength;
    uint8_t ranging_en_bit;
};

/**
 * Configure the mac layer
 * @param inst     Pointer to struct uwb_dev.
 * @param config   Pointer to struct uwb_dev_config.
 * @return struct uwb_dev_status
 *
 */
typedef struct uwb_dev_status (*uwb_mac_config_func_t)(struct uwb_dev * dev, struct uwb_dev_config * config);

/**
 * Configuration of the uwb transmitter
 * including the power and pulse generator delay. The input is a pointer to the data structure
 * of type struct uwb_dev_txrf_config that holds all the configurable items.
 *
 * @param inst      Pointer to struct uwb_dev
 * @param config    Pointer to struct uwb_dev_txrf_config.
 * @return void
 */
typedef void (*uwb_txrf_config_func_t)(struct uwb_dev * dev, struct uwb_dev_txrf_config *config);

/**
 * Translate coarse and fine power levels to a registry value used in struct uwb_dev_txrf_config.
 *
 * @param inst      Pointer to struct uwb_dev
 * @param reg       Pointer to where to store the registry value
 * @param coarse    Coarse power control value in dBm (DA)
 * @param fine      Fine power value in dBm (Mixer)
 * @return true on success, false otherwise
 */
typedef bool (*uwb_txrf_power_value_func_t)(struct uwb_dev * dev, uint8_t *reg, dpl_float32_t coarse, dpl_float32_t fine);

/**
 *  Configure the device for both DEEP_SLEEP and SLEEP modes, and on-wake mode
 *  i.e., before entering the sleep, the device can be programmed for TX or RX, then upon "waking up" the TX/RX settings
 *  will be preserved and the device can immediately perform the desired action TX/RX.
 *
 * NOTE: e.g. Tag operation - after deep sleep, the device needs to just load the TX buffer and send the frame.
 *
 * @param inst  Pointer to struct uwb_dev.
 *
 * @return void
 */
typedef void (*uwb_sleep_config_func_t)(struct uwb_dev * dev);

/**
 * API to enter device into sleep mode.
 *
 * @param inst   Pointer to struct uwb_dev.
 * @return struct uwb_dev_status
 */
typedef struct uwb_dev_status (*uwb_enter_sleep_func_t)(struct uwb_dev * dev);

/**
 * Set the auto TX to sleep bit. This means that after a frame
 * transmission the device will enter deep sleep mode. The uwb_sleep_config() function
 * needs to be called before this to configure the on-wake settings.
 *
 * NOTE: the IRQ line has to be low/inactive (i.e. no pending events)
 *
 * @param inst       Pointer to struct uwb_dev.
 * @param enable     1 to configure the device to enter deep sleep after TX,
 *                   0 to disables the configuration.
 * @return struct uwb_dev_status
 */
typedef struct uwb_dev_status (*uwb_enter_sleep_after_tx_func_t)(struct uwb_dev * dev, uint8_t enable);

/**
 *  Sets the auto RX to sleep bit. This means that after a frame
 *  received the device will enter deep sleep mode. The dev_configure_sleep() function
 *  needs to be called before this to configure the on-wake settings.
 *
 * NOTE: the IRQ line has to be low/inactive (i.e. no pending events).
 * @param inst       Pointer to struct uwb_dev.
 * @param enable     1 to configure the device to enter deep sleep after TX,
 *                   0 to disables the configuration
 * @return struct uwb_dev_status
 */
typedef struct uwb_dev_status (*uwb_enter_sleep_after_rx_func_t)(struct uwb_dev * dev, uint8_t enable);

/**
 * Wakeup device from sleep to init.
 *
 * @param inst  struct uwb_dev.
 * @return struct uwb_dev_status
 */
typedef struct uwb_dev_status (*uwb_wakeup_func_t)(struct uwb_dev * dev);

/**
 * Enable/Disable the double receive buffer mode.
 *
 * @param inst    Pointer to struct uwb_dev.
 * @param enable  1 to enable, 0 to disable the double buffer mode.
 *
 * @return struct uwb_dev_status
 *
 */
typedef struct uwb_dev_status (*uwb_set_dblrxbuff_func_t)(struct uwb_dev * dev, bool enable);

/**
 * Set Receive Wait Timeout period.
 *
 * @param dev       pointer to struct uwb_dev.
 * @param timeout   Indicates how long the receiver remains on from the RX enable command.
 *                  The time parameter used here is in 1.0256 * us (512/499.2MHz) units.
 *                  If set to 0 the timeout is disabled.
 * @return struct uwb_dev_status
 *
 * @brief The Receive Frame Wait Timeout period is a 32-bit field. The units for this parameter are roughly 1μs,
 * (the exact unit is 512 counts of the fundamental 499.2 MHz UWB clock, or 1.026 μs). When employing the frame wait timeout,
 * RXFWTO should be set to a value greater than the expected RX frame duration and include an allowance for any uncertainly
 * attaching to the expected transmission start time of the awaited frame.
 * When using .rxauto_enable feature it is important to understand the role of rx_timeout, in this situation it is the timeout
 * that actually turns-off the receiver and returns the transeiver to the idle state.
 *
 * NOTE: On dw1000 the timeout is a 16bit field only.
 */
typedef struct uwb_dev_status (*uwb_set_rx_timeout_func_t)(struct uwb_dev *dev, uint32_t timeout);

/**
 * Adjust RX Wait Timeout period.
 *
 * @param inst      pointer to struct uwb_dev.
 * @param timeout   Indicates how long the receiver remains on from the RX enable command. The time parameter used here is in 1.0256
 * us (512/499.2MHz) units If set to 0 the timeout is disabled.
 * @return struct uwb_dev_status
 * @brief The Receive Frame Wait Timeout period is a 32-bit field. The units for this parameter are roughly 1μs,
 * (the exact unit is 512 counts of the fundamental 499.2 MHz UWB clock, or 1.026 μs). When employing the frame wait timeout,
 * RXFWTO should be set to a value greater than the expected RX frame duration and include an allowance for any uncertainly
 * attaching to the expected transmission start time of the awaited frame.
 * When using .rxauto_enable feature it is important to understand the role of rx_timeout, in this situation it is the timeout
 * that actually turns-off the receiver and returns the transeiver to the idle state.
 *
 * OBSERVE: This only works if the receiver has been setup to use a timeout with uwb_set_rx_timeout first.
 *
 * NOTE: On dw1000 the timeout is a 16bit field only.
 */
typedef struct uwb_dev_status (*uwb_adj_rx_timeout_func_t)(struct uwb_dev *dev, uint32_t timeout);

/**
 * To specify a time in future to either turn on the receiver to be ready to receive a frame,
 * or to turn on the transmitter and send a frame.
 * The low-order 9-bits of this register are ignored.
 * The dx_time is in UWB microseconds * 65535 = DTU.
 *
 * @param dev     Pointer to struct uwb_dev.
 * @param dx_time Delayed Send or receive Time, in dwt timeunits (uwb usecs << 16).
 * @return uwb_dev_status
 */
typedef struct uwb_dev_status (*uwb_set_delay_start_func_t)(struct uwb_dev *dev, uint64_t dx_time);

/**
 * Set Absolute rx start and end, in dtu
 *
 * @param dev           pointer to struct uwb_dev.
 * @param dx_time_start The future time at which the RX will start, in dwt timeunits (uwb usecs << 16).
 * @param dx_time_end   The future time at which the RX will end, in dwt timeunits.
 *
 * @return struct uwb_dev_status
 *
 * @brief Automatically adjusts the rx_timeout after each received frame to match dx_time_end
 */
typedef struct uwb_dev_status (*uwb_set_rx_window_func_t)(struct uwb_dev *dev, uint64_t dx_time_start, uint64_t dx_time_end);

/**
 * Set Absolute rx end, in dtu. NOTE: Doesn't set the initial
 *
 * @param dev      pointer to struct uwb_dev.
 * @param rx_end   The future time at which the RX will end, in dwt timeunits.
 *
 * @return struct uwb_dev_status
 *
 * @brief Automatically adjusts the rx_timeout after each received frame to match dx_time_end.
 */
typedef struct uwb_dev_status (*uwb_set_abs_timeout_func_t)(struct uwb_dev * dev, uint64_t rx_end);

/**
 * Start transmission.
 *
 * @param inst  pointer to struct uwb_dev.
 * @return struct uwb_dev_status
 */
typedef struct uwb_dev_status (*uwb_start_tx_func_t)(struct uwb_dev * dev);

/**
 * Activate reception mode (rx).
 *
 * @param inst  pointer to struct uwb_dev.
 * @return struct uwb_dev_status
 *
 */
typedef struct uwb_dev_status (*uwb_start_rx_func_t)(struct uwb_dev * dev);

/**
 * Gracefully turn off reception mode.
 *
 * @param inst  pointer to struct uwb_dev
 * @return struct uwb_dev_status
 *
 */
typedef struct uwb_dev_status (*uwb_stop_rx_func_t)(struct uwb_dev *dev);

/**
 * Write the supplied TX data into the tranceiver's
 * TX buffer.The input parameters are the data length in bytes and a pointer
 * to those data bytes.
 *
 * Note: This is the length of TX message (including the 2 byte CRC) - max is 1023 standard PHR mode allows up to 127 bytes
 * if > 127 is programmed, DWT_PHRMODE_EXT needs to be set in the phrMode configuration.
 *
 * @param dev                 Pointer to uwb_dev.
 * @param tx_frame_bytes      Pointer to the user buffer containing the data to send.
 * @param tx_buffer_offset    This specifies an offset in the tranceiver's TX Buffer where writing of data starts.
 * @param tx_frame_length     This is the length of TX message, excluding the two byte CRC when auto-FCS
 *                            transmission is enabled.
 *                            In standard PHR mode, maximum is 127 (125 with auto-FCS Transmission).
 *                            With DWT_PHRMODE_EXT set in the phrMode configuration, maximum is 1023 (1021 with
 *                            auto-FCS Transmission).
 * @return struct uwb_dev_status
 */
typedef struct uwb_dev_status (*uwb_write_tx_func_t)(struct uwb_dev* dev, uint8_t *tx_frame_bytes,
                                                uint16_t tx_buffer_offset, uint16_t tx_frame_length);

/**
 * Configure the TX frame control register before the transmission of a frame.
 *
 * @param dev               Pointer to struct uwb_dev.
 * @param tx_frame_length   This is the length of TX message, excluding the two byte CRC when auto-FCS
 *                          transmission is enabled.
 *                          In standard PHR mode, maximum is 127 (125 with auto-FCS Transmission).
 *                          With DWT_PHRMODE_EXT set in the phrMode configuration, maximum is 1023 (1021 with
 *                          auto-FCS Transmission).
 * @param tx_buffer_offset  The offset in the tx buffer to send data from.
 * @return void
 */
typedef void (*uwb_write_tx_fctrl_func_t)(struct uwb_dev* dev, uint16_t tx_frame_length, uint16_t tx_buffer_offset);

/**
 * Configure the TX frame control register before the transmission of a frame with extended options.
 *
 * @param dev               Pointer to struct uwb_dev.
 * @param tx_frame_length   This is the length of TX message (excluding the 2 byte CRC) - max is 1023
 *                          NOTE: standard PHR mode allows up to 127 bytes.
 * @param tx_buffer_offset  The offset in the tx buffer to send data from.
 * @param ext               Optional pointer to struct uwb_fctrl_ext with additional parameters
 * @return void
 */
typedef void (*uwb_write_tx_fctrl_ext_func_t)(struct uwb_dev* dev, uint16_t tx_frame_length,
                                              uint16_t tx_buffer_offset, struct uwb_fctrl_ext *ext);

/**
 * Wait for a DMA transfer
 *
 * @param inst     Pointer to struct uwb_dev.
 * @param timeout_ms Time in ms to wait, use DPL_TIMEOUT_NEVER (UINT32_MAX) to wait indefinitely
 * @return int     Returns 0 on success, error code otherwise
 */
typedef int (*uwb_hal_noblock_wait_func_t)(struct uwb_dev * dev, uint32_t timeout_ms);

/**
 * Wait for a tx to complete
 *
 * @param inst     Pointer to struct uwb_dev.
 * @param timeout  Time in ticks to wait, use DPL_TIMEOUT_NEVER (UINT32_MAX) to wait indefinitely
 * @return int     Returns 0 on success, error code otherwise
 */
typedef int (*uwb_tx_wait_func_t)(struct uwb_dev * dev, uint32_t timeout);

/**
 * Enable wait for response feature.
 *
 * @param dev               Pointer to struct uwb_dev.
 * @param enable            Enables/disables the wait for response feature.
 * @return struct uwb_dev_status
 */
typedef struct uwb_dev_status (*uwb_set_wait4resp_func_t)(struct uwb_dev *dev, bool enable);

/**
 * Wait-for-Response turn-around Time. This 20-bit field is used to configure the turn-around time between TX complete
 * and RX enable when the wait for response function is being used. This function is enabled by the WAIT4RESP control in
 * Register file: 0x0D – System Control Register. The time specified by this W4R_TIM parameter is in units of approximately 1 μs,
 * or 128 system clock cycles. This configuration may be used to save power by delaying the turn-on of the receiver,
 * to align with the response time of the remote system, rather than turning on the receiver immediately after transmission completes.
 *
 * @param inst   Pointer to struct uwb_dev.
 * @param delay  The delay is in UWB microseconds.
 *
 * @return struct uwb_dev_status
 *
 */
typedef struct uwb_dev_status (*uwb_set_wait4resp_delay_func_t)(struct uwb_dev * dev, uint32_t delay);

/**
 * Set rxauto disable
 *
 * @param dev      Pointer to struct uwb_dev.
 * @param disable  Disable mac-layer auto rx-reenable feature. The default behavior is rxauto enable, this API overrides default behavior.
 *                 on an individual transaction.
 *                 This flag is cleared automatically on receiving a good frame, rx timeout, or by uwb_phy_forcetrxoff.
 *
 */
typedef struct uwb_dev_status (*uwb_set_rxauto_disable_func_t)(struct uwb_dev * dev, bool disable);

/**
 * Read the current system time of the uwb tranceiver
 *
 * @param dev     Pointer to struct uwb_dev.
 * @return time   64bit uwt usecs
 */
typedef uint64_t (*uwb_read_systime_func_t)(struct uwb_dev* dev);

/**
 * Read lower 32bit of system time.
 *
 * @param dev      Pointer to struct uwb_dev.
 *
 * @return time
 */
typedef uint32_t (*uwb_read_systime_lo32_func_t)(struct uwb_dev* dev);

/**
 * Read receive time. (As adjusted by the LDE)
 *
 * @param inst  Pointer to struct uwb_dev.
 *
 * @return time
 */
typedef uint64_t (*uwb_read_rxtime_func_t)(struct uwb_dev* dev);

/**
 * Read lower 32bit of receive time.
 *
 * @param dev      Pointer to struct uwb_dev.
 *
 * @return time
 */
typedef uint32_t (*uwb_read_rxtime_lo32_func_t)(struct uwb_dev* dev);

/**
 * Read STS receive time. (As adjusted by the LDE)
 *
 * @param inst  Pointer to struct uwb_dev.
 *
 * @return time if valid, 0xFFFFFFFFFFFFFFFFULL otherwise
 */
typedef uint64_t (*uwb_read_sts_rxtime_func_t)(struct uwb_dev* dev);

/**
 * Read transmit time.
 *
 * @param dev      Pointer to struct uwb_dev.
 *
 * @return time
 */
typedef uint64_t (*uwb_read_txtime_func_t)(struct uwb_dev* dev);

/**
 * Read lower 32bit of transmit time.
 *
 * @param dev      Pointer to struct uwb_dev.
 *
 * @return time
 */
typedef uint32_t (*uwb_read_txtime_lo32_func_t)(struct uwb_dev* dev);

/**
 * Calculate the frame duration (airtime) in usecs (not uwb usecs).
 * @param attrib    Pointer to struct uwb_phy_attributes_t *. The phy attritubes are part of the IEEE802.15.4-2011 standard.
 * Note the morphology of the frame depends on the mode of operation, see the dw*000_hal.c for the default behaviour
 * @param nlen      The length of the frame to be transmitted/received excluding crc
 * @return uint16_t duration in usec (not uwb usec)
 */
typedef uint16_t (*uwb_phy_frame_duration_func_t)(struct uwb_dev* dev, uint16_t nlen);

/**
 * API to calculate the SHR (Preamble + SFD) duration. This is used to calculate the correct rx_timeout.
 * @param attrib    Pointer to struct uwb_phy_attributes *. The phy attritubes are part of the IEEE802.15.4-2011 standard.
 * Note the morphology of the frame depends on the mode of operation, see the dw*000_hal.c for the default behaviour
 * @param nlen      The length of the frame to be transmitted/received excluding crc
 * @return uint16_t duration in usec (not uwb usec)
 */
typedef uint16_t (*uwb_phy_SHR_duration_func_t)(struct uwb_dev* dev);

/**
 * API to calculate the data duration. This is used to calculate the correct rx_timeout.
 * @param attrib    Pointer to struct uwb_phy_attributes *. The phy attritubes are part of the IEEE802.15.4-2011 standard.
 * Note the morphology of the frame depends on the mode of operation, see the dw*000_hal.c for the default behaviour
 * @param nlen      The length of the frame to be transmitted/received excluding crc
 * @return uint16_t duration in usec (not uwb usec)
 */
typedef uint16_t (*uwb_phy_data_duration_func_t)(struct uwb_dev* dev, uint16_t nlen);

/**
 * Turn off the transceiver.
 *
 * @param inst  Pointer to struct uwb_dev.
 * @return void
 */
typedef void (*uwb_phy_forcetrxoff_func_t)(struct uwb_dev* dev);

/**
 * Reset the UWB receiver.
 *
 * @param inst   Pointer to struct uwb_dev.
 * @return void
 */
typedef void (*uwb_phy_rx_reset_func_t)(struct uwb_dev * dev);

/**
 * Repeated tx mode
 *
 * @param inst   Pointer to struct uwb_dev.
 * @param rate   Rate at which frames with be repeated in dtu, set to 0 to disable
 * @return void
 */
typedef void (*uwb_phy_repeated_frames_func_t)(struct uwb_dev * dev, uint64_t rate);

/**
 * Enable rx regardless of hpdwarning
 *
 * @param inst    Pointer to struct uwb_dev.
 * @param enable  weather to continue with rx regardless of error
 *
 */
typedef struct uwb_dev_status (*uwb_set_on_error_continue_func_t)(struct uwb_dev * dev, bool enable);

/**
 * Update PAN ID in device
 *
 * @param inst    Pointer to struct uwb_dev.
 * @param uint16_t pan id
 *
 */
typedef void (*uwb_set_panid_func_t)(struct uwb_dev * dev, uint16_t pan_id);

/**
 * Update UID in device
 *
 * @param inst    Pointer to struct uwb_dev.
 * @param uint16_t uid
 *
 */
typedef void (*uwb_set_uid_func_t)(struct uwb_dev * dev, uint16_t uid);

/**
 * Update EUID in device
 *
 * @param inst    Pointer to struct uwb_dev.
 * @param uint64_t euid
 *
 */
typedef void (*uwb_set_euid_func_t)(struct uwb_dev * dev, uint64_t euid);

/**
 * Calculate the clock offset ratio from the carrior integrator value
 *
 * @param inst    Pointer to struct uwb_dev.
 * @param val     Carrier integrator value / TTCO value
 * @param type    uwb_cr_types_t
 *
 * @return dpl_float64  the relative clock offset ratio (nan or 0.0 for failed?)
 */
typedef dpl_float64_t (*uwb_calc_clock_offset_ratio_func_t)(struct uwb_dev * inst, int32_t integrator_val, uwb_cr_types_t type);

/**
 * Calculate rssi from last RX in dBm.
 * Note that config.rxdiag_enable needs to be set.
 *
 * @param inst  Pointer to struct uwb_dev.
 *
 * @return rssi (dBm) on success
 */
typedef dpl_float32_t (*uwb_get_rssi_func_t)(struct uwb_dev * dev);

/**
 * Calculate First Path Power Level from last RX in dBm,
 * Note that config.rxdiag_enable needs to be set.
 *
 * @param inst  Pointer to struct uwb_dev.
 *
 * @return fppl (dBm) on success
 */
typedef dpl_float32_t (*uwb_get_fppl_func_t)(struct uwb_dev * inst);

/**
 * Calculate rssi from an rxdiag structure
 *
 * @param inst  Pointer to struct uwb_dev.
 * @param diag  Pointer to struct uwb_dev_rxdiag.
 *
 * @return rssi (dBm) on success
 */
typedef dpl_float32_t (*uwb_calc_rssi_func_t)(struct uwb_dev * dev, struct uwb_dev_rxdiag * diag);

/**
 * Calculate rssi from specific sequence from last RX in dBm.
 * Note that config.rxdiag_enable needs to be set.
 *
 * @param inst  Pointer to struct uwb_dev.
 * @param diag  Pointer to struct uwb_dev_rxdiag.
 * @param type  One of UWB_RXDIAG_IPATOV, UWB_RXDIAG_STS, UWB_RXDIAG_STS2
 *
 * @return rssi (dBm) on success
 */
typedef dpl_float32_t (*uwb_calc_seq_rssi_func_t)(struct uwb_dev * dev, struct uwb_dev_rxdiag * diag, uint16_t type);

/**
 * Calculate First Path Power Level (fppl) from an rxdiag structure
 *
 * @param inst  Pointer to struct uwb_dev.
 * @param diag  Pointer to struct uwb_dev_rxdiag.
 *
 * @return fppl (dBm) on success
 */
typedef dpl_float32_t (*uwb_calc_fppl_func_t)(struct uwb_dev * dev, struct uwb_dev_rxdiag * diag);

/**
 * Give a rough estimate of how likely the received packet is
 * line of sight (LOS).
 *
 * @param rssi rssi as calculated by uwb_calc_rssi
 * @param fppl fppl as calculated by uwb_calc_fppl
 *
 * @return 1.0 for likely LOS, 0.0 for non-LOS, with a sliding scale in between.
 */
typedef dpl_float32_t (*uwb_estimate_los_func_t)(struct uwb_dev * dev, dpl_float32_t rssi, dpl_float32_t fppl);

/**
 * Calculate pdoa from an rxdiag structure
 *
 * @param inst  Pointer to struct uwb_dev.
 * @param diag  Pointer to struct uwb_dev_rxdiag.
 *
 * @return pdoa (radians) on success
 */
typedef dpl_float32_t (*uwb_calc_pdoa_func_t)(struct uwb_dev * dev, struct uwb_dev_rxdiag * diag);

/**
 * Synchronise clock to external clock if present
 *
 * @param inst  Pointer to struct uwb_dev.
 *
 * @return struct uwb_dev_status
 */
typedef struct uwb_dev_status (*uwb_sync_to_ext_clock_func_t)(struct uwb_dev * dev);

/**
 * Enable frame filtering
 *
 * @param inst  Pointer to struct uwb_dev.
 * @param bitmask  Enables/disables the frame filtering options according to
 *      UWB_FF_NOTYPE_EN        no frame types allowed (Frame filter disabled)
 *      UWB_FF_BEACON_EN        beacon frames allowed
 *      UWB_FF_DATA_EN          data frames allowed
 *      UWB_FF_ACK_EN           ack frames allowed
 *      UWB_FF_MAC_EN           mac control frames allowed
 *      UWB_FF_RSVD_EN          reserved frame types allowed
 *      UWB_FF_MULTI_EN         multipurpose frames allowed
 *      UWB_FF_FRAG_EN          fragmented frame types allowed
 *      UWB_FF_EXTEND_EN        extended frame types allowed
 *      UWB_FF_COORD_EN         behave as coordinator (can receive frames with no destination address (PAN ID has to match))
 *      UWB_FF_IMPBRCAST_EN     allow MAC implicit broadcast
 * @return struct uwb_dev_status
 */
typedef struct uwb_dev_status (*uwb_mac_framefilter_func_t)(struct uwb_dev * dev, uint16_t enable);

/**
 * Enable the auto-ACK feature.
 * NOTE: needs to have frame filtering enabled as well.
 *
 * @param inst    Pointer to struct uwb_dev.
 * @param enable  If non-zero the ACK is sent after this delay, max is 255.
 * @return struct uwb_dev_status
 */
typedef struct uwb_dev_status (*uwb_set_autoack_func_t)(struct uwb_dev * dev, bool enable);

/**
 * Set the auto-ACK delay. If the delay (parameter) is 0, the ACK will
 * be sent as-soon-as-possable
 * otherwise it will be sent with a programmed delay (in symbols), max is 255.
 * NOTE: needs to have frame filtering enabled as well.
 *
 * @param inst    Pointer to struct uwb_dev.
 * @param delay   If non-zero the ACK is sent after this delay, max is 255.
 * @return struct uwb_dev_status
 */
typedef struct uwb_dev_status (*uwb_set_autoack_delay_func_t)(struct uwb_dev * dev, uint8_t delay);

/**
 * Enable and/or reset device event counters
 *
 * @param inst    Pointer to struct uwb_dev.
 * @param enable  Enables the device internal event counters
 * @param reset   If true, reset counters
 * @return struct uwb_dev_status
 */
typedef struct uwb_dev_status (*uwb_event_cnt_ctrl_func_t)(struct uwb_dev * dev, bool enable, bool reset);

/**
 * Read device event counters
 *
 * @param inst    Pointer to struct uwb_dev.
 * @param res     Pointer to struct uwb_dev_evcnt
 * @return struct uwb_dev_status
 */
typedef struct uwb_dev_status (*uwb_event_cnt_read_func_t)(struct uwb_dev * dev, struct uwb_dev_evcnt *res);

struct uwb_driver_funcs {
    uwb_mac_config_func_t uf_mac_config;
    uwb_txrf_config_func_t uf_txrf_config;
    uwb_txrf_power_value_func_t uf_txrf_power_value;
    uwb_sleep_config_func_t uf_sleep_config;
    uwb_enter_sleep_func_t uf_enter_sleep;
    uwb_enter_sleep_after_tx_func_t uf_enter_sleep_after_tx;
    uwb_enter_sleep_after_rx_func_t uf_enter_sleep_after_rx;
    uwb_wakeup_func_t uf_wakeup;
    uwb_set_dblrxbuff_func_t uf_set_dblrxbuf;
    uwb_set_rx_timeout_func_t uf_set_rx_timeout;
    uwb_adj_rx_timeout_func_t uf_adj_rx_timeout;
    uwb_set_rx_window_func_t uf_set_rx_window;
    uwb_set_abs_timeout_func_t uf_set_abs_timeout;
    uwb_set_delay_start_func_t uf_set_delay_start;
    uwb_start_tx_func_t uf_start_tx;
    uwb_start_rx_func_t uf_start_rx;
    uwb_stop_rx_func_t uf_stop_rx;
    uwb_write_tx_func_t uf_write_tx;
    /* uwb_write_tx_fctrl_func_t uf_write_tx_fctrl; Replaced by below */
    uwb_write_tx_fctrl_ext_func_t uf_write_tx_fctrl_ext;
    uwb_hal_noblock_wait_func_t uf_hal_noblock_wait;
    uwb_tx_wait_func_t uf_tx_wait;
    uwb_set_wait4resp_func_t uf_set_wait4resp;
    uwb_set_wait4resp_delay_func_t uf_set_wait4resp_delay;
    uwb_set_rxauto_disable_func_t uf_set_rxauto_disable;
    uwb_read_systime_func_t uf_read_systime;
    uwb_read_systime_lo32_func_t uf_read_systime_lo32;
    uwb_read_rxtime_func_t uf_read_rxtime;
    uwb_read_rxtime_lo32_func_t uf_read_rxtime_lo32;
    uwb_read_sts_rxtime_func_t uf_read_sts_rxtime;
    uwb_read_txtime_func_t uf_read_txtime;
    uwb_read_txtime_lo32_func_t uf_read_txtime_lo32;
    uwb_phy_frame_duration_func_t uf_phy_frame_duration;
    uwb_phy_SHR_duration_func_t uf_phy_SHR_duration;
    uwb_phy_data_duration_func_t uf_phy_data_duration;
    uwb_phy_forcetrxoff_func_t uf_phy_forcetrxoff;
    uwb_phy_rx_reset_func_t uf_phy_rx_reset;
    uwb_phy_repeated_frames_func_t uf_phy_repeated_frames;
    uwb_set_on_error_continue_func_t uf_set_on_error_continue;
    uwb_set_panid_func_t uf_set_panid;
    uwb_set_uid_func_t uf_set_uid;
    uwb_set_euid_func_t uf_set_euid;
    uwb_calc_clock_offset_ratio_func_t uf_calc_clock_offset_ratio;
    uwb_get_rssi_func_t uf_get_rssi;
    uwb_get_fppl_func_t uf_get_fppl;
    uwb_calc_rssi_func_t uf_calc_rssi;
    uwb_calc_seq_rssi_func_t uf_calc_seq_rssi;
    uwb_calc_fppl_func_t uf_calc_fppl;
    uwb_estimate_los_func_t uf_estimate_los;
    uwb_calc_pdoa_func_t uf_calc_pdoa;
    uwb_sync_to_ext_clock_func_t uf_sync_to_ext_clock;
    uwb_mac_framefilter_func_t uf_mac_framefilter;
    uwb_set_autoack_func_t uf_set_autoack;
    uwb_set_autoack_delay_func_t uf_set_autoack_delay;
    uwb_event_cnt_ctrl_func_t uf_event_cnt_ctrl;
    uwb_event_cnt_read_func_t uf_event_cnt_read;
};

struct uwb_driver_dyn_funcs {
    uwb_sync_to_ext_clock_func_t uf_sync_to_ext_clock;
};

struct uwb_dev {
    struct os_dev uw_dev;
    const struct uwb_driver_funcs *uw_funcs;
    struct uwb_driver_dyn_funcs uw_dyn_funcs;

    /* Interrupt handling */
    uint8_t task_prio;                          //!< Priority of the interrupt task
    uint32_t irq_at_ticks;                      //!< When the interrupt occured in cpu os ticks
    struct dpl_sem irq_sem;                     //!< Semaphore for Interrupt service routine
    struct dpl_eventq eventq;                   //!< Structure of dpl_eventq that has event queue
    struct dpl_event interrupt_ev;              //!< Structure of dpl_event that trigger interrupts
    struct dpl_task task_str;                   //!< Structure of dpl_task that has interrupt task
#ifndef __KERNEL__
    dpl_stack_t task_stack[MYNEWT_VAL(UWB_DEV_TASK_STACK_SZ)] //!< Stack of the interrupt task
        __attribute__((aligned(DPL_STACK_ALIGNMENT)));
#endif
    /* Device parameters */
    uint8_t  idx;                               //!< instance number number {0, 1, 2 etc}
    struct uwb_dev_capabilities capabilities;   //!< Bitfield of capabilities
    uint16_t role;                              //!< Roles for this device (bitfield)
    union {
        uint16_t my_short_address;              //!< Short address of tag/node
        uint16_t uid;
    };
    union {
        uint64_t my_long_address;               //!< Long address of tag/node
        uint64_t euid;                          //!< Extended Unique Identifier
    };
    uint16_t pan_id;                            //!< Private network interface id
    uint16_t slot_id;                           //!< Slot id
    uint16_t cell_id;                           //!< Cell id
    uint32_t device_id;                         //!< Device id
    uint16_t rx_antenna_delay;                  //!< Receive antenna delay
    uint16_t tx_antenna_delay;                  //!< Transmit antenna delay
    dpl_float32_t rx_ant_separation;            //!< Antenna distance, NOTE: Will be replaced by antenna coordinates
    int32_t  ext_clock_delay;                   //!< External clock delay
    uint64_t abs_timeout;                       //!< Absolute timeout, dtu

    /* RX data from latest frame received */
    union {
        uint16_t fctrl;                         //!< Reported frame control
        uint8_t fctrl_array[sizeof(uint16_t)];  //!< Endianness safe interface
    };
    uint16_t frame_len;                         //!< Reported frame length
    uint64_t rxtimestamp;                       //!< Receive timestamp
    int32_t carrier_integrator;                 //!< Carrier integrator (should prob not be here)
    int32_t rxttcko;                            //!< Preamble Integrator
    struct uwb_dev_rxdiag *rxdiag;              //!< Pointer to rx diagnostics structure
    uint8_t *rxbuf;                             //!< Local receive buffer
    uint8_t *txbuf;                             //!< Local transmit buffer, needs aligned allocation
    uint16_t rxbuf_size;                        //!< Size of local receive buffer
    uint16_t txbuf_size;                        //!< Size of local transmit buffer

    struct uwb_dev_status status;               //!< Device status
    struct uwb_dev_config config;               //!< Device configuration
    SLIST_HEAD(, uwb_mac_interface) interface_cbs;
    struct uwb_phy_attributes attrib;
#if MYNEWT_VAL(CIR_ENABLED)
    struct cir_instance *cir;                   //!< CIR instance
#endif
};

/* When building for non-kernel we can inline the api translation.
 * The kernel has instead these functions explicitly implemented in uwb.c */
#ifndef __KERNEL__
    #define UWB_API_IMPL_PREFIX static inline
    #define UWB_API_IMPL(__RET, __F, __P, __I) static inline __RET __F(__P) __I
    #define EXPORT_SYMBOL(__S)
    #include "uwb_api_impl.h"
#else
    struct uwb_dev_status uwb_mac_config(struct uwb_dev * dev, struct uwb_dev_config * config);
    void uwb_txrf_config(struct uwb_dev * dev, struct uwb_dev_txrf_config *config);
    bool uwb_txrf_power_value(struct uwb_dev * dev, uint8_t *reg, dpl_float32_t coarse, dpl_float32_t fine);
    void uwb_sleep_config(struct uwb_dev * dev);
    struct uwb_dev_status uwb_enter_sleep(struct uwb_dev * dev);
    struct uwb_dev_status uwb_enter_sleep_after_tx(struct uwb_dev * dev, uint8_t enable);
    struct uwb_dev_status uwb_enter_sleep_after_rx(struct uwb_dev * dev, uint8_t enable);
    struct uwb_dev_status uwb_wakeup(struct uwb_dev * dev);
    struct uwb_dev_status uwb_set_dblrxbuff(struct uwb_dev * dev, bool enable);
    struct uwb_dev_status uwb_set_rx_timeout(struct uwb_dev *dev, uint32_t to);
    struct uwb_dev_status uwb_adj_rx_timeout(struct uwb_dev *dev, uint32_t to);
    struct uwb_dev_status uwb_set_rx_window(struct uwb_dev *dev, uint64_t rx_start, uint64_t rx_end);
    struct uwb_dev_status uwb_set_abs_timeout(struct uwb_dev *dev, uint64_t rx_end);
    struct uwb_dev_status uwb_set_delay_start(struct uwb_dev *dev, uint64_t dx_time);
    struct uwb_dev_status uwb_start_tx(struct uwb_dev * dev);
    struct uwb_dev_status uwb_start_rx(struct uwb_dev * dev);
    struct uwb_dev_status uwb_stop_rx(struct uwb_dev *dev);
    struct uwb_dev_status uwb_write_tx(struct uwb_dev* dev, uint8_t *tx_frame_bytes, uint16_t tx_buffer_offset, uint16_t tx_frame_length);
    void uwb_write_tx_fctrl(struct uwb_dev* dev, uint16_t tx_frame_length, uint16_t tx_buffer_offset);
    void uwb_write_tx_fctrl_ext(struct uwb_dev* dev, uint16_t tx_frame_length, uint16_t tx_buffer_offset, struct uwb_fctrl_ext *ext);
    int uwb_hal_noblock_wait(struct uwb_dev * dev, uint32_t timeout);
    int uwb_tx_wait(struct uwb_dev * dev, uint32_t timeout);
    struct uwb_dev_status uwb_set_wait4resp(struct uwb_dev *dev, bool enable);
    struct uwb_dev_status uwb_set_wait4resp_delay(struct uwb_dev * dev, uint32_t delay);
    struct uwb_dev_status uwb_set_rxauto_disable(struct uwb_dev * dev, bool disable);
    uint64_t uwb_read_systime(struct uwb_dev* dev);
    uint32_t uwb_read_systime_lo32(struct uwb_dev* dev);
    uint64_t uwb_read_rxtime(struct uwb_dev* dev);
    uint32_t uwb_read_rxtime_lo32(struct uwb_dev* dev);
    uint64_t uwb_read_sts_rxtime(struct uwb_dev* dev);
    uint64_t uwb_read_txtime(struct uwb_dev* dev);
    uint32_t uwb_read_txtime_lo32(struct uwb_dev* dev);
    uint16_t uwb_phy_frame_duration(struct uwb_dev* dev, uint16_t nlen);
    uint16_t uwb_phy_SHR_duration(struct uwb_dev* dev);
    uint16_t uwb_phy_data_duration(struct uwb_dev* dev, uint16_t nlen);
    void uwb_phy_forcetrxoff(struct uwb_dev* dev);
    void uwb_phy_rx_reset(struct uwb_dev * dev);
    void uwb_phy_repeated_frames(struct uwb_dev * dev, uint64_t rate);
    struct uwb_dev_status uwb_set_on_error_continue(struct uwb_dev * dev, bool enable);
    void uwb_set_panid(struct uwb_dev * dev, uint16_t pan_id);
    void uwb_set_uid(struct uwb_dev * dev, uint16_t uid);
    void uwb_set_euid(struct uwb_dev * dev, uint64_t euid);
    dpl_float64_t uwb_calc_clock_offset_ratio(struct uwb_dev * dev, int32_t integrator_val, uwb_cr_types_t type);
    dpl_float32_t uwb_get_rssi(struct uwb_dev * dev);
    dpl_float32_t uwb_get_fppl(struct uwb_dev * dev);
    dpl_float32_t uwb_calc_rssi(struct uwb_dev * dev, struct uwb_dev_rxdiag * diag);
    dpl_float32_t uwb_calc_seq_rssi(struct uwb_dev * dev, struct uwb_dev_rxdiag * diag, uint16_t type);
    dpl_float32_t uwb_calc_fppl(struct uwb_dev * dev, struct uwb_dev_rxdiag * diag);
    dpl_float32_t uwb_estimate_los(struct uwb_dev * dev, dpl_float32_t rssi, dpl_float32_t fppl);
    dpl_float32_t uwb_calc_pdoa(struct uwb_dev * dev, struct uwb_dev_rxdiag * diag);
    struct uwb_dev_status uwb_sync_to_ext_clock(struct uwb_dev * dev);
    struct uwb_dev_status uwb_mac_framefilter(struct uwb_dev * dev, uint16_t enable);
    struct uwb_dev_status uwb_set_autoack(struct uwb_dev * dev, bool enable);
    struct uwb_dev_status uwb_set_autoack_delay(struct uwb_dev * dev, uint8_t delay);
    struct uwb_dev_status uwb_event_cnt_ctrl(struct uwb_dev * dev, bool enable, bool reset);
    struct uwb_dev_status uwb_event_cnt_read(struct uwb_dev * dev, struct uwb_dev_evcnt *res);
    s32 uwb_float32_to_s32(dpl_float32_t);
    s32 uwb_float32_to_s32x1000(dpl_float32_t);
#endif

#define UWB_DWT_USECS_TO_DTU(_X) ((uint64_t)_X << 16)
#define UWB_DTU_TO_DWT_USECS(_X) ((uint64_t)_X >> 16)
#define UWB_DTU_40BMASK (0x00FFFFFFFFFFULL)

#ifndef __KERNEL__
#define uwb_dwt_usecs_to_usecs(_t) (double)( (_t) * (0x10000UL/(128*499.2)))
#define uwb_usecs_to_dwt_usecs(_t) (double)( (_t) / uwb_dwt_usecs_to_usecs(1.0))
#else
#define uwb_dwt_usecs_to_usecs(_t) ( div64_s64((s64)(_t) * 0x10000UL, (s64)(128*499.2)) )
#define uwb_usecs_to_dwt_usecs(_t) ( div64_s64((s64)(_t), uwb_dwt_usecs_to_usecs(1)) )
#endif

#ifndef SPEED_OF_LIGHT
#define SPEED_OF_LIGHT (299792458.0l)
#endif

/*!
 * @fn uwb_calc_aoa(dpl_float32_t pdoa, dpl_float32_t wavelength, dpl_float32_t antenna_separation)
 *
 * @brief Calculate the angle of arrival
 *
 * @param pdoa       - phase difference of arrival in radians
 * @param channel    - uwb-channel used
 * @param antenna_separation - distance between centers of antennas in meters
 *
 * output parameters
 *
 * returns angle of arrival - dpl_float32_t, in radians
 */
dpl_float32_t uwb_calc_aoa(dpl_float32_t pdoa, int channel, dpl_float32_t antenna_separation);

/**
 * Set Absolute rx end post tx, in dtu
 *
 * @param dev      pointer to struct uwb_dev.
 * @param tx_ts    The future timestamp of the tx will start, in dwt timeunits (uwb usecs << 16).
 * @param tx_post_rmarker_len Length of package after rmarker.
 * @param rx_end   The future time at which the RX will end, in dwt timeunits.
 *
 * @return struct uwb_dev_status
 *
 * @brief Automatically adjusts the rx_timeout after each received frame to match dx_time_end
 * in a rx window after a tx-frame. Note that the rx-timeout needs to be calculated as the
 * length in time after the completion of the transmitted frame, not after the rmarker (set timestamp).
 */
struct uwb_dev_status uwb_set_rx_post_tx_window(struct uwb_dev * dev, uint64_t tx_ts,
                                                uint32_t tx_post_rmarker_len, uint64_t rx_end);

struct uwb_mac_interface *uwb_mac_append_interface(struct uwb_dev* dev, struct uwb_mac_interface * cbs);
void uwb_mac_remove_interface(struct uwb_dev* dev, uwb_extension_id_t id);
void* uwb_mac_find_cb_inst_ptr(struct uwb_dev *dev, uint16_t id);
struct uwb_mac_interface *uwb_mac_get_interface(struct uwb_dev* dev, uwb_extension_id_t id);

struct uwb_dev* uwb_dev_idx_lookup(int idx);

void uwb_task_init(struct uwb_dev * inst, void (*irq_ev_cb)(struct dpl_event*));
void uwb_task_deinit(struct uwb_dev * inst);
void uwb_dev_init(struct uwb_dev * inst);
void uwb_dev_deinit(struct uwb_dev * inst);


#ifdef __cplusplus
}
#endif

#endif /* __UWB_H__ */
