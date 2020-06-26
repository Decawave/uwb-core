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

#include <dpl/dpl.h>
#include <dpl/dpl_tasks.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize a task.  -NOT USED IN KERNEL in favour of workqueues instead
 *
 * This function initializes the task structure pointed to by t,
 * clearing and setting it's stack pointer, provides sane defaults
 * and sets the task as ready to run, and inserts it into the operating
 * system scheduler.
 *
 * @param t The task to initialize
 * @param name The name of the task to initialize
 * @param func The task function to call
 * @param arg The argument to pass to this task function
 * @param prio The priority at which to run this task
 * @param sanity_itvl The time at which this task should check in with the
 *                    sanity task.  OS_WAIT_FOREVER means never check in
 *                    here.
 * @param stack_bottom A pointer to the bottom of a task's stack
 * @param stack_size The overall size of the task's stack.
 *
 * @return 0 on success, non-zero on failure.
 */
int
dpl_task_init(struct dpl_task *t, const char *name, dpl_task_func_t func,
        void *arg, uint8_t prio, dpl_time_t sanity_itvl,
        dpl_stack_t *stack_bottom, uint16_t stack_size)
{
    if ((t == NULL) || (func == NULL)) {
        return DPL_INVALID_PARAM;
    }
    t->name = name;
    return 0;
}

int
dpl_task_remove(struct dpl_task *t)
{
    return 0;
}

/**
 * Return the number of tasks initialized.
 *
 * @return number of tasks initialized
 */
uint8_t
dpl_task_count(void)
{
    return 0;
}

bool dpl_os_started(void)
{
    return true;
}

void dpl_task_yield(void)
{
    /* Dummy */
}

#ifdef __cplusplus
}
#endif
