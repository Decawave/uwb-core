#include <string.h>
#include <math.h>
#include <os/mynewt.h>
#include <syscfg/syscfg.h>
#if MYNEWT_VAL(UWB_CCP_ENABLED)
#include <uwb_ccp/uwb_ccp.h>
#endif
#include <tofdb/tofdb.h>

int tofdb_cli_register();

static struct tofdb_node nodes[MYNEWT_VAL(TOFDB_MAXNUM_NODES)]; /* ca 18b/node */

struct tofdb_node*
tofdb_get_nodes()
{
    return nodes;
}

int tofdb_get_tof(uint16_t addr, uint32_t *tof)
{
    if (!tof) {
        return OS_EINVAL;
    }
    for (int i=0;i<MYNEWT_VAL(TOFDB_MAXNUM_NODES);i++) {
        if (addr && addr == nodes[i].addr) {
            *tof = (uint32_t)nodes[i].tof;
            goto ret;
        }
    }
    return DPL_ENOENT;
ret:
    return OS_OK;
}

void clear_nodes()
{
    memset(nodes, 0, sizeof(nodes));
}

int tofdb_set_tof(uint16_t addr, uint32_t tof)
{
    int i;

    /* See if this entry exist in our database already */
    for (i=0;i<MYNEWT_VAL(TOFDB_MAXNUM_NODES);i++) {
        if (addr && addr == nodes[i].addr) {
            float d = tof - nodes[i].tof;
            if (fabsf(d) > (2.0f/0.047f)) {
                /* Filter out measurements more than 2m from previous average */
                goto ret;
            }
#if MYNEWT_VAL(TOFDB_MAXNUM_UPDATES) > 1
            if (nodes[i].num > (MYNEWT_VAL(TOFDB_MAXNUM_UPDATES)-1)) {
                goto ret;
            }
#endif
            nodes[i].num++;
            nodes[i].sum += tof;
            nodes[i].sum_sq += (float)tof*(float)tof;
            nodes[i].tof = nodes[i].sum/nodes[i].num;

            nodes[i].last_updated = os_cputime_get32();
            goto ret;
        }
    }

    /* No match, look for a free spot */
    for (i=0;i<MYNEWT_VAL(TOFDB_MAXNUM_NODES);i++) {
        if (nodes[i].addr) {
            continue;
        }

        nodes[i].addr = addr;
        nodes[i].last_updated = os_cputime_get32();
        nodes[i].tof = tof;
        nodes[i].sum = tof;
        nodes[i].sum_sq = (float)tof*(float)tof;
        nodes[i].num = 1;
        goto ret;
    }

    if (i==MYNEWT_VAL(TOFDB_MAXNUM_NODES)) {
        return OS_ENOMEM;
    }
ret:
    return OS_OK;
}

#if MYNEWT_VAL(UWB_CCP_ENABLED)
#if MYNEWT_VAL(UWB_DEVICE_0)
static uint32_t
ccp_cb(uint16_t short_addr)
{
    uint32_t tof=0;
    tofdb_get_tof(short_addr, &tof);
    return tof;
}
#endif
#endif

void
tofdb_pkg_init(void)
{
    int rc;
#if MYNEWT_VAL(TOFDB_CLI)
    rc = tofdb_cli_register();
    SYSINIT_PANIC_ASSERT(rc == 0);
#endif

    memset(nodes, 0, sizeof(nodes));
    /*  */
#if MYNEWT_VAL(UWB_CCP_ENABLED)

#if MYNEWT_VAL(UWB_DEVICE_0)
    struct uwb_ccp_instance *ccp = (struct uwb_ccp_instance*)uwb_mac_find_cb_inst_ptr(uwb_dev_idx_lookup(0), UWBEXT_CCP);
    uwb_ccp_set_tof_comp_cb(ccp, ccp_cb);
#endif
#endif
}
