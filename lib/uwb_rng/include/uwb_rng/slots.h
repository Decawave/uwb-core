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
 * @file slots.h
 * @author UWB Core <uwbcore@gmail.com>
 * @date 12/2018
 * @brief Slots
 *
 * @details Help function to calculate the numerical ordering of a bit within a bitmask
 */
#ifndef _SLOTS_H_
#define _SLOTS_H_

#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _slot_mode_t{
    SLOT_REMAINING=0,      //!< no. of slots remaining
    SLOT_POSITION        //!< slot position
}slot_mode_t;

typedef enum _slot_ptype_t{
    PTYPE_CELL=0,         //!< Cell network
    PTYPE_BITFIELD,       //!< single cell network
    PTYPE_RANGE           //!< specify slots as a range
}slot_ptype_t;

typedef struct _slot_payload_t{
    uint32_t ptype:2;           //!< payload type
    union {
        uint32_t bitfield:30;
        struct {
            uint32_t cell_id:14;
            uint32_t slot_mask:16;
        };
        struct {
            uint32_t start_slot_id:14;
            uint32_t end_slot_id:16;
        };
    };
}slot_payload_t;

uint32_t NumberOfBits(uint32_t bitfield);
uint32_t BitIndex(uint32_t mask, uint32_t slot, slot_mode_t mode);
uint32_t BitPosition(uint32_t n);

#ifdef __cplusplus
}
#endif

#endif //_SLOTS_H_
