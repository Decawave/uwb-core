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

#ifndef _DPL_TASKS_H_
#define _DPL_TASKS_H_

#include <stddef.h>
#include <dpl/dpl_types.h>

#ifdef __cplusplus
extern "C" {
#endif
typedef void *(*dpl_task_func_t)(void *);

int dpl_task_init(struct dpl_task *t, const char *name, dpl_task_func_t func,
		 void *arg, uint8_t prio, dpl_time_t sanity_itvl,
		 dpl_stack_t *stack_bottom, uint16_t stack_size);

int dpl_task_remove(struct dpl_task *t);
uint8_t dpl_task_count(void);
void dpl_task_yield(void);


#ifdef __cplusplus
}
#endif

#endif  /* _DPL_TASKS_H_ */
