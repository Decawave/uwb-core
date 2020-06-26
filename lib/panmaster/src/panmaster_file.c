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

#include <os/os.h>
#include <string.h>
#include <stdio.h>
#include <syscfg/syscfg.h>
#if MYNEWT_VAL(PANMASTER_NFFS)

#include <fs/fs.h>
#include <panmaster/panmaster.h>
#include "panmaster_priv.h"


static int
panm_line_parse(char *buf, struct panmaster_node *node)
{
    int i;
    int num_fields;
    char *tok;
    char *tok_ptr;
    char *sep = PANM_FIELD_SEPARATOR;
    char *fields[PANMASTER_NODE_NUM_FIELDS+2];

    tok = strtok_r(buf, sep, &tok_ptr);

    i = 0;
    while (tok) {
        fields[i++] = tok;
        tok = strtok_r(NULL, sep, &tok_ptr);
    }
    num_fields = i;
    if (num_fields < PANMASTER_NODE_NUM_FIELDS)
    {
        printf("[%d < %d] %s\n", num_fields, PANMASTER_NODE_NUM_FIELDS, buf);
        return 1;
    }
    i=0;
    node->index = strtoll(fields[i++], NULL, 16);
    node->first_seen_utc = strtoll(fields[i++], NULL, 16);
    node->euid  = strtoll(fields[i++], NULL, 16);
    node->addr  = strtol(fields[i++], NULL, 16);
    node->flags = strtol(fields[i++], NULL, 16);
    node->role = strtol(fields[i++], NULL, 16);

    node->fw_ver.iv_major = strtol(fields[i++], NULL, 16);
    node->fw_ver.iv_minor = strtol(fields[i++], NULL, 16);
    node->fw_ver.iv_revision = strtol(fields[i++], NULL, 16);
    node->fw_ver.iv_build_num = strtol(fields[i++], NULL, 16);

    return 0;
}

static int
panm_line_make(char *dst, int dlen, struct panmaster_node *node)
{
    int off;

    off = snprintf(dst, dlen, "%X,", node->index);
    if (dlen - off < 0) {
        return -1;
    }
    off+= snprintf(dst+off, dlen-off, "%llX,", node->first_seen_utc);
    if (dlen - off < 0) {
        return -1;
    }
    off+= snprintf(dst+off, dlen-off, "%llX,", node->euid);
    if (dlen - off < 0) {
        return -1;
    }
    off+= snprintf(dst+off, dlen-off, "%X,", node->addr);
    if (dlen - off < 0) {
        return -1;
    }
    off+= snprintf(dst+off, dlen-off, "%X,", node->flags);
    if (dlen - off < 0) {
        return -1;
    }
    off+= snprintf(dst+off, dlen-off, "%X,", node->role);
    if (dlen - off < 0) {
        return -1;
    }
    off+= snprintf(dst+off, dlen-off, "%X,", node->fw_ver.iv_major);
    if (dlen - off < 0) {
        return -1;
    }
    off+= snprintf(dst+off, dlen-off, "%X,", node->fw_ver.iv_minor);
    if (dlen - off < 0) {
        return -1;
    }
    off+= snprintf(dst+off, dlen-off, "%X,", node->fw_ver.iv_revision);
    if (dlen - off < 0) {
        return -1;
    }
    off+= snprintf(dst+off, dlen-off, "%lX,", node->fw_ver.iv_build_num);
    if (dlen - off < 0) {
        return -1;
    }
    return off;
}


static int
panm_getnext_line(struct fs_file *file, char *buf, int blen, uint32_t *loc)
{
    int rc;
    char *end;
    uint32_t len;

    rc = fs_seek(file, *loc);
    if (rc < 0) {
        *loc = 0;
        return -1;
    }
    rc = fs_read(file, blen, buf, &len);
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


int
panm_file_load(struct panm_file *pf, panm_load_cb cb, void* cb_arg)
{
    struct fs_file *file;
    uint32_t loc;
    char tmpbuf[PANM_MAX_ROW_LEN+32];
    struct panmaster_node tmpnode;
    int rc;
    int lines;

    rc = fs_open(pf->pf_name, FS_ACCESS_READ, &file);
    if (rc != FS_EOK) {
        return OS_EINVAL;
    }

    loc = 0;
    lines = 0;
    while (1) {
        rc = panm_getnext_line(file, tmpbuf, sizeof(tmpbuf), &loc);
        if (loc == 0) {
            break;
        }
        if (rc < 0) {
            continue;
        }
        rc = panm_line_parse(tmpbuf, &tmpnode);
        if (rc != 0) {
            continue;
        }
        cb(&tmpnode, cb_arg);
        lines++;
    }
    fs_close(file);
    pf->pf_lines = lines;
    return OS_OK;
}
#if 0
static void
full_node_load_cb(struct panmaster_node *node, void *cb_arg)
{
    struct panmaster_node *nodes = (struct panmaster_node*)cb_arg;

    if (node->index <= MYNEWT_VAL(PANMASTER_MAXNUM_NODES)) {
        memcpy(&(nodes[node->index]), node, sizeof(struct panmaster_node));
    }
}
#endif

static void
node_idx_load_cb(struct panmaster_node *node, void *cb_arg)
{
    struct panmaster_node_idx *nodes = (struct panmaster_node_idx*)cb_arg;

    if (node->index <= MYNEWT_VAL(PANMASTER_MAXNUM_NODES) &&
        node->addr != 0xffff) {
        nodes[node->index].addr = node->addr;
    }
}

int
panm_file_load_idx(struct panm_file *pf, struct panmaster_node_idx *nodes)
{
    return panm_file_load(pf, node_idx_load_cb, (void*)nodes);
}


static void
find_node_load_cb(struct panmaster_node *node, void *cb_arg)
{
    struct find_node_s *fn = (struct find_node_s*)cb_arg;

    if ((fn->find.index != 0     && fn->find.index == node->index) ||
        (fn->find.addr != 0xffff && fn->find.addr == node->addr) ||
        (fn->find.euid != 0xffff && fn->find.euid == node->euid)) {
        memcpy(fn->results, node, sizeof(struct panmaster_node));
        fn->is_found = 1;
    }
}

int
panm_file_find_node(struct panm_file *pf, struct find_node_s *fns)
{
    fns->is_found = 0;
    return panm_file_load(pf, find_node_load_cb, (void*)fns);
}

int
panm_file_save(struct panm_file *pf, struct panmaster_node *node)
{
    struct fs_file *file;
    char buf[PANM_MAX_ROW_LEN+32];
    int len;
    int rc;

    if (!node) {
        return OS_INVALID_PARM;
    }

    len = panm_line_make(buf, sizeof(buf), node);
    if (len < 0 || len + 2 > sizeof(buf)) {
        return OS_INVALID_PARM;
    }
    buf[len++] = '\n';
    /*
     * Open the file to add this one value.
     */
    if (fs_open(pf->pf_name, FS_ACCESS_WRITE | FS_ACCESS_APPEND, &file)) {
        return OS_EINVAL;
    }
    if (fs_write(file, buf, len)) {
        rc = OS_EINVAL;
    } else {
        rc = 0;
        pf->pf_lines++;
    }
    fs_close(file);
    return rc;
}

void
panm_file_compress(struct panm_file *file, struct panmaster_node_idx *node_idx)
{
    int i;
    struct panmaster_node node;
    struct find_node_s fns = { .results = &node };
    static struct panm_file tmp_file = {
        .pf_name = "/pm/nds.tmp",
        .pf_maxlines = MYNEWT_VAL(PANMASTER_NFFS_MAX_LINES)
    };

    for (i=0;i<MYNEWT_VAL(PANMASTER_MAXNUM_NODES);i++) {
        if (node_idx[i].addr == 0xffff) {
            continue;
        }
        PANMASTER_NODE_DEFAULT(fns.find);
        fns.find.addr = node_idx[i].addr;
        panmaster_find_node_general(&fns);

        if (fns.is_found)
        {
            panm_file_save(&tmp_file, &node);
        }
    }

    fs_unlink(file->pf_name);
    fs_rename("/pm/nds.tmp", file->pf_name);
}

void
panm_nffs_load(struct panm_file *file, struct panmaster_node_idx *node_idx)
{
    //panm_file_load(pf, full_node_load_cb, (void*)nodes);

    /* Load from disk */
    panm_file_load_idx(file, node_idx);

    /* Check if we need to compress the node-file */
    if (file->pf_lines >
        file->pf_maxlines) {

        panm_file_compress(file, node_idx);
    }
}

#endif // MYNEWT_VAL(PANMASTER_NFFS)
