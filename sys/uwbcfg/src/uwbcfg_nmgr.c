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


#include <syscfg/syscfg.h>

#if MYNEWT_VAL(UWBCFG_NMGR)

#include <limits.h>
#include <assert.h>
#include <string.h>
#include "os/mynewt.h"
#include "hal/hal_bsp.h"
#include "mgmt/mgmt.h"
#include "cborattr/cborattr.h"

#include <console/console.h>
#include <config/config.h>
#include <uwbcfg/uwbcfg.h>
#include "uwbcfg_priv.h"


static int uwbcfg_write(struct mgmt_cbuf *);
//static int uwbcfg_read(struct mgmt_cbuf *);

static const struct mgmt_handler uwbcfg_nmgr_handlers[] = {
    [0] = {
        .mh_write = uwbcfg_write
    }
};

#define UWBCFG_HANDLER_CNT                                                \
    sizeof(uwbcfg_nmgr_handlers) / sizeof(uwbcfg_nmgr_handlers[0])

static struct mgmt_group uwbcfg_nmgr_group = {
    .mg_handlers = (struct mgmt_handler *)uwbcfg_nmgr_handlers,
    .mg_handlers_count = UWBCFG_HANDLER_CNT,
    .mg_group_id = MGMT_GROUP_ID_UWBCFG,
};

static int
uwbcfg_write(struct mgmt_cbuf *cb)
{
    uint64_t fields = UINT_MAX;
    bool do_save = false;
    int count = 0;

    char *value_ptrs[CFGSTR_MAX] = {0};
    char values[CFGSTR_MAX*CFGSTR_STRLEN] = {0};

    const struct cbor_attr_t off_attr[] = {
        [0] = {
            .attribute = "cfgs",
            .type = CborAttrArrayType,
            .addr.array.element_type = CborAttrTextStringType,
            .addr.array.arr.strings.ptrs = value_ptrs,
            .addr.array.arr.strings.store = values,
            .addr.array.arr.strings.storelen = sizeof(values),
            .addr.array.count = &count,
            .addr.array.maxlen = CFGSTR_MAX
        },
        [1] = {
            .attribute = "flds",
            .type = CborAttrUnsignedIntegerType,
            .addr.uinteger = &fields,
            .nodefault = true
        },
        [2] = {
            .attribute = "save",
            .type = CborAttrBooleanType,
            .addr.boolean = &do_save,
            .nodefault = true
        },
        [3] = { 0 },
    };

    CborError g_err = CborNoError;
    int rc = cbor_read_object(&cb->it, off_attr);
    if (rc || fields == UINT_MAX) {
        UC_ERR("read_failed rc %d fields %llx\n", rc, fields);
        return MGMT_ERR_EINVAL;
    }

    UC_DEBUG("uwbcfg: flds:%lx count:%d s:%d\n",
             (uint32_t)fields, count, do_save);

    int n=0;
    for (int i=0;i<CFGSTR_MAX && n<count;i++) {
        if (fields&0x1) {
            UC_DEBUG("  [i%d][n%d]: '%s'->'%s'\n", i, n, g_uwbcfg_str[i],value_ptrs[n]);
            uwbcfg_internal_set(i, value_ptrs[n]);
            n++;
        }
        fields = (fields >> 1);
    }

    uwbcfg_commit();
    conf_save();

    goto out;
out:
    g_err |= cbor_encode_text_stringz(&cb->encoder, "rc");
    g_err |= cbor_encode_int(&cb->encoder, MGMT_ERR_EOK);

    if (g_err) {
        return MGMT_ERR_ENOMEM;
    }
    return 0;
}


void
uwbcfg_nmgr_module_init(void)
{
    int rc;

    /* Ensure this function only gets called by sysinit. */
    SYSINIT_ASSERT_ACTIVE();

    rc = mgmt_group_register(&uwbcfg_nmgr_group);
    SYSINIT_PANIC_ASSERT(rc == 0);
}

#endif // MYNEWT_VAL(UWBCFG_NMGR)
