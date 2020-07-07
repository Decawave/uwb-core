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

#include "pan_utils/pan_utils.h"

void
lease_expiry_cb(struct dpl_event * ev)
{
    assert(ev != NULL);
    assert(dpl_event_get_arg(ev));

    struct uwb_pan_instance * pan = (struct uwb_pan_instance *) dpl_event_get_arg(ev);
#if !MYNEWT_VAL(SELFTEST)
    STATS_INC(g_stat, lease_expiry);
#endif
    pan->status.valid = false;
    pan->status.lease_expired = true;
    pan->dev_inst->slot_id = 0xffff;

    /*DIAGMSG("{\"utime\": %lu,\"msg\": \"pan_lease_expired\"}\n",dpl_cputime_ticks_to_usecs(dpl_cputime_get32()));*/
    if (pan->control.postprocess) {
        dpl_eventq_put(&pan->dev_inst->eventq, &pan->postprocess_event);
    }
}

/**
 * @fn uwb_pan_set_request_cb(struct uwb_pan_instance * inst, uwb_pan_request_cb_func_t * callback)
 * @brief API to set request callback.
 *
 * @param inst       Pointer to struct uwb_pan_instance.
 * @param callback   Pointer uwb_pan_request_cb_func_t
 *
 * @return void
 */
void
uwb_pan_set_request_cb(struct uwb_pan_instance *pan, uwb_pan_request_cb_func_t callback)
{
    pan->request_cb = callback;
    pan->control.request_cb = true;
}

/**
 * @fn uwb_pan_set_postprocess(struct uwb_dev * inst, dpl_event_fn * pan_postprocess)
 * @brief API to set pan_postprocess.
 *
 * @param inst              Pointer to struct uwb_dev.
 * @param pan_postprocess   Pointer to dpl_event_fn.
 *
 * @return void
 */
void
uwb_pan_set_postprocess(struct uwb_pan_instance *pan, dpl_event_fn * cb)
{
    dpl_event_init(&pan->postprocess_event, cb, (void *) pan);
    dpl_callout_init(&pan->pan_lease_callout_expiry, dpl_eventq_dflt_get(),
                    lease_expiry_cb, (void *) pan);

    pan->control.postprocess = true;
}
