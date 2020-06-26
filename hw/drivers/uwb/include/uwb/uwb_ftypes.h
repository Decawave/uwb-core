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

/**
 * @file uwb_ftypes.h
 * @author UWB Core <uwbcore@gmail.com>
 * @date 2018
 * @brief ftypes file
 *
 * @details This is the ftypes base class which include the main uwb frames types
 */

#ifndef _UWB_FTYPES_H_
#define _UWB_FTYPES_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UWB_FCTRL_FRAME_TYPE_BEACON  ((uint16_t)0x0000)
#define UWB_FCTRL_FRAME_TYPE_DATA    ((uint16_t)0x0001)
#define UWB_FCTRL_FRAME_TYPE_ACK     ((uint16_t)0x0002)
#define UWB_FCTRL_FRAME_TYPE_MAC     ((uint16_t)0x0003)

#define UWB_FCTRL_SECURITY_ENABLED   ((uint16_t)0x0008)
#define UWB_FCTRL_FRAME_PENDING      ((uint16_t)0x0010)
#define UWB_FCTRL_ACK_REQUESTED      ((uint16_t)0x0020)
#define UWB_FCTRL_PANID_COMPRESSION  ((uint16_t)0x0040)

#define UWB_FCTRL_DEST_ADDR_NONE     ((uint16_t)0x0000)
#define UWB_FCTRL_DEST_ADDR_16BIT    ((uint16_t)0x0800)
#define UWB_FCTRL_DEST_ADDR_64BIT    ((uint16_t)0x0C00)

#define UWB_FCTRL_FRAME_VER_IEEE     ((uint16_t)0x0000)
#define UWB_FCTRL_FRAME_VER_IEEE2003 ((uint16_t)0x1000)

#define UWB_FCTRL_SRC_ADDR_NONE      ((uint16_t)0x0000)
#define UWB_FCTRL_SRC_ADDR_16BIT     ((uint16_t)0x8000)
#define UWB_FCTRL_SRC_ADDR_64BIT     ((uint16_t)0xC000)

#define UWB_FCTRL_STD_DATA_FRAME     (UWB_FCTRL_FRAME_TYPE_DATA|UWB_FCTRL_PANID_COMPRESSION|UWB_FCTRL_DEST_ADDR_16BIT|UWB_FCTRL_SRC_ADDR_16BIT)

#define FCNTL_IEEE_BLINK_CCP_64 0xC5        //!< CCP blink frame control
#define FCNTL_IEEE_BLINK_TAG_64 0x56        //!< Tag blink frame control
#define FCNTL_IEEE_BLINK_ANC_64 0x57        //!< Anchor blink frame control
#define FCNTL_IEEE_RANGE_16     0x8841      //!< Range frame control
#define FCNTL_IEEE_PROVISION_16 0x8844      //!< Provision frame control

//! Code types for identifying Data Type frames
typedef enum _uwb_dataframe_code_t{
    UWB_DATA_CODE_NONE = 0,                    //!< Invalid CODE

    //! Ranging Codes
    UWB_DATA_CODE_TWR_INVALID = 0x0100,        //!< Invalid TWR CODE
    UWB_DATA_CODE_SS_TWR = 0x0110,             //!< Single sided TWR
    UWB_DATA_CODE_SS_TWR_T1,                   //!< Response for single sided TWR
    UWB_DATA_CODE_SS_TWR_FINAL,                //!< Final response of single sided TWR
    UWB_DATA_CODE_SS_TWR_END,                  //!< End of single sided TWR
    UWB_DATA_CODE_SS_TWR_EXT,                  //!< Single sided TWR in extended mode
    UWB_DATA_CODE_SS_TWR_EXT_T1,               //!< Response for single sided TWR in extended mode
    UWB_DATA_CODE_SS_TWR_EXT_FINAL,            //!< Final response of single sided TWR in extended mode
    UWB_DATA_CODE_SS_TWR_EXT_END,              //!< End of single sided TWR in extended mode
    UWB_DATA_CODE_SS_TWR_ACK,                  //!< Single sided TWR in ack mode
    UWB_DATA_CODE_SS_TWR_ACK_T1,               //!< Response for single sided TWR in ack mode
    UWB_DATA_CODE_SS_TWR_ACK_FINAL,            //!< Final response of single sided TWR in ack mode
    UWB_DATA_CODE_SS_TWR_ACK_END,              //!< End of single sided TWR in ack mode
    UWB_DATA_CODE_DS_TWR = 0x0120,             //!< Double sided TWR
    UWB_DATA_CODE_DS_TWR_T1,                   //!< Response for double sided TWR
    UWB_DATA_CODE_DS_TWR_T2,                   //!< Response for double sided TWR
    UWB_DATA_CODE_DS_TWR_FINAL,                //!< Final response of double sided TWR
    UWB_DATA_CODE_DS_TWR_END,                  //!< End of double sided TWR
    UWB_DATA_CODE_DS_TWR_EXT,                  //!< Double sided TWR in extended mode
    UWB_DATA_CODE_DS_TWR_EXT_T1,               //!< Response for double sided TWR in extended mode
    UWB_DATA_CODE_DS_TWR_EXT_T2,               //!< Response for double sided TWR in extended mode
    UWB_DATA_CODE_DS_TWR_EXT_FINAL,            //!< Final response of double sided TWR in extended mode
    UWB_DATA_CODE_DS_TWR_EXT_END,              //!< End of double sided TWR in extended mode

    UWB_DATA_CODE_SS_TWR_NRNG = 0x0130,        //!< Single sided N-range TWR
    UWB_DATA_CODE_SS_TWR_NRNG_T1,
    UWB_DATA_CODE_SS_TWR_NRNG_FINAL,
    UWB_DATA_CODE_SS_TWR_NRNG_END,
    UWB_DATA_CODE_SS_TWR_NRNG_EXT,
    UWB_DATA_CODE_SS_TWR_NRNG_EXT_T1,
    UWB_DATA_CODE_SS_TWR_NRNG_EXT_FINAL,
    UWB_DATA_CODE_SS_TWR_NRNG_EXT_END,
    UWB_DATA_CODE_DS_TWR_NRNG = 0x0140,        //!< Double sided N-range TWR
    UWB_DATA_CODE_DS_TWR_NRNG_T1,
    UWB_DATA_CODE_DS_TWR_NRNG_T2,
    UWB_DATA_CODE_DS_TWR_NRNG_FINAL,
    UWB_DATA_CODE_DS_TWR_NRNG_END,
    UWB_DATA_CODE_DS_TWR_NRNG_EXT,
    UWB_DATA_CODE_DS_TWR_NRNG_EXT_T1,
    UWB_DATA_CODE_DS_TWR_NRNG_EXT_T2,
    UWB_DATA_CODE_DS_TWR_NRNG_EXT_FINAL,
    UWB_DATA_CODE_DS_TWR_NRNG_EXT_END,
    UWB_DATA_CODE_DS_TWR_NRNG_INVALID,

    //! RTDoA Codes
    UWB_DATA_CODE_RTDOA_INVALID = 0x0310,
    UWB_DATA_CODE_RTDOA_REQUEST,
    UWB_DATA_CODE_RTDOA_RESP,

    //! Transport
    UWB_DATA_CODE_TRNSPRT_INVALID = 0x0410,
    UWB_DATA_CODE_TRNSPRT_REQUEST,
    UWB_DATA_CODE_TRNSPRT_RESPONSE,
    UWB_DATA_CODE_TRNSPRT_END = 0x041F,

    //! NMGR over UWB
    UWB_DATA_CODE_NMGR_INVALID = 0x0420,
    UWB_DATA_CODE_NMGR_REQUEST,
    UWB_DATA_CODE_NMGR_RESPONSE,
    UWB_DATA_CODE_NMGR_END = 0x042F,

    //! Other Codes
    UWB_DATA_CODE_SURVEY_REQUEST = 0x0430,
    UWB_DATA_CODE_SURVEY_BROADCAST,

    //! LWIP over UWB
    UWB_DATA_CODE_LWIP_INVALID = 0x0440,
    UWB_DATA_CODE_LWIP,
    UWB_DATA_CODE_LWIP_END = 0x044F,

    UWB_DATA_CODE_PROVISION_START = 0x04f0,    //!< Start of provision
    UWB_DATA_CODE_PROVISION_RESP,              //!< End of provision

    //! Desense tests
    UWB_DATA_CODE_DESENSE_REQUEST = 0x0500,
    UWB_DATA_CODE_DESENSE_START,
    UWB_DATA_CODE_DESENSE_TEST,
    UWB_DATA_CODE_DESENSE_END,
} uwb_dataframe_code_t;


//! IEEE 802.15.4e standard blink. It is a 12-byte frame composed of the following fields.
typedef union{
//! Structure of IEEE blink frame
    struct _ieee_blink_frame_t{
        uint8_t fctrl;              //!< Frame type (0xC5 for a blink) using 64-bit addressing
        uint8_t seq_num;            //!< Sequence number, incremented for each new frame
        union {
            uint64_t long_address;  //!< Device ID TODOs::depreciated nomenclature
            uint64_t euid;          //!< extended unique identifier
        };
    }__attribute__((__packed__,aligned(1)));
    uint8_t array[sizeof(struct _ieee_blink_frame_t)]; //!< Array of size blink frame
}ieee_blink_frame_t;

//! ISO/IEC 24730-62:2013 standard blink. It is a 14-byte frame composed of the following fields.
typedef union {
//! Structure of extended blink frame
    struct _ieee_blink_frame_ext_t{
        uint8_t fctrl;              //!< Frame type (0xC5 for a blink) using 64-bit addressing
        uint8_t seq_num;            //!< Sequence number, incremented for each new frame
        union {
            uint64_t address;       //!< Device ID TODOs::depreciated nomenclature
            uint64_t euid;          //!< extended unique identifier
        };
        uint8_t encoding;           //!< 0x43 to indicate no extended ID
        uint8_t EXT_header ;        //!< 0x02 to indicate tag is listening for a response immediately
    }__attribute__((__packed__,aligned(1)));
    uint8_t array[sizeof(struct _ieee_blink_frame_ext_t)];  //!< Array of size extended blink frame
}ieee_blink_frame_ext_t;

//! IEEE 802.15.4 standard ranging frames
typedef union {
//! Structure of range request frame
    struct _ieee_rng_request_frame_t{
        uint16_t fctrl;             //!< Frame control (0x8841 to indicate a data frame using 16-bit addressing)
        uint8_t seq_num;            //!< Sequence number, incremented for each new frame
        uint16_t PANID;             //!< PANID
        uint16_t dst_address;       //!< Destination address
        uint16_t src_address;       //!< Source address
        uint16_t code;              //!< Response code for the request
    }__attribute__((__packed__,aligned(1)));
    uint8_t array[sizeof(struct _ieee_rng_request_frame_t)];  //!< Array of size range request frame
} ieee_rng_request_frame_t;

//! Standard response frame
typedef union {
//! Structure of range response frame
    struct  _ieee_rng_response_frame_t{
        struct _ieee_rng_request_frame_t;
        uint32_t reception_timestamp;    //!< Request reception timestamp
        uint32_t transmission_timestamp; //!< Response transmission timestamp
    }__attribute__((__packed__,aligned(1)));
    uint8_t array[sizeof(struct _ieee_rng_response_frame_t)]; //!< Array of size range response frame
} ieee_rng_response_frame_t;

//! IEEE 802.15.4e standard beacon frame MHR
typedef union{
//! Structure of IEEE beacon frame with 64bit src addressing
    struct _ieee_beacon_frame_t{
        uint16_t fctrl;             //!< Frame control
        uint8_t seq_num;            //!< Sequence number, incremented for each new frame
        uint64_t euid;              //!< extended unique identifier
        uint16_t superframe_spec;   //!<
        uint8_t gts;                //!< Guaranteed Time Slot, set to zero
        uint8_t pending_addr_spec;  //!< set to zero
    }__attribute__((__packed__,aligned(1)));
    uint8_t array[sizeof(struct _ieee_beacon_frame_t)]; //!< Array of size blink frame
}ieee_beacon_frame_t;

//! IEEE 802.15.4 standard data frame.
typedef union {
//! Structure of standard frame
    struct _ieee_std_frame_t{
        uint16_t fctrl;             //!< Frame control (0x8841 to indicate a data frame using 16-bit addressing)
        uint8_t seq_num;            //!< Sequence number, incremented for each new frame
        uint16_t PANID;             //!< PANID
        uint16_t dst_address;       //!< Destination address
        uint16_t src_address;       //!< Source address
        uint16_t code;              //!< Response code for the request
    }__attribute__((__packed__,aligned(1)));
    uint8_t array[sizeof(struct _ieee_std_frame_t)];  //!< Array of size standard frame
} ieee_std_frame_t;


#ifdef __cplusplus
}
#endif
#endif /* _UWB_FTYPES_H_ */
