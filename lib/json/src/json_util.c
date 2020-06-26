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
 * @file ccp_util.c
 * @author UWB Core <uwbcore@gmail.com>
 * @date Apr 10 2019
 * @brief
 *
 */
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <json/json_util.h>

#ifdef __KERNEL__
#include <linux/kernel.h>
#else
#define EXPORT_SYMBOL(__S)
#endif
int
json_write(void *buf, char* data, int len)
{
    if (buf)
        sprintf(buf,"%s",data);
    else
        printf("%s", data);
    return len;
}

char
json_read_next(struct json_buffer * jb)
{
    json_decoder_t * decoder = (json_decoder_t *) jb;
    char c;

    if ((decoder->start_buf + decoder->current_position) <= decoder->end_buf) {
        c = *(decoder->start_buf + decoder->current_position);
        decoder->current_position++;
        return c;
    }
   return '\0';
}
EXPORT_SYMBOL(json_read_next);

/* this goes backward in the buffer one character */
char
json_read_prev(struct json_buffer * jb)
{
    json_decoder_t * decoder = (json_decoder_t *) jb;

    char c;
    if (decoder->current_position) {
       decoder->current_position--;
       c = *(decoder->start_buf + decoder->current_position);
       return c;
    }
    /* can't rewind */
    return '\0';
}
EXPORT_SYMBOL(json_read_prev);

int
json_readn(struct json_buffer * jb, char *buf, int size)
{
    json_decoder_t * decoder = (json_decoder_t *) jb;
    int remlen;

    remlen = (int)(decoder->end_buf - (decoder->start_buf + decoder->current_position));
    if (size > remlen) {
        size = remlen;
    }

    memcpy(buf, decoder->start_buf + decoder->current_position, size);
    decoder->current_position += size;
    return size;
}
EXPORT_SYMBOL(json_readn);
