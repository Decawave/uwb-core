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
  Unit tests for the Semaphore api (dpl_sem):
*/

#include "test_util.h"
#include "dpl/dpl.h"
#include "os/os.h"

#define TEST_ITERATIONS   100

#define TASK1_PRIO        1
#define TASK2_PRIO        1

#define TASK1_STACK_SIZE  1028
#define TASK2_STACK_SIZE  1028

static struct dpl_task    task1;
static struct dpl_task    task2;

static dpl_stack_t task1_stack[TASK1_STACK_SIZE];
static dpl_stack_t task2_stack[TASK2_STACK_SIZE];

struct dpl_sem task1_sem;
struct dpl_sem task2_sem;

/* Task 1 handler function */
void *
task1_handler(void *arg)
{
    for (int i = 0; i < TEST_ITERATIONS; i++)
    {
	/* Release semaphore to task 2 */
        SuccessOrQuit(dpl_sem_release(&task1_sem),
		      "dpl_sem_release: error releasing task2_sem.");

	/* Wait for semaphore from task 2 */
        SuccessOrQuit(dpl_sem_pend(&task2_sem, DPL_WAIT_FOREVER),
		      "dpl_sem_pend: error waiting for task2_sem.");
    }

    printf("All tests passed\n");
    exit(PASS);

    return NULL;
}

/* Task 2 handler function */
void *
task2_handler(void *arg)
{
    while(1)
    {
        /* Wait for semaphore from task1 */
        SuccessOrQuit(dpl_sem_pend(&task1_sem, DPL_WAIT_FOREVER),
		      "dpl_sem_pend: error waiting for task1_sem.");

	    /* Release task2 semaphore */
        SuccessOrQuit(dpl_sem_release(&task2_sem),
		      "dpl_sem_release: error releasing task1_sem.");
    }

    return NULL;
}


/* Initialize task 1 exposed data objects */
void
task1_init(void)
{
    /* Initialize task1 semaphore */
    SuccessOrQuit(dpl_sem_init(&task1_sem, 0),
		  "dpl_sem_init: task1 returned error.");
}

/* Initialize task 2 exposed data objects */
void
task2_init(void)
{
    /* Initialize task1 semaphore */
    SuccessOrQuit(dpl_sem_init(&task2_sem, 0),
		  "dpl_sem_init: task2 returned error.");
}

/**
 * init_app_tasks
 *
 * This function performs initializations that are required before tasks run.
 *
 * @return int 0 success; error otherwise.
 */
static int
init_app_tasks(void)
{
    /*
     * Call task specific initialization functions to initialize any shared objects
     * before initializing the tasks with the OS.
     */
    task1_init();
    task2_init();

    /*
     * Initialize tasks 1 and 2 with the OS.
     */
    dpl_task_init(&task1, "task1", task1_handler, NULL, TASK1_PRIO,
		 DPL_WAIT_FOREVER, task1_stack, TASK1_STACK_SIZE);

    dpl_task_init(&task2, "task2", task2_handler, NULL, TASK2_PRIO,
		 DPL_WAIT_FOREVER, task2_stack, TASK2_STACK_SIZE);

    return 0;
}

/**
 * main
 *
 * The main function for the application. This function initializes the system and packages,
 * calls the application specific task initialization function, then waits and dispatches
 * events from the OS default event queue in an infinite loop.
 */
int
main(int argc, char **arg)
{
    /* Initialize application specific tasks */
    init_app_tasks();

    while (1)
    {
        dpl_eventq_run(dpl_eventq_dflt_get());
    }
    /* main never returns */
}
