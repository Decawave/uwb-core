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

#include <os/os_time.h>
#include <syscfg/syscfg.h>
#include <datetime/datetime.h>

#if MYNEWT_VAL(BCAST_OTA_CLI)

#include <bcast_ota/bcast_ota.h>

#include "os/mynewt.h"
#include <string.h>

#include <defs/error.h>
#include <bootutil/image.h>
#include <flash_map/flash_map.h>
#include <hal/hal_bsp.h>
#include <console/console.h>

#include <shell/shell.h>
#include <console/console.h>


#include <tinycbor/cbor.h>
#include <tinycbor/cborjson.h>
#include <tinycbor/cbor_mbuf_writer.h>
#include <tinycbor/cbor_mbuf_reader.h>
#include <cborattr/cborattr.h>

#include <nmgr_os/nmgr_os.h>

#include <mgmt/mgmt.h>
#include <newtmgr/newtmgr.h>
#include <imgmgr/imgmgr.h>

#include "bcast_ota_priv.h"
#include <base64/hex.h>
#include <base64/base64.h>

#if MYNEWT_VAL(NMGR_UWB_ENABLED)
#include <nmgr_uwb/nmgr_uwb.h>
#endif
static struct nmgr_transport nmgr_mstr_transport;

static uint16_t
nmgr_mstr_get_mtu(struct os_mbuf *m)
{
    return 196;
}

static int
nmgr_mstr_out(struct nmgr_transport *nt, struct os_mbuf *req)
{
    int rc;
    int64_t rc_attr;
    CborError g_err = CborNoError;
    struct mgmt_cbuf n_b;
    struct cbor_mbuf_reader reader;

    struct cbor_attr_t attrs[] = {
        [0] = {
            .attribute = "rc",
            .type = CborAttrIntegerType,
            .addr.integer = &rc_attr,
            .nodefault = true
        },
        [1] = {
            .attribute = NULL
        }
    };
    rc = 0;
    cbor_mbuf_reader_init(&reader, req, sizeof(struct nmgr_hdr));
    cbor_parser_init(&reader.r, 0, &n_b.parser, &n_b.it);

    struct mgmt_cbuf *cb = &n_b;

    g_err |= cbor_read_object(&cb->it, attrs);
    if (g_err) {
        console_printf("gerr: '%d\n", g_err);
    }
    if (rc)
    {
        console_printf("nmgr_out: rc=%d\n", (int)(rc_attr&0xffffffff));
    }

#if MYNEWT_VAL(BCAST_OTA_DEBUG)
    printf("json:\n=========\n");
    CborValue it;
    CborParser p;
    cbor_mbuf_reader_init(&reader, req, sizeof(struct nmgr_hdr));
    cbor_parser_init(&reader.r, 0, &p, &it);

    cbor_value_to_json(stdout, &it,
                       CborConvertRequireMapStringKeys);
    printf("\n===========\n");
#endif

    os_mbuf_free_chain(req);
    return (rc);
}

static int bota_cli_cmd(int argc, char **argv);

#if MYNEWT_VAL(SHELL_CMD_HELP)
const struct shell_param cmd_bota_param[] = {
    {"check", "<fa_id>"},
    {"txim", "<addr> <fa_id>"},
    {"txrst", "<addr> tx reset cmd"},
    // {"txd", "<addr> <offset> <base64>"},
    {NULL,NULL},
};

const struct shell_cmd_help cmd_bota_help = {
	"bcast_ota commands", "<check>|<txim>", cmd_bota_param
};
#endif


static struct shell_cmd shell_bota_cmd = {
    .sc_cmd = "bota",
    .sc_cmd_func = bota_cli_cmd,
#if MYNEWT_VAL(SHELL_CMD_HELP)
    .help = &cmd_bota_help
#endif
};

#define TMPBUF_SZ  256
static int
check_image(const struct flash_area *fap)
{
    int rc;
    struct image_header hdr;
    uint8_t hash[IMGMGR_HASH_LEN];
    char hash_str[IMGMGR_HASH_LEN * 2 + 1];

    rc = flash_area_read(fap, 0, &hdr, sizeof(struct image_header));
    if (rc!=0) {
        return OS_ENOMEM;
    }

    void *tmpbuf = malloc(TMPBUF_SZ);
    if (!tmpbuf) {
        return OS_ENOMEM;
    }
    rc = bootutil_img_validate(&hdr, fap, tmpbuf, TMPBUF_SZ,
                               NULL, 0, hash);
    free(tmpbuf);

    console_printf("computed hash: %s\n",
                   hex_format(hash, IMGMGR_HASH_LEN, hash_str, sizeof(hash_str)));
    return rc;
}

int
check_image_fid(int fid)
{
    int rc;
    const struct flash_area *fa;

    rc = flash_area_open(fid, &fa);
    if (rc!=0) {
        return rc;
    }
    rc = check_image(fa);
    flash_area_close(fa);
    return rc;
}

static struct {
    int8_t reset;
    int8_t resend_end;
    uint16_t addr;
    int slot_id;
    int blocksize;
    struct os_callout callout;
    uint64_t flags;
} tx_im_inst = {0};

static void
txim_ev_cb(struct os_event *ev)
{
    struct os_mbuf *om = 0;
    /* Don't proceed unless there's plenty of room in the queue */
    if (os_msys_num_free() < os_msys_count()/4) {
        os_callout_reset(&tx_im_inst.callout, OS_TICKS_PER_SEC/10);
        return;
    }
    nmgr_uwb_instance_t *nmgruwb = (nmgr_uwb_instance_t*)uwb_mac_find_cb_inst_ptr(uwb_dev_idx_lookup(0), UWBEXT_NMGR_UWB);
    bcast_ota_get_packet(tx_im_inst.slot_id, (tx_im_inst.reset>0)?
                         BCAST_MODE_RESET_OFFSET : BCAST_MODE_NONE,
                         tx_im_inst.blocksize, &om, tx_im_inst.flags);
    if (om) {
        uwb_nmgr_queue_tx(nmgruwb, tx_im_inst.addr, UWB_DATA_CODE_NMGR_REQUEST, om);
        if (tx_im_inst.reset>0) {
            os_callout_reset(&tx_im_inst.callout, OS_TICKS_PER_SEC/5);
            tx_im_inst.reset--;
        } else {
            os_callout_reset(&tx_im_inst.callout, OS_TICKS_PER_SEC/50);
        }
        tx_im_inst.resend_end = 5;
    } else if (--tx_im_inst.resend_end > 0){
        printf("bota: resending end\n");
        bcast_ota_get_packet(tx_im_inst.slot_id, BCAST_MODE_RESEND_END,
                             (128-8), &om, tx_im_inst.flags);
        uwb_nmgr_queue_tx(nmgruwb, tx_im_inst.addr, UWB_DATA_CODE_NMGR_REQUEST, om);
        os_callout_reset(&tx_im_inst.callout, OS_TICKS_PER_SEC/4);
    } else {
        printf("bota: txim finished\n");
    }
}

static int
bota_cli_cmd(int argc, char **argv)
{
    int rc;
    int fa_id;

    if (argc < 2) {
        console_printf("Too few args\n");
        return 0;
    }
    if (!strcmp(argv[1], "check")) {
        if (argc < 3) {
            console_printf("pls provide fa id\n");
            console_printf("  im0: %d\n", flash_area_id_from_image_slot(0));
            console_printf("  im1: %d\n", flash_area_id_from_image_slot(1));
#if MYNEWT_VAL(BCAST_OTA_SCRATCH_ENABLED)
            console_printf("  lnota scratch: %d\n", MYNEWT_VAL(BCAST_OTA_FLASH_SCRATCH));
#endif
            return 0;
        }
        fa_id = strtol(argv[2], NULL, 0);
        rc = check_image_fid(fa_id);
        console_printf("rc=%d\n", rc);
    } else if (!strcmp(argv[1], "txim")) {
        if (argc < 4) {
            console_printf("pls provide <addr> and slot src id [0 or 1]\n");
            return 0;
        }
        tx_im_inst.addr = strtol(argv[2], NULL, 0);
        tx_im_inst.reset = 5;
        tx_im_inst.slot_id = strtol(argv[3], NULL, 0);
        tx_im_inst.blocksize = 256;
        tx_im_inst.flags = BOTA_FLAGS_SET_PERMANENT;
        os_callout_reset(&tx_im_inst.callout, 0);
    } else if (!strcmp(argv[1], "txrst")) {
        if (argc < 3) {
            console_printf("pls provide <addr>\n");
            return 0;
        }
        uint16_t addr = strtol(argv[2], NULL, 0);
        struct os_mbuf *om = bcast_ota_get_reset_mbuf();
        nmgr_uwb_instance_t *nmgruwb = (nmgr_uwb_instance_t*)uwb_mac_find_cb_inst_ptr(uwb_dev_idx_lookup(0), UWBEXT_NMGR_UWB);
        uwb_nmgr_queue_tx(nmgruwb, addr, UWB_DATA_CODE_NMGR_REQUEST, om);
    } else {
        console_printf("Unknown cmd\n");
    }
    return 0;
}

int
bota_cli_register(void)
{
    int rc;
    rc = nmgr_transport_init(&nmgr_mstr_transport, nmgr_mstr_out,
                             nmgr_mstr_get_mtu);
    assert(rc == 0);
    os_callout_init(&tx_im_inst.callout, os_eventq_dflt_get(), txim_ev_cb, NULL);

    return shell_cmd_register(&shell_bota_cmd);
}
#endif /* MYNEWT_VAL(BCAST_OTA_CLI) */
