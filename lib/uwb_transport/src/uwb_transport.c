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
 * @file uwb_transport.c
 * @author UWB Core <uwbcore@gmail.com>
 * @date 2019
 * @brief Non-ranging Transport Layer
 *
 * @details UWB Transport Layer
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <dpl/dpl.h>
#include <dpl/dpl_cputime.h>
#include <dpl/dpl_mempool.h>
#include <uwb/uwb.h>
#include <uwb/uwb_mac.h>
#include <uwb/uwb_ftypes.h>
#include <uwb_transport/uwb_transport.h>
#include <stats/stats.h>

#ifdef __KERNEL__
#define slog(fmt, ...)                                                  \
    pr_info("uwbcore: %s(): %d: "fmt, __func__, __LINE__, ##__VA_ARGS__)
#else
#define slog(fmt, ...) printf(fmt, ##__VA_ARGS__)
#endif


#if MYNEWT_VAL(UWB_TRANSPORT_STATS)
STATS_NAME_START(uwb_transport_stat_section)
    STATS_NAME(uwb_transport_stat_section, tx_bytes)
    STATS_NAME(uwb_transport_stat_section, tx_packets)
    STATS_NAME(uwb_transport_stat_section, tx_err)
    STATS_NAME(uwb_transport_stat_section, rx_bytes)
    STATS_NAME(uwb_transport_stat_section, rx_packets)
    STATS_NAME(uwb_transport_stat_section, rx_acks)
    STATS_NAME(uwb_transport_stat_section, rx_err)
    STATS_NAME(uwb_transport_stat_section, rx_dup)
    STATS_NAME(uwb_transport_stat_section, rx_start_err)
    STATS_NAME(uwb_transport_stat_section, om_writes)
#if MYNEWT_VAL(UWB_TRANSPORT_STATS_BITRATE)
    STATS_NAME(uwb_transport_stat_section, tx_bitrate)
    STATS_NAME(uwb_transport_stat_section, rx_bitrate)
#endif
STATS_NAME_END(uwb_transport_stat_section)
#define UWB_TRANSPORT_INC(__X) STATS_INC(uwb_transport->stat, __X)
#define UWB_TRANSPORT_INCN(__X, __Y) STATS_INCN(uwb_transport->stat, __X, __Y)
#define UWB_TRANSPORT_UPDATE(__X, __BR, __LAST_T, __N) STATS_SET(uwb_transport->stat, __X, filter_bitrate(__BR, __LAST_T, __N))
#else
#define UWB_TRANSPORT_INC(__X) {}
#define UWB_TRANSPORT_INCN(__X, __Y) {}
#define UWB_TRANSPORT_UPDATE(__X, __BR, __LAST_T, __N) {}
#endif

#if MYNEWT_VAL(UWB_TRANSPORT_STATS_BITRATE)
static uint32_t
filter_bitrate(float *br, uint32_t *last_ticks, float n) {
    uint32_t t = os_cputime_get32();
    if (t-*last_ticks > 0) {
        float nbr = 8.0e6f*n / os_cputime_ticks_to_usecs(t-*last_ticks);
        *br = 0.95f*(*br)+0.05f*nbr;
        *last_ticks = t;
    }
    return (uint32_t)*br;
}
#endif

static bool rx_complete_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs);
static bool rx_timeout_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs);
static bool tx_begins_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs) __attribute__((unused));
static bool tx_complete_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs);
static bool reset_cb(struct uwb_dev *inst, struct uwb_mac_interface * cbs);
static void uwb_transport_process_rx_queue(struct dpl_event *ev);

static struct uwb_mac_interface g_cbs[] = {
        [0] = {
            .id = UWBEXT_TRANSPORT,
            .rx_complete_cb = rx_complete_cb,
            .tx_begins_cb = tx_begins_cb,
            .tx_complete_cb = tx_complete_cb,
            .rx_timeout_cb = rx_timeout_cb,
            .reset_cb = reset_cb
        },
#if MYNEWT_VAL(UWB_DEVICE_1)
        [1] = {
            .id = UWBEXT_TRANSPORT,
            .rx_complete_cb = rx_complete_cb,
            .tx_begins_cb = tx_begins_cb,
            .tx_complete_cb = tx_complete_cb,
            .rx_timeout_cb = rx_timeout_cb,
            .reset_cb = reset_cb
        },
#endif
#if MYNEWT_VAL(UWB_DEVICE_2)
        [2] = {
            .id = UWBEXT_TRANSPORT,
            .rx_complete_cb = rx_complete_cb,
            .tx_begins_cb = tx_begins_cb,
            .tx_complete_cb = tx_complete_cb,
            .rx_timeout_cb = rx_timeout_cb,
            .reset_cb = reset_cb
        }
#endif
};


#define MBUF_PKTHDR_OVERHEAD    sizeof(struct dpl_mbuf_pkthdr) + sizeof(struct _uwb_transport_user_header)
#define MBUF_MEMBLOCK_OVERHEAD  sizeof(struct dpl_mbuf) + MBUF_PKTHDR_OVERHEAD
#define MBUF_NUM_MBUFS      MYNEWT_VAL(UWB_TRANSPORT_NUM_MBUFS)
#define MBUF_PAYLOAD_SIZE   MYNEWT_VAL(UWB_TRANSPORT_MBUF_SIZE)
#define MBUF_BUF_SIZE       DPL_ALIGN(MBUF_PAYLOAD_SIZE, 4)
#define MBUF_MEMBLOCK_SIZE  (MBUF_BUF_SIZE + MBUF_MEMBLOCK_OVERHEAD)
#define MBUF_MEMPOOL_SIZE   DPL_MEMPOOL_SIZE(MBUF_NUM_MBUFS, MBUF_MEMBLOCK_SIZE)

#if MYNEWT_VAL(UWB_TRANSPORT_MBUF_POOL_ENABLE)
static struct dpl_mbuf_pool g_mbuf_pool;
static struct dpl_mempool g_mbuf_mempool;
static dpl_membuf_t g_mbuf_buffer[MBUF_MEMPOOL_SIZE];

void
uwb_transport_create_mbuf_pool(struct _uwb_transport_instance * uwb_transport)
{
    int rc;

    rc = dpl_mempool_init(&g_mbuf_mempool, MBUF_NUM_MBUFS,
                          MBUF_MEMBLOCK_SIZE, &g_mbuf_buffer[0], "uwb_transport_mbuf_pool");
    assert(rc == 0);

    rc = dpl_mbuf_pool_init(&g_mbuf_pool, &g_mbuf_mempool, MBUF_MEMBLOCK_SIZE,
                           MBUF_NUM_MBUFS);
    assert(rc == 0);

    uwb_transport->omp = &g_mbuf_pool;
    uwb_transport->config.os_msys_mpool = false;
}
#endif



uint16_t
uwb_transport_mtu(struct dpl_mbuf *m, int idx)
{
    struct uwb_dev* inst = uwb_dev_idx_lookup(idx);
    if (!inst) {
        return 0;
    }
    return (inst->config.rx.phrMode==DWT_PHRMODE_STD) ? UWB_TRANSPORT_MTU_STD : UWB_TRANSPORT_MTU_EXT;
}
EXPORT_SYMBOL(uwb_transport_mtu);

uwb_transport_instance_t*
uwb_transport_init(struct uwb_dev * dev)
{
    uwb_transport_instance_t * uwb_transport = (uwb_transport_instance_t*)uwb_mac_find_cb_inst_ptr(dev, UWBEXT_TRANSPORT);
    if (uwb_transport == NULL) {
        uwb_transport = (uwb_transport_instance_t *)calloc(1, sizeof(uwb_transport_instance_t));
        assert(uwb_transport);
        uwb_transport->dev_inst = dev;
        uwb_transport->config = (uwb_transport_config_t){
            .os_msys_mpool = 1,
            .dflt_eventq = 1
        };
    }
    dpl_sem_init(&uwb_transport->sem, 0x1);
    dpl_sem_init(&uwb_transport->write_tx_lock, 0x1);
    dpl_sem_init(&uwb_transport->ack_sem, 0x1);
    dpl_mqueue_init(&uwb_transport->tx_q, NULL, NULL);
    dpl_mqueue_init(&uwb_transport->rx_q, (dpl_event_fn *) uwb_transport_process_rx_queue, uwb_transport);
    snprintf(uwb_transport->device_name, sizeof(uwb_transport->device_name), "uwbtp%d", dev->idx);

    uwb_transport->config.request_acks = 1;

#if MYNEWT_VAL(UWB_TRANSPORT_MBUF_POOL_ENABLE)
    uwb_transport_create_mbuf_pool(uwb_transport);
#endif
#if MYNEWT_VAL(UWB_TRANSPORT_STATS)
    int rc = stats_init(
        STATS_HDR(uwb_transport->stat),
        STATS_SIZE_INIT_PARMS(uwb_transport->stat, STATS_SIZE_32),
        STATS_NAME_INIT_PARMS(uwb_transport_stat_section));
    assert(rc == 0);
    rc = stats_register(uwb_transport->device_name, STATS_HDR(uwb_transport->stat));
    assert(rc == 0);
#endif
    uwb_transport->status.has_init = 1;
    return uwb_transport;
}

/**
 * @fn uwb_transport_free(struct _uwb_transport_instance * uwb_transport)
 * @brief API to free the allocated resources.
 *
 * @param inst  Pointer to struct struct _uwb_transport_instance.
 *
 * @return void
 */
void
uwb_transport_free(struct _uwb_transport_instance * uwb_transport)
{
    struct dpl_mbuf * mbuf;
    uwb_transport->status.has_init = 0;

    /* Empty queues */
    while ((mbuf = dpl_mqueue_get(&uwb_transport->rx_q))) {
        dpl_mbuf_free_chain(mbuf);
    }
    while ((mbuf = dpl_mqueue_get(&uwb_transport->tx_q))) {
        dpl_mbuf_free_chain(mbuf);
    }

    free(uwb_transport);
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
    uwb_transport_instance_t * uwb_transport = (uwb_transport_instance_t *)cbs->inst_ptr;
    if(dpl_sem_get_count(&uwb_transport->ack_sem) == 0) {
        uwb_transport->ack_seq_num = -1;
        dpl_sem_release(&uwb_transport->ack_sem);
        return true;
    }

    if(dpl_sem_get_count(&uwb_transport->sem) == 0){
        dpl_sem_release(&uwb_transport->sem);
        return true;
    }
    return false;
}

/**
 * @brief API to get uwb_transport extension.
 *
 * @param inst          Pointer to struct _uwb_transport_instance.
 * @param tsp_code      Code representing the extension ID
 *
 * @return struct _uwb_transport_extension
 */
struct _uwb_transport_extension *
uwb_transport_get_extension(struct _uwb_transport_instance * uwb_transport, uint16_t tsp_code)
{
    assert(uwb_transport);
    struct _uwb_transport_extension * extension;
    if(!(SLIST_EMPTY(&uwb_transport->extension_list))){
        SLIST_FOREACH(extension, &uwb_transport->extension_list, next){
            if (extension != NULL && extension->tsp_code == tsp_code) {
                return extension;
            }
        }
    }
    return NULL;
}

/**
 * @brief API to add new extension to uwb_transport.
 *
 * @param uwb_transport  Pointer to struct _uwb_transport_instance
 * @param extension struct _uwb_transport_extension containing callback structure.
 * @return void
 */
void
uwb_transport_append_extension(struct _uwb_transport_instance * uwb_transport, struct _uwb_transport_extension  * extension)
{
    assert(uwb_transport);
    if(!(SLIST_EMPTY(&uwb_transport->extension_list))) {
        struct _uwb_transport_extension * prev = NULL;
        struct _uwb_transport_extension * cur = NULL;
        SLIST_FOREACH(cur, &uwb_transport->extension_list, next){
            prev = cur;
        }
        SLIST_INSERT_AFTER(prev, extension, next);
    } else {
        SLIST_INSERT_HEAD(&uwb_transport->extension_list, extension, next);
    }
}
EXPORT_SYMBOL(uwb_transport_append_extension);

/**
 * @brief API to remove specified extension, by code reference
 *
 * @param inst  Pointer to struct uwb_rng_instance.
 * @param tsp_code  code ID of the service.
 * @return void
 */
void
uwb_transport_remove_extension(struct _uwb_transport_instance * uwb_transport, uint16_t tsp_code)
{
    assert(uwb_transport);
    struct _uwb_transport_extension * extension = NULL;
    SLIST_FOREACH(extension, &uwb_transport->extension_list, next){
        if(extension->tsp_code == tsp_code){
            SLIST_REMOVE(&uwb_transport->extension_list, extension, _uwb_transport_extension, next);
            break;
        }
    }
}
EXPORT_SYMBOL(uwb_transport_remove_extension);

static void
extension_signal_tx(struct _uwb_transport_instance * uwb_transport)
{
    struct _uwb_transport_extension * extension;
    if(!(SLIST_EMPTY(&uwb_transport->extension_list))){
        SLIST_FOREACH(extension, &uwb_transport->extension_list, next){
            if (extension->transmit_cb != NULL) {
                extension->transmit_cb(uwb_transport->dev_inst);
            }
        }
    }
}

/**
 * @brief API for relinquish mbuf to upper layer
 *
 * @param inst  Pointer to struct uwb_dev.
 * @return void
 */
static void
uwb_transport_process_rx_queue(struct dpl_event *ev)
{
    uwb_transport_instance_t * uwb_transport = (uwb_transport_instance_t *) dpl_event_get_arg(ev);
    struct dpl_mbuf * mbuf;
    uwb_transport_user_header_t * hdr;
    uwb_transport_extension_t * extension;

    while ((mbuf = dpl_mqueue_get(&uwb_transport->rx_q)) &&
           uwb_transport->status.has_init) {
        hdr = (uwb_transport_user_header_t * ) DPL_MBUF_USRHDR(mbuf);
        extension = uwb_transport_get_extension(uwb_transport, hdr->tsp_code);
        if (extension)
            extension->receive_cb(uwb_transport->dev_inst, hdr->uid, mbuf);
        else
            dpl_mbuf_free_chain(mbuf);
    }
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
    uwb_transport_instance_t * uwb_transport = (uwb_transport_instance_t *)cbs->inst_ptr;
    bool ret = false;
    struct dpl_mbuf * mbuf;
    uint32_t cputime = dpl_cputime_get32();

    if (inst->fctrl == UWB_FCTRL_FRAME_TYPE_ACK) {
        if(dpl_sem_get_count(&uwb_transport->ack_sem) == 0) {
            UWB_TRANSPORT_INC(rx_acks);
            uwb_transport->ack_seq_num = inst->rxbuf[2];
            dpl_sem_release(&uwb_transport->ack_sem);
        }
        return true;
    }

    if(inst->fctrl != UWB_TRANSPORT_FCTRL &&
       inst->fctrl != (UWB_TRANSPORT_FCTRL|UWB_FCTRL_ACK_REQUESTED) &&
       inst->fctrl != UWB_FCTRL_FRAME_TYPE_ACK) {
        goto early_ret;
    }

    if (inst->status.autoack_triggered) {
        uwb_transport->status.awaiting_ack_tx = 1;
    }

    uwb_transport_frame_header_t * frame = (uwb_transport_frame_header_t* )inst->rxbuf;
    if(frame->code < UWB_DATA_CODE_TRNSPRT_REQUEST ||
       frame->code > UWB_DATA_CODE_TRNSPRT_END) {
        goto early_ret;
    }

    if(frame->dst_address != inst->my_short_address && frame->dst_address != 0xffff) {
        goto early_ret;
    }

    /* Reject the same package received within 10ms */
    if ((cputime - uwb_transport->last_frame_time) < dpl_cputime_usecs_to_ticks(10000) &&
        !memcmp(&uwb_transport->last_frame, frame, sizeof(*frame))) {
        UWB_TRANSPORT_INC(rx_dup);
        goto early_ret;
    }
    memcpy(&uwb_transport->last_frame, frame, sizeof(*frame));
    uwb_transport->last_frame_time = cputime;
    UWB_TRANSPORT_INC(rx_packets);

    ret = true;
    if (uwb_transport->config.os_msys_mpool){
        mbuf = dpl_msys_get_pkthdr(inst->frame_len - sizeof(uwb_transport_frame_header_t),
                                      sizeof(uwb_transport_user_header_t));
    } else {
        mbuf = dpl_mbuf_get_pkthdr(uwb_transport->omp, sizeof(uwb_transport_user_header_t));
    }

    if (mbuf) {
        /* Copy the instance index and UWB header info so that we can use
        * it during sending the response */
        uwb_transport_user_header_t * hdr = (uwb_transport_user_header_t * )DPL_MBUF_USRHDR(mbuf);
        hdr->tsp_code = frame->tsp_code;
        hdr->uid = frame->src_address;
        hdr->uwb_transport = uwb_transport;
        UWB_TRANSPORT_INCN(rx_bytes, inst->frame_len - sizeof(uwb_transport_frame_header_t));
#if MYNEWT_VAL(UWB_TRANSPORT_STATS_BITRATE)
        UWB_TRANSPORT_UPDATE(rx_bitrate, &uwb_transport->rx_bits_per_second,
                             &uwb_transport->rx_br_last, inst->frame_len - sizeof(uwb_transport_frame_header_t));
#endif
        /* Copy the payload to mqueue */
        int rc = dpl_mbuf_copyinto(mbuf, 0, inst->rxbuf + sizeof(uwb_transport_frame_header_t),
                                      (inst->frame_len - sizeof(uwb_transport_frame_header_t)));
        if (rc == 0){
            if (uwb_transport->config.dflt_eventq){
                dpl_mqueue_put(&uwb_transport->rx_q, dpl_eventq_dflt_get(), mbuf);
            }
            else{
                dpl_mqueue_put(&uwb_transport->rx_q, uwb_transport->oeq, mbuf);
            }
        } else {
            dpl_mbuf_free_chain(mbuf);
        }
        ret = true;
    }
    else{
        UWB_TRANSPORT_INC(rx_err);
    }
    return ret;

early_ret:
    if(dpl_sem_get_count(&uwb_transport->sem) == 0){
        dpl_sem_release(&uwb_transport->sem);
    }
    return ret;
}

/**
 * API for frame transmission begins callback.
 *
 * @param inst  Pointer to struct uwb_dev
 *
 * @return true on sucess
 */
static bool
tx_begins_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
{
    uwb_transport_instance_t * uwb_transport = (uwb_transport_instance_t *)cbs->inst_ptr;
    if(dpl_sem_get_count(&uwb_transport->write_tx_lock) == 0) {
        dpl_sem_release(&uwb_transport->write_tx_lock);
        return true;
    }
    return false;
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
    uwb_transport_instance_t * uwb_transport = (uwb_transport_instance_t *)cbs->inst_ptr;

    if (uwb_transport->status.awaiting_ack_tx) {
        uwb_transport->status.awaiting_ack_tx = 0;
    }

    if(dpl_sem_get_count(&uwb_transport->sem) == 0) {
        dpl_sem_release(&uwb_transport->sem);
        return true;
    }
    return false;
}

/**
 * @fn reset_cb(struct uwb_dev*, struct uwb_mac_interface*)
 * @brief API for reset_cb of uwb_transport.
 *
 * @param inst   Pointer to struct uwb_dev *inst.
 * @param cbs    Pointer to struct uwb_mac_interface.
 *
 * @return void
 */
static bool
reset_cb(struct uwb_dev *inst, struct uwb_mac_interface * cbs)
{
    uwb_transport_instance_t * uwb_transport = (uwb_transport_instance_t *)cbs->inst_ptr;
    if(dpl_sem_get_count(&uwb_transport->ack_sem) == 0) {
        uwb_transport->ack_seq_num = -1;
        dpl_sem_release(&uwb_transport->ack_sem);
    }
    if(dpl_sem_get_count(&uwb_transport->sem) == 0) {
        dpl_sem_release(&uwb_transport->sem);
    }
    if(dpl_sem_get_count(&uwb_transport->write_tx_lock) == 0) {
        dpl_sem_release(&uwb_transport->write_tx_lock);
    }
    return false;  // Pass it forward
}

/**
 * Listen for an incoming uwb transport layer data
 *
 * @param uwb_transport pointer to struct _uwb_transport_instance_t
 * @param mode UWB_BLOCKING or UWB_NONBLOCKING
 * @param dx_time transeiver turn on time in dtu.
 * @param dx_time_end end of listen window in dtu
 *
 * @return struct uwb_dev_status
 */
struct uwb_dev_status
uwb_transport_listen(struct _uwb_transport_instance *uwb_transport, uwb_dev_modes_t mode, uint64_t dx_time, uint64_t dx_time_end)
{
    dpl_error_t err;
    dpl_sem_pend(&uwb_transport->sem, DPL_TIMEOUT_NEVER);
    struct uwb_dev * inst = uwb_transport->dev_inst;
    /* Stop listening in time to get ready for next slot */
    if (dx_time) {
        dx_time_end -= (MYNEWT_VAL(UWB_TRANSPORT_PERIOD_END_GUARD) << 16);
        dx_time_end &= UWB_DTU_40BMASK;
        uwb_set_rx_window(inst, dx_time, dx_time_end);
    } else {
        uwb_set_rx_timeout(inst, dx_time_end>>16);
    }

    uwb_set_autoack(inst, true);
    uwb_set_autoack_delay(inst, 12);
    uwb_set_wait4resp(inst, 1);
    uwb_set_wait4resp_delay(inst, 0);

    uwb_set_on_error_continue(inst, true);
    if(uwb_start_rx(inst).start_rx_error) {
        UWB_TRANSPORT_INC(rx_start_err);
        err = dpl_sem_release(&uwb_transport->sem);
        assert(err == DPL_OK);
    }

    if (mode == UWB_BLOCKING){
        err = dpl_sem_pend(&uwb_transport->sem, DPL_TIMEOUT_NEVER);
        assert(err == DPL_OK);
        err = dpl_sem_release(&uwb_transport->sem);
        assert(err == DPL_OK);
    }
    return inst->status;
}
EXPORT_SYMBOL(uwb_transport_listen);

/**
 * Write packet to the device
 *
 * @param uwb_transport pointer to struct _uwb_transport_instance_t
 * @param om  pointer to struct dpl_mbuf containing the data
 * @param idx index used to select which of the tx swing buffers to write into
 *
 * @return struct uwb_dev_status
 */
struct uwb_dev_status
uwb_transport_write_tx(struct _uwb_transport_instance *uwb_transport,
                       struct dpl_mbuf *om, uint16_t idx)
{
    int rc;
    struct uwb_dev * inst = uwb_transport->dev_inst;
    uwb_transport_frame_header_t uwb_hdr;
    uint8_t buf[128];
    int mbuf_offset = 0;
    int device_offset;
    struct uwb_dev_status status;
    int tx_buffer_offset = (idx%2)?512:0;
    int tx_len = DPL_MBUF_PKTLEN(om) - 6;
    size_t mtu = (inst->config.rx.phrMode==DWT_PHRMODE_STD) ? UWB_TRANSPORT_MTU_STD : UWB_TRANSPORT_MTU_EXT;
    if (tx_len > mtu) {
        slog("uwb_transport: ERROR %d > MTU %zd", tx_len, mtu);
    }

    /* Prepare header and write to device */
    uwb_hdr.src_address = inst->uid;
    uwb_hdr.code = UWB_DATA_CODE_TRNSPRT_REQUEST;
    uwb_hdr.seq_num = ++uwb_transport->frame_seq_num;
    uwb_hdr.PANID = inst->pan_id;
    uwb_hdr.fctrl = UWB_TRANSPORT_FCTRL;
    if (uwb_transport->config.request_acks) {
        uwb_hdr.fctrl |= UWB_FCTRL_ACK_REQUESTED;
    }

    /* Extract dest address and tsp_code */
    rc = dpl_mbuf_copydata(om, DPL_MBUF_PKTLEN(om)-6, sizeof(uwb_hdr.dst_address), &uwb_hdr.dst_address);
    assert(rc==0);
    rc = dpl_mbuf_copydata(om, DPL_MBUF_PKTLEN(om)-4, sizeof(uwb_hdr.tsp_code), &uwb_hdr.tsp_code);
    assert(rc==0);

    status = uwb_write_tx(inst, (uint8_t*)&uwb_hdr, tx_buffer_offset, sizeof(uwb_transport_frame_header_t));
    device_offset = sizeof(uwb_transport_frame_header_t) + tx_buffer_offset;

    /* Copy the mbuf payload data to the device to be sent */
    while (mbuf_offset < tx_len) {
        int cpy_len = tx_len - mbuf_offset;
        cpy_len = (cpy_len > sizeof(buf)) ? sizeof(buf) : cpy_len;

        /* The uwb_write_tx can do a dma transfer, make sure we wait
         * until that's finished before updating the buffer */
        uwb_hal_noblock_wait(inst, DPL_TIMEOUT_NEVER);
        dpl_mbuf_copydata(om, mbuf_offset, cpy_len, buf);
        status = uwb_write_tx(inst, buf, device_offset, cpy_len);
        mbuf_offset += cpy_len;
        device_offset += cpy_len;
    }

    /* Store next fctrl values but don't write them here as this affects the frame thay may be still sending */
    uwb_transport->tx_buffer_len = sizeof(uwb_transport_frame_header_t) + DPL_MBUF_PKTLEN(om) - 6;
    uwb_transport->tx_buffer_offset = tx_buffer_offset;
    return status;
}

/**
 * Start the transfer
 *
 * @param uwb_transport pointer to struct _uwb_transport_instance_t
 * @param om  pointer to struct dpl_mbuf containing the data
 * @param dx_time time of send in dtu, if 0 send immediately.
 *
 * @return struct uwb_dev_status
 */
struct uwb_dev_status
uwb_transport_start_tx(struct _uwb_transport_instance *uwb_transport, struct dpl_mbuf *om, uint64_t dx_time)
{
    struct uwb_dev * inst = uwb_transport->dev_inst;
    struct uwb_dev_status status;

    /* If dx_time provided, delay until then with tx */
    if (dx_time) {
        uwb_set_delay_start(inst, dx_time);
    }

    /* Wait for ack? */
    if (uwb_transport->config.request_acks) {
        uwb_set_wait4resp(inst, 1);
        uwb_set_rx_timeout(inst, 12 + 10 + uwb_phy_frame_duration(uwb_transport->dev_inst, 5));
        uwb_set_rxauto_disable(inst, true);
    }

    uwb_write_tx_fctrl(inst, uwb_transport->tx_buffer_len, uwb_transport->tx_buffer_offset);
    status = uwb_start_tx(inst);
    return status;
}

/**
 * Transfer any packets in out tx-queue
 *
 * @param uwb_transport pointer to struct _uwb_transport_instance_t
 * @param arg_dx_time  time of start send in dtu, if 0 send immediately.
 * @param dx_time_end  end of transfer window in dtu. Still valid if !arg_dx_time
 *
 * @return int 1 if some tx was attemted, 0 otherwise
 */
int
uwb_transport_dequeue_tx(struct _uwb_transport_instance *uwb_transport, uint64_t arg_dx_time, uint64_t dx_time_end)
{
    int rc, n_sent = 0;
    bool membuf_transferred = false;
    struct dpl_mbuf *om = NULL;
    struct dpl_mbuf_pkthdr * mp = NULL;
    uint16_t idx=0, retries, *retries_p;
    uint64_t dx_time, systime;
    uint64_t preamble_duration, data_duration;
    uint64_t last_duration = 0, next_duration;

    if (STAILQ_FIRST(&uwb_transport->tx_q.mq_head) == NULL) {
        return false;
    }
    dx_time = arg_dx_time;
    preamble_duration = (uint64_t) ceilf(uwb_usecs_to_dwt_usecs(uwb_phy_SHR_duration(uwb_transport->dev_inst)));

    /* Transmit frames as long as time permits */
    do {
        if (!uwb_transport->status.has_init) {
            break;
        }
        if (!mp) {
            mp = STAILQ_FIRST(&uwb_transport->tx_q.mq_head);
            if(mp == NULL)
                break;
            om = DPL_MBUF_PKTHDR_TO_MBUF(mp);
            membuf_transferred = false;
        }

        dx_time += last_duration;
        dx_time &= UWB_DTU_40BMASK;
        data_duration = (uint64_t) ceilf(uwb_usecs_to_dwt_usecs(uwb_phy_data_duration(uwb_transport->dev_inst,
                            DPL_MBUF_PKTLEN(om) + sizeof(uwb_transport_frame_header_t))));
        next_duration = ((preamble_duration + data_duration + MYNEWT_VAL(UWB_TRANSPORT_SUBSLOT_GUARD)) << 16);
        last_duration = next_duration;

        /* Can we fit this package in before the dx_time_end */
        uint64_t tx_time_remaining = dx_time_end - ((dx_time + next_duration)&UWB_DTU_40BMASK);
        tx_time_remaining -= (MYNEWT_VAL(UWB_TRANSPORT_PERIOD_END_GUARD) << 16);
        if (dx_time_end && tx_time_remaining > 0x7fffffffffULL) {
            /*  */
            break;
        }

        /* DW1000 Errata 1.1, inhibit overlapping writes until transmission begins */
        dpl_sem_pend(&uwb_transport->write_tx_lock, DPL_TIMEOUT_NEVER);
        if (!membuf_transferred) {
            UWB_TRANSPORT_INC(om_writes);
            uwb_transport_write_tx(uwb_transport, om, ++idx);
            n_sent = 0;
        }
        membuf_transferred = true;

        /* Wait for overlapping transmission to complete */
        dpl_sem_pend(&uwb_transport->sem, DPL_TIMEOUT_NEVER);
        if (uwb_transport->config.request_acks) {
            dpl_sem_pend(&uwb_transport->ack_sem, DPL_TIMEOUT_NEVER);
        }
        if(uwb_transport_start_tx(uwb_transport, om, (arg_dx_time) ? dx_time : 0).start_tx_error){
            UWB_TRANSPORT_INC(tx_err);
            if(dpl_sem_get_count(&uwb_transport->sem) == 0) {
                dpl_sem_release(&uwb_transport->sem);
            }
            if(dpl_sem_get_count(&uwb_transport->write_tx_lock) == 0) {
                dpl_sem_release(&uwb_transport->write_tx_lock);
            }
            if(dpl_sem_get_count(&uwb_transport->ack_sem) == 0) {
                dpl_sem_release(&uwb_transport->ack_sem);
            }
            /* Check if we've slipped far behind systime and correct if so */
            systime = uwb_read_systime(uwb_transport->dev_inst);
            if (dx_time - systime > 0x7fffffffffULL) {
                dx_time = systime + (preamble_duration<<16);
            }
            continue;
        } else {
            n_sent++;
        }

        if (uwb_transport->config.request_acks) {
            dpl_sem_pend(&uwb_transport->ack_sem, DPL_TIMEOUT_NEVER);
            if(dpl_sem_get_count(&uwb_transport->ack_sem) == 0) {
                dpl_sem_release(&uwb_transport->ack_sem);
            }

            if (uwb_transport->ack_seq_num != uwb_transport->frame_seq_num) {
                rc = dpl_mbuf_copydata(om, DPL_MBUF_PKTLEN(om)-2, sizeof(uint16_t), &retries);
                assert(rc==0);
                /* Check for retries */
                if (n_sent > retries) {
                    printf("dropped n:%d r:%d\n", n_sent, retries);
                    /* Drop packet */
                    om = dpl_mqueue_get(&uwb_transport->tx_q);
                    dpl_mbuf_free_chain(om);
                    mp = NULL;
                    om = NULL;
                    membuf_transferred = false;
                } else {
                    /* Remove current retries value */
                    dpl_mbuf_adj(om, -2);
                    retries_p = dpl_mbuf_extend(om, sizeof(uint16_t));
                    if (retries_p) {
                        *retries_p = retries - n_sent;
                        n_sent = 0;
                    }
                }
                /* Ack failed, retransmit */
                continue;
            }
        }

        UWB_TRANSPORT_INC(tx_packets);
        UWB_TRANSPORT_INCN(tx_bytes, DPL_MBUF_PKTLEN(om) - 4);
#if MYNEWT_VAL(UWB_TRANSPORT_STATS_BITRATE)
        UWB_TRANSPORT_UPDATE(tx_bitrate, &uwb_transport->tx_bits_per_second,
                             &uwb_transport->tx_br_last, DPL_MBUF_PKTLEN(om) - 4);
#endif

        /* Successfully sent, dequeue and free */
        om = dpl_mqueue_get(&uwb_transport->tx_q);
        if (om) {
            dpl_mbuf_free_chain(om);
        }
        mp = NULL;
        om = NULL;
        membuf_transferred = false;

        extension_signal_tx(uwb_transport);
    } while(STAILQ_FIRST(&uwb_transport->tx_q.mq_head) != NULL &&
            uwb_transport->status.has_init);

    dpl_sem_pend(&uwb_transport->sem, DPL_TIMEOUT_NEVER);
    if(dpl_sem_get_count(&uwb_transport->sem) == 0) {
        dpl_sem_release(&uwb_transport->sem);
    }

    return true;
}
EXPORT_SYMBOL(uwb_transport_dequeue_tx);

/**
 * Enqueue a packet for transfer later by uwb_transport_dequeue_tx
 *
 * @param uwb_transport pointer to struct _uwb_transport_instance_t
 * @param dst_addr 16-bit destination address
 * @param retries number of ack-wait-retries before dropping packet
 * @param om pointer to struct dpl_mbuf containing the data
 *
 * @return struct uwb_dev_status
 */
int
uwb_transport_enqueue_tx(struct _uwb_transport_instance *uwb_transport, uint16_t dst_addr,
                         uint16_t tsp_code, uint16_t retries, struct dpl_mbuf *om)
{
    int rc;
    uint16_t *p;
    size_t mtu = uwb_transport_mtu(om, uwb_transport->dev_inst->idx);
    size_t tx_len = DPL_MBUF_PKTLEN(om);
    if (tx_len > mtu) {
        slog("uwb_transport: ERROR %zd > MTU %zd", tx_len, mtu);
        return DPL_EINVAL;
    }

    /* Append the tsp_code, address and retries to the end of the mbuf */
    p = dpl_mbuf_extend(om, sizeof(uint16_t)*3);
    if (!p) {
        uint32_t utime = dpl_cputime_ticks_to_usecs(dpl_cputime_get32());
        slog("{\"utime\": %"PRIu32",\"error\": \"dpl_mbuf_extend %s:%d\"\"}\n",
             utime, __FILE__,__LINE__);
        rc = dpl_mbuf_free_chain(om);
        return DPL_EINVAL;
    }
    p[0] = dst_addr;
    p[1] = tsp_code;
    p[2] = retries;

    /* Enqueue the packet for sending at the next slot */
    rc = dpl_mqueue_put(&uwb_transport->tx_q, NULL, om);
    if (rc != 0) {
        uint32_t utime = dpl_cputime_ticks_to_usecs(dpl_cputime_get32());
        slog("{\"utime\": %"PRIu32",\"error\": \"dpl_mqueue_put %s:%d\"\"}\n",
             utime,__FILE__,__LINE__);
        rc = dpl_mbuf_free_chain(om);
        return DPL_EINVAL;
    }

    return 0;
}
EXPORT_SYMBOL(uwb_transport_enqueue_tx);

struct dpl_mbuf*
uwb_transport_new_mbuf(struct _uwb_transport_instance *uwb_transport)
{
    struct dpl_mbuf *mbuf = 0;
    if (uwb_transport->config.os_msys_mpool) {
        /* TODO: Set better value / argument for dsize instead of 32 */
        mbuf = dpl_msys_get_pkthdr(32, sizeof(uwb_transport_user_header_t));
    } else{
        mbuf = dpl_mbuf_get_pkthdr(uwb_transport->omp, sizeof(uwb_transport_user_header_t));
    }
    return mbuf;
}
EXPORT_SYMBOL(uwb_transport_new_mbuf);


/**
 * API to initialise the uwb_transport_pkg_init.
 *
 *
 * @return void
 */
void uwb_transport_pkg_init(void)
{
    int i;
    struct uwb_dev* udev;
#if MYNEWT_VAL(UWB_PKG_INIT_LOG)
    printf("{\"utime\": %"PRIu32",\"msg\": \"uwb_transport_pkg_init\"}\n",
           dpl_cputime_ticks_to_usecs(dpl_cputime_get32()));
#endif

    for (i=0;i < sizeof(g_cbs)/sizeof(g_cbs[0]);i++) {
        if (i >= MYNEWT_VAL(UWB_DEVICE_MAX)) {
            break;
        }
        udev = uwb_dev_idx_lookup(i);
        if (!udev) {
            continue;
        }
        g_cbs[i].inst_ptr = uwb_transport_init(udev);
        uwb_mac_append_interface(udev, &g_cbs[i]);
    }
}

int
uwb_transport_pkg_down(int reason)
{
    int i;
    struct _uwb_transport_instance * tp;

    for (i=0;i < sizeof(g_cbs)/sizeof(g_cbs[0]);i++) {
        tp = (struct _uwb_transport_instance *)g_cbs[i].inst_ptr;
        if (!tp) continue;
        uwb_mac_remove_interface(tp->dev_inst, g_cbs[i].id);
        uwb_transport_free(tp);
        g_cbs[i].inst_ptr = 0;
    }

    return 0;
}
