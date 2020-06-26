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
 * @file nmgr_uwb.c
 * @author UWB Core <uwbcore@gmail.com>
 * @date 2018
 * @brief Range
 *
 * @details UWB Transport Layer for NewtMgr
 *
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <os/os.h>
#include <hal/hal_spi.h>
#include <hal/hal_gpio.h>
#include <stats/stats.h>

#include <mgmt/mgmt.h>
#include <newtmgr/newtmgr.h>

#include <dpl/dpl.h>
#include <uwb/uwb.h>
#include <uwb/uwb_mac.h>
#include <uwb/uwb_ftypes.h>
#include <nmgr_uwb/nmgr_uwb.h>

//#define DIAGMSG(s,u) printf(s,u)
#ifndef DIAGMSG
#define DIAGMSG(s,u)
#endif

struct nmgr_uwb_usr_hdr{
    struct _nmgr_uwb_instance_t *nmgruwb_inst;
    nmgr_uwb_frame_header_t uwb_hdr;
}__attribute__((__packed__,aligned(1)));

static bool rx_complete_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs);
static bool tx_complete_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs);
static bool rx_timeout_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs);
static int nmgr_resp_cb(struct nmgr_transport *nt, struct os_mbuf *m);

static struct nmgr_transport uwb_transport_0;
#if MYNEWT_VAL(UWB_DEVICE_1)
static struct nmgr_transport uwb_transport_1;
#endif
#if MYNEWT_VAL(UWB_DEVICE_2)
static struct nmgr_transport uwb_transport_2;
#endif

static struct nmgr_transport*
uwb_transport(int idx) {
    if (idx == 0) return &uwb_transport_0;
#if MYNEWT_VAL(UWB_DEVICE_1)
    if (idx == 1) return &uwb_transport_1;
#endif
#if MYNEWT_VAL(UWB_DEVICE_2)
    if (idx == 2) return &uwb_transport_2;
#endif
    return 0;
}

static struct uwb_mac_interface g_cbs[] = {
        [0] = {
            .id = UWBEXT_NMGR_UWB,
            .rx_complete_cb = rx_complete_cb,
            .tx_complete_cb = tx_complete_cb,
            .rx_timeout_cb = rx_timeout_cb,
        },
#if MYNEWT_VAL(UWB_DEVICE_1)
        [1] = {
            .id = UWBEXT_NMGR_UWB,
            .rx_complete_cb = rx_complete_cb,
            .tx_complete_cb = tx_complete_cb,
            .rx_timeout_cb = rx_timeout_cb,
        },
#endif
#if MYNEWT_VAL(UWB_DEVICE_2)
        [2] = {
            .id = UWBEXT_NMGR_UWB,
            .rx_complete_cb = rx_complete_cb,
            .tx_complete_cb = tx_complete_cb,
            .rx_timeout_cb = rx_timeout_cb,
        }
#endif
};


uint16_t
nmgr_uwb_mtu(struct os_mbuf *m, int idx)
{
    struct uwb_dev* inst = uwb_dev_idx_lookup(idx);
    return (inst->config.rx.phrMode==DWT_PHRMODE_STD) ? NMGR_UWB_MTU_STD : NMGR_UWB_MTU_EXT;
}

static uint16_t
nmgr_uwb_mtu_0(struct os_mbuf *m)
{
    return nmgr_uwb_mtu(m, 0);
}


#if MYNEWT_VAL(UWB_DEVICE_1)
static uint16_t
nmgr_uwb_mtu_1(struct os_mbuf *m)
{
    return nmgr_uwb_mtu(m, 1);
}
#endif
#if MYNEWT_VAL(UWB_DEVICE_2)
static uint16_t
nmgr_uwb_mtu_2(struct os_mbuf *m)
{
    return nmgr_uwb_mtu(m, 2);
}
#endif

nmgr_uwb_instance_t*
nmgr_uwb_init(struct uwb_dev* dev)
{
    assert(dev != NULL);
    nmgr_uwb_instance_t *nmgruwb = (nmgr_uwb_instance_t*)uwb_mac_find_cb_inst_ptr(dev, UWBEXT_NMGR_UWB);
    if(nmgruwb == NULL){
        nmgruwb = (nmgr_uwb_instance_t*)malloc(sizeof(nmgr_uwb_instance_t));
        memset(nmgruwb,0,sizeof(nmgr_uwb_instance_t));
        assert(nmgruwb);
        nmgruwb->dev_inst = dev;
    }
    dpl_sem_init(&nmgruwb->sem, 0x1);
    os_mqueue_init(&nmgruwb->tx_q, NULL, NULL);

    return nmgruwb;
}

/**
 * API to initialise the rng package.
 *
 *
 * @return void
 */
void nmgr_uwb_pkg_init(void)
{
    SYSINIT_ASSERT_ACTIVE();
#if MYNEWT_VAL(UWB_PKG_INIT_LOG)
    printf("{\"utime\": %lu,\"msg\": \"nmgr_uwb_init\"}\n", os_cputime_ticks_to_usecs(os_cputime_get32()));
#endif

    struct uwb_dev* udev;
#if MYNEWT_VAL(UWB_DEVICE_0)
    udev = uwb_dev_idx_lookup(0);
    nmgr_transport_init(uwb_transport(0), nmgr_resp_cb, nmgr_uwb_mtu_0);
    g_cbs[0].inst_ptr = nmgr_uwb_init(udev);
    uwb_mac_append_interface(udev, &g_cbs[0]);
#endif
#if MYNEWT_VAL(UWB_DEVICE_1)
    udev = uwb_dev_idx_lookup(1);
    nmgr_transport_init(uwb_transport(1), nmgr_resp_cb, nmgr_uwb_mtu_1);
    g_cbs[1].inst_ptr = nmgr_uwb_init(udev);
    uwb_mac_append_interface(udev, &g_cbs[1]);
#endif
#if MYNEWT_VAL(UWB_DEVICE_2)
    udev = uwb_dev_idx_lookup(2);
    nmgr_transport_init(uwb_transport(2), nmgr_resp_cb, nmgr_uwb_mtu_2);
    g_cbs[2].inst_ptr = nmgr_uwb_init(udev);
    uwb_mac_append_interface(udev, &g_cbs[2]);
#endif
}

/**
 * API for receive timeout callback.
 *
 * @param inst  Pointer to struct uwb_dev.
 *
 * @return true on sucess
 */
static bool
rx_timeout_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
{
    nmgr_uwb_instance_t * nmgruwb = (nmgr_uwb_instance_t *)cbs->inst_ptr;
    if(dpl_sem_get_count(&nmgruwb->sem) == 0){
        dpl_sem_release(&nmgruwb->sem);
        return true;
    }
    return false;
}

/**
 * API for forming mngr response data
 *
 * @param nt   Pointer to newt manager transport structure
 * @param m    pointer to mbuf where the response data will be available
 * @return true on sucess
 */
static int
nmgr_resp_cb(struct nmgr_transport *nt, struct os_mbuf *m)
{
    int rc;
    /* Prepare header and write to device */
    if (OS_MBUF_USRHDR_LEN(m) != sizeof(struct nmgr_uwb_usr_hdr)) {
        rc = os_mbuf_free_chain(m);
        return rc;
    }
    struct nmgr_uwb_usr_hdr *hdr = (struct nmgr_uwb_usr_hdr*)OS_MBUF_USRHDR(m);
    struct _nmgr_uwb_instance_t *nmgruwb = hdr->nmgruwb_inst;
    assert(nmgruwb);
    nmgr_uwb_frame_header_t *frame = &hdr->uwb_hdr;

    if (hdr->uwb_hdr.dst_address == UWB_BROADCAST_ADDRESS) {
        rc = os_mbuf_free_chain(m);
        assert(rc==0);
        goto early_exit;
    }

    if (uwb_nmgr_queue_tx(nmgruwb, frame->src_address,
                          UWB_DATA_CODE_NMGR_RESPONSE, m) != 0) {
        rc = os_mbuf_free_chain(m);
        assert(rc==0);
    }

early_exit:
    if(dpl_sem_get_count(&nmgruwb->sem) == 0){
        rc = dpl_sem_release(&nmgruwb->sem);
        assert(rc==0);
    }
    return 0;
}

/**
 * API for receive complete callback.
 *
 * @param inst  Pointer to struct uwb_dev.
 *
 * @return true on sucess
 */
static bool
rx_complete_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
{
    nmgr_uwb_instance_t * nmgruwb = (nmgr_uwb_instance_t *)cbs->inst_ptr;
    bool ret = false;
    struct os_mbuf * mbuf;
    static uint16_t last_rpt_src=0;
    static uint8_t last_rpt_seq_num=0;

    if(inst->fctrl != NMGR_UWB_FCTRL) {
        goto early_ret;
    }

    nmgr_uwb_frame_header_t *frame = (nmgr_uwb_frame_header_t*)inst->rxbuf;
    if(frame->code < UWB_DATA_CODE_NMGR_INVALID ||
       frame->code > UWB_DATA_CODE_NMGR_END) {
        goto early_ret;
    }

    /* If this packet should be repeated, repeat it (unless already repeated) */
    if (frame->rpt_count < frame->rpt_max &&
        frame->dst_address != inst->my_short_address &&
        frame->src_address != inst->my_short_address &&
        !(frame->src_address = last_rpt_src && frame->seq_num != last_rpt_seq_num)
        ) {
        /* Avoid repeating more than once */
        last_rpt_src = frame->src_address;
        last_rpt_seq_num = frame->seq_num;
        frame->rpt_count++;

        uwb_set_wait4resp(inst, true);
        uwb_write_tx_fctrl(inst, inst->frame_len, 0);
        if (uwb_start_tx(inst).start_tx_error) {
            /* Fail silently */
        } else {
            uwb_write_tx(inst, inst->rxbuf, 0, inst->frame_len);
        }
    }

    if(frame->dst_address != inst->my_short_address && frame->dst_address != 0xffff) {
        goto early_ret;
    }

    switch(frame->code) {
        case UWB_DATA_CODE_NMGR_RESPONSE: {
            /* Don't process responses here */
            break;
        }
        case UWB_DATA_CODE_NMGR_REQUEST: {
            ret = true;
            mbuf = os_msys_get_pkthdr(inst->frame_len - sizeof(nmgr_uwb_frame_header_t),
                                      sizeof(struct nmgr_uwb_usr_hdr));
            if (!mbuf) {
                printf("ERRMEM %d\n", inst->frame_len - sizeof(nmgr_uwb_frame_header_t) +
                       sizeof(struct nmgr_uwb_usr_hdr));
                break;
            }

            /* Copy the instance index and UWB header info so that we can use
             * it during sending the response */
            struct nmgr_uwb_usr_hdr *hdr = (struct nmgr_uwb_usr_hdr*)OS_MBUF_USRHDR(mbuf);
            hdr->nmgruwb_inst = nmgruwb;
            memcpy(&hdr->uwb_hdr, inst->rxbuf, sizeof(nmgr_uwb_frame_header_t));

            /* Copy the nmgr hdr & payload */
            int rc = os_mbuf_copyinto(mbuf, 0, inst->rxbuf + sizeof(nmgr_uwb_frame_header_t),
                                      (inst->frame_len - sizeof(nmgr_uwb_frame_header_t)));
            if (rc == 0) {
                /* Place on the newtmgr queue for processing */
                nmgr_rx_req(&uwb_transport_0, mbuf);
            } else {
                os_mbuf_free_chain(mbuf);
            }
            break;
        }
        default: {
            break;
        }
    }

early_ret:
    /* TODO: Check and reduce slot timeout */
    if(dpl_sem_get_count(&nmgruwb->sem) == 0) {
        dpl_sem_release(&nmgruwb->sem);
    }

    return ret;
}

/**
 * API for transmission complete callback.
 *
 * @param inst  Pointer to struct uwb_dev
 *
 * @return true on sucess
 */
static bool
tx_complete_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
{
    nmgr_uwb_instance_t * nmgruwb = (nmgr_uwb_instance_t *)cbs->inst_ptr;
    if(dpl_sem_get_count(&nmgruwb->sem) == 0) {
        dpl_sem_release(&nmgruwb->sem);
        return true;
    }
    return false;
}

/**
 * Listen for an incoming newtmgr data
 *
 * @param nmgruwb pointer to struct _nmgr_uwb_instance_t
 * @param mode UWB_BLOCKING or UWB_NONBLOCKING
 * @param inst Pointer to struct uwb_dev.
 *
 * @return struct uwb_dev_status
 */
struct uwb_dev_status
nmgr_uwb_listen(struct _nmgr_uwb_instance_t *nmgruwb, uwb_dev_modes_t mode,
                uint64_t delay, uint16_t timeout)
{
    dpl_error_t err;
    dpl_sem_pend(&nmgruwb->sem, DPL_TIMEOUT_NEVER);
    struct uwb_dev* inst = nmgruwb->dev_inst;

    /* TODO: Persist listening until finished */
    uwb_set_rx_timeout(inst, timeout);

    if (delay) {
        uwb_set_delay_start(inst, delay);
    }

    if(uwb_start_rx(inst).start_rx_error){
        err = dpl_sem_release(&nmgruwb->sem);
        assert(err == DPL_OK);
    }

    if (mode == UWB_BLOCKING){
        err = dpl_sem_pend(&nmgruwb->sem, DPL_TIMEOUT_NEVER);
        assert(err == DPL_OK);
        err = dpl_sem_release(&nmgruwb->sem);
        assert(err == DPL_OK);
    }
    return inst->status;
}


int
nmgr_uwb_tx(struct _nmgr_uwb_instance_t *nmgruwb, uint16_t dst_addr, uint16_t code,
            struct os_mbuf *m, uint64_t dx_time)
{
    struct uwb_dev* inst = nmgruwb->dev_inst;
    nmgr_uwb_frame_header_t uwb_hdr;

    uint8_t buf[32];
    int mbuf_offset = 0;
    int device_offset;
    dpl_sem_pend(&nmgruwb->sem, DPL_TIMEOUT_NEVER);

    /* Prepare header and write to device */
    uwb_hdr.fctrl = NMGR_UWB_FCTRL;
    uwb_hdr.src_address = inst->uid;
    uwb_hdr.code = code;
    uwb_hdr.dst_address = dst_addr;
    uwb_hdr.seq_num = nmgruwb->frame_seq_num++;
    uwb_hdr.PANID = inst->pan_id;
    uwb_hdr.rpt_count = 0;
#if MYNEWT_VAL(UWB_CCP_ENABLED)
    uwb_hdr.rpt_max = MYNEWT_VAL(UWB_CCP_MAX_CASCADE_RPTS);
#else
    uwb_hdr.rpt_max = 0;
#endif

    /* If fx_time provided, delay until then with tx */
    if (dx_time) {
        uwb_set_delay_start(inst, dx_time);
    }

    uwb_write_tx(inst, (uint8_t*)&uwb_hdr, 0, sizeof(nmgr_uwb_frame_header_t));
    device_offset = sizeof(nmgr_uwb_frame_header_t);

    /* Copy the mbuf payload data to the device to be sent */
    while (mbuf_offset < OS_MBUF_PKTLEN(m)) {
        int cpy_len = OS_MBUF_PKTLEN(m) - mbuf_offset;
        cpy_len = (cpy_len > sizeof(buf)) ? sizeof(buf) : cpy_len;

        /* The uwb_write_tx can do a dma transfer, make sure we wait
         * until that's finished before updating the buffer */
        uwb_hal_noblock_wait(inst, OS_TIMEOUT_NEVER);
        os_mbuf_copydata(m, mbuf_offset, cpy_len, buf);
        uwb_write_tx(inst, buf, device_offset, cpy_len);
        mbuf_offset += cpy_len;
        device_offset += cpy_len;
    }

    uwb_write_tx_fctrl(inst, sizeof(nmgr_uwb_frame_header_t) + OS_MBUF_PKTLEN(m), 0);

    if(uwb_start_tx(inst).start_tx_error){
        dpl_sem_release(&nmgruwb->sem);
        printf("UWB NMGR_tx: Tx Error \n");
    }

    dpl_sem_pend(&nmgruwb->sem, DPL_TIMEOUT_NEVER);
    if(dpl_sem_get_count(&nmgruwb->sem) == 0) {
        dpl_sem_release(&nmgruwb->sem);
    }

    os_mbuf_free_chain(m);
    return 0;
}

int
uwb_nmgr_process_tx_queue(struct _nmgr_uwb_instance_t *nmgruwb, uint64_t dx_time)
{
    int rc;
    uint16_t dst_addr = 0;
    uint16_t code = 0;
    struct os_mbuf *om;

    if ((om = os_mqueue_get(&nmgruwb->tx_q)) != NULL) {
        /* Extract dest address and code */
        rc = os_mbuf_copydata(om, OS_MBUF_PKTLEN(om)-4, sizeof(dst_addr), &dst_addr);
        assert(rc==0);
        rc = os_mbuf_copydata(om, OS_MBUF_PKTLEN(om)-2, sizeof(code), &code);
        assert(rc==0);
        os_mbuf_adj(om, -4);
        /* nmgr_uwb_tx consumes the mbuf */
        nmgr_uwb_tx(nmgruwb, dst_addr, code, om, dx_time);
        return true;
    }
    return false;
}

int
uwb_nmgr_queue_tx(struct _nmgr_uwb_instance_t *nmgruwb, uint16_t dst_addr, uint16_t code, struct os_mbuf *om)
{
#if MYNEWT_VAL(NMGR_UWB_LOOPBACK)
    nmgr_rx_req(&uwb_transport_0, om);
#else
    int rc;
    if (code==0) {
         code = UWB_DATA_CODE_NMGR_REQUEST;
    }

    /* Append the code and address to the end of the mbuf */
    uint16_t *p = os_mbuf_extend(om, sizeof(uint16_t)*2);
    if (!p) {
        printf("##### ERROR uwb_nmgr_q ext_failed\n");
        rc = os_mbuf_free_chain(om);
        return OS_EINVAL;
    }
    p[0] = dst_addr;
    p[1] = code;

    /* Enqueue the packet for sending at the next slot */
    rc = os_mqueue_put(&nmgruwb->tx_q, NULL, om);
    if (rc != 0) {
        printf("##### ERROR uwb_nmgr_q rc:%d\n", rc);
        rc = os_mbuf_free_chain(om);
        return OS_EINVAL;
    }
#endif
    return 0;
}
