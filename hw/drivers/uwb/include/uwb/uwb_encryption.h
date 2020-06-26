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

#ifndef __UWB_ENCRYPTION_H__
#define __UWB_ENCRYPTION_H__

/*****************************************************************************/
/*                               AES BLOCK                                   */
/*****************************************************************************/

/* enums below are defined in such a way as to allow a simple write to
 * DW3000 AES configuration registers */

/* For MIC size definition */
typedef enum {
    MIC_0 = 0,
    MIC_4, MIC_6, MIC_8, MIC_10, MIC_12, MIC_14, MIC_16
}uwb_mic_size_t;

/* Key size definition */
typedef enum {
    AES_KEY_128bit = 0,
    AES_KEY_192bit = 1,
    AES_KEY_256bit = 2
} uwb_aes_key_size_t;

/* Load key from RAM selection */
typedef enum {
    AES_KEY_No_Load = 0,
    AES_KEY_Load
}uwb_aes_key_load_t;

/* Key source - RAM or registers */
typedef enum {
    AES_KEY_Src_Register = 0,
    AES_KEY_Src_Ram
}uwb_aes_key_src_t;

/* Operation selection */
typedef enum {
    AES_Encrypt = 0,
    AES_Decrypt
}uwb_aes_mode_t;

/* This defines the source port for encrypted/unencrypted data */
typedef enum {
    AES_Src_Scratch = 0,
    AES_Src_Rx_buf_0,
    AES_Src_Rx_buf_1,
    AES_Src_Tx_buf,
    AES_Src_CP_Seed_Register
}uwb_aes_src_port_t;

/* This defines the dest port for encrypted/unencrypted data */
typedef enum {
    AES_Dst_Scratch = 0,
    AES_Dst_Rx_buf_0,
    AES_Dst_Rx_buf_1,
    AES_Dst_Tx_buf
}uwb_aes_dst_port_t;

/* storage for 128/192/256-bit key */
struct uwb_aes_key {
      uint32_t key0;
      uint32_t key1;
      uint32_t key2;
      uint32_t key3;
      uint32_t key4;
      uint32_t key5;
      uint32_t key6;
      uint32_t key7;
};

/* storage for high part of 96-bit nonce */
struct uwb_aes_iv {
      uint32_t iv0;
      uint32_t iv1;
      uint32_t iv2;
};

struct uwb_aes_config {
    uwb_mic_size_t      mic;        //!< Message integrity code size
    uwb_aes_key_size_t  key_size;   //!< AES key length configuration corresponding to AES_KEY_128/192/256bit
    uwb_aes_key_load_t  key_load;   //!< Loads key from RAM
    uint8_t             key_addr;   //!< Address offset of AES key in AES key RAM
    uwb_aes_key_src_t   key_src;    //!< Location of the key: either as programmed in registers(128 bit) or in the RAM
    uwb_aes_mode_t      mode;       //!< Operation type encrypt/decrypt
};

struct uwb_aes_job {
    struct uwb_aes_iv   *nonce;      //!< Pointer to the nonce
    uint8_t             *header;     //!< Pointer to header (this is not encrypted/decrypted)
    uint8_t             *payload;    //!< Pointer to payload (this is encrypted/decrypted)
    uint8_t             header_len;  //!< Header size
    uint16_t            payload_len; //!< Payload size
    uwb_aes_src_port_t  src_port;    //!< Source port
    uwb_aes_dst_port_t  dst_port;    //!< Dest port
    uwb_aes_mode_t      mode;        //!< Encryption or decryption
    uint8_t             mic_size;    //!< tag_size;
};

/* storage for 128-bit STS CP key */
struct uwb_sts_cp_key {
    union {
        struct _uwb_sts_cp_key_s{
            uint32_t key0;
            uint32_t key1;
            uint32_t key2;
            uint32_t key3;
        };
        uint8_t array[sizeof(struct _uwb_sts_cp_key_s)];
    } __attribute__((__packed__,aligned(1)));
};

/* storage for 128-bit STS CP IV (nonce) */
struct uwb_sts_cp_iv {
    union {
        struct _uwb_sts_cp_iv_s{
            uint32_t iv0;
            uint32_t iv1;
            uint32_t iv2;
            uint32_t iv3;
        };
        uint8_t array[sizeof(struct _uwb_sts_cp_iv_s)];
    } __attribute__((__packed__,aligned(1)));
};

#endif  // __UWB_ENCRYPTION_H__
