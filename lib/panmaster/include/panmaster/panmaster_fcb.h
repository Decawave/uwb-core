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
#ifndef __PANMASTER_FCB_H_
#define __PANMASTER_FCB_H_

#include "panmaster/panmaster.h"

#ifdef __cplusplus
extern "C" {
#endif

struct panm_fcb {
    struct fcb pm_fcb;
};

int panm_fcb_src(struct panm_fcb *pm);
int panm_fcb_load_idx(struct panm_fcb *pm, struct panmaster_node_idx *nodes);
int panm_fcb_find_node(struct panm_fcb *pf, struct find_node_s *fns);
int panm_fcb_save(struct panm_fcb *pm, struct panmaster_node *node);
int panm_fcb_clear(struct panm_fcb *pm);
int panm_fcb_load(struct panm_fcb *pm, panm_load_cb cb, void *cb_arg);
void panm_fcb_compress(struct panm_fcb *pm);
void panm_fcb_sort(struct panm_fcb *pm);

#ifdef __cplusplus
}
#endif

#endif /* __SYS_CONFIG_FCB_H_ */
