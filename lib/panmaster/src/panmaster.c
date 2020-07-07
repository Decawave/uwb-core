#include <string.h>
#include <dpl/dpl.h>
#include <syscfg/syscfg.h>
#include <sysinit/sysinit.h>
#include <log/log.h>
#include <config/config.h>

#include "panmaster/panmaster.h"
#include "panmaster_priv.h"

// #define VERBOSE

#if MYNEWT_VAL(UWB_PAN_ENABLED)
#include <uwb_pan/uwb_pan.h>
#endif

static struct panmaster_node_idx node_idx[MYNEWT_VAL(PANMASTER_MAXNUM_NODES)];
static struct dpl_mutex save_mutex;
static struct panmaster_node nodes_to_save[MYNEWT_VAL(PANMASTER_NODES_TO_SAVE)];

static uint16_t pan_id = 0x0000;
static volatile int nodes_loaded = 0;

#define LOG_MODULE_PAN_MASTER (91)
#define PM_INFO(...)     LOG_INFO(&_log, LOG_MODULE_PAN_MASTER, __VA_ARGS__)
#define PM_DEBUG(...)    LOG_DEBUG(&_log, LOG_MODULE_PAN_MASTER, __VA_ARGS__)
#define PM_WARN(...)     LOG_WARN(&_log, LOG_MODULE_PAN_MASTER, __VA_ARGS__)
#define PM_ERR(...)      LOG_ERROR(&_log, LOG_MODULE_PAN_MASTER, __VA_ARGS__)
static struct log _log;

/*
 * Config
 */
static char *pm_conf_get(int argc, char **argv, char *val, int val_len_max);
static int pm_conf_set(int argc, char **argv, char *val);
static int pm_conf_commit(void);
static int pm_conf_export(void (*export_func)(char *name, char *val),
  enum conf_export_tgt tgt);

static struct pm_config_s {
    char pan_id[8];
} pm_config = {.pan_id=MYNEWT_VAL(PANMASTER_DEFAULT_PANID)};

static struct conf_handler pm_conf_cbs = {
    .ch_name = "panmstr",
    .ch_get = pm_conf_get,
    .ch_set = pm_conf_set,
    .ch_commit = pm_conf_commit,
    .ch_export = pm_conf_export,
};

static char *
pm_conf_get(int argc, char **argv, char *val, int val_len_max)
{
    if (argc == 1) {
        if (!strcmp(argv[0], "pan_id")) {
            return pm_config.pan_id;
        }
    }
    return NULL;
}

static int
pm_conf_set(int argc, char **argv, char *val)
{
    if (argc == 1) {
        if (!strcmp(argv[0], "pan_id")) {
            return CONF_VALUE_SET(val, CONF_STRING, pm_config.pan_id);
        }
    }
    return DPL_ENOENT;
}

static int
pm_conf_commit(void)
{
    conf_value_from_str(pm_config.pan_id, CONF_INT16,
                        (void*)&pan_id, 0);
    return 0;
}

static int
pm_conf_export(void (*export_func)(char *name, char *val),
  enum conf_export_tgt tgt)
{
    export_func("panmstr/pan_id", pm_config.pan_id);
    return 0;
}

#if MYNEWT_VAL(PANMASTER_NFFS)
#include "fs/fs.h"

static struct panm_file panmaster_storage_file = {
    .pf_name = MYNEWT_VAL(PANMASTER_NFFS_FILE),
    .pf_maxlines = MYNEWT_VAL(PANMASTER_NFFS_MAX_LINES)
};

#elif MYNEWT_VAL(PANMASTER_FCB)
#include "fcb/fcb.h"
#include "panmaster/panmaster_fcb.h"

static struct flash_area pm_fcb_area[MYNEWT_VAL(PANMASTER_FCB_NUM_AREAS) + 1];

static struct panm_fcb pm_init_conf_fcb = {
    .pm_fcb.f_magic = MYNEWT_VAL(PANMASTER_FCB_MAGIC),
    .pm_fcb.f_sectors = pm_fcb_area,
};

static void
panm_init_fcb(void)
{
    int cnt;
    int rc;

    rc = flash_area_to_sectors(MYNEWT_VAL(PANMASTER_FCB_FLASH_AREA), &cnt, NULL);
    SYSINIT_PANIC_ASSERT(rc == 0);
    SYSINIT_PANIC_ASSERT(
        cnt <= sizeof(pm_fcb_area) / sizeof(pm_fcb_area[0]));
    flash_area_to_sectors(
        MYNEWT_VAL(PANMASTER_FCB_FLASH_AREA), &cnt, pm_fcb_area);

    pm_init_conf_fcb.pm_fcb.f_sector_cnt = cnt;

    rc = panm_fcb_src(&pm_init_conf_fcb);
    if (rc) {
        for (cnt = 0;
             cnt < pm_init_conf_fcb.pm_fcb.f_sector_cnt;
             cnt++) {

            flash_area_erase(&pm_fcb_area[cnt], 0,
                             pm_fcb_area[cnt].fa_size);
        }
        rc = panm_fcb_src(&pm_init_conf_fcb);
    }
    SYSINIT_PANIC_ASSERT(rc == 0);
}
#endif

bool
panrequest_cb(uint64_t euid, struct pan_req_resp *request,
                                                struct pan_req_resp *response)
{
    struct dpl_timeval tv;
    struct panmaster_node *node = 0;

    panmaster_idx_find_node(euid, request->role, &node);
    if (!node) {
        return false;
    }

    /* Copy the fw_version before overwriting the union */
    memcpy(&node->fw_ver, &request->fw_ver, sizeof(struct pan_image_version));

    /* Prepare response */
    response->short_address = node->addr;
    response->slot_id = node_idx[node->index].slot_id;
    dpl_get_uptime(&tv);
    if (response->lease_time == 0) {
        response->lease_time = MYNEWT_VAL(PANMASTER_DEFAULT_LEASE_TIME);
    }
    /* Calculate when this lease ends in ms */
    node_idx[node->index].lease_ends = tv.tv_sec*1000 + tv.tv_usec/1000 + (uint32_t)request->lease_time*1000;
    response->pan_id = pan_id;
    response->role = node->role;

    /* PAN Request frame */
    return true;
}

void
panmaster_postprocess(void)
{
    int i, slots;
    struct panmaster_node node;

    if (dpl_mutex_pend(&save_mutex, DPL_WAIT_FOREVER) != DPL_OK) {
        return;
    }
    slots = sizeof(nodes_to_save)/sizeof(nodes_to_save[0]);
    for (i=0;i < slots;i++) {
        if (nodes_to_save[i].euid) {
            /* Take a tmp copy of this entry and save */
            memcpy(&node, &nodes_to_save[i], sizeof(node));
            dpl_mutex_release(&save_mutex);
            panmaster_update_node(node.euid, &node);
            if (dpl_mutex_pend(&save_mutex, DPL_WAIT_FOREVER) != DPL_OK) {
                return;
            }
            memset(&nodes_to_save[i], 0, sizeof(node));
        }
    }

    dpl_mutex_release(&save_mutex);
}

void
postprocess_cb(struct dpl_event * ev)
{
    assert(ev != NULL);
    assert(dpl_event_get_arg(ev));

    struct uwb_pan_instance * pan = (struct uwb_pan_instance *)ev->ev.ev_arg;

    if (pan->config->role != UWB_PAN_ROLE_MASTER) {
        return;
    }
    panmaster_postprocess();
}

void
panmaster_node_idx(struct panmaster_node_idx **node_idx_arg, int *num_nodes)
{
    *node_idx_arg = node_idx;
    *num_nodes = MYNEWT_VAL(PANMASTER_MAXNUM_NODES);
}

int
panmaster_find_node_general(struct find_node_s *fns)
{
#if MYNEWT_VAL(PANMASTER_NFFS)
    return panm_file_find_node(&panmaster_storage_file, fns);
#elif MYNEWT_VAL(PANMASTER_FCB)
    return panm_fcb_find_node(&pm_init_conf_fcb, fns);
#endif
}

int
panmaster_clear_list()
{
    int i;
    for (i=0;i<MYNEWT_VAL(PANMASTER_MAXNUM_NODES);i++)
    {
        PANMASTER_NODE_IDX_DEFAULT(node_idx[i]);
    }
#if MYNEWT_VAL(PANMASTER_NFFS)
    return fs_unlink(panmaster_storage_file.pf_name);
#elif MYNEWT_VAL(PANMASTER_FCB)
    panm_fcb_clear(&pm_init_conf_fcb);
#endif
    return 0;
}

static uint16_t
first_free_short_addr(uint64_t euid)
{
    int i, j;
    uint16_t addr = euid&0xffff;
    for (i=0;i<MYNEWT_VAL(PANMASTER_MAXNUM_NODES);i++)
    {
        if (i>0) {
            addr = (i<<12) | (euid&0x0fff);
        }

        for (j=0;j<MYNEWT_VAL(PANMASTER_MAXNUM_NODES);j++)
        {
            if (addr == node_idx[j].addr)
            {
                addr = 0;
                break;
            }
        }

        if (addr) {
            return addr;
        }
    }

    return addr;
}

static bool
short_addr_is_free(uint16_t addr)
{
    int j;
    for (j=0;j<MYNEWT_VAL(PANMASTER_MAXNUM_NODES);j++)
    {
        if (addr == node_idx[j].addr)
        {
            return false;
        }
    }
    return true;
}

static bool
slot_lease_expired(int idx)
{
    struct dpl_timeval tv;
    dpl_get_uptime(&tv);
    int32_t le_ms = node_idx[idx].lease_ends;
    if (le_ms==0) return false;
    int32_t now_ms = tv.tv_sec*1000 + tv.tv_usec/1000;
    return now_ms > le_ms;
}

static uint16_t
first_free_slot_id(uint16_t node_addr, uint16_t role)
{
    int j;
    uint16_t slot_id=0;
    while (slot_id < 0xffff)
    {
        for (j=0;j<MYNEWT_VAL(PANMASTER_MAXNUM_NODES);j++)
        {
            if (node_idx[j].addr == 0xffff || role != node_idx[j].role) {
                continue;
            }

            if (slot_id == node_idx[j].slot_id &&
                node_addr != node_idx[j].addr &&
                (slot_lease_expired(j) == false || node_idx[j].has_perm_slot)
                )
            {
                goto next_slot;
            }
        }

        return slot_id;
    next_slot:
        slot_id++;
    }

    return 0xffff;
}

static bool
find_first_empty_nodes_to_save(uint64_t euid, struct panmaster_node **results)
{
    int i, slots;
    if (dpl_mutex_pend(&save_mutex, 0) != DPL_OK) {
        return false;
    }
    slots = sizeof(nodes_to_save)/sizeof(nodes_to_save[0]);
    for (i=0;i < slots;i++) {
        if (!nodes_to_save[i].euid) {
            nodes_to_save[i].euid = euid;
            *results = &nodes_to_save[i];
            break;
        }
    }

    dpl_mutex_release(&save_mutex);
    /* If i == slots then we failed to find a free slot */
    return (i!=slots) ? true : false;
}

int
panmaster_idx_find_node(uint64_t euid, uint16_t role, struct panmaster_node **results)
{
    int i;
    struct dpl_timeval utctime;
    struct panmaster_node *node;
    bool node_is_found = false;

    if (!find_first_empty_nodes_to_save(euid, &node)) {
        return 1;
    }

    /* Look for an existing node */
    for (i=0;i<MYNEWT_VAL(PANMASTER_MAXNUM_NODES);i++) {
        if (node_idx[i].addr == 0xffff) {
            continue;
        }
        if (euid == node_idx[i].euid) {
            node_is_found = true;
            node->index = i;
            break;
        }
    }

    if (node_is_found) {
        *results = node;

        node->addr = node_idx[i].addr;
        node->euid = node_idx[i].euid;
        node->role = node_idx[i].role;
        node->has_perm_slot = node_idx[i].has_perm_slot;
        node->slot_id = node_idx[node->index].slot_id;
        if (!node->has_perm_slot) {
            node->slot_id = first_free_slot_id(node->addr, role);
            node_idx[node->index].slot_id = node->slot_id;
        }

        /* Only check role if given */
        if (node->role != role && role > 0) {
            node->role = role;
            node_idx[i].save_needed = 1;
        }
        return 0;
    }

    /* This node is unknown, find a free spot for it */
    for (i=0;i<MYNEWT_VAL(PANMASTER_MAXNUM_NODES);i++)
    {
        if (node_idx[i].addr != 0xffff) {
            continue;
        }

        PANMASTER_NODE_DEFAULT(*node);
        dpl_gettimeofday(&utctime, 0);
        node->euid = euid;
        node->addr = first_free_short_addr(euid);
        node_idx[i].addr = node->addr;
        node_idx[i].euid = node->euid;
        node_idx[i].role = role;
        node->role = role;
        /* New-node, default is non-permanent slot */
        node->slot_id = first_free_slot_id(node->addr, role);
        node_idx[i].slot_id = node->slot_id;
        node->first_seen_utc = utctime.tv_sec;
        node->index = i;

        *results = node;
        node_idx[i].save_needed = 1;
        break;
    }

    return 0;
}

int
panmaster_load(panm_load_cb cb, void *cb_arg)
{
#if MYNEWT_VAL(PANMASTER_NFFS)
    return panm_file_load(&panmaster_storage_file, cb, cb_arg);
#elif MYNEWT_VAL(PANMASTER_FCB)
    return panm_fcb_load(&pm_init_conf_fcb, cb, cb_arg);
#endif
}

void
panmaster_add_version(uint64_t euid, struct pan_image_version *ver)
{
    struct panmaster_node node;
    struct find_node_s fns = { .results = &node };

    PANMASTER_NODE_DEFAULT(fns.find);
    fns.find.euid = euid;
    panmaster_find_node_general(&fns);

    if (fns.is_found)
    {
        if (ver->iv_major     == node.fw_ver.iv_major &&
            ver->iv_minor     == node.fw_ver.iv_minor &&
            ver->iv_revision  == node.fw_ver.iv_revision &&
            ver->iv_build_num == node.fw_ver.iv_build_num) {
            return;
        }

        memcpy(&node.fw_ver, ver, sizeof(*ver));
        panmaster_save_node(&node);
        return;
    }
    /* Node not found, just return */
    return;
}

void
panmaster_update_node(uint64_t euid, struct panmaster_node *upd_node)
{
    bool save_needed = true;
    struct panmaster_node stored_node;
    struct find_node_s fns = { .results = &stored_node };

    PANMASTER_NODE_DEFAULT(fns.find);
    fns.find.euid = euid;
    panmaster_find_node_general(&fns);

    if (fns.is_found) {
        save_needed = false;
        /* Compare fw version */
        if (memcmp(&upd_node->fw_ver, &stored_node.fw_ver, sizeof(upd_node->fw_ver)) != 0 ||
            upd_node->first_seen_utc != stored_node.first_seen_utc ||
            upd_node->addr != stored_node.addr ||
            upd_node->flags != stored_node.flags ||
            upd_node->index != stored_node.index
            ) {
            save_needed = true;
        }
    }

    if (save_needed) {
        panmaster_save_node(upd_node);
    }
}


void
panmaster_add_node(uint16_t short_addr, uint16_t role, uint8_t *euid_u8)
{
    int i;
    int rc;
    struct dpl_timeval utctime;
    struct panmaster_node node;
    struct find_node_s fns = { .results = &node };
    uint64_t euid;

    /* No point to use data if short_addr == 0*/
    if (short_addr==0) {
        return;
    }

    /* First check so this isn't a reset package */
    rc = 1;
    for (i=0;i<sizeof(uint64_t)&&rc;i++) {
        if (euid_u8[i]!=0xff) {
            rc = 0;
        }
    }
    if (rc==1) {
        return;
    }
    memcpy(&euid, euid_u8, sizeof(uint64_t));

    PANMASTER_NODE_DEFAULT(fns.find);
    fns.find.euid = euid;
    panmaster_find_node_general(&fns);

    if (fns.is_found)
    {
        if (node.addr == short_addr) {
            return;
        }
        node.addr = short_addr;

#if MYNEWT_VAL(PANMASTER_NFFS)
        panm_file_save(&panmaster_storage_file, &node);
#elif MYNEWT_VAL(PANMASTER_FCB)
        panm_fcb_save(&pm_init_conf_fcb, &node);
#endif
        PM_DEBUG("panm: node upd\n");
        return;
    }

    /* New node */
    PANMASTER_NODE_DEFAULT(node);

    /* This node is unknown, find a free spot for it */
    for (i=0;i<MYNEWT_VAL(PANMASTER_MAXNUM_NODES);i++)
    {
        if (node_idx[i].addr != 0xffff) {
            continue;
        }

        dpl_gettimeofday(&utctime, 0);
        node.euid = euid;

        if (!short_addr_is_free(short_addr)) {
            PM_ERR("Dupl short addr %x\n", short_addr);
        }
        node.addr = short_addr;
        node_idx[i].addr = node.addr;
        node.first_seen_utc = utctime.tv_sec;
        node.index = i;

        panmaster_save_node(&node);
        PM_DEBUG("panm: node added\n");
        return;
    }

    return;
}

void
panmaster_delete_node(uint64_t euid)
{
    struct panmaster_node node;
    struct find_node_s fns = { .results = &node };

    PANMASTER_NODE_DEFAULT(fns.find);
    fns.find.euid = euid;
    panmaster_find_node_general(&fns);

    if (!fns.is_found) {
        return;
    }

    if (node.index >= MYNEWT_VAL(PANMASTER_MAXNUM_NODES)) {
        return;
    }

    if (node_idx[node.index].addr != node.addr)
    {
        return;
    }

    node.addr = 0xFFFF;
    node_idx[node.index].addr = 0xFFFF;
    node_idx[node.index].euid = 0xFFFFFFFFFFFFFFFFULL;
    node_idx[node.index].slot_id = 0xFFFF;
    node_idx[node.index].has_perm_slot = 0;

    panmaster_save_node(&node);
    PM_DEBUG("panmaster_delete_node: node deleted\n");

    return;
}

int
panmaster_save_node(struct panmaster_node *node)
{
    /* Make sure index is up to date */
    node_idx[node->index].role = node->role;
    node_idx[node->index].has_perm_slot = node->has_perm_slot;

#if MYNEWT_VAL(PANMASTER_NFFS)
    return panm_file_save(&panmaster_storage_file, node);
#elif MYNEWT_VAL(PANMASTER_FCB)
    return panm_fcb_save(&pm_init_conf_fcb, node);
#endif
}

uint16_t
panmaster_highest_node_addr()
{
    int i;
    uint16_t addr = 0xffff;

    /* Find the index with highest valid address */
    for (i=0;i<MYNEWT_VAL(PANMASTER_MAXNUM_NODES);i++)
    {
        if (node_idx[i].addr == 0xffff) {
            continue;
        }
        if (node_idx[i].addr > addr || addr == 0xffff) {
            addr = node_idx[i].addr;
        }
    }
    return addr;
}

void
panmaster_compress()
{
#if MYNEWT_VAL(PANMASTER_NFFS)
    panm_file_compress(&panmaster_storage_file, node_idx);
#elif MYNEWT_VAL(PANMASTER_FCB)
    panm_fcb_compress(&pm_init_conf_fcb);
#endif
}

void
panmaster_sort()
{
#if MYNEWT_VAL(PANMASTER_NFFS)
    // Do nothing
#elif MYNEWT_VAL(PANMASTER_FCB)
    panm_fcb_sort(&pm_init_conf_fcb);
    panm_fcb_load_idx(&pm_init_conf_fcb, node_idx);
#endif
}

void
panmaster_pkg_init(void)
{
    int i;
    int rc;

    /* Init log and Config */
    log_register("panmstr", &_log, &log_console_handler,
                 NULL, LOG_SYSLEVEL);

    rc = conf_register(&pm_conf_cbs);
    SYSINIT_PANIC_ASSERT(rc == 0);

    /* Ensure this function only gets called by sysinit. */
    SYSINIT_ASSERT_ACTIVE();

    for (i=0;i<MYNEWT_VAL(PANMASTER_MAXNUM_NODES);i++) {
        PANMASTER_NODE_IDX_DEFAULT(node_idx[i]);
    }
    rc = dpl_mutex_init(&save_mutex);
    assert(rc == DPL_OK);
    memset(&nodes_to_save, 0, sizeof(nodes_to_save));

#if MYNEWT_VAL(UWB_PAN_ENABLED)

#if MYNEWT_VAL(UWB_DEVICE_0)
    struct uwb_pan_instance * pan = (struct uwb_pan_instance*)uwb_mac_find_cb_inst_ptr(uwb_dev_idx_lookup(0), UWBEXT_PAN);
    assert(pan);
    uwb_pan_set_postprocess(pan, postprocess_cb);
    uwb_pan_set_request_cb(pan, panrequest_cb);
#endif

#endif

#if MYNEWT_VAL(PANMASTER_CLI)
    rc = panmaster_cli_register();
    SYSINIT_PANIC_ASSERT(rc == 0);
#endif

#if MYNEWT_VAL(PANMASTER_NFFS)
    fs_mkdir(MYNEWT_VAL(PANMASTER_NFFS_DIR));
    panm_nffs_load(&panmaster_storage_file, node_idx);

#if MYNEWT_VAL(PANMASTER_SORT_AT_INIT)
    panm_file_compress(&panmaster_storage_file, node_idx);
#endif

#elif MYNEWT_VAL(PANMASTER_FCB)
    panm_init_fcb();

#if MYNEWT_VAL(PANMASTER_SORT_AT_INIT)
    panmaster_sort();
#endif
    panm_fcb_load_idx(&pm_init_conf_fcb, node_idx);
#endif
}
