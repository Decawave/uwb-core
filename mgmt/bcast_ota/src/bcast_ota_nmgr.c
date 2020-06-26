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

#include <limits.h>
#include <assert.h>
#include <string.h>

#include "os/mynewt.h"
#include "hal/hal_bsp.h"
#include "flash_map/flash_map.h"
#include "cborattr/cborattr.h"
#include "bootutil/image.h"
#include "bootutil/bootutil.h"
#include "mgmt/mgmt.h"

#include "imgmgr/imgmgr.h"
#include <console/console.h>
#include <bcast_ota/bcast_ota.h>
#include <hal/hal_system.h>

#include "bcast_ota_priv.h"

struct bota_state {
    struct {
        uint32_t off;
        uint32_t size;
        uint32_t upl_errs;
        uint8_t slot_id;
        uint8_t fa_id;
        uint64_t flags;
    } upload;
};

static int bota_upload(struct mgmt_cbuf *);
static int bota_confirm(struct mgmt_cbuf *);
static new_fw_cb *_new_image_cb;

static const struct mgmt_handler bota_nmgr_handlers[] = {
    [IMGMGR_NMGR_ID_UPLOAD] = {
        .mh_read = NULL,
        .mh_write = bota_upload
    },
    [IMGMGR_NMGR_ID_STATE] = {
        .mh_read = NULL,
        .mh_write = bota_confirm
    }
};

#define BOTA_HANDLER_CNT                                                \
    sizeof(bota_nmgr_handlers) / sizeof(bota_nmgr_handlers[0])

static struct mgmt_group bota_nmgr_group = {
    .mg_handlers = (struct mgmt_handler *)bota_nmgr_handlers,
    .mg_handlers_count = BOTA_HANDLER_CNT,
    .mg_group_id = MGMT_GROUP_ID_BOTA,
};

struct bota_state bota_state = {0};

#define TMPBUF_SZ  256
static int
bota_check_image(const struct flash_area *fap)
{
    int rc;
    struct image_header hdr;
    uint8_t hash[IMGMGR_HASH_LEN];

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
    return rc;
}

static int
fa_copy(const struct flash_area *s_fa, const struct flash_area *d_fa)
{
    int rc;
    int len;
    uint32_t off;
    const int bufsz = 256;
    uint8_t *buf = (uint8_t*)malloc(bufsz);
    if (!buf) {
        BOTA_ERR("FA cp failed, not enough memory\n");
        return 1;
    }

    off = 0;
    rc = flash_area_erase(d_fa, 0, s_fa->fa_size);
    while (off < s_fa->fa_size) {
        len = (int)s_fa->fa_size - (int)off;
        len = (len>bufsz) ? bufsz : len;
        rc = flash_area_read(s_fa, off, buf, len);
        rc = flash_area_write(d_fa, off, buf, len);
        off += len;
    }
    BOTA_INFO("FA cp complete, off=%6ld\n", off);
    free(buf);
    return rc;
}

int
fa_copy_by_id(int s_fa_id, int d_fa_id)
{
    int rc;
    const struct flash_area *s_fa;
    const struct flash_area *d_fa;

    rc = flash_area_open(s_fa_id, &s_fa);
    rc = flash_area_open(d_fa_id, &d_fa);

    rc = fa_copy(s_fa, d_fa);

    flash_area_close(s_fa);
    flash_area_close(d_fa);
    return rc;
}


static int
bota_upload(struct mgmt_cbuf *cb)
{
    uint8_t *img_data = (uint8_t*)malloc(MYNEWT_VAL(IMGMGR_MAX_CHUNK_SIZE));
    uint64_t off = UINT_MAX;
    uint64_t size = UINT_MAX;
    uint64_t slot = UINT_MAX;
    uint64_t flags = UINT_MAX;
    bool erase = 0;
    size_t data_len = 0;

    if (!img_data) {
        BOTA_ERR("ERR no mem\n");
        assert(0);
        return OS_ENOMEM;
    }

    const struct cbor_attr_t off_attr[] = {
        [0] = {
            .attribute = "d",
            .type = CborAttrByteStringType,
            .addr.bytestring.data = img_data,
            .addr.bytestring.len = &data_len,
            .len = MYNEWT_VAL(IMGMGR_MAX_CHUNK_SIZE)
        },
        [1] = {
            .attribute = "l",
            .type = CborAttrUnsignedIntegerType,
            .addr.uinteger = &size,
            .nodefault = true
        },
        [2] = {
            .attribute = "o",
            .type = CborAttrUnsignedIntegerType,
            .addr.uinteger = &off,
            .nodefault = true
        },
        [3] = {
            .attribute = "s",
            .type = CborAttrUnsignedIntegerType,
            .addr.uinteger = &slot,
            .nodefault = true
        },
        [4] = {
            .attribute = "f",
            .type = CborAttrUnsignedIntegerType,
            .addr.uinteger = &flags,
            .nodefault = true
        },
        [5] = { 0 },
    };
    struct image_header *new_hdr;
    struct image_header tmp_hdr;
    int rc;
    CborError g_err = CborNoError;
    const struct flash_area *tmp_fa;

    rc = cbor_read_object(&cb->it, off_attr);
    if (rc || off == UINT_MAX) {
        BOTA_ERR("ERR read_failed rc %d off %llx\n", rc, off);
        free(img_data);
        return MGMT_ERR_EINVAL;
    }

    BOTA_DEBUG("bota: l%ld(%lu),o%ld,s%ld(%d),e%d,f%llx(%llx) ec:%ld\n",
               (uint32_t)size, bota_state.upload.size,
               (uint32_t)off, (uint32_t)slot, bota_state.upload.slot_id, erase,
               flags, bota_state.upload.flags, bota_state.upload.upl_errs
        );

    if (slot != UINT_MAX) {
        bota_state.upload.slot_id = slot;
        bota_state.upload.fa_id = flash_area_id_from_image_slot(slot);
    }

    if (size != UINT_MAX) {
        bota_state.upload.size = size;
    }

    if (flags != UINT_MAX) {
        bota_state.upload.flags = flags;
    }

    /* Check that we're not corrupting the image in slot 0 */
    if (bota_state.upload.fa_id < flash_area_id_from_image_slot(1)) {
        BOTA_ERR("ERR Unknown fa_id(%d)\n", bota_state.upload.fa_id);
        free(img_data);
        return MGMT_ERR_EINVAL;
    }

#if MYNEWT_VAL(BCAST_OTA_SCRATCH_ENABLED)
    rc = flash_area_open(MYNEWT_VAL(BCAST_OTA_FLASH_SCRATCH), &tmp_fa);
#else
    rc = flash_area_open(bota_state.upload.fa_id, &tmp_fa);
#endif

    if(rc){
        free(img_data);
        return MGMT_ERR_EINVAL;
    }
    if (!tmp_fa) {
        free(img_data);
        return MGMT_ERR_EINVAL;
    }

    /* Only allow erase for certain paramters */
    if (off == 0) {
        if (data_len < sizeof(struct image_header)) {
            /*
             * Image header is the first thing in the image.
             */
            rc = MGMT_ERR_EINVAL;
            goto err_close;
        }
        new_hdr = (struct image_header *)img_data;
        if (new_hdr->ih_magic != IMAGE_MAGIC) {
            rc = MGMT_ERR_EINVAL;
            goto err_close;
        }
        /* TODO: Also compare target / bps + app names and use a seq_num on the
         * txim to match? Or use core/util/crc/crc16 to encode bsp+app and label with */
        /* Only erase flash if new version is different to what we have */
        rc = flash_area_read(tmp_fa, 0, &tmp_hdr, sizeof(struct image_header));
        erase = 0;
        if (tmp_hdr.ih_magic != IMAGE_MAGIC ||
            new_hdr->ih_ver.iv_major     != tmp_hdr.ih_ver.iv_major ||
            new_hdr->ih_ver.iv_minor     != tmp_hdr.ih_ver.iv_minor ||
            new_hdr->ih_ver.iv_revision  != tmp_hdr.ih_ver.iv_revision ||
            new_hdr->ih_ver.iv_build_num != tmp_hdr.ih_ver.iv_build_num) {
            erase = 1;
        }
        bota_state.upload.off = 0;
        bota_state.upload.size = size;
        bota_state.upload.upl_errs = 0;

        /*
         * New upload?
         */
        if (erase==1) {
            BOTA_DEBUG("### New upload: %d.%d.%d.%d\n", new_hdr->ih_ver.iv_major,
                       new_hdr->ih_ver.iv_minor, new_hdr->ih_ver.iv_revision,
                       new_hdr->ih_ver.iv_build_num);

            BOTA_DEBUG("### Erasing flash ###\n");
            rc = flash_area_erase(tmp_fa, 0, tmp_fa->fa_size);
            if (rc) {
                goto err_close;
            }
        } else {
            BOTA_DEBUG("### Continuing upload of: %d.%d.%d.%d\n", tmp_hdr.ih_ver.iv_major,
                       tmp_hdr.ih_ver.iv_minor, tmp_hdr.ih_ver.iv_revision,
                       tmp_hdr.ih_ver.iv_build_num);
        }
    }


    if (data_len) {
        rc = flash_area_write(tmp_fa, off, img_data, data_len);
        if (rc) {
            rc = MGMT_ERR_EINVAL;
            goto err_close;
        }
        if (bota_state.upload.off != off) {
            bota_state.upload.upl_errs++;
        }
        bota_state.upload.off = off+data_len;
    }

    if (bota_state.upload.size == bota_state.upload.off) {
        /* Done */
        BOTA_DEBUG("#### All done, checking image\n");
        rc = bota_check_image(tmp_fa);

        if (rc != 0) {
            BOTA_DEBUG("#### Hash / image failed\n");
            goto out;
        }

#if MYNEWT_VAL(BCAST_OTA_SCRATCH_ENABLED)
        const struct flash_area *mcu_fa;
        BOTA_DEBUG("#### Hash ok, copy from scratch \n");
        rc = flash_area_open(bota_state.upload.fa_id, &mcu_fa);
        fa_copy(tmp_fa, mcu_fa);
        rc = bota_check_image(mcu_fa);
        flash_area_close(mcu_fa);
        if (rc != 0) {
            BOTA_DEBUG("#### Hash post copy image failed\n");
            goto out;
        }
#endif
        BOTA_DEBUG("#### Hash ok, set perm? %d \n",
                   (bota_state.upload.flags&BOTA_FLAGS_SET_PERMANENT)?1:0);
        rc = boot_set_pending((bota_state.upload.flags&BOTA_FLAGS_SET_PERMANENT)?1:0);
        BOTA_INFO("#### Will boot into new image at next boot\n", rc);
        os_time_delay(OS_TICKS_PER_SEC);
        if (_new_image_cb) {
            _new_image_cb();
        }
    }

out:
    free(img_data);
    flash_area_close(tmp_fa);


    g_err |= cbor_encode_text_stringz(&cb->encoder, "rc");
    g_err |= cbor_encode_int(&cb->encoder, MGMT_ERR_EOK);
    g_err |= cbor_encode_text_stringz(&cb->encoder, "off");
    g_err |= cbor_encode_int(&cb->encoder, bota_state.upload.off);

    if (g_err) {
        return MGMT_ERR_ENOMEM;
    }
    return 0;
err_close:
    free(img_data);
    flash_area_close(tmp_fa);
    return rc;
}

static int
bota_confirm(struct mgmt_cbuf *cb)
{
    CborError g_err = CborNoError;
    g_err |= cbor_encode_text_stringz(&cb->encoder, "rc");
    uint64_t rc = boot_set_confirmed();
    if (rc == OS_OK) {
        BOTA_INFO("#### Image confirmed, rc=%d\n", rc);
        g_err |= cbor_encode_int(&cb->encoder, MGMT_ERR_EOK);
    } else {
        g_err |= cbor_encode_int(&cb->encoder, MGMT_ERR_EUNKNOWN);
    }

    if (g_err) {
        return MGMT_ERR_ENOMEM;
    }
    return 0;
}


void
bcast_ota_set_new_fw_cb(new_fw_cb *cb)
{
    _new_image_cb = cb;
}

void
bcast_ota_nmgr_module_init(void)
{
    int rc;

    /* Ensure this function only gets called by sysinit. */
    SYSINIT_ASSERT_ACTIVE();

    rc = mgmt_group_register(&bota_nmgr_group);
    SYSINIT_PANIC_ASSERT(rc == 0);

#if MYNEWT_VAL(BCAST_OTA_REBOOT_ON_NEW_IMAGE)
    bcast_ota_set_new_fw_cb(hal_system_reset);
#endif
}
