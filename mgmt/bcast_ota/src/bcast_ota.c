#include <string.h>
#include <stdio.h>
#include <os/os.h>
#include <os/os_time.h>
#include <syscfg/syscfg.h>
#include <sysinit/sysinit.h>
#include <log/log.h>

#include <defs/error.h>
#include <bootutil/image.h>
#include <flash_map/flash_map.h>
#include <hal/hal_bsp.h>

#include <tinycbor/cbor.h>
#include <tinycbor/cborjson.h>
#include <tinycbor/cbor_mbuf_writer.h>
#include <tinycbor/cbor_mbuf_reader.h>
#include <cborattr/cborattr.h>
#include "base64/hex.h"

#include <nmgr_os/nmgr_os.h>
#include <mgmt/mgmt.h>
#include <newtmgr/newtmgr.h>
#include <imgmgr/imgmgr.h>

#include <bcast_ota/bcast_ota.h>
#include "bcast_ota_cli.h"

#include "bcast_ota_priv.h"
struct log g_bcast_ota_log;
static struct os_mbuf_pool *g_mbuf_pool=0;

#define CBOR_OVERHEAD    (36)

static struct os_mbuf*
buf_to_bota_nmgr_mbuf(uint8_t *buf, uint64_t len, uint64_t off, uint32_t size,
                      uint64_t src_slot, uint64_t dst_slot, uint64_t flags)
{
    int rc;
    CborEncoder payload_enc;
    struct mgmt_cbuf n_b;
    struct cbor_mbuf_writer writer;
    struct nmgr_hdr *hdr;
    struct os_mbuf *rsp;

    if (g_mbuf_pool) {
        rsp = os_mbuf_get_pkthdr(g_mbuf_pool, 0);
    } else {
        rsp = os_msys_get_pkthdr(0, 0);
    }

    if (!rsp) {
        BOTA_ERR("could not get mbuf %d\n", CBOR_OVERHEAD);
        return 0;
    }

    hdr = (struct nmgr_hdr *) os_mbuf_extend(rsp, sizeof(struct nmgr_hdr));
    if (!hdr) {
        BOTA_ERR("could not get hdr\n");
        goto exit_err;
    }
    hdr->nh_len = 0;
    hdr->nh_flags = 0;
    hdr->nh_op = NMGR_OP_WRITE;
    hdr->nh_group = htons(MGMT_GROUP_ID_BOTA);
    hdr->nh_seq = 0;
    hdr->nh_id = IMGMGR_NMGR_ID_UPLOAD;

    cbor_mbuf_writer_init(&writer, rsp);
    cbor_encoder_init(&n_b.encoder, &writer.enc, 0);
    rc = cbor_encoder_create_map(&n_b.encoder, &payload_enc, CborIndefiniteLength);
    if (rc != 0) {
        BOTA_ERR("could not create map\n");
        goto exit_err;
    }

    struct mgmt_cbuf *cb = &n_b;

    CborError g_err = CborNoError;
    /* Only pack in the flags, slotid etc on the first offset sent */
    if (off == 0) {
        g_err |= cbor_encode_text_stringz(&cb->encoder, "s");
        g_err |= cbor_encode_uint(&cb->encoder, dst_slot);
        g_err |= cbor_encode_text_stringz(&cb->encoder, "l");
        g_err |= cbor_encode_uint(&cb->encoder, size);
        g_err |= cbor_encode_text_stringz(&cb->encoder, "f");
        g_err |= cbor_encode_uint(&cb->encoder, flags);
    }
    g_err |= cbor_encode_text_stringz(&cb->encoder, "o");
    g_err |= cbor_encode_uint(&cb->encoder, off);
    g_err |= cbor_encode_text_stringz(&cb->encoder, "d");
    g_err |= cbor_encode_byte_string(&cb->encoder, buf, len);

    rc = cbor_encoder_close_container(&n_b.encoder, &payload_enc);
    if (rc != 0) {
        BOTA_ERR("could not close container\n");
        goto exit_err;
    }
    hdr->nh_len += cbor_encode_bytes_written(&n_b.encoder);
    hdr->nh_len = htons(hdr->nh_len);
    return rsp;

exit_err:
    os_mbuf_free_chain(rsp);
    return 0;
}

/* TODO: Cull leading/trailing 0xffff to speed up transfer  */
int
bcast_ota_get_packet(int src_slot, bcast_ota_mode_t mode, int max_transfer_unit,
                     struct os_mbuf **rsp, uint64_t flags)
{
    static uint32_t off = 0;
    int rc;
    int len;
    uint32_t img_flags;
    int bufsz;
    uint8_t *buf;
    const struct flash_area *s_fa;
    struct image_version ver;
    int fa_id = flash_area_id_from_image_slot(src_slot);

    /* Check source image */
    bufsz = max_transfer_unit-CBOR_OVERHEAD;
    if (mode == BCAST_MODE_RESET_OFFSET) {
        bufsz = sizeof(struct image_header)+8;
        off = 0;
        rc = imgr_read_info(src_slot, &ver, 0, &img_flags);
        if (rc != 0) {
            return 1;
        }
        BOTA_INFO("ver: %d.%d.%d.%d\n", ver.iv_major, ver.iv_minor,
                   ver.iv_revision, ver.iv_build_num);
    }

    buf = (uint8_t*)malloc(bufsz);
    if (!buf) {
        return OS_ENOMEM;
    }

    rc = flash_area_open(fa_id, &s_fa);
    if (mode == BCAST_MODE_RESEND_END) {
        /* Resend the end */
        off = s_fa->fa_size - bufsz;
    }

    if (off < s_fa->fa_size) {
        len = (int)s_fa->fa_size - (int)off;
        len = (len>bufsz) ? bufsz : len;
        rc = flash_area_read(s_fa, off, buf, len);

        BOTA_DEBUG("Reading flash at %lX, %d bytes rc=%d\n", off, len, rc);
        *rsp = buf_to_bota_nmgr_mbuf(buf, len, off, s_fa->fa_size, src_slot, 1, flags);

        if (*rsp == 0) {
            BOTA_ERR("Could not convert flash data to mbuf\n");
            rc = OS_ENOMEM;
            goto exit_err;
        }

        off += len;
    } else {
        *rsp = 0;
    }
exit_err:
    flash_area_close(s_fa);
    free(buf);
    return rc;
}


struct os_mbuf*
bcast_ota_get_reset_mbuf(void)
{
    struct nmgr_hdr *hdr;
    struct os_mbuf *rsp;

    if (g_mbuf_pool) {
        rsp = os_mbuf_get_pkthdr(g_mbuf_pool, 0);
    } else {
        rsp = os_msys_get_pkthdr(0, 0);
    }

    if (!rsp) {
        BOTA_ERR("could not get mbuf %d\n", CBOR_OVERHEAD);
        return 0;
    }

    hdr = (struct nmgr_hdr *) os_mbuf_extend(rsp, sizeof(struct nmgr_hdr));
    if (!hdr) {
        BOTA_ERR("could not get hdr\n");
        goto exit_err;
    }
    hdr->nh_len = 0;
    hdr->nh_flags = 0;
    hdr->nh_op = NMGR_OP_WRITE;
    hdr->nh_group = htons(MGMT_GROUP_ID_DEFAULT);
    hdr->nh_seq = 0;
    hdr->nh_id = NMGR_ID_RESET;
    return rsp;

exit_err:
    os_mbuf_free_chain(rsp);
    return 0;
}



void
bcast_ota_set_mpool(struct os_mbuf_pool *mbuf_pool)
{
    g_mbuf_pool = mbuf_pool;
}

void
bcast_ota_pkg_init(void)
{
    /* Ensure this function only gets called by sysinit. */
    SYSINIT_ASSERT_ACTIVE();

    /* Init log */
    log_register("bota", &g_bcast_ota_log, &log_console_handler,
                 NULL, LOG_SYSLEVEL);


    bcast_ota_nmgr_module_init();

#if MYNEWT_VAL(BCAST_OTA_CLI)
    int rc;
    rc = bota_cli_register();
    SYSINIT_PANIC_ASSERT(rc == 0);
#endif
}
