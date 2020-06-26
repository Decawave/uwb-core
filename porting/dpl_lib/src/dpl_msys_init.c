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

#include <assert.h>
#include "dpl/dpl.h"
#include "dpl/dpl_os.h"
#include "dpl/dpl_mem.h"
#include "dpl/dpl_mbuf.h"
#include "dpl/dpl_mempool.h"

#if MYNEWT_VAL(MSYS_1_BLOCK_COUNT) > 0
#define SYSINIT_MSYS_1_MEMBLOCK_SIZE                \
    DPL_ALIGN(MYNEWT_VAL(MSYS_1_BLOCK_SIZE), DPL_ALIGNMENT)
#define SYSINIT_MSYS_1_MEMPOOL_SIZE                 \
    DPL_MEMPOOL_SIZE(MYNEWT_VAL(MSYS_1_BLOCK_COUNT),  \
                    SYSINIT_MSYS_1_MEMBLOCK_SIZE)
static dpl_membuf_t dpl_msys_init_1_data[SYSINIT_MSYS_1_MEMPOOL_SIZE];
static struct dpl_mbuf_pool dpl_msys_init_1_mbuf_pool;
static struct dpl_mempool dpl_msys_init_1_mempool;
#endif

#if MYNEWT_VAL(MSYS_2_BLOCK_COUNT) > 0
#define SYSINIT_MSYS_2_MEMBLOCK_SIZE                \
    DPL_ALIGN(MYNEWT_VAL(MSYS_2_BLOCK_SIZE), OS_ALIGNMENT)
#define SYSINIT_MSYS_2_MEMPOOL_SIZE                 \
    DPL_MEMPOOL_SIZE(MYNEWT_VAL(MSYS_2_BLOCK_COUNT),  \
                    SYSINIT_MSYS_2_MEMBLOCK_SIZE)
static dpl_membuf_t dpl_msys_init_2_data[SYSINIT_MSYS_2_MEMPOOL_SIZE];
static struct dpl_mbuf_pool dpl_msys_init_2_mbuf_pool;
static struct dpl_mempool dpl_msys_init_2_mempool;
#endif

static void
dpl_msys_init_once(void *data, struct dpl_mempool *mempool,
                  struct dpl_mbuf_pool *mbuf_pool,
                  int block_count, int block_size, char *name)
{
    int rc;

    rc = mem_init_mbuf_pool(data, mempool, mbuf_pool, block_count, block_size,
                            name);
    assert(rc == 0);

    rc = dpl_msys_register(mbuf_pool);
    assert(rc == 0);
}

void
dpl_msys_init(void)
{
    dpl_msys_reset();

    (void)dpl_msys_init_once;
#if MYNEWT_VAL(MSYS_1_BLOCK_COUNT) > 0
    dpl_msys_init_once(dpl_msys_init_1_data,
                      &dpl_msys_init_1_mempool,
                      &dpl_msys_init_1_mbuf_pool,
                      MYNEWT_VAL(MSYS_1_BLOCK_COUNT),
                      SYSINIT_MSYS_1_MEMBLOCK_SIZE,
                      "msys_1");
#endif

#if MYNEWT_VAL(MSYS_2_BLOCK_COUNT) > 0
    dpl_msys_init_once(dpl_msys_init_2_data,
                      &dpl_msys_init_2_mempool,
                      &dpl_msys_init_2_mbuf_pool,
                      MYNEWT_VAL(MSYS_2_BLOCK_COUNT),
                      SYSINIT_MSYS_2_MEMBLOCK_SIZE,
                      "msys_2");
#endif
}
