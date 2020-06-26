#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <dpl/dpl.h>
#include <sysinit/sysinit.h>
#if MYNEWT_VAL(PANMASTER_CBOR_EXPORT)

#include <shell/shell.h>
#include <console/console.h>

#include "panmaster/panmaster.h"
#include "panmaster_priv.h"
#include "imgmgr/imgmgr.h"

#include <tinycbor/cbor.h>
#include <tinycbor/cborjson.h>
#include <tinycbor/cbor_mbuf_writer.h>
#include <tinycbor/cbor_mbuf_reader.h>
#include <cborattr/cborattr.h>
#include <cbor_flash_writer/cbor_flash_writer.h>


static void
list_nodes_load_cb(struct panmaster_node *node, void *cb_arg)
{
    struct list_nodes_extract *lne = (struct list_nodes_extract*)cb_arg;

    if (node->index < lne->index_max  &&
        node->index >= lne->index_off) {
        memcpy(&lne->nodes[node->index - lne->index_off], node,
               sizeof(struct panmaster_node));
    }
}

#define LIST_NODES_BLK_NNODES (32)
static int
cbor_nodes_list(struct CborEncoder *encoder)
{
    int i,j;
    struct panmaster_node_idx *node_idx;
    struct dpl_timeval utctime;
    struct dpl_timezone timezone;
    struct list_nodes_extract lne;

    struct CborEncoder payload_enc;
    struct CborEncoder data_array_enc;
    struct CborEncoder sns_enc;

    int lne_nodes_sz = sizeof(struct panmaster_node)*LIST_NODES_BLK_NNODES;
    lne.nodes = (struct panmaster_node*)malloc(lne_nodes_sz);

    if (!lne.nodes) {
        console_printf("err:not enough memory\n");
        return 0;
    }

    char ver_str[IMGMGR_NMGR_MAX_VER];
    int num_nodes;

    cbor_encoder_create_map(encoder, &payload_enc, CborIndefiniteLength);

    panmaster_node_idx(&node_idx, &num_nodes);
    dpl_gettimeofday(&utctime, &timezone);

    CborError g_err = CborNoError;
    /* Format version */
    g_err |= cbor_encode_text_stringz(encoder, "v");
    g_err |= cbor_encode_uint(encoder, 1);

    g_err |= cbor_encode_text_stringz(encoder, "panm");
    g_err |= cbor_encode_uint(encoder, 1);

    /* Time of report */
    g_err |= cbor_encode_text_stringz(encoder, "t0");
    g_err |= cbor_encode_int(encoder, utctime.tv_sec);

    /* Node-data array */
    g_err |= cbor_encode_text_stringz(encoder, "d");
    cbor_encoder_create_array(encoder, &data_array_enc, CborIndefiniteLength);

    for (i=0;i<num_nodes;i+=LIST_NODES_BLK_NNODES) {
        lne.index_off = i;
        lne.index_max = i+LIST_NODES_BLK_NNODES;
        memset(lne.nodes, 0xffff, lne_nodes_sz);
        panmaster_load(list_nodes_load_cb, &lne);

        for (j=0;j<LIST_NODES_BLK_NNODES;j++) {

            if (lne.nodes[j].addr == 0xffff) {
                continue;
            }

            cbor_encoder_create_map(encoder, &sns_enc, CborIndefiniteLength);

            g_err |= cbor_encode_text_stringz(encoder, "i");
            g_err |= cbor_encode_uint(encoder, i+j);

            g_err |= cbor_encode_text_stringz(encoder, "saddr");
            g_err |= cbor_encode_uint(encoder, lne.nodes[j].addr);

            g_err |= cbor_encode_text_stringz(encoder, "euid");
            g_err |= cbor_encode_uint(encoder, lne.nodes[j].euid);

            if (lne.nodes[j].flags) {
                g_err |= cbor_encode_text_stringz(encoder, "f");
                g_err |= cbor_encode_uint(encoder, lne.nodes[j].flags);
            }

            g_err |= cbor_encode_text_stringz(encoder, "addt");
            g_err |= cbor_encode_int(encoder, lne.nodes[j].first_seen_utc -
                                     utctime.tv_sec);

            imgr_ver_str(&lne.nodes[j].fw_ver, ver_str);
            g_err |= cbor_encode_text_stringz(encoder, "v");
            g_err |= cbor_encode_text_stringz(encoder, ver_str);

            /* End of node map */
            cbor_encoder_close_container(encoder, &sns_enc);
        }
    }
    cbor_encoder_close_container(encoder, &data_array_enc);
    cbor_encoder_close_container(encoder, &payload_enc);

    free(lne.nodes);
    return 0;
}

int
panmaster_cbor_nodes_list_fa(const struct flash_area *fa, int *fa_offset)
{
    int rc;
    struct CborEncoder encoder;
    struct cbor_flash_writer writer;
    cbor_flash_writer_init(&writer, fa, *fa_offset);
    cbor_encoder_init(&encoder, &writer.enc, 0);

    rc = cbor_nodes_list(&encoder);

    *fa_offset = writer.offset;
    return rc;
}

struct os_mbuf*
panmaster_cbor_nodes_list(struct os_mbuf_pool *mbuf_pool)
{
    int rc;
    struct CborEncoder encoder;
    struct cbor_mbuf_writer writer;
    struct os_mbuf *rsp;

    rsp = os_mbuf_get_pkthdr(mbuf_pool,0);
    if (!rsp)
    {
        console_printf("mbuf fail\n");
        return 0;
    }

    cbor_mbuf_writer_init(&writer, rsp);
    cbor_encoder_init(&encoder, &writer.enc, 0);

    rc = cbor_nodes_list(&encoder);
    if (rc != 0) {
        os_mbuf_free_chain(rsp);
        return 0;
    }
    return rsp;
}

#endif /* MYNEWT_VAL(PANMASTER_CBOR_EXPORT) */
