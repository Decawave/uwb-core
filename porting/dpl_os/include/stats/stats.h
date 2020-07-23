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

#ifndef __STATS_H__
#define __STATS_H__

#include <stddef.h>
#include <syscfg/syscfg.h>
#include <stdint.h>
#include <os/queue.h>


#ifdef __cplusplus
extern "C" {
#endif

#if MYNEWT_VAL(STATS_ENABLED)
/** The stat group is periodically written to sys/config. */
#define STATS_HDR_F_PERSIST             0x01

struct stats_name_map {
    uint16_t snm_off;
    char *snm_name;
} __attribute__((packed));

struct stats_hdr {
    const char *s_name;
    uint8_t s_size;
    uint8_t s_cnt;
    uint16_t s_flags;
#if MYNEWT_VAL(STATS_NAMES)
    const struct stats_name_map *s_map;
    int s_map_cnt;
#endif
    STAILQ_ENTRY(stats_hdr) s_next;
};

#define STATS_SECT_DECL(__name)             \
    struct stats_ ## __name

#define STATS_SECT_START(__name)            \
STATS_SECT_DECL(__name) {                   \
    struct stats_hdr s_hdr;

#define STATS_SECT_END };

#define STATS_SECT_VAR(__var) \
    s##__var

#define STATS_HDR(__sectname) &(__sectname).s_hdr

#define STATS_SIZE_16 (sizeof(uint16_t))
#define STATS_SIZE_32 (sizeof(uint32_t))
#define STATS_SIZE_64 (sizeof(uint64_t))

#define STATS_SECT_ENTRY(__var) uint32_t STATS_SECT_VAR(__var);
#define STATS_SECT_ENTRY16(__var) uint16_t STATS_SECT_VAR(__var);
#define STATS_SECT_ENTRY32(__var) uint32_t STATS_SECT_VAR(__var);
#define STATS_SECT_ENTRY64(__var) uint64_t STATS_SECT_VAR(__var);

/**
 * @brief Resets all stats in the provided group to 0.
 *
 * NOTE: This must only be used with non-persistent stat groups.
 */
#define STATS_RESET(__sectvarname)                                      \
    memset((uint8_t *)&__sectvarname + sizeof(struct stats_hdr), 0,     \
           sizeof(__sectvarname) - sizeof(struct stats_hdr))

#define STATS_SIZE_INIT_PARMS(__sectvarname, __size)                        \
    (__size),                                                               \
    ((sizeof (__sectvarname)) - sizeof (__sectvarname.s_hdr)) / (__size)

#define STATS_GET(__sectvarname, __var)             \
    ((__sectvarname).STATS_SECT_VAR(__var))

#define STATS_SET_RAW(__sectvarname, __var, __val)  \
    (STATS_GET(__sectvarname, __var) = (__val))

#define STATS_SET(__sectvarname, __var, __val) do               \
{                                                               \
    STATS_SET_RAW(__sectvarname, __var, __val);                 \
} while (0)

/**
 * @brief Adjusts a stat's in-RAM value by the specified delta.
 *
 * For non-persistent stats, this is more efficient than `STATS_INCN()`.  This
 * must only be used with non-persistent stats; for persistent stats the
 * behavior is undefined.
 *
 * @param __sectvarname         The name of the stat group containing the stat
 *                                  to modify.
 * @param __var                 The name of the individual stat to modify.
 * @param __n                   The amount to add to the specified stat.
 */
#define STATS_INCN_RAW(__sectvarname, __var, __n)   \
    (STATS_SET_RAW(__sectvarname, __var,            \
                   STATS_GET(__sectvarname, __var) + (__n))

/**
 * @brief Increments a stat's in-RAM value.
 *
 * For non-persistent stats, this is more efficient than `STATS_INCN()`.  This
 * must only be used with non-persistent stats; for persistent stats the
 * behavior is undefined.
 *
 * @param __sectvarname         The name of the stat group containing the stat
 *                                  to modify.
 * @param __var                 The name of the individual stat to modify.
 */
#define STATS_INC_RAW(__sectvarname, __var)       \
    STATS_INCN_RAW(__sectvarname, __var, 1)

/**
 * @brief Adjusts a stat's value by the specified delta.
 *
 * If the specified stat group is persistent, this also schedules the group to
 * be flushed to disk.
 *
 * @param __sectvarname         The name of the stat group containing the stat
 *                                  to modify.
 * @param __var                 The name of the individual stat to modify.
 * @param __n                   The amount to add to the specified stat.
 */
#define STATS_INCN(__sectvarname, __var, __n)       \
    STATS_SET(__sectvarname, __var, STATS_GET(__sectvarname, __var) + (__n))

/**
 * @brief Increments a stat's value.
 *
 * If the specified stat group is persistent, this also schedules the group to
 * be flushed to disk.
 *
 * @param __sectvarname         The name of the stat group containing the stat
 *                                  to modify.
 * @param __var                 The name of the individual stat to modify.
 */
#define STATS_INC(__sectvarname, __var)             \
    STATS_INCN(__sectvarname, __var, 1)

#define STATS_CLEAR(__sectvarname, __var)           \
    STATS_SET(__sectvarname, __var, 0)

#if MYNEWT_VAL(STATS_NAMES)

#define STATS_NAME_MAP_NAME(__sectname) g_stats_map_ ## __sectname

#define STATS_NAME_START(__sectname)                                        \
const struct stats_name_map STATS_NAME_MAP_NAME(__sectname)[] = {

#define STATS_NAME(__sectname, __entry)                                     \
    { offsetof(STATS_SECT_DECL(__sectname), STATS_SECT_VAR(__entry)),       \
      #__entry },

#define STATS_NAME_END(__sectname)                                          \
};

#define STATS_NAME_INIT_PARMS(__name)                                       \
    &(STATS_NAME_MAP_NAME(__name)[0]),                                      \
    (sizeof(STATS_NAME_MAP_NAME(__name)) / sizeof(struct stats_name_map))

#else /* MYNEWT_VAL(STATS_NAME) */

#define STATS_NAME_START(__name)
#define STATS_NAME(__name, __entry)
#define STATS_NAME_END(__name)
#define STATS_NAME_INIT_PARMS(__name) NULL, 0

#endif /* MYNEWT_VAL(STATS_NAME) */

int stats_init(struct stats_hdr *shdr, uint8_t size, uint8_t cnt,
    const struct stats_name_map *map, uint8_t map_cnt);
int stats_register(const char *name, struct stats_hdr *shdr);
int stats_init_and_reg(struct stats_hdr *shdr, uint8_t size, uint8_t cnt,
                       const struct stats_name_map *map, uint8_t map_cnt,
                       const char *name);
void stats_reset(struct stats_hdr *shdr);

typedef int (*stats_walk_func_t)(struct stats_hdr *, void *, char *,
        uint16_t);
int stats_walk(struct stats_hdr *, stats_walk_func_t, void *);

typedef int (*stats_group_walk_func_t)(struct stats_hdr *, void *);
int stats_group_walk(stats_group_walk_func_t, void *);

struct stats_hdr *stats_group_find(const char *name);

/* Private */
#if MYNEWT_VAL(STATS_SMP)
int stats_smp_register_group(void);
#endif
#if MYNEWT_VAL(STATS_CLI)
int stats_shell_register(void);
#endif

#else  // MYNEWT_VAL(STATS_ENABLED)

#define STATS_SECT_DECL(__name)         struct stats_ ## __name
#define STATS_SECT_END                  };

#define STATS_SECT_START(__name)        STATS_SECT_DECL(__name) {
#define STATS_SECT_VAR(__var)

#define STATS_HDR(__sectname)           NULL

#define STATS_SECT_ENTRY(__var)
#define STATS_SECT_ENTRY16(__var)
#define STATS_SECT_ENTRY32(__var)
#define STATS_SECT_ENTRY64(__var)
#define STATS_RESET(__var)

#define STATS_SIZE_INIT_PARMS(__sectvarname, __size) \
                                        0, 0

#define STATS_INC(__sectvarname, __var)
#define STATS_INCN(__sectvarname, __var, __n)
#define STATS_CLEAR(__sectvarname, __var)

#define STATS_NAME_START(__name)
#define STATS_NAME(__name, __entry)
#define STATS_NAME_END(__name)
#define STATS_NAME_INIT_PARMS(__name)   NULL, 0

static inline int
stats_init(void *a, uint8_t b, uint8_t c, void *d, uint8_t e)
{
    /* dummy */
    return 0;
}

static inline int
stats_register(void *a, void *b)
{
    /* dummy */
    return 0;
}

static inline int
stats_init_and_reg(void *a, uint8_t b, uint8_t c, void *d, uint8_t e, void *f)
{
    /* dummy */
    return 0;
}

#endif  // MYNEWT_VAL(STATS_ENABLED)

#ifdef __cplusplus
}
#endif

#endif /* __STATS_H__ */
