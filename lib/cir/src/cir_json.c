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
 * @file cir_json.c
 * @author UWB Core <uwbcore@gmail.com>
 * @date May 5 2020
 * @brief
 *
 */
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include <cir/cir_json.h>

#ifndef __KERNEL__
static_assert(MYNEWT_VAL(CIR_VERBOSE_BUFFER_SIZE) > (64+12*MYNEWT_VAL(CIR_MAX_SIZE)),
              "CIR_VERBOSE_BUFFER_SIZE needs to be large enough to hold the entire CIR_MAX_SIZE vector");
#endif

static int cir_write_line(void *buf, char* data, int len);

struct cir_json * cir_json_init(struct cir_json * json)
{
    if (json == NULL){
        json = (struct cir_json *) calloc(1, sizeof(struct cir_json));
        json->status.selfmalloc=1;
    }else{
        json_status_t tmp = json->status;
        memset(json, 0, offsetof(struct cir_json, utime));
        json->status = tmp;
        json->status.initialized = 1;
    }
    return json;
}

void cir_json_free(struct cir_json * json)
{
    if(json->status.selfmalloc) {
        free(json);
    }
}

int
cir_write_line(void *buf, char* data, int len)
{
    uint16_t i;
    struct cir_json * json = buf;
    for (i=0; i < len; i++) {
        json->iobuf[json->idx++] = data[i];
        if (data[i]=='\0'){
            break;
        }
    }
    if (json->iobuf[json->idx-1]=='\0') {
        json->idx = 0;
    }

    return len;
}


int
cir_json_write(struct cir_json * json)
{
    int rc;
    int i;
    struct json_value value;

    json->encoder.je_write = cir_write_line;
    json->encoder.je_arg = (void *)json;
    json->decoder.json_buf.jb_read_next = json_read_next;
    json->decoder.json_buf.jb_read_prev = json_read_prev;
    json->decoder.json_buf.jb_readn = json_readn;

    json->encoder.je_wr_commas = 0;
    rc = json_encode_object_start(&json->encoder);
    JSON_VALUE_UINT(&value, json->utime);
    rc |= json_encode_object_entry(&json->encoder, "utime", &value);

    JSON_VALUE_STRING(&value, json->type);
    rc |= json_encode_object_entry(&json->encoder, "cir_type", &value);

    JSON_VALUE_UINT(&value, json->raw_ts);
    rc |= json_encode_object_entry(&json->encoder, "raw_ts", &value);

    JSON_VALUE_UINT(&value, json->resampler_delay);
    rc |= json_encode_object_entry(&json->encoder, "resam_dly", &value);

    JSON_VALUE_FLOAT64(&value, json->fp_idx);
    rc |= json_encode_object_entry(&json->encoder, "fp_idx", &value);

    JSON_VALUE_FLOAT64(&value, json->fp_power);
    rc |= json_encode_object_entry(&json->encoder, "fp_power", &value);

    JSON_VALUE_FLOAT64(&value, json->angle);
    rc |= json_encode_object_entry(&json->encoder, "angle", &value);

    JSON_VALUE_UINT(&value, json->accumulator_count);
    rc |= json_encode_object_entry(&json->encoder, "acc_cnt", &value);

    if (json->cir_count) {
        JSON_VALUE_UINT(&value, json->cir_offset);
        rc |= json_encode_object_entry(&json->encoder, "offset", &value);

        rc |= json_encode_array_name(&json->encoder, "real");
        rc |= json_encode_array_start(&json->encoder);
        for (i = 0; i< json->cir_count; i++) {
            JSON_VALUE_INT(&value, json->real[i]);
            rc |= json_encode_array_value(&json->encoder, &value);
        }
        rc |= json_encode_array_finish(&json->encoder);

        rc |= json_encode_array_name(&json->encoder, "imag");
        rc |= json_encode_array_start(&json->encoder);
        for (i = 0; i< json->cir_count; i++) {
            JSON_VALUE_INT(&value, json->imag[i]);
            rc |= json_encode_array_value(&json->encoder, &value);
        }
        rc |= json_encode_array_finish(&json->encoder);
    }

    rc |= json_encode_object_finish(&json->encoder);
    json->encoder.je_wr_commas = 0;
    assert(rc == 0);

    cir_write_line(json->encoder.je_arg, "\0", 1);
    return rc;
}
