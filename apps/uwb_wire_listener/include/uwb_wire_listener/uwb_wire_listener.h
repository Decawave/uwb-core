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
 * @file wire_listener.h
 * @author UWB Core <uwbcore@gmail.com>
 * @date 2020
 * @brief Wireshark compatible listener
 *
 * @details Wireshark compatible listener / sniffer module
 */

#ifndef __UWB_WIRE_LISTENER_H_
#define __UWB_WIRE_LISTENER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <dpl/dpl.h>
#include <dpl/dpl_types.h>
#include <uwb/uwb.h>
#include <uwb/uwb_mac.h>

typedef struct pcap_hdr_s {
    uint32_t magic_number;
    uint16_t version_major;
    uint16_t version_minor;
    int32_t  thiszone;
    uint32_t sigfigs;
    uint32_t snaplen;
    uint32_t network;
} pcap_hdr_t;

typedef struct pcapprec_hdr_s {
    uint32_t ts_sec;
    uint32_t ts_usec;
    uint32_t incl_len;
    uint32_t orig_len;
    uint8_t  buf_arr[1024];
} pcapprec_hdr_t;

typedef struct _uwb_rng_status_t {
    uint16_t selfmalloc:1;
    uint16_t initialized:1;
} uwb_wire_listener_status_t;

struct uwb_wire_listener_instance {
    struct uwb_dev *dev_inst;
    uwb_wire_listener_status_t status;
    pcapprec_hdr_t pcap_header;
};

struct uwb_wire_listener_instance * uwb_wire_listener_init(struct uwb_dev * dev);
void uwb_wire_listener_free(struct uwb_wire_listener_instance * rng);
void uwb_wire_listener_chrdev_destroy(int idx);
int uwb_wire_listener_chrdev_create(int idx);
int uwb_wire_listener_chrdev_output(int idx, char *buf, size_t len);
#ifdef __cplusplus
}
#endif

#endif /* __UWB_WIRE_LISTENER_H_ */
