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
 * KIND, either expess or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <uwb/uwb.h>
#include <unistd.h>
#include <panmaster/panmaster.h>
#include "panmaster_test.h"

#define PAN_OS_TEST_STACK_SIZE      256
#define PAN_OS_TEST_APP_STACK_SIZE  256

#define PAN_OS_TEST_APP_PRIO         9
#define PAN_OS_TEST_TASK_PRIO        10

static struct os_task pan_os_test_task;
static struct os_task pan_os_test_app_task;
static os_stack_t pan_os_test_stack[OS_STACK_ALIGN(PAN_OS_TEST_STACK_SIZE)];

static os_stack_t
pan_os_test_app_stack[OS_STACK_ALIGN(PAN_OS_TEST_APP_STACK_SIZE)];

static void pan_os_test_app_task_handler(void *arg);

static void
pan_os_test_init_app_task(void)
{
    int rc;

    rc = os_task_init(&pan_os_test_app_task,
                      "pan_terminate_test_task",
                      pan_os_test_app_task_handler, NULL,
                      PAN_OS_TEST_APP_PRIO, OS_WAIT_FOREVER,
                      pan_os_test_app_stack,
                      OS_STACK_ALIGN(PAN_OS_TEST_APP_STACK_SIZE));
    TEST_ASSERT_FATAL(rc == 0);
}

static void
pan_os_test_misc_init(void)
{
    extern os_time_t g_os_time;

    /* Allow the OS to approach tick rollover.  This will help ensure host
     * timers don't break when the tick counter resets.
     */
    g_os_time = UINT32_MAX - 10 * OS_TICKS_PER_SEC;

    /* Receive acknowledgements for the startup sequence.  We sent the
     * corresponding requests when the host task was started.
     */

    pan_os_test_init_app_task();
}

static void
pan_os_test_app_task_handler(void *arg)
{
    while (1) {
        os_eventq_run(os_eventq_dflt_get());
    }
}

static void
pan_set_callbacks_test_task_handler(void *arg)
{
    int rc;
    struct uwb_pan_instance *test_pan = calloc(sizeof(*test_pan), 1);

    uwb_pan_set_request_cb(test_pan, panrequest_cb);
    uwb_pan_set_postprocess(test_pan, postprocess_cb);

    struct pan_req_resp *req_1 = calloc(sizeof(*req_1), 1);
    struct pan_req_resp *rsp_1 = calloc(sizeof(*rsp_1), 1);

    req_1->short_address = 0x29A;
    req_1->role = UWB_PAN_ROLE_MASTER;
    req_1->slot_id = 1;

    rc = test_pan->request_cb(0x29A, req_1, rsp_1);
    panmaster_postprocess();
    TEST_ASSERT(rc);

    rc = test_pan->request_cb(0x29A, req_1, rsp_1);
    TEST_ASSERT(rc);

    tu_restart();
}

static void
pan_add_nodes_test_task_handler(void *arg)
{
    int rc;
    struct uwb_pan_instance *test_pan = calloc(sizeof(*test_pan), 1);

    uwb_pan_set_request_cb(test_pan, panrequest_cb);
    uwb_pan_set_postprocess(test_pan, postprocess_cb);

    struct pan_req_resp *req_1 = calloc(sizeof(*req_1), 1);
    struct pan_req_resp *rsp_1 = calloc(sizeof(*rsp_1), 1);

    struct pan_req_resp *req_2 = calloc(sizeof(*req_2), 1);
    struct pan_req_resp *rsp_2 = calloc(sizeof(*rsp_2), 1);

    struct pan_req_resp *req_3 = calloc(sizeof(*req_3), 1);
    struct pan_req_resp *rsp_3 = calloc(sizeof(*rsp_3), 1);

    req_1->short_address = 0x29A;

    req_2->short_address = 0x29B;

    req_3->short_address = 0x29C;

    rc = test_pan->request_cb(0x29A, req_1, rsp_1);
    panmaster_postprocess();
    TEST_ASSERT(rc);
    TEST_ASSERT(rsp_1->short_address == req_1->short_address);

    rc = test_pan->request_cb(0x29B, req_2, rsp_2);
    panmaster_postprocess();
    TEST_ASSERT(rc);
    TEST_ASSERT(rsp_2->short_address == req_2->short_address);

    rc = test_pan->request_cb(0x29C, req_3, rsp_3);
    panmaster_postprocess();
    TEST_ASSERT(rc);
    TEST_ASSERT(rsp_3->short_address == req_3->short_address);

    tu_restart();
}

static void
pan_node_roles_test_task_handler(void *arg)
{
    int rc;
    struct uwb_pan_instance *test_pan = calloc(sizeof(*test_pan), 1);

    uwb_pan_set_request_cb(test_pan, panrequest_cb);
    uwb_pan_set_postprocess(test_pan, postprocess_cb);

    struct pan_req_resp *req_1 = calloc(sizeof(*req_1), 1);
    struct pan_req_resp *rsp_1 = calloc(sizeof(*rsp_1), 1);

    struct pan_req_resp *req_2 = calloc(sizeof(*req_2), 1);
    struct pan_req_resp *rsp_2 = calloc(sizeof(*rsp_2), 1);

    struct pan_req_resp *req_3 = calloc(sizeof(*req_3), 1);
    struct pan_req_resp *rsp_3 = calloc(sizeof(*rsp_3), 1);

    req_1->short_address = 0x29A;
    req_1->role = UWB_PAN_ROLE_MASTER;

    req_2->short_address = 0x29B;
    req_2->role = UWB_PAN_ROLE_RELAY;

    req_3->short_address = 0x29C;
    req_3->role = UWB_PAN_ROLE_SLAVE;

    rc = test_pan->request_cb(0x29A, req_1, rsp_1);
    panmaster_postprocess();
    TEST_ASSERT(rc);
    TEST_ASSERT(rsp_1->role == req_1->role);

    rc = test_pan->request_cb(0x29B, req_2, rsp_2);
    panmaster_postprocess();
    TEST_ASSERT(rc);
    TEST_ASSERT(rsp_2->role == req_2->role);

    rc = test_pan->request_cb(0x29C, req_3, rsp_3);
    panmaster_postprocess();
    TEST_ASSERT(rc);
    TEST_ASSERT(rsp_3->role == req_3->role);

    tu_restart();
}

static void
pan_lease_time_test_task_handler(void *arg)
{
    int rc;
    struct uwb_pan_instance *test_pan = calloc(sizeof(*test_pan), 1);

    uwb_pan_set_request_cb(test_pan, panrequest_cb);
    uwb_pan_set_postprocess(test_pan, postprocess_cb);

    struct pan_req_resp *req_1 = calloc(sizeof(*req_1), 1);
    struct pan_req_resp *rsp_1 = calloc(sizeof(*rsp_1), 1);

    req_1->short_address = 0x29A;
    req_1->lease_time = 0;

    rc = test_pan->request_cb(0x29A, req_1, rsp_1);
    panmaster_postprocess();
    TEST_ASSERT(rc);
    TEST_ASSERT(rsp_1->short_address == req_1->short_address);

    TEST_ASSERT(rsp_1->lease_time == MYNEWT_VAL(PANMASTER_DEFAULT_LEASE_TIME));

    tu_restart();
}

static void
pan_slot_id_test_task_handler(void *arg)
{
    int rc;
    struct uwb_pan_instance *test_pan = calloc(sizeof(*test_pan), 1);

    uwb_pan_set_request_cb(test_pan, panrequest_cb);
    uwb_pan_set_postprocess(test_pan, postprocess_cb);

    struct pan_req_resp *req_1 = calloc(sizeof(*req_1), 1);
    struct pan_req_resp *rsp_1 = calloc(sizeof(*rsp_1), 1);

    struct pan_req_resp *req_2 = calloc(sizeof(*req_2), 1);
    struct pan_req_resp *rsp_2 = calloc(sizeof(*rsp_2), 1);

    struct pan_req_resp *req_3 = calloc(sizeof(*req_3), 1);
    struct pan_req_resp *rsp_3 = calloc(sizeof(*rsp_3), 1);

    req_1->short_address = 0x29A;
    req_2->short_address = 0x29B;
    req_3->short_address = 0x29C;

    rc = test_pan->request_cb(0x29A, req_1, rsp_1);
    panmaster_postprocess();
    TEST_ASSERT(rc);
    TEST_ASSERT(rsp_1->slot_id == 0);

    rc = test_pan->request_cb(0x29B, req_2, rsp_2);
    panmaster_postprocess();
    TEST_ASSERT(rc);
    TEST_ASSERT(rsp_2->slot_id == 1);

    rc = test_pan->request_cb(0x29C, req_3, rsp_3);
    panmaster_postprocess();
    TEST_ASSERT(rc);
    TEST_ASSERT(rsp_3->slot_id == 2);

    tu_restart();
}

static void
pan_same_slot_id_test_task_handler(void *arg)
{
    int rc;
    struct uwb_pan_instance *test_pan = calloc(sizeof(*test_pan), 1);

    uwb_pan_set_request_cb(test_pan, panrequest_cb);
    uwb_pan_set_postprocess(test_pan, postprocess_cb);

    struct pan_req_resp *req_1 = calloc(sizeof(*req_1), 1);
    struct pan_req_resp *rsp_1 = calloc(sizeof(*rsp_1), 1);

    req_1->short_address = 0x29A;

    rc = test_pan->request_cb(0x29A, req_1, rsp_1);
    panmaster_postprocess();
    TEST_ASSERT(rc);
    TEST_ASSERT(rsp_1->slot_id == 0);

    rc = test_pan->request_cb(0x29A, req_1, rsp_1);
    panmaster_postprocess();
    TEST_ASSERT(rc);
    TEST_ASSERT(rsp_1->slot_id == 0);

    tu_restart();
}

static void
pan_os_lease_time_expire_test_task_handler(void *arg)
{
    int rc;
    struct uwb_pan_instance *test_pan = calloc(sizeof(*test_pan), 1);

    uwb_pan_set_request_cb(test_pan, panrequest_cb);
    uwb_pan_set_postprocess(test_pan, postprocess_cb);

    struct pan_req_resp *req_1 = calloc(sizeof(*req_1), 1);
    struct pan_req_resp *rsp_1 = calloc(sizeof(*rsp_1), 1);
    struct pan_req_resp *req_2 = calloc(sizeof(*req_2), 1);
    struct pan_req_resp *rsp_2 = calloc(sizeof(*rsp_2), 1);

    req_1->short_address = 0x29A;
    req_1->lease_time = 10;

    req_2->short_address = 0x29B;
    req_2->lease_time = 15;

    rc = test_pan->request_cb(0x29A, req_1, rsp_1);
    panmaster_postprocess();
    TEST_ASSERT(rc);
    TEST_ASSERT(rsp_1->slot_id == 0);

    os_time_delay(OS_TICKS_PER_SEC * 15);

    rc = test_pan->request_cb(0x29B, req_2, rsp_2);
    panmaster_postprocess();
    TEST_ASSERT(rc);
    TEST_ASSERT(rsp_2->slot_id == 0);

    os_time_delay(OS_TICKS_PER_SEC * 5);

    rc = test_pan->request_cb(0x29B, req_2, rsp_2);
    panmaster_postprocess();
    TEST_ASSERT(rc);
    TEST_ASSERT(rsp_2->slot_id == 0);

    tu_restart();
}

TEST_CASE_SELF(pan_os_add_nodes)
{
    pan_os_test_misc_init();

    os_task_init(&pan_os_test_task,
                "pan_os_set_callbacks_test_task",
                pan_add_nodes_test_task_handler, NULL,
                PAN_OS_TEST_TASK_PRIO, OS_WAIT_FOREVER, pan_os_test_stack,
                OS_STACK_ALIGN(PAN_OS_TEST_STACK_SIZE));
    
    os_start();
}

TEST_CASE_SELF(pan_os_set_callbacks)
{
    pan_os_test_misc_init();

    os_task_init(&pan_os_test_task,
                "pan_os_set_callbacks_test_task",
                pan_set_callbacks_test_task_handler, NULL,
                PAN_OS_TEST_TASK_PRIO, OS_WAIT_FOREVER, pan_os_test_stack,
                OS_STACK_ALIGN(PAN_OS_TEST_STACK_SIZE));
    
    os_start();
}

TEST_CASE_SELF(pan_os_node_roles)
{
    pan_os_test_misc_init();

    os_task_init(&pan_os_test_task,
                 "pan_os_check_node_roles_test_task",
                 pan_node_roles_test_task_handler, NULL,
                 PAN_OS_TEST_TASK_PRIO, OS_WAIT_FOREVER, pan_os_test_stack,
                 OS_STACK_ALIGN(PAN_OS_TEST_STACK_SIZE));

    os_start();
}

TEST_CASE_SELF(pan_os_lease_time)
{
    pan_os_test_misc_init();

    os_task_init(&pan_os_test_task,
                 "pan_os_test_lease_time",
                 pan_lease_time_test_task_handler, NULL,
                 PAN_OS_TEST_TASK_PRIO, OS_WAIT_FOREVER, pan_os_test_stack,
                 OS_STACK_ALIGN(PAN_OS_TEST_STACK_SIZE));

    os_start();
}

TEST_CASE_SELF(pan_os_slot_id)
{
    pan_os_test_misc_init();

    os_task_init(&pan_os_test_task,
                 "pan_os_test_slot_id",
                 pan_slot_id_test_task_handler, NULL,
                 PAN_OS_TEST_TASK_PRIO, OS_WAIT_FOREVER, pan_os_test_stack,
                 OS_STACK_ALIGN(PAN_OS_TEST_STACK_SIZE));

    os_start();
}

TEST_CASE_SELF(pan_os_same_slot_id)
{
    pan_os_test_misc_init();

    os_task_init(&pan_os_test_task,
                 "pan_os_test_slot_id",
                 pan_same_slot_id_test_task_handler, NULL,
                 PAN_OS_TEST_TASK_PRIO, OS_WAIT_FOREVER, pan_os_test_stack,
                 OS_STACK_ALIGN(PAN_OS_TEST_STACK_SIZE));

    os_start();
}

TEST_CASE_SELF(pan_os_lease_time_expire)
{
    pan_os_test_misc_init();

    os_task_init(&pan_os_test_task,
                 "pan_os_test_slot_id",
                 pan_os_lease_time_expire_test_task_handler, NULL,
                 PAN_OS_TEST_TASK_PRIO, OS_WAIT_FOREVER, pan_os_test_stack,
                 OS_STACK_ALIGN(PAN_OS_TEST_STACK_SIZE));

    os_start();
}
