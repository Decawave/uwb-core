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

#include <sys/mman.h>
#include <pthread.h>
#include <sched.h>
#include "dpl/dpl_tasks.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize a task.
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
    int err;
    pthread_attr_t          attr;
    struct sched_param      param;

    if ((t == NULL) || (func == NULL)) {
        return DPL_INVALID_PARAM;
    }
    err = pthread_attr_init(&t->attr);
    if (err) return err;

#if !defined(ANDROID_APK_BUILD)
    /* These features should not be used when integrating the library
       in an overall framework as they will lock the memory of the entire
       framework from being swapped out or moved. */
    err = mlockall(MCL_CURRENT|MCL_FUTURE);
    if (err) return err;
    err = pthread_attr_getschedparam (&t->attr, &t->param);
    if (err) return err;
    err = pthread_attr_setschedpolicy(&t->attr, SCHED_FIFO);
    if (err) return err;
    int max_prio = sched_get_priority_max(SCHED_FIFO);
    int min_prio = sched_get_priority_min(SCHED_FIFO);
    t->param.sched_priority = max_prio - prio - 10;
    if (t->param.sched_priority < min_prio) {
        t->param.sched_priority = min_prio;
    }
    err = pthread_attr_setschedparam (&t->attr, &t->param);
    if (err) return err;
#endif
    t->name = name;
    err = pthread_create(&t->handle, &t->attr, func, arg);

    return err;
}

/*
 * Removes specified task
 * XXX
 * NOTE: This interface is currently experimental and not ready for common use
 */
int
dpl_task_remove(struct dpl_task *t)
{
#if defined(ANDROID_APK_BUILD)
    return 0;
#else
    return pthread_cancel(t->handle);
#endif
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

void *
dpl_get_current_task_id(void)
{
    return (void *)pthread_self();
}

bool dpl_os_started(void)
{
    return true;
}

void dpl_task_yield(void)
{
    sched_yield();
}

#ifdef __cplusplus
}
#endif
