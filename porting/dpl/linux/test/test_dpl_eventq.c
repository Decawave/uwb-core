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
  Unit tests for the dpl_eventq api:

  void dpl_eventq_init(struct dpl_eventq *);
  void dpl_eventq_put(struct dpl_eventq *, struct dpl_event *);
  struct dpl_event *dpl_eventq_get_no_wait(struct dpl_eventq *evq);
  struct dpl_event *dpl_eventq_get(struct dpl_eventq *);
  void dpl_eventq_run(struct dpl_eventq *evq);
  struct dpl_event *dpl_eventq_poll(struct dpl_eventq **, int, dpl_time_t);
  void dpl_eventq_remove(struct dpl_eventq *, struct dpl_event *);
  struct dpl_eventq *dpl_eventq_dflt_get(void);
*/

#include <assert.h>
#include <pthread.h>
#include "test_util.h"
#include "dpl/dpl.h"

#define TEST_ARGS_VALUE  (55)
#define TEST_STACK_SIZE  (1024)

static bool                   s_tests_running = true;
static struct dpl_task    s_task_runner;
static struct dpl_task    s_task_dispatcher;

static struct dpl_eventq  s_eventq;
static struct dpl_event   s_event;
static int                    s_event_args = TEST_ARGS_VALUE;


void on_event(struct dpl_event *ev)
{
    VerifyOrQuit(ev->ev_arg == &s_event_args,
		 "callout: wrong args passed");

    VerifyOrQuit(*(int*)ev->ev_arg == TEST_ARGS_VALUE,
		 "callout: args corrupted");

    s_tests_running = false;
}

int test_init()
{
    //VerifyOrQuit(!dpl_eventq_inited(&s_eventq), "eventq: empty q initialized");
    dpl_eventq_init(&s_eventq);
    //VerifyOrQuit(dpl_eventq_inited(&s_eventq), "eventq: not initialized");

    return PASS;
}

int test_run()
{
    while (s_tests_running)
    {
        dpl_eventq_run(&s_eventq);
    }

    return PASS;
}

int test_put()
{
    s_event.ev_cb = on_event;
    s_event.ev_arg = &s_event_args;
    dpl_eventq_put(&s_eventq, &s_event);
    return PASS;
}

int test_get_no_wait()
{
    //struct dpl_event *ev = dpl_eventq_get_no_wait(&s_eventq);
    return FAIL;
}

int test_get()
{
    struct dpl_event *ev = dpl_eventq_get(&s_eventq,
						  DPL_WAIT_FOREVER);
    VerifyOrQuit(ev == &s_event,
		 "callout: wrong event passed");

    return PASS;
}


void *task_test_runner(void *args)
{
    int count = 1000000000;

    SuccessOrQuit(test_init(), "eventq_init failed");
    SuccessOrQuit(test_put(),  "eventq_put failed");
    SuccessOrQuit(test_get(),  "eventq_get failed");
    SuccessOrQuit(test_put(),  "eventq_put failed");
    SuccessOrQuit(test_run(),  "eventq_run failed");

    printf("All tests passed\n");
    exit(PASS);

    return NULL;
}

int main(void)
{
    SuccessOrQuit(dpl_task_init(&s_task_runner,
			      "task_test_runner",
			      task_test_runner,
			      NULL, 1, 0, NULL, 0),
		          "task: error initializing");

    while (1) {}
}
