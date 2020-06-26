/*
 * Software in this file is based heavily on code written in the FreeBSD source
 * code repostiory.  While the code is written from scratch, it contains
 * many of the ideas and logic flow in the original source, this is a
 * derivative work, and the following license applies as well:
 *
 * Copyright (c) 1982, 1986, 1988, 1991, 1993
 *  The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include "dpl/dpl_os.h"
#include "dpl/dpl_mbuf.h"
#include "dpl/dpl_mempool.h"
#include "dpl/queue.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>

#ifndef __KERNEL__
#include <limits.h>
#ifndef EXPORT_SYMBOL
#define EXPORT_SYMBOL(__X)
#endif
#endif

#ifndef mynewt_min
#define mynewt_min(a, b) ((a)<(b)?(a):(b))
#endif

#ifndef mynewt_max
#define mynewt_max(a, b) ((a)>(b)?(a):(b))
#endif

/**
 * @addtogroup OSKernel
 * @{
 *   @defgroup OSMqueue Queue of Mbufs
 *   @{
 */

STAILQ_HEAD(, dpl_mbuf_pool) g_msys_pool_list =
    STAILQ_HEAD_INITIALIZER(g_msys_pool_list);


int
dpl_mqueue_init(struct dpl_mqueue *mq, dpl_event_fn *ev_cb, void *arg)
{
    struct dpl_event *ev;

    STAILQ_INIT(&mq->mq_head);

    ev = &mq->mq_ev;
    dpl_event_init(ev, ev_cb, arg);
    dpl_mutex_init(&mq->mutex);

    return (0);
}

struct dpl_mbuf *
dpl_mqueue_get(struct dpl_mqueue *mq)
{
    struct dpl_mbuf_pkthdr *mp;
    struct dpl_mbuf *m;
    dpl_sr_t sr;

    DPL_ENTER_CRITICAL(sr);
    dpl_mutex_pend(&mq->mutex, DPL_WAIT_FOREVER);
    mp = STAILQ_FIRST(&mq->mq_head);
    if (mp) {
        STAILQ_REMOVE_HEAD(&mq->mq_head, omp_next);
    }
    dpl_mutex_release(&mq->mutex);
    DPL_EXIT_CRITICAL(sr);

    if (mp) {
        m = DPL_MBUF_PKTHDR_TO_MBUF(mp);
    } else {
        m = NULL;
    }

    return (m);
}

int
dpl_mqueue_put(struct dpl_mqueue *mq, struct dpl_eventq *evq, struct dpl_mbuf *m)
{
    struct dpl_mbuf_pkthdr *mp;
    dpl_sr_t sr;
    int rc;

    /* Can only place the head of a chained mbuf on the queue. */
    if (!DPL_MBUF_IS_PKTHDR(m)) {
        rc = DPL_EINVAL;
        goto err;
    }

    mp = DPL_MBUF_PKTHDR(m);

    DPL_ENTER_CRITICAL(sr);
    dpl_mutex_pend(&mq->mutex, DPL_WAIT_FOREVER);
    STAILQ_INSERT_TAIL(&mq->mq_head, mp, omp_next);
    dpl_mutex_release(&mq->mutex);
    DPL_EXIT_CRITICAL(sr);

    /* Only post an event to the queue if its specified */
    if (evq) {
        dpl_eventq_put(evq, &mq->mq_ev);
    }

    return (0);
err:
    return (rc);
}

int
dpl_msys_register(struct dpl_mbuf_pool *new_pool)
{
    struct dpl_mbuf_pool *pool;

    pool = NULL;
    STAILQ_FOREACH(pool, &g_msys_pool_list, omp_next) {
        if (new_pool->omp_databuf_len > pool->omp_databuf_len) {
            break;
        }
    }

    if (pool) {
        STAILQ_INSERT_AFTER(&g_msys_pool_list, pool, new_pool, omp_next);
    } else {
        STAILQ_INSERT_TAIL(&g_msys_pool_list, new_pool, omp_next);
    }

    return (0);
}

void
dpl_msys_reset(void)
{
    STAILQ_INIT(&g_msys_pool_list);
}

static struct dpl_mbuf_pool *
_dpl_msys_find_pool(uint16_t dsize)
{
    struct dpl_mbuf_pool *pool;

    pool = NULL;
    STAILQ_FOREACH(pool, &g_msys_pool_list, omp_next) {
        if (dsize <= pool->omp_databuf_len) {
            break;
        }
    }

    if (!pool) {
        pool = STAILQ_LAST(&g_msys_pool_list, dpl_mbuf_pool, omp_next);
    }

    return (pool);
}


struct dpl_mbuf *
dpl_msys_get(uint16_t dsize, uint16_t leadingspace)
{
    struct dpl_mbuf *m;
    struct dpl_mbuf_pool *pool;

    pool = _dpl_msys_find_pool(dsize);
    if (!pool) {
        goto err;
    }

    m = dpl_mbuf_get(pool, leadingspace);
    return (m);
err:
    return (NULL);
}

struct dpl_mbuf *
dpl_msys_get_pkthdr(uint16_t dsize, uint16_t user_hdr_len)
{
    uint16_t total_pkthdr_len;
    struct dpl_mbuf *m;
    struct dpl_mbuf_pool *pool;

    total_pkthdr_len =  user_hdr_len + sizeof(struct dpl_mbuf_pkthdr);
    pool = _dpl_msys_find_pool(dsize + total_pkthdr_len);
    if (!pool) {
        goto err;
    }

    m = dpl_mbuf_get_pkthdr(pool, user_hdr_len);
    return (m);
err:
    return (NULL);
}

int
dpl_msys_count(void)
{
    struct dpl_mbuf_pool *omp;
    int total;

    total = 0;
    STAILQ_FOREACH(omp, &g_msys_pool_list, omp_next) {
        total += omp->omp_pool->mp_num_blocks;
    }

    return total;
}

int
dpl_msys_num_free(void)
{
    struct dpl_mbuf_pool *omp;
    int total;

    total = 0;
    STAILQ_FOREACH(omp, &g_msys_pool_list, omp_next) {
        total += omp->omp_pool->mp_num_free;
    }

    return total;
}


int
dpl_mbuf_pool_init(struct dpl_mbuf_pool *omp, struct dpl_mempool *mp,
                  uint16_t buf_len, uint16_t nbufs)
{
    omp->omp_databuf_len = buf_len - sizeof(struct dpl_mbuf);
    omp->omp_pool = mp;

    return (0);
}

struct dpl_mbuf *
dpl_mbuf_get(struct dpl_mbuf_pool *omp, uint16_t leadingspace)
{
    struct dpl_mbuf *om;

    if (leadingspace > omp->omp_databuf_len) {
        goto err;
    }

    om = dpl_memblock_get(omp->omp_pool);
    if (!om) {
        goto err;
    }

    SLIST_NEXT(om, om_next) = NULL;
    om->om_flags = 0;
    om->om_pkthdr_len = 0;
    om->om_len = 0;
    om->om_data = (&om->om_databuf[0] + leadingspace);
    om->om_omp = omp;

    return (om);
err:
    return (NULL);
}

struct dpl_mbuf *
dpl_mbuf_get_pkthdr(struct dpl_mbuf_pool *omp, uint8_t user_pkthdr_len)
{
    uint16_t pkthdr_len;
    struct dpl_mbuf_pkthdr *pkthdr;
    struct dpl_mbuf *om;

    /* User packet header must fit inside mbuf */
    pkthdr_len = user_pkthdr_len + sizeof(struct dpl_mbuf_pkthdr);
    if ((pkthdr_len > omp->omp_databuf_len) || (pkthdr_len > 255)) {
        return NULL;
    }

    om = dpl_mbuf_get(omp, 0);
    if (om) {
        om->om_pkthdr_len = pkthdr_len;
        om->om_data += pkthdr_len;

        pkthdr = DPL_MBUF_PKTHDR(om);
        pkthdr->omp_len = 0;
        pkthdr->omp_flags = 0;
        STAILQ_NEXT(pkthdr, omp_next) = NULL;
    }

    return om;
}

int
dpl_mbuf_free(struct dpl_mbuf *om)
{
    int rc;

    if (om->om_omp != NULL) {
        rc = dpl_memblock_put(om->om_omp->omp_pool, om);
        if (rc != 0) {
            goto err;
        }
    }

    return (0);
err:
    return (rc);
}

int
dpl_mbuf_free_chain(struct dpl_mbuf *om)
{
    struct dpl_mbuf *next;
    int rc;

    while (om != NULL) {
        next = SLIST_NEXT(om, om_next);

        rc = dpl_mbuf_free(om);
        if (rc != 0) {
            goto err;
        }

        om = next;
    }

    return (0);
err:
    return (rc);
}
EXPORT_SYMBOL(dpl_mbuf_free_chain);

/**
 * Copy a packet header from one mbuf to another.
 *
 * @param omp The mbuf pool associated with these buffers
 * @param new_buf The new buffer to copy the packet header into
 * @param old_buf The old buffer to copy the packet header from
 */
static inline void
_dpl_mbuf_copypkthdr(struct dpl_mbuf *new_buf, struct dpl_mbuf *old_buf)
{
    assert(new_buf->om_len == 0);

    memcpy(&new_buf->om_databuf[0], &old_buf->om_databuf[0],
           old_buf->om_pkthdr_len);
    new_buf->om_pkthdr_len = old_buf->om_pkthdr_len;
    new_buf->om_data = new_buf->om_databuf + old_buf->om_pkthdr_len;
}

int
dpl_mbuf_append(struct dpl_mbuf *om, const void *data,  uint16_t len)
{
    struct dpl_mbuf_pool *omp;
    struct dpl_mbuf *last;
    struct dpl_mbuf *new;
    int remainder;
    int space;
    int rc;

    if (om == NULL) {
        rc = DPL_EINVAL;
        goto err;
    }

    omp = om->om_omp;

    /* Scroll to last mbuf in the chain */
    last = om;
    while (SLIST_NEXT(last, om_next) != NULL) {
        last = SLIST_NEXT(last, om_next);
    }

    remainder = len;
    space = DPL_MBUF_TRAILINGSPACE(last);

    /* If room in current mbuf, copy the first part of the data into the
     * remaining space in that mbuf.
     */
    if (space > 0) {
        if (space > remainder) {
            space = remainder;
        }

        memcpy(DPL_MBUF_DATA(last, uint8_t *) + last->om_len , data, space);

        last->om_len += space;
        data += space;
        remainder -= space;
    }

    /* Take the remaining data, and keep allocating new mbufs and copying
     * data into it, until data is exhausted.
     */
    while (remainder > 0) {
        new = dpl_mbuf_get(omp, 0);
        if (!new) {
            break;
        }

        new->om_len = mynewt_min(omp->omp_databuf_len, remainder);
        memcpy(DPL_MBUF_DATA(new, void *), data, new->om_len);
        data += new->om_len;
        remainder -= new->om_len;
        SLIST_NEXT(last, om_next) = new;
        last = new;
    }

    /* Adjust the packet header length in the buffer */
    if (DPL_MBUF_IS_PKTHDR(om)) {
        DPL_MBUF_PKTHDR(om)->omp_len += len - remainder;
    }

    if (remainder != 0) {
        rc = DPL_ENOMEM;
        goto err;
    }


    return (0);
err:
    return (rc);
}

int
dpl_mbuf_appendfrom(struct dpl_mbuf *dst, const struct dpl_mbuf *src,
                   uint16_t src_off, uint16_t len)
{
    const struct dpl_mbuf *src_cur_om;
    uint16_t src_cur_off;
    uint16_t chunk_sz;
    int rc;

    src_cur_om = dpl_mbuf_off(src, src_off, &src_cur_off);
    while (len > 0) {
        if (src_cur_om == NULL) {
            return DPL_EINVAL;
        }

        chunk_sz = mynewt_min(len, src_cur_om->om_len - src_cur_off);
        rc = dpl_mbuf_append(dst, src_cur_om->om_data + src_cur_off, chunk_sz);
        if (rc != 0) {
            return rc;
        }

        len -= chunk_sz;
        src_cur_om = SLIST_NEXT(src_cur_om, om_next);
        src_cur_off = 0;
    }

    return 0;
}

struct dpl_mbuf *
dpl_mbuf_dup(struct dpl_mbuf *om)
{
    struct dpl_mbuf_pool *omp;
    struct dpl_mbuf *head;
    struct dpl_mbuf *copy;

    omp = om->om_omp;

    head = NULL;
    copy = NULL;

    for (; om != NULL; om = SLIST_NEXT(om, om_next)) {
        if (head) {
            SLIST_NEXT(copy, om_next) = dpl_mbuf_get(omp,
                    DPL_MBUF_LEADINGSPACE(om));
            if (!SLIST_NEXT(copy, om_next)) {
                dpl_mbuf_free_chain(head);
                goto err;
            }

            copy = SLIST_NEXT(copy, om_next);
        } else {
            head = dpl_mbuf_get(omp, DPL_MBUF_LEADINGSPACE(om));
            if (!head) {
                goto err;
            }

            if (DPL_MBUF_IS_PKTHDR(om)) {
                _dpl_mbuf_copypkthdr(head, om);
            }
            copy = head;
        }
        copy->om_flags = om->om_flags;
        copy->om_len = om->om_len;
        memcpy(DPL_MBUF_DATA(copy, uint8_t *), DPL_MBUF_DATA(om, uint8_t *),
                om->om_len);
    }

    return (head);
err:
    return (NULL);
}

struct dpl_mbuf *
dpl_mbuf_off(const struct dpl_mbuf *om, int off, uint16_t *out_off)
{
    struct dpl_mbuf *next;
    struct dpl_mbuf *cur;

    /* Cast away const. */
    cur = (struct dpl_mbuf *)om;

    while (1) {
        if (cur == NULL) {
            return NULL;
        }

        next = SLIST_NEXT(cur, om_next);

        if (cur->om_len > off ||
            (cur->om_len == off && next == NULL)) {

            *out_off = off;
            return cur;
        }

        off -= cur->om_len;
        cur = next;
    }
}

int
dpl_mbuf_copydata(const struct dpl_mbuf *m, int off, int len, void *dst)
{
    unsigned int count;
    uint8_t *udst;

    if (!len) {
        return 0;
    }

    udst = dst;

    while (off > 0) {
        if (!m) {
            return (-1);
        }

        if (off < m->om_len)
            break;
        off -= m->om_len;
        m = SLIST_NEXT(m, om_next);
    }
    while (len > 0 && m != NULL) {
        count = mynewt_min(m->om_len - off, len);
        memcpy(udst, m->om_data + off, count);
        len -= count;
        udst += count;
        off = 0;
        m = SLIST_NEXT(m, om_next);
    }

    return (len > 0 ? -1 : 0);
}
EXPORT_SYMBOL(dpl_mbuf_copydata);

void
dpl_mbuf_adj(struct dpl_mbuf *mp, int req_len)
{
    int len = req_len;
    struct dpl_mbuf *m;
    int count;

    if ((m = mp) == NULL)
        return;
    if (len >= 0) {
        /*
         * Trim from head.
         */
        while (m != NULL && len > 0) {
            if (m->om_len <= len) {
                len -= m->om_len;
                m->om_len = 0;
                m = SLIST_NEXT(m, om_next);
            } else {
                m->om_len -= len;
                m->om_data += len;
                len = 0;
            }
        }
        if (DPL_MBUF_IS_PKTHDR(mp))
            DPL_MBUF_PKTHDR(mp)->omp_len -= (req_len - len);
    } else {
        /*
         * Trim from tail.  Scan the mbuf chain,
         * calculating its length and finding the last mbuf.
         * If the adjustment only affects this mbuf, then just
         * adjust and return.  Otherwise, rescan and truncate
         * after the remaining size.
         */
        len = -len;
        count = 0;
        for (;;) {
            count += m->om_len;
            if (SLIST_NEXT(m, om_next) == (struct dpl_mbuf *)0)
                break;
            m = SLIST_NEXT(m, om_next);
        }
        if (m->om_len >= len) {
            m->om_len -= len;
            if (DPL_MBUF_IS_PKTHDR(mp))
                DPL_MBUF_PKTHDR(mp)->omp_len -= len;
            return;
        }
        count -= len;
        if (count < 0)
            count = 0;
        /*
         * Correct length for chain is "count".
         * Find the mbuf with last data, adjust its length,
         * and toss data from remaining mbufs on chain.
         */
        m = mp;
        if (DPL_MBUF_IS_PKTHDR(m))
            DPL_MBUF_PKTHDR(m)->omp_len = count;
        for (; m; m = SLIST_NEXT(m, om_next)) {
            if (m->om_len >= count) {
                m->om_len = count;
                if (SLIST_NEXT(m, om_next) != NULL) {
                    dpl_mbuf_free_chain(SLIST_NEXT(m, om_next));
                    SLIST_NEXT(m, om_next) = NULL;
                }
                break;
            }
            count -= m->om_len;
        }
    }
}

int
dpl_mbuf_cmpf(const struct dpl_mbuf *om, int off, const void *data, int len)
{
    uint16_t chunk_sz;
    uint16_t data_off;
    uint16_t om_off;
    int rc;

    if (len <= 0) {
        return 0;
    }

    data_off = 0;
    om = dpl_mbuf_off(om, off, &om_off);
    while (1) {
        if (om == NULL) {
            return INT_MAX;
        }

        chunk_sz = mynewt_min(om->om_len - om_off, len - data_off);
        if (chunk_sz > 0) {
            rc = memcmp(om->om_data + om_off, data + data_off, chunk_sz);
            if (rc != 0) {
                return rc;
            }
        }

        data_off += chunk_sz;
        if (data_off == len) {
            return 0;
        }

        om = SLIST_NEXT(om, om_next);
        om_off = 0;

        if (om == NULL) {
            return INT_MAX;
        }
    }
}

int
dpl_mbuf_cmpm(const struct dpl_mbuf *om1, uint16_t offset1,
             const struct dpl_mbuf *om2, uint16_t offset2,
             uint16_t len)
{
    const struct dpl_mbuf *cur1;
    const struct dpl_mbuf *cur2;
    uint16_t bytes_remaining;
    uint16_t chunk_sz;
    uint16_t om1_left;
    uint16_t om2_left;
    uint16_t om1_off;
    uint16_t om2_off;
    int rc;

    om1_off = 0;
    om2_off = 0;

    cur1 = dpl_mbuf_off(om1, offset1, &om1_off);
    cur2 = dpl_mbuf_off(om2, offset2, &om2_off);

    bytes_remaining = len;
    while (1) {
        if (bytes_remaining == 0) {
            return 0;
        }

        while (cur1 != NULL && om1_off >= cur1->om_len) {
            cur1 = SLIST_NEXT(cur1, om_next);
            om1_off = 0;
        }
        while (cur2 != NULL && om2_off >= cur2->om_len) {
            cur2 = SLIST_NEXT(cur2, om_next);
            om2_off = 0;
        }

        if (cur1 == NULL || cur2 == NULL) {
            return INT_MAX;
        }

        om1_left = cur1->om_len - om1_off;
        om2_left = cur2->om_len - om2_off;
        chunk_sz = mynewt_min(mynewt_min(om1_left, om2_left), bytes_remaining);

        rc = memcmp(cur1->om_data + om1_off, cur2->om_data + om2_off,
                    chunk_sz);
        if (rc != 0) {
            return rc;
        }

        om1_off += chunk_sz;
        om2_off += chunk_sz;
        bytes_remaining -= chunk_sz;
    }
}

struct dpl_mbuf *
dpl_mbuf_prepend(struct dpl_mbuf *om, int len)
{
    struct dpl_mbuf *p;
    int leading;

    while (1) {
        /* Fill the available space at the front of the head of the chain, as
         * needed.
         */
        leading = mynewt_min(len, DPL_MBUF_LEADINGSPACE(om));

        om->om_data -= leading;
        om->om_len += leading;
        if (DPL_MBUF_IS_PKTHDR(om)) {
            DPL_MBUF_PKTHDR(om)->omp_len += leading;
        }

        len -= leading;
        if (len == 0) {
            break;
        }

        /* The current head didn't have enough space; allocate a new head. */
        if (DPL_MBUF_IS_PKTHDR(om)) {
            p = dpl_mbuf_get_pkthdr(om->om_omp,
                om->om_pkthdr_len - sizeof (struct dpl_mbuf_pkthdr));
        } else {
            p = dpl_mbuf_get(om->om_omp, 0);
        }
        if (p == NULL) {
            dpl_mbuf_free_chain(om);
            om = NULL;
            break;
        }

        if (DPL_MBUF_IS_PKTHDR(om)) {
            _dpl_mbuf_copypkthdr(p, om);
            om->om_pkthdr_len = 0;
        }

        /* Move the new head's data pointer to the end so that data can be
         * prepended.
         */
        p->om_data += DPL_MBUF_TRAILINGSPACE(p);

        SLIST_NEXT(p, om_next) = om;
        om = p;
    }

    return om;
}

struct dpl_mbuf *
dpl_mbuf_prepend_pullup(struct dpl_mbuf *om, uint16_t len)
{
    om = dpl_mbuf_prepend(om, len);
    if (om == NULL) {
        return NULL;
    }

    om = dpl_mbuf_pullup(om, len);
    if (om == NULL) {
        return NULL;
    }

    return om;
}

int
dpl_mbuf_copyinto(struct dpl_mbuf *om, int off, const void *src, int len)
{
    struct dpl_mbuf *next;
    struct dpl_mbuf *cur;
    const uint8_t *sptr;
    uint16_t cur_off;
    int copylen;
    int rc;

    /* Find the mbuf,offset pair for the start of the destination. */
    cur = dpl_mbuf_off(om, off, &cur_off);
    if (cur == NULL) {
        return -1;
    }

    /* Overwrite existing data until we reach the end of the chain. */
    sptr = src;
    while (1) {
        copylen = mynewt_min(cur->om_len - cur_off, len);
        if (copylen > 0) {
            memcpy(cur->om_data + cur_off, sptr, copylen);
            sptr += copylen;
            len -= copylen;

            copylen = 0;
        }

        if (len == 0) {
            /* All the source data fit in the existing mbuf chain. */
            return 0;
        }

        next = SLIST_NEXT(cur, om_next);
        if (next == NULL) {
            break;
        }

        cur = next;
        cur_off = 0;
    }

    /* Append the remaining data to the end of the chain. */
    rc = dpl_mbuf_append(cur, sptr, len);
    if (rc != 0) {
        return rc;
    }

    /* Fix up the packet header, if one is present. */
    if (DPL_MBUF_IS_PKTHDR(om)) {
        DPL_MBUF_PKTHDR(om)->omp_len =
            mynewt_max(DPL_MBUF_PKTHDR(om)->omp_len, off + len);
    }

    return 0;
}
EXPORT_SYMBOL(dpl_mbuf_copyinto);

void
dpl_mbuf_concat(struct dpl_mbuf *first, struct dpl_mbuf *second)
{
    struct dpl_mbuf *next;
    struct dpl_mbuf *cur;

    /* Point 'cur' to the last buffer in the first chain. */
    cur = first;
    while (1) {
        next = SLIST_NEXT(cur, om_next);
        if (next == NULL) {
            break;
        }

        cur = next;
    }

    /* Attach the second chain to the end of the first. */
    SLIST_NEXT(cur, om_next) = second;

    /* If the first chain has a packet header, calculate the length of the
     * second chain and add it to the header length.
     */
    if (DPL_MBUF_IS_PKTHDR(first)) {
        if (DPL_MBUF_IS_PKTHDR(second)) {
            DPL_MBUF_PKTHDR(first)->omp_len += DPL_MBUF_PKTHDR(second)->omp_len;
        } else {
            for (cur = second; cur != NULL; cur = SLIST_NEXT(cur, om_next)) {
                DPL_MBUF_PKTHDR(first)->omp_len += cur->om_len;
            }
        }
    }

    second->om_pkthdr_len = 0;
}

void *
dpl_mbuf_extend(struct dpl_mbuf *om, uint16_t len)
{
    struct dpl_mbuf *newm;
    struct dpl_mbuf *last;
    void *data;

    if (len > om->om_omp->omp_databuf_len) {
        return NULL;
    }

    /* Scroll to last mbuf in the chain */
    last = om;
    while (SLIST_NEXT(last, om_next) != NULL) {
        last = SLIST_NEXT(last, om_next);
    }

    if (DPL_MBUF_TRAILINGSPACE(last) < len) {
        newm = dpl_mbuf_get(om->om_omp, 0);
        if (newm == NULL) {
            return NULL;
        }

        SLIST_NEXT(last, om_next) = newm;
        last = newm;
    }

    data = last->om_data + last->om_len;
    last->om_len += len;

    if (DPL_MBUF_IS_PKTHDR(om)) {
        DPL_MBUF_PKTHDR(om)->omp_len += len;
    }

    return data;
}


struct dpl_mbuf *
dpl_mbuf_pullup(struct dpl_mbuf *om, uint16_t len)
{
    struct dpl_mbuf_pool *omp;
    struct dpl_mbuf *next;
    struct dpl_mbuf *om2;
    int count;
    int space;

    omp = om->om_omp;

    /*
     * If first mbuf has no cluster, and has room for len bytes
     * without shifting current data, pullup into it,
     * otherwise allocate a new mbuf to prepend to the chain.
     */
    if (om->om_len >= len) {
        return (om);
    }
    if (om->om_len + DPL_MBUF_TRAILINGSPACE(om) >= len &&
        SLIST_NEXT(om, om_next)) {
        om2 = om;
        om = SLIST_NEXT(om, om_next);
        len -= om2->om_len;
    } else {
        if (len > omp->omp_databuf_len - om->om_pkthdr_len) {
            goto bad;
        }

        om2 = dpl_mbuf_get(omp, 0);
        if (om2 == NULL) {
            goto bad;
        }

        if (DPL_MBUF_IS_PKTHDR(om)) {
            _dpl_mbuf_copypkthdr(om2, om);
        }
    }
    space = DPL_MBUF_TRAILINGSPACE(om2);
    do {
        count = mynewt_min(mynewt_min(len, space), om->om_len);
        memcpy(om2->om_data + om2->om_len, om->om_data, count);
        len -= count;
        om2->om_len += count;
        om->om_len -= count;
        space -= count;
        if (om->om_len) {
            om->om_data += count;
        } else {
            next = SLIST_NEXT(om, om_next);
            dpl_mbuf_free(om);
            om = next;
        }
    } while (len > 0 && om);
    if (len > 0) {
        dpl_mbuf_free(om2);
        goto bad;
    }
    SLIST_NEXT(om2, om_next) = om;
    return (om2);
bad:
    dpl_mbuf_free_chain(om);
    return (NULL);
}

struct dpl_mbuf *
dpl_mbuf_trim_front(struct dpl_mbuf *om)
{
    struct dpl_mbuf *next;
    struct dpl_mbuf *cur;

    /* Abort early if there is nothing to trim. */
    if (om->om_len != 0) {
        return om;
    }

    /* Starting with the second mbuf in the chain, continue removing and
     * freeing mbufs until an non-empty one is encountered.
     */
    cur = SLIST_NEXT(om, om_next);
    while (cur != NULL && cur->om_len == 0) {
        next = SLIST_NEXT(cur, om_next);

        SLIST_NEXT(om, om_next) = next;
        dpl_mbuf_free(cur);

        cur = next;
    }

    if (cur == NULL) {
        /* All buffers after the first have been freed. */
        return om;
    }

    /* Try to remove the first mbuf in the chain.  If this buffer contains a
     * packet header, make sure the second buffer can accommodate it.
     */
    if (DPL_MBUF_LEADINGSPACE(cur) >= om->om_pkthdr_len) {
        /* Second buffer has room; copy packet header. */
        cur->om_pkthdr_len = om->om_pkthdr_len;
        memcpy(DPL_MBUF_PKTHDR(cur), DPL_MBUF_PKTHDR(om), om->om_pkthdr_len);

        /* Free first buffer. */
        dpl_mbuf_free(om);
        om = cur;
    }

    return om;
}
