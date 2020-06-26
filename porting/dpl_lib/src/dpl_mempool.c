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

#include "dpl/dpl.h"
#include "dpl/dpl_os.h"
#include "dpl/dpl_mempool.h"
#include "dpl/dpl_error.h"
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#define DPL_MEM_TRUE_BLOCK_SIZE(bsize) DPL_ALIGN(bsize, DPL_ALIGNMENT)
#define DPL_MEMPOOL_TRUE_BLOCK_SIZE(mp) DPL_MEM_TRUE_BLOCK_SIZE(mp->mp_block_size)

STAILQ_HEAD(, dpl_mempool) g_dpl_mempool_list =
    STAILQ_HEAD_INITIALIZER(g_dpl_mempool_list);

#if MYNEWT_VAL(DPL_MEMPOOL_POISON)
static uint32_t dpl_mem_poison = 0xde7ec7ed;

static void
dpl_mempool_poison(void *start, int sz)
{
    int i;
    char *p = start;

    for (i = sizeof(struct dpl_memblock); i < sz;
         i = i + sizeof(dpl_mem_poison)) {
        memcpy(p + i, &dpl_mem_poison, min(sizeof(dpl_mem_poison), sz - i));
    }
}

static void
dpl_mempool_poison_check(void *start, int sz)
{
    int i;
    char *p = start;

    for (i = sizeof(struct dpl_memblock); i < sz;
         i = i + sizeof(dpl_mem_poison)) {
        assert(!memcmp(p + i, &dpl_mem_poison,
               min(sizeof(dpl_mem_poison), sz - i)));
    }
}
#else
#define dpl_mempool_poison(start, sz)
#define dpl_mempool_poison_check(start, sz)
#endif

dpl_error_t
dpl_mempool_init(struct dpl_mempool *mp, uint16_t blocks, uint32_t block_size,
                void *membuf, char *name)
{
    int true_block_size;
    uint8_t *block_addr;
    struct dpl_memblock *block_ptr;

    /* Check for valid parameters */
    if (!mp || (block_size == 0)) {
        return DPL_INVALID_PARAM;
    }

    if ((!membuf) && (blocks != 0)) {
        return DPL_INVALID_PARAM;
    }

    if (membuf != NULL) {
        /* Blocks need to be sized properly and memory buffer should be
         * aligned
         */
        if (((uintptr_t)membuf & (DPL_ALIGNMENT - 1)) != 0) {
            return DPL_MEM_NOT_ALIGNED;
        }
    }
    true_block_size = DPL_MEM_TRUE_BLOCK_SIZE(block_size);

    /* Initialize the memory pool structure */
    mp->mp_block_size = block_size;
    mp->mp_num_free = blocks;
    mp->mp_min_free = blocks;
    mp->mp_flags = 0;
    mp->mp_num_blocks = blocks;
    mp->mp_membuf_addr = (uintptr_t)membuf;
    mp->name = name;
    dpl_mempool_poison(membuf, true_block_size);
    SLIST_FIRST(mp) = membuf;
    dpl_mutex_init(&mp->mutex);

    /* Chain the memory blocks to the free list */
    block_addr = (uint8_t *)membuf;
    block_ptr = (struct dpl_memblock *)block_addr;
    while (blocks > 1) {
        block_addr += true_block_size;
        dpl_mempool_poison(block_addr, true_block_size);
        SLIST_NEXT(block_ptr, mb_next) = (struct dpl_memblock *)block_addr;
        block_ptr = (struct dpl_memblock *)block_addr;
        --blocks;
    }

    /* Last one in the list should be NULL */
    SLIST_NEXT(block_ptr, mb_next) = NULL;

    STAILQ_INSERT_TAIL(&g_dpl_mempool_list, mp, mp_list);

    return DPL_OK;
}

dpl_error_t
dpl_mempool_ext_init(struct dpl_mempool_ext *mpe, uint16_t blocks,
                    uint32_t block_size, void *membuf, char *name)
{
    int rc;

    rc = dpl_mempool_init(&mpe->mpe_mp, blocks, block_size, membuf, name);
    if (rc != 0) {
        return rc;
    }

    mpe->mpe_mp.mp_flags = DPL_MEMPOOL_F_EXT;
    mpe->mpe_put_cb = NULL;
    mpe->mpe_put_arg = NULL;

    return 0;
}

dpl_error_t
dpl_mempool_clear(struct dpl_mempool *mp)
{
    struct dpl_memblock *block_ptr;
    int true_block_size;
    uint8_t *block_addr;
    uint16_t blocks;

    if (!mp) {
        return DPL_INVALID_PARAM;
    }

    true_block_size = DPL_MEM_TRUE_BLOCK_SIZE(mp->mp_block_size);

    /* cleanup the memory pool structure */
    mp->mp_num_free = mp->mp_num_blocks;
    mp->mp_min_free = mp->mp_num_blocks;
    dpl_mempool_poison((void *)mp->mp_membuf_addr, true_block_size);
    SLIST_FIRST(mp) = (void *)mp->mp_membuf_addr;

    /* Chain the memory blocks to the free list */
    block_addr = (uint8_t *)mp->mp_membuf_addr;
    block_ptr = (struct dpl_memblock *)block_addr;
    blocks = mp->mp_num_blocks;

    while (blocks > 1) {
        block_addr += true_block_size;
        dpl_mempool_poison(block_addr, true_block_size);
        SLIST_NEXT(block_ptr, mb_next) = (struct dpl_memblock *)block_addr;
        block_ptr = (struct dpl_memblock *)block_addr;
        --blocks;
    }

    /* Last one in the list should be NULL */
    SLIST_NEXT(block_ptr, mb_next) = NULL;

    return DPL_OK;
}

bool
dpl_mempool_is_sane(const struct dpl_mempool *mp)
{
    struct dpl_memblock *block;

    /* Verify that each block in the free list belongs to the mempool. */
    SLIST_FOREACH(block, mp, mb_next) {
        if (!dpl_memblock_from(mp, block)) {
            return false;
        }
        dpl_mempool_poison_check(block, DPL_MEMPOOL_TRUE_BLOCK_SIZE(mp));
    }

    return true;
}

int
dpl_memblock_from(const struct dpl_mempool *mp, const void *block_addr)
{
    uintptr_t true_block_size;
    uintptr_t baddr_ptr;
    uintptr_t end;

    _Static_assert(sizeof block_addr == sizeof baddr_ptr,
                   "Pointer to void must be native word size.");

    baddr_ptr = (uintptr_t)block_addr;
    true_block_size = DPL_MEMPOOL_TRUE_BLOCK_SIZE(mp);
    end = mp->mp_membuf_addr + (mp->mp_num_blocks * true_block_size);

    /* Check that the block is in the memory buffer range. */
    if ((baddr_ptr < mp->mp_membuf_addr) || (baddr_ptr >= end)) {
        return 0;
    }

    /* All freed blocks should be on true block size boundaries! */
    if (((baddr_ptr - mp->mp_membuf_addr) % true_block_size) != 0) {
        return 0;
    }

    return 1;
}

void *
dpl_memblock_get(struct dpl_mempool *mp)
{
    dpl_sr_t sr;
    struct dpl_memblock *block;

    /* Check to make sure they passed in a memory pool (or something) */
    block = NULL;
    if (mp) {
        DPL_ENTER_CRITICAL(sr);
        dpl_mutex_pend(&mp->mutex, DPL_WAIT_FOREVER);
        /* Check for any free */
        if (mp->mp_num_free) {
            /* Get a free block */
            block = SLIST_FIRST(mp);

            /* Set new free list head */
            SLIST_FIRST(mp) = SLIST_NEXT(block, mb_next);

            /* Decrement number free by 1 */
            mp->mp_num_free--;
            if (mp->mp_min_free > mp->mp_num_free) {
                mp->mp_min_free = mp->mp_num_free;
            }
        }
        dpl_mutex_release(&mp->mutex);
        DPL_EXIT_CRITICAL(sr);

        if (block) {
            dpl_mempool_poison_check(block, DPL_MEMPOOL_TRUE_BLOCK_SIZE(mp));
        }
    }

    return (void *)block;
}

dpl_error_t
dpl_memblock_put_from_cb(struct dpl_mempool *mp, void *block_addr)
{
    dpl_sr_t sr;
    struct dpl_memblock *block;

    dpl_mempool_poison(block_addr, DPL_MEMPOOL_TRUE_BLOCK_SIZE(mp));

    block = (struct dpl_memblock *)block_addr;
    DPL_ENTER_CRITICAL(sr);
    dpl_mutex_pend(&mp->mutex, DPL_WAIT_FOREVER);

    /* Chain current free list pointer to this block; make this block head */
    SLIST_NEXT(block, mb_next) = SLIST_FIRST(mp);
    SLIST_FIRST(mp) = block;

    /* XXX: Should we check that the number free <= number blocks? */
    /* Increment number free */
    mp->mp_num_free++;

    dpl_mutex_release(&mp->mutex);
    DPL_EXIT_CRITICAL(sr);

    return DPL_OK;
}

dpl_error_t
dpl_memblock_put(struct dpl_mempool *mp, void *block_addr)
{
    struct dpl_mempool_ext *mpe;
    int rc;
#if MYNEWT_VAL(DPL_MEMPOOL_CHECK)
    struct dpl_memblock *block;
#endif

    /* Make sure parameters are valid */
    if ((mp == NULL) || (block_addr == NULL)) {
        return DPL_INVALID_PARAM;
    }

#if MYNEWT_VAL(DPL_MEMPOOL_CHECK)
    /* Check that the block we are freeing is a valid block! */
    assert(dpl_memblock_from(mp, block_addr));

    /*
     * Check for duplicate free.
     */
    SLIST_FOREACH(block, mp, mb_next) {
        assert(block != (struct spl_memblock *)block_addr);
    }
#endif

    /* If this is an extended mempool with a put callback, call the callback
     * instead of freeing the block directly.
     */
    if (mp->mp_flags & DPL_MEMPOOL_F_EXT) {
        mpe = (struct dpl_mempool_ext *)mp;
        if (mpe->mpe_put_cb != NULL) {
            rc = mpe->mpe_put_cb(mpe, block_addr, mpe->mpe_put_arg);
            return rc;
        }
    }

    /* No callback; free the block. */
    return dpl_memblock_put_from_cb(mp, block_addr);
}

struct dpl_mempool *
dpl_mempool_info_get_next(struct dpl_mempool *mp, struct dpl_mempool_info *omi)
{
    struct dpl_mempool *cur;

    if (mp == NULL) {
        cur = STAILQ_FIRST(&g_dpl_mempool_list);
    } else {
        cur = STAILQ_NEXT(mp, mp_list);
    }

    if (cur == NULL) {
        return (NULL);
    }

    omi->omi_block_size = cur->mp_block_size;
    omi->omi_num_blocks = cur->mp_num_blocks;
    omi->omi_num_free = cur->mp_num_free;
    omi->omi_min_free = cur->mp_min_free;
    strncpy(omi->omi_name, cur->name, sizeof(omi->omi_name));

    return (cur);
}
