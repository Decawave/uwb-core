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
 * @file rng_json.c
 * @author UWB Core <uwbcore@gmail.com>
 * @date May 25 2019
 * @brief
 *
 */
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include <uwb_rng/rng_json.h>

static int rng_write_line(void *buf, char* data, int len);

rng_json_t * rng_json_init(rng_json_t * json){

    if (json == NULL){
        json = (rng_json_t *) calloc(1, sizeof(rng_json_t));
        json->status.selfmalloc=1;
    }else{
        json_status_t tmp = json->status;
        memset(json,0,offsetof(rng_json_t,utime));
        json->status = tmp;
        json->status.initialized = 1;
    }
    return json;
}

void
rng_json_free(rng_json_t * json){
    if(json->status.selfmalloc)
        free(json);
}

static int
rng_write_line(void *buf, char* data, int len)
{
    rng_json_t * json = buf;
    for (uint16_t i=0; i < len; i++){
        json->iobuf[(json->idx++)%MYNEWT_VAL(UWB_RNG_JSON_BUFSIZE)] = data[i];
        if (data[i]=='\0'){
            break;
        }
    }
    if (json->iobuf[json->idx-1]=='\0')
        json->idx = 0;
    return len;
}

int
rng_json_write(rng_json_t * json){

    struct json_value value;
    int rc;

    json->encoder.je_write = rng_write_line;
    json->encoder.je_arg = (void *)json;
    json->decoder.json_buf.jb_read_next = json_read_next;
    json->decoder.json_buf.jb_read_prev = json_read_prev;
    json->decoder.json_buf.jb_readn = json_readn;
    json->encoder.je_wr_commas = 0;

    rc = json_encode_object_start(&json->encoder);
    JSON_VALUE_UINT(&value, json->utime);
    rc |= json_encode_object_entry(&json->encoder, "utime", &value);

    if (json->seq){
        JSON_VALUE_UINT(&value, json->seq);
        rc |= json_encode_object_entry(&json->encoder, "seq", &value);
    }
    if (json->code){
        JSON_VALUE_UINT(&value, json->code);
        rc |= json_encode_object_entry(&json->encoder, "c", &value);
    }
    if (json->uid){
        JSON_VALUE_UINT(&value, json->uid);
        rc |= json_encode_object_entry(&json->encoder, "uid", &value);
    }
    if (json->ouid){
        JSON_VALUE_UINT(&value, json->ouid);
        rc |= json_encode_object_entry(&json->encoder, "ouid", &value);
    }

    if(DPL_FLOAT64_ISNAN(json->raz.azimuth) && DPL_FLOAT64_ISNAN(json->raz.zenith)){
        rc |= json_encode_array_name(&json->encoder, "raz");
        rc |= json_encode_array_start(&json->encoder);
        JSON_VALUE_FLOAT64(&value, json->raz.range);
        rc |= json_encode_array_value(&json->encoder, &value);
        rc |= json_encode_array_finish(&json->encoder);
    }else{
        rc |= json_encode_array_name(&json->encoder, "raz");
        rc |= json_encode_array_start(&json->encoder);
        for (uint8_t i = 0; i< sizeof(json->raz)/sizeof(dpl_float64_t); i++){
            JSON_VALUE_FLOAT64(&value, json->raz.array[i]);
            rc |= json_encode_array_value(&json->encoder, &value);
        }
        rc |= json_encode_array_finish(&json->encoder);
    }

    if(!DPL_FLOAT64_ISNAN(json->braz.range)){
        if(DPL_FLOAT64_ISNAN(json->braz.azimuth) && DPL_FLOAT64_ISNAN(json->braz.zenith)){
            rc |= json_encode_array_name(&json->encoder, "braz");
            rc |= json_encode_array_start(&json->encoder);
            JSON_VALUE_FLOAT64(&value, json->braz.range);
            rc |= json_encode_array_value(&json->encoder, &value);
            rc |= json_encode_array_finish(&json->encoder);
        }else{
            rc |= json_encode_array_name(&json->encoder, "braz");
            rc |= json_encode_array_start(&json->encoder);
            for (uint8_t i = 0; i< sizeof(json->braz)/sizeof(dpl_float64_t); i++){
                JSON_VALUE_FLOAT64(&value, json->braz.array[i]);
                rc |= json_encode_array_value(&json->encoder, &value);
            }
            rc |= json_encode_array_finish(&json->encoder);
        }
    }

    if(!DPL_FLOAT64_ISNAN(json->rssi[0])){
        if(!DPL_FLOAT64_ISNAN(json->rssi[0]) && DPL_FLOAT64_ISNAN(json->rssi[1]) && DPL_FLOAT64_ISNAN(json->rssi[2])){
                JSON_VALUE_FLOAT64(&value, json->rssi[0]);
                rc |= json_encode_array_name(&json->encoder, "rssi");
                rc |= json_encode_array_start(&json->encoder);
                rc |= json_encode_array_value(&json->encoder, &value);
                rc |= json_encode_array_finish(&json->encoder);
        }else{
                rc |= json_encode_array_name(&json->encoder, "rssi");
                rc |= json_encode_array_start(&json->encoder);
                for (uint8_t i = 0; i< sizeof(json->rssi)/sizeof(dpl_float64_t); i++){
                    JSON_VALUE_FLOAT64(&value, json->rssi[i]);
                    rc |= json_encode_array_value(&json->encoder, &value);
                }
                rc |= json_encode_array_finish(&json->encoder);
        }
    }

    if(!DPL_FLOAT64_ISNAN(json->los[0])){
        if(!DPL_FLOAT64_ISNAN(json->los[0]) && DPL_FLOAT64_ISNAN(json->los[1]) && DPL_FLOAT64_ISNAN(json->los[2])){
                JSON_VALUE_FLOAT64(&value, json->los[0]);
                rc |= json_encode_array_name(&json->encoder, "los");
                rc |= json_encode_array_start(&json->encoder);
                rc |= json_encode_array_value(&json->encoder, &value);
                rc |= json_encode_array_finish(&json->encoder);
        }else{
                rc |= json_encode_array_name(&json->encoder, "los");
                rc |= json_encode_array_start(&json->encoder);
                for (uint8_t i = 0; i< sizeof(json->los)/sizeof(dpl_float64_t); i++){
                    JSON_VALUE_FLOAT64(&value, json->los[i]);
                    rc |= json_encode_array_value(&json->encoder, &value);
                }
                rc |= json_encode_array_finish(&json->encoder);
        }
    }

    if (!DPL_FLOAT64_ISNAN(json->ppm)){
        JSON_VALUE_FLOAT64(&value, json->ppm);
        rc |= json_encode_object_entry(&json->encoder, "ppm", &value);
    }

    if (!DPL_FLOAT64_ISNAN(json->sts)){
        JSON_VALUE_FLOAT64(&value, json->sts);
        rc |= json_encode_object_entry(&json->encoder, "sts", &value);
    }
    rc |= json_encode_object_finish(&json->encoder);
    json->encoder.je_wr_commas = 0;
    assert(rc == 0);

    rng_write_line(json->encoder.je_arg, "\0", 1);

    return rc;
}


#ifdef FLOAT_SUPPORT
int
rng_json_read(rng_json_t * json, char * line){

    struct json_attr_t ccp_attr[] = {
        [0] = {
            .attribute = "utime",
            .type = t_uinteger,
            .addr.uinteger = (uint64_t *)&json->utime,
            .nodefault = true
        },
        [1] = {
            .attribute = "seq",
            .type = t_uinteger,
            .addr.uinteger = (uint64_t *)&json->seq,
            .nodefault = false,
            .dflt.uinteger = 0
        },
        [2] = {
            .attribute = "uid",
            .type = t_uinteger,
            .addr.uinteger = (uint64_t *)&json->uid,
            .nodefault = false,
            .dflt.uinteger = 0
        },
        [3] = {
            .attribute = "ouid",
            .type = t_uinteger,
            .addr.uinteger = (uint64_t *)&json->ouid,
            .nodefault = false,
            .dflt.uinteger = 0
        },
        [4] = {
            .attribute = "rng",
            .type = t_real,
            .addr.real = &json->raz.range,
            .nodefault = false,
            .dflt.real = DPL_FLOAT64_NAN()
        },
        [5] = {
            .attribute = "raz",
            .type = t_array,
            .addr.array = {
                .element_type = t_real,
                .arr.reals.store = (dpl_float64_t *)json->raz.array,
                .maxlen = sizeof(json->raz)/sizeof(json->raz.array[0]),
            },
            .nodefault = false,
            .dflt.real = DPL_FLOAT64_NAN(),
            .len = sizeof(json->raz)
        },
        [6] = {
            .attribute = "rssi",
            .type = t_array,
            .addr.array = {
                .element_type = t_real,
                .arr.reals.store = (dpl_float64_t *)json->rssi,
                .maxlen = sizeof(json->rssi)/sizeof(json->rssi[0]),
            },
            .nodefault = false,
            .dflt.real = DPL_FLOAT64_NAN(),
            .len = sizeof(json->rssi)
        },
        [7] = {
            .attribute = "los",
            .type = t_array,
            .addr.array = {
                .element_type = t_real,
                .arr.reals.store = (dpl_float64_t *)json->los,
                .maxlen = sizeof(json->los)/sizeof(json->los[0]),
            },
            .nodefault = false,
            .dflt.real = DPL_FLOAT64_NAN(),
            .len = sizeof(json->los)
        },
        [8] = {
            .attribute = "ppm",
            .type = t_real,
            .addr.real = &json->ppm,
            .nodefault = false,
            .dflt.real = DPL_FLOAT64_NAN(),
        },
        [9] = {
            .attribute = "sts",
            .type = t_real,
            .addr.real = &json->sts,
            .nodefault = false,
            .dflt.real = DPL_FLOAT64_NAN(),
        },
        [10] = {
            .attribute = NULL
        }
    };

    json->encoder.je_write = rng_write_line;
    json->encoder.je_arg = NULL;
    json->decoder.json_buf.jb_read_next = json_read_next;
    json->decoder.json_buf.jb_read_prev = json_read_prev;
    json->decoder.json_buf.jb_readn = json_readn;

    json->decoder.start_buf = line;
    json->decoder.end_buf = line + strlen(line);
    json->decoder.current_position = 0;

    int rc = json_read_object(&json->decoder.json_buf, ccp_attr);

    return rc;
}
#endif
