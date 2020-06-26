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

#ifndef __PANMASTER_PRIV_H_
#define __PANMASTER_PRIV_H_

#include <stdint.h>
#include "syscfg/syscfg.h"

#define PANM_MAX_ROW_LEN     (32*3+4+4+2+5) /* max length for panm node-row */
#define PANM_FILE_NAME_MAX   (32)           /* max length for panm filename */

#define PANM_FIELD_SEPARATOR ","
#define PANMASTER_NODE_NUM_FIELDS (10)

struct panm_file {
    const char *pf_name;                /* filename */
    int pf_maxlines;                    /* max # of lines before compressing */
    int pf_lines;                       /* private */
};

struct panmaster_node;

struct list_nodes_extract {
    struct panmaster_node *nodes;
    int index_off;
    int index_max;
};

#ifdef __cplusplus
extern "C" {
#endif

int panmaster_cli_register(void);

int panm_file_save(struct panm_file *pf, struct panmaster_node *node);
int panm_file_load_idx(struct panm_file *pf, struct panmaster_node_idx *nodes);
int panm_file_find_node(struct panm_file *pf, struct find_node_s *fns);
void panm_nffs_load(struct panm_file *file, struct panmaster_node_idx *node_idx);
int panm_file_load(struct panm_file *pf, panm_load_cb cb, void* cb_arg);
void panm_file_compress(struct panm_file *file, struct panmaster_node_idx *node_idx);


#ifdef __cplusplus
}
#endif

#endif /* __PANMASTER_PRIV_H */
