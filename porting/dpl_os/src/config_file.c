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

#include "syscfg/syscfg.h"


#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include "config/config.h"
#include "config/config_file.h"
#include "config_priv.h"

static int conf_file_load(struct conf_store *, conf_store_load_cb cb,
                          void *cb_arg);
static int conf_file_save(struct conf_store *, const char *name,
  const char *value);

static struct conf_store_itf conf_file_itf = {
    .csi_load = conf_file_load,
    .csi_save = conf_file_save,
};

/*
 * Register a file to be a source of configuration.
 */
int
conf_file_src(struct conf_file *cf)
{
    if (!cf->cf_name) {
        return EINVAL;
    }
    cf->cf_store.cs_itf = &conf_file_itf;
    conf_src_register(&cf->cf_store);

    return 0;
}

int
conf_file_dst(struct conf_file *cf)
{
    if (!cf->cf_name) {
        return EINVAL;
    }
    cf->cf_store.cs_itf = &conf_file_itf;
    conf_dst_register(&cf->cf_store);

    return 0;
}

int
conf_getnext_line(FILE *file, char *buf, int blen, uint32_t *loc)
{
    int rc;
    char *end;
    uint32_t len;

    rc = fseek(file, *loc, SEEK_SET);
    if (rc < 0) {
        *loc = 0;
        return -1;
    }
    len = fread(buf, 1, blen, file);
    if (rc < 0 || len == 0) {
        *loc = 0;
        return -1;
    }
    if (len == blen) {
        len--;
    }
    buf[len] = '\0';

    end = strchr(buf, '\n');
    if (end) {
        *end = '\0';
    } else {
        end = strchr(buf, '\0');
    }
    blen = end - buf;
    *loc += (blen + 1);
    return blen;
}

/*
 * Called to load configuration items. cb must be called for every configuration
 * item found.
 */
static int
conf_file_load(struct conf_store *cs, conf_store_load_cb cb, void *cb_arg)
{
    struct conf_file *cf = (struct conf_file *)cs;
    FILE *file;
    uint32_t loc;
    char tmpbuf[CONF_MAX_NAME_LEN + CONF_MAX_VAL_LEN + 32];
    char *name_str;
    char *val_str;
    int rc;
    int lines;

    file = fopen(cf->cf_name, "r");
    if (!file) {
        return EINVAL;
    }

    loc = 0;
    lines = 0;
    while (1) {
        rc = conf_getnext_line(file, tmpbuf, sizeof(tmpbuf), &loc);
        if (loc == 0) {
            break;
        }
        if (rc < 0) {
            continue;
        }
        rc = conf_line_parse(tmpbuf, &name_str, &val_str);
        if (rc != 0) {
            continue;
        }
        lines++;
        cb(name_str, val_str, cb_arg);
    }
    fclose(file);
    cf->cf_lines = lines;
    return 0;
}

static void
conf_tmpfile(char *dst, const char *src, char *pfx)
{
    int len;
    int pfx_len;

    len = strlen(src);
    pfx_len = strlen(pfx);
    if (len + pfx_len >= CONF_FILE_NAME_MAX) {
        len = CONF_FILE_NAME_MAX - pfx_len - 1;
    }
    memcpy(dst, src, len);
    memcpy(dst + len, pfx, pfx_len);
    dst[len + pfx_len] = '\0';
}

/*
 * Try to compress configuration file by keeping unique names only.
 */
void
conf_file_compress(struct conf_file *cf)
{
    int rc;
    FILE *rf;
    FILE *wf;
    char tmp_file[CONF_FILE_NAME_MAX];
    char buf1[CONF_MAX_NAME_LEN + CONF_MAX_VAL_LEN + 32];
    char buf2[CONF_MAX_NAME_LEN + CONF_MAX_VAL_LEN + 32];
    uint32_t loc1, loc2;
    char *name1, *val1;
    char *name2, *val2;
    int copy;
    int len, len2;
    int lines;

    rf = fopen(cf->cf_name, "r");
    if (!rf) {
        return;
    }
    conf_tmpfile(tmp_file, cf->cf_name, ".cmp");
    wf = fopen(tmp_file, "w");
    if (!wf) {
        fclose(rf);
        return;
    }

    loc1 = 0;
    lines = 0;
    while (1) {
        len = conf_getnext_line(rf, buf1, sizeof(buf1), &loc1);
        if (loc1 == 0 || len < 0) {
            break;
        }
        rc = conf_line_parse(buf1, &name1, &val1);
        if (rc) {
            continue;
        }
        if (!val1) {
            continue;
        }
        loc2 = loc1;
        copy = 1;
        while ((len2 = conf_getnext_line(rf, buf2, sizeof(buf2), &loc2)) > 0) {
            rc = conf_line_parse(buf2, &name2, &val2);
            if (rc) {
                continue;
            }
            if (!strcmp(name1, name2)) {
                copy = 0;
                break;
            }
        }
        if (!copy) {
            continue;
        }

        /*
         * Can't find one. Must copy.
         */
        len = conf_line_make(buf2, sizeof(buf2), name1, val1);
        if (len < 0 || len + 2 > sizeof(buf2)) {
            continue;
        }
        buf2[len++] = '\n';
        fwrite(buf2, 1, len, wf);
        lines++;
    }
    fclose(wf);
    fclose(rf);
    unlink(cf->cf_name);
    rename(tmp_file, cf->cf_name);
    cf->cf_lines = lines;
    /*
     * XXX at conf_file_load(), look for .cmp if actual file does not
     * exist.
     */
}

/*
 * Called to save configuration.
 */
static int
conf_file_save(struct conf_store *cs, const char *name, const char *value)
{
    struct conf_file *cf = (struct conf_file *)cs;
    FILE *file;
    char buf[CONF_MAX_NAME_LEN + CONF_MAX_VAL_LEN + 32];
    int len;
    int rc;

    if (!name) {
        return EINVAL;
    }

    if (cf->cf_maxlines && (cf->cf_lines + 1 >= cf->cf_maxlines)) {
        /*
         * Compress before config file size exceeds the max number of lines.
         */
        conf_file_compress(cf);
    }
    len = conf_line_make(buf, sizeof(buf), name, value);
    if (len < 0 || len + 2 > sizeof(buf)) {
        return EINVAL;
    }
    buf[len++] = '\n';

    /*
     * Open the file to add this one value.
     */
    file = fopen(cf->cf_name, "a");
    if (!file) {
        return EINVAL;
    }
    if (fwrite(buf, 1, len, file)) {
        rc = EINVAL;
    } else {
        rc = 0;
        cf->cf_lines++;
    }
    fclose(file);
    return rc;
}
