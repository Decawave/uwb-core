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
 * @file ccp_json.c
 * @author UWB Core <uwbcore@gmail.com>
 * @date Apr 10 2019
 * @brief
 *
 */
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include <uwb_ccp/ccp_json.h>


static int ccp_write_line(void *buf, char* data, int len);

ccp_json_t * ccp_json_init(ccp_json_t * json){

    if (json == NULL){
        json = (ccp_json_t *) calloc(1, sizeof(ccp_json_t));
        json->status.selfmalloc=1;
    }else{
        json_status_t tmp = json->status;
        memset(json,0,offsetof(ccp_json_t,utime));
        json->status = tmp;
        json->status.initialized = 1;
    }
    return json;
}

void ccp_json_free(ccp_json_t * json){
    if(json->status.selfmalloc)
        free(json);
}

int
ccp_write_line(void *buf, char* data, int len)
{
    ccp_json_t * json = buf;
    for (uint16_t i=0; i < len; i++){
        json->iobuf[(json->idx++)%MYNEWT_VAL(UWB_CCP_JSON_BUFSIZE)] = data[i];
        if (data[i]=='\0'){
            break;
        }
    }
    if (json->iobuf[json->idx-1]=='\0')
        json->idx = 0;

    return len;
}


int
ccp_json_write(ccp_json_t * json){

    struct json_value value;
    int rc;

    json->encoder.je_write = ccp_write_line;
    json->encoder.je_arg = (void *)json;
    json->decoder.json_buf.jb_read_next = json_read_next;
    json->decoder.json_buf.jb_read_prev = json_read_prev;
    json->decoder.json_buf.jb_readn = json_readn;

    json->encoder.je_wr_commas = 0;
    rc = json_encode_object_start(&json->encoder);
    JSON_VALUE_UINT(&value, json->utime);
    rc |= json_encode_object_entry(&json->encoder, "utime", &value);
    rc |= json_encode_array_name(&json->encoder, "ccp");
    rc |= json_encode_array_start(&json->encoder);
    for (uint8_t i = 0; i< sizeof(json->ccp)/sizeof(dpl_float64_t); i++){
        JSON_VALUE_FLOAT64(&value, json->ccp[i]);
        rc |= json_encode_array_value(&json->encoder, &value);
    }
    rc |= json_encode_array_finish(&json->encoder);
    JSON_VALUE_UINT(&value, json->seq);
    rc |= json_encode_object_entry(&json->encoder, "seq", &value);
    if (json->uid){
        JSON_VALUE_UINT(&value, json->uid);
        rc |= json_encode_object_entry(&json->encoder, "uid", &value);
    }
    if (DPL_FLOAT64_ISNAN(json->ppm) == 0){
        JSON_VALUE_FLOAT64(&value, json->ppm);
        rc |= json_encode_object_entry(&json->encoder, "ppm", &value);
    }
    rc |= json_encode_object_finish(&json->encoder);
    json->encoder.je_wr_commas = 0;
    assert(rc == 0);

    ccp_write_line(json->encoder.je_arg, "\0", 1);
    return rc;
}


int
ccp_json_write_uint64(ccp_json_t * json){

    struct json_value value;
    int rc;

    json->encoder.je_write = ccp_write_line;
    json->encoder.je_arg = (void *) json;
    json->decoder.json_buf.jb_read_next = json_read_next;
    json->decoder.json_buf.jb_read_prev = json_read_prev;
    json->decoder.json_buf.jb_readn = json_readn;

    json->encoder.je_wr_commas = 0;
    rc = json_encode_object_start(&json->encoder);
    JSON_VALUE_UINT(&value, json->utime);
    rc |= json_encode_object_entry(&json->encoder, "utime", &value);
    rc |= json_encode_array_name(&json->encoder, "ccp");
    rc |= json_encode_array_start(&json->encoder);
    for (uint8_t i = 0; i< sizeof(json->ccp)/sizeof(uint64_t); i++){
        JSON_VALUE_UINT(&value, json->ccp_uint64[i]);
        rc |= json_encode_array_value(&json->encoder, &value);
    }
    rc |= json_encode_array_finish(&json->encoder);
    JSON_VALUE_UINT(&value, json->seq);
    rc |= json_encode_object_entry(&json->encoder, "seq", &value);
    if (json->uid){
        JSON_VALUE_UINT(&value, json->uid);
        rc |= json_encode_object_entry(&json->encoder, "uid", &value);
    }
    if (DPL_FLOAT64_ISNAN(json->ppm) == 0){
        JSON_VALUE_UINT(&value, json->ppm_uint64);
        rc |= json_encode_object_entry(&json->encoder, "ppm", &value);
    }
    rc |= json_encode_object_finish(&json->encoder);
    json->encoder.je_wr_commas = 0;
    assert(rc == 0);

    ccp_write_line(json->encoder.je_arg, "\0", 1);
    return rc;
}

#ifdef FLOAT_SUPPORT
int
ccp_json_read(ccp_json_t * json, char * line){

    struct json_attr_t ccp_attr[6] = {
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
            .attribute = "ccp",
            .type = t_array,
            .addr.array = {
                .element_type =  t_real,
                .arr.reals.store = (dpl_float64_t *)json->ccp,
                .maxlen = sizeof(json->ccp)/sizeof(json->ccp[0]),
                .count = (uint64_t *)&json->count,
            },
            .nodefault = false,
            .dflt.real = DPL_FLOAT64_NAN(),
            .len = sizeof(json->ccp)
        },
        [4] = {
            .attribute = "ppm",
            .type = t_real,
            .addr.real = &json->ppm,
            .nodefault = false,
            .dflt.real = DPL_FLOAT64_NAN()
            },
        [5] = {
            .attribute = NULL
        }
    };

    json->encoder.je_write = ccp_write_line;
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

int
ccp_json_read_uint64(ccp_json_t * json, char * line){

    struct json_attr_t ccp_attr[6] = {
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
            .nodefault = true
        },
        [2] = {
            .attribute = "uid",
            .type = t_uinteger,
            .addr.uinteger = (uint64_t *)&json->uid,
            .nodefault = true
        },
        [3] = {
            .attribute = "ccp",
            .type = t_array,
            .addr.array = {
                .element_type =  t_uinteger,
                .arr.uintegers.store = (uint64_t *)json->ccp_uint64,
                .maxlen = sizeof(json->ccp)/sizeof(json->ccp_uint64[0]),
                .count = (uint64_t *)&json->count,
            },
            .nodefault = true,
            .len = sizeof(json->ccp)
        },
        [4] = {
            .attribute = "ppm",
            .type = t_uinteger,
            .addr.uinteger = &json->ppm_uint64,
            .nodefault = true
        },
        [5] = {
            .attribute = NULL
        }
    };


    json->encoder.je_write = ccp_write_line;
    json->encoder.je_arg = NULL;
    json->decoder.json_buf.jb_read_next = json_read_next;
    json->decoder.json_buf.jb_read_prev = json_read_prev;
    json->decoder.json_buf.jb_readn = json_readn;

    json->decoder.start_buf = line;
    json->decoder.end_buf = line + strlen(line);
    json->decoder.current_position = 0;

    memset((void *)json + offsetof(ccp_json_t,utime), 0, offsetof(ccp_json_t,ppm) - offsetof(ccp_json_t,utime) + sizeof(json->ppm));

    int rc = json_read_object(&json->decoder.json_buf, ccp_attr);

    return rc;
}
