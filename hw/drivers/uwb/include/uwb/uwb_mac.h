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

#ifndef _UWB_MAC_H_
#define _UWB_MAC_H_

#ifdef __cplusplus
extern "C" {
#endif

#define UWB_BROADCAST_ADDRESS  0xffff  //!< UWB Broadcast addresss

//! constants for specifying TX Preamble length in symbols.
//! These are defined to allow them be directly written into byte 2 of the TX_FCTRL register.
//! (i.e. a four bit value destined for bits 20..18 but shifted left by 2 for byte alignment)
#define DWT_PLEN_4096   0x0C    //! Standard preamble length 4096 symbols
#define DWT_PLEN_2048   0x28    //! Non-standard preamble length 2048 symbols
#define DWT_PLEN_1536   0x18    //! Non-standard preamble length 1536 symbols
#define DWT_PLEN_1024   0x08    //! Standard preamble length 1024 symbols
#define DWT_PLEN_512    0x34    //! Non-standard preamble length 512 symbols
#define DWT_PLEN_256    0x24    //! Non-standard preamble length 256 symbols
#define DWT_PLEN_128    0x14    //! Non-standard preamble length 128 symbols
#define DWT_PLEN_64     0x04    //! Standard preamble length 64 symbols
#define DWT_PLEN_32     0x00    //! When setting length 32 symbols this is 0x0,  which is programmed to byte 2 of the TX_FCTRL register
#define DWT_PLEN_72     0x01    //! Non-standard length 72

//! constants for specifying the (Nominal) mean Pulse Repetition Frequency.
//! These are defined for direct write (with a shift if necessary) to CHAN_CTRL and TX_FCTRL regs.
#define DWT_PRF_16M     1   //!< UWB PRF 16 MHz
#define DWT_PRF_64M     2   //!< UWB PRF 64 MHz
#define DWT_PRF_SCP     3   //!< SCP UWB PRF ~100 MHz

#define DWT_SFDTOC_DEF  129     //!< Default SFD timeout value
#define DWT_PHRMODE_STD 0x0     //!< standard PHR mode
#define DWT_PHRMODE_EXT 0x3     //!< DW proprietary extended frames PHR mode

#define DWT_PHRRATE_STD 0x0     // standard PHR rate
#define DWT_PHRRATE_DTA 0x1     // PHR at data rate (6M81)

// Define STS modes
#define DWT_STS_MODE_OFF         0x0     //!< STS preamble is off
#define DWT_STS_MODE_1           0x1     //!< STS preamble mode 1 (CP immediately after SFD)
#define DWT_STS_MODE_2           0x2     //!< STS preamble mode 2 (CP follows data)
#define DWT_STS_MODE_ND          0x3     //!< STS with no data
#define DWT_STS_MODE_SDC         0x8     //!< Enable Super Deterministic Codes (Not encrypted!)
#define DWT_STS_CONFIG_MASK      0xB

//! Multiplication factors to convert carrier integrator value to a frequency offset in Hz
#define DWT_FREQ_OFFSET_MULTIPLIER          (998.4e6/2.0/1024.0/131072.0)
#define DWT_FREQ_OFFSET_MULTIPLIER_110KB    (998.4e6/2.0/8192.0/131072.0)

//! UWB Channel center frequency
#define UWB_CH_FREQ_CHAN_1   (3494.4e6f)
#define UWB_CH_FREQ_CHAN_2   (3993.6e6f)
#define UWB_CH_FREQ_CHAN_3   (4492.8e6f)
#define UWB_CH_FREQ_CHAN_4   (4492.8e6f)
#define UWB_CH_FREQ_CHAN_5   (6489.6e6f)
#define UWB_CH_FREQ_CHAN_7   (6489.6e6f)
#define UWB_CH_FREQ_CHAN_9   (7987.2e6f)

//! Multiplication factors to convert frequency offset in Hertz to PPM crystal offset
#define DWT_HZ_TO_PPM_MULTIPLIER_CHAN_1     (1.0e6/UWB_CH_FREQ_CHAN_1)
#define DWT_HZ_TO_PPM_MULTIPLIER_CHAN_2     (1.0e6/UWB_CH_FREQ_CHAN_2)
#define DWT_HZ_TO_PPM_MULTIPLIER_CHAN_3     (1.0e6/UWB_CH_FREQ_CHAN_3)
#define DWT_HZ_TO_PPM_MULTIPLIER_CHAN_4     (1.0e6/UWB_CH_FREQ_CHAN_4)
#define DWT_HZ_TO_PPM_MULTIPLIER_CHAN_5     (1.0e6/UWB_CH_FREQ_CHAN_5)
#define DWT_HZ_TO_PPM_MULTIPLIER_CHAN_7     (1.0e6/UWB_CH_FREQ_CHAN_7)
#define DWT_HZ_TO_PPM_MULTIPLIER_CHAN_9     (1.0e6/UWB_CH_FREQ_CHAN_9)

//! Frame filtering configuration options (taken from dw3000, needs conversion for dw1000)
#define UWB_FF_NOTYPE_EN            0x000           //!< No frame types allowed (FF disabled)
#define UWB_FF_BEACON_EN            0x001           //!< beacon frames allowed
#define UWB_FF_DATA_EN              0x002           //!< data frames allowed
#define UWB_FF_ACK_EN               0x004           //!< ack frames allowed
#define UWB_FF_MAC_EN               0x008           //!< mac control frames allowed
#define UWB_FF_RSVD_EN              0x010           //!< reserved frame types allowed
#define UWB_FF_MULTI_EN             0x020           //!< multipurpose frames allowed
#define UWB_FF_FRAG_EN              0x040           //!< fragmented frame types allowed
#define UWB_FF_EXTEND_EN            0x080           //!< extended frame types allowed
#define UWB_FF_COORD_EN             0x100           //!< behave as coordinator (can receive frames with no dest address (PAN ID has to match))
#define UWB_FF_IMPBRCAST_EN         0x200           //!< allow MAC implicit broadcast

#define DBL_BUFF_OFF             0x0
#define DBL_BUFF_ACCESS_BUFFER_A 0x1
#define DBL_BUFF_ACCESS_BUFFER_B 0x3

#ifdef __cplusplus
}
#endif
#endif /* _UWB_MAC_H_ */
