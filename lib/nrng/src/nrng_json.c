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
 * @file nrng_json.c
 * @author UWB Core <uwbcore@gmail.com>
 * @date May 1 2020
 * @brief
 *
 */
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include <dpl/dpl_types.h>
#include <nrng/nrng_json.h>

static int nrng_write_line(void *buf, char* data, int len);

static int
nrng_write_line(void *buf, char* data, int len)
{
    nrng_json_t * json = buf;
    for (uint16_t i=0; i < len; i++){
        json->iobuf[json->idx++] = data[i];
        if (data[i]=='\0'){
            break;
        }
    }
    if (json->iobuf[json->idx-1]=='\0')
        json->idx = 0;
    return len;
}

int
nrng_json_write(nrng_json_t * json){

    struct json_value value;
    int rc;

    json->encoder.je_write = nrng_write_line;
    json->encoder.je_arg = (void *)json;
    json->decoder.json_buf.jb_read_next = json_read_next;
    json->decoder.json_buf.jb_read_prev = json_read_prev;
    json->decoder.json_buf.jb_readn = json_readn;
    json->encoder.je_wr_commas = 0;

    rc = json_encode_object_start(&json->encoder);
    JSON_VALUE_UINT(&value, json->utime);
    rc |= json_encode_object_entry(&json->encoder, "utime", &value);
    JSON_VALUE_UINT(&value, json->seq);
    rc |= json_encode_object_entry(&json->encoder, "seq", &value);
    if (json->uid){
        JSON_VALUE_UINT(&value, json->uid);
        rc |= json_encode_object_entry(&json->encoder, "uid", &value);
    }
    rc |= json_encode_array_name(&json->encoder, "ouid");
    rc |= json_encode_array_start(&json->encoder);
    for (uint8_t i = 0; i< json->nsize; i++){
        JSON_VALUE_UINT(&value, json->ouid[i]);
        rc |= json_encode_array_value(&json->encoder, &value);
    }
    rc |= json_encode_array_finish(&json->encoder);

    rc |= json_encode_array_name(&json->encoder, "rng");
    rc |= json_encode_array_start(&json->encoder);
    for (uint8_t i = 0; i< json->nsize; i++){
        JSON_VALUE_FLOAT64(&value, json->rng[i]);
        rc |= json_encode_array_value(&json->encoder, &value);
    }
    rc |= json_encode_array_finish(&json->encoder);
    rc |= json_encode_object_finish(&json->encoder);
    json->encoder.je_wr_commas = 0;
    assert(rc == 0);

    nrng_write_line(json->encoder.je_arg, "\0", 1);

    return rc;
}

int
nrng_json_read(nrng_json_t * json, char * line){

    struct json_attr_t ccp_attr[6] = {
        [0] = {
            .attribute = "utime",
            .type = t_uinteger,
            .addr.uinteger = (uint64_t *)&json->utime,
            .dflt.uinteger = 0
        },
        [1] = {
            .attribute = "seq",
            .type = t_uinteger,
            .addr.uinteger = (uint64_t *)&json->seq,
            .dflt.uinteger = 0
        },
        [2] = {
            .attribute = "uid",
            .type = t_uinteger,
            .addr.uinteger = (uint64_t *)&json->uid,
            .dflt.uinteger = 0
        },
        [3] = {
            .attribute = "ouid",
            .type = t_array,
            .addr.array = {
                .element_type = t_uinteger,
                .arr.uintegers.store = (uint64_t *)json->ouid,
                .maxlen = sizeof(json->ouid)/sizeof(json->ouid[0]),
                .count = (uint64_t *)&json->nsize
            },
            .nodefault = true,
            .len = sizeof(json->ouid)
        },
        [4] = {
            .attribute = "rng",
            .type = t_array,
            .addr.array = {
                .element_type = t_real,
                .arr.reals.store = (dpl_float64_t *)json->rng,
                .maxlen = sizeof(json->rng)/sizeof(json->rng[0]),
                .count = (uint64_t *)&json->nsize
             },
            .nodefault = true,
            .len = sizeof(json->rng)
        },
        [5] = {
            .attribute = NULL
        }
    };

    json->encoder.je_write = nrng_write_line;
    json->encoder.je_arg = (void *)json;
    json->decoder.json_buf.jb_read_next = json_read_next;
    json->decoder.json_buf.jb_read_prev = json_read_prev;
    json->decoder.json_buf.jb_readn = json_readn;

    json->decoder.start_buf = line;
    json->decoder.end_buf = line + strlen(line);
    json->decoder.current_position = 0;

    memset((void *)json + offsetof(nrng_json_t,utime), 0, offsetof(nrng_json_t,iobuf) - offsetof(nrng_json_t,utime));

    int rc = json_read_object(&json->decoder.json_buf, ccp_attr);

    return rc;
}
