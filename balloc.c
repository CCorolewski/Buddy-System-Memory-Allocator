#include <stdio.h>
#include <stddef.h>

#include "balloc.h"
#include "freelist.h"
#include "bbm.h"
#include "utils.h"


/*
 * One entry for every minimum-sized block in the pool.
 *
 * A value of 0 means this address is not the beginning
 * of an allocated block.
 *
 * Otherwise the value stores the exponent of the
 * allocation beginning at that address.
 */
typedef struct {
    void *base;
    unsigned int size;

    int l;
    int u;

    FreeList freelist;
    BBM *buddy_maps;

    unsigned char *alloc_sizes;
    size_t alloc_count;

} BallocData;


/*
 * ------------------------------------------------------------
 * Helper functions
 * ------------------------------------------------------------
 */

static BBM getmap(BallocData *bp, int e)
{
    return bp->buddy_maps[e - bp->l];
}


/*
 * Return the index in alloc_sizes corresponding to mem.
 */
static size_t allocindex(BallocData *bp, void *mem)
{
    size_t offset =
        (size_t)((char *)mem - (char *)bp->base);

    return offset / e2size(bp->l);
}


/*
 * Record the exponent of an allocation.
 */
static void recordalloc(BallocData *bp, void *mem, int e)
{
    size_t index = allocindex(bp, mem);

    if (index < bp->alloc_count)
        bp->alloc_sizes[index] = (unsigned char)e;
}


/*
 * Remove the allocation record.
 */
static void clearalloc(BallocData *bp, void *mem)
{
    size_t index = allocindex(bp, mem);

    if (index < bp->alloc_count)
        bp->alloc_sizes[index] = 0;
}


/*
 * Return the recorded exponent for mem.
 */
static int getalloce(BallocData *bp, void *mem)
{
    if (mem == NULL)
        return -1;

    char *base = (char *)bp->base;
    char *p = (char *)mem;

    if (p < base ||
        p >= base + bp->size) {
        return -1;
    }

    size_t offset = (size_t)(p - base);

    /*
     * Allocations must begin on a minimum-block boundary.
     */
    if (offset % e2size(bp->l) != 0)
        return -1;

    size_t index = allocindex(bp, mem);

    if (index >= bp->alloc_count)
        return -1;

    if (bp->alloc_sizes[index] == 0)
        return -1;

    return (int)bp->alloc_sizes[index];
}


/*
 * Remove a specific block from a free list.
 *
 * freelist.c stores its array of heads as void ** and stores
 * the next pointer in the beginning of each free block.
 */
static int removefreeblock(BallocData *bp, void *target, int e)
{
    void **heads = (void **)bp->freelist;

    int listindex = e - bp->l;

    void *current = heads[listindex];
    void *previous = NULL;

    while (current != NULL) {

        if (current == target) {

            void *next = *(void **)current;

            if (previous == NULL) {
                heads[listindex] = next;
            }
            else {
                *(void **)previous = next;
            }

            return 1;
        }

        previous = current;
        current = *(void **)current;
    }

    return 0;
}


/*
 * ------------------------------------------------------------
 * bcreate
 * ------------------------------------------------------------
 */

extern Balloc bcreate(unsigned int size, int l, int u)
{
    if (size == 0 || l > u)
        return NULL;

    if (size < e2size(l))
        return NULL;

    int levels = u - l + 1;

    BallocData *bp =
        mmalloc(sizeof(BallocData));

    if ((long)bp == -1)
        return NULL;

    bp->base = NULL;
    bp->size = size;
    bp->l = l;
    bp->u = u;

    bp->freelist = NULL;
    bp->buddy_maps = NULL;
    bp->alloc_sizes = NULL;
    bp->alloc_count = 0;


    /*
     * Allocate managed memory.
     */
    bp->base = mmalloc(size);

    if ((long)bp->base == -1) {
        mmfree(bp, sizeof(BallocData));
        return NULL;
    }


    /*
     * Create free lists.
     */
    bp->freelist =
        freelistcreate(size, l, u);

    if (bp->freelist == NULL) {

        mmfree(bp->base, size);
        mmfree(bp, sizeof(BallocData));

        return NULL;
    }


    /*
     * Allocate buddy-map pointer array.
     */
    bp->buddy_maps =
        mmalloc((size_t)levels * sizeof(BBM));

    if ((long)bp->buddy_maps == -1) {

        freelistdelete(bp->freelist, l, u);

        mmfree(bp->base, size);
        mmfree(bp, sizeof(BallocData));

        return NULL;
    }


    /*
     * Create buddy maps.
     */
    for (int e = l; e <= u; e++) {

        bp->buddy_maps[e - l] =
            bbmcreate(size, e);

        if (bp->buddy_maps[e - l] == NULL) {

            for (int i = l; i < e; i++) {
                bbmdelete(
                    bp->buddy_maps[i - l]
                );
            }

            mmfree(
                bp->buddy_maps,
                (size_t)levels * sizeof(BBM)
            );

            freelistdelete(
                bp->freelist,
                l,
                u
            );

            mmfree(bp->base, size);
            mmfree(bp, sizeof(BallocData));

            return NULL;
        }
    }


    /*
     * Allocate allocation-size metadata.
     *
     * There is one byte for every minimum-sized block.
     */
    bp->alloc_count =
        divup(size, e2size(l));

    bp->alloc_sizes =
        mmalloc(bp->alloc_count *
                sizeof(unsigned char));

    if ((long)bp->alloc_sizes == -1) {

        for (int e = l; e <= u; e++) {
            bbmdelete(
                bp->buddy_maps[e - l]
            );
        }

        mmfree(
            bp->buddy_maps,
            (size_t)levels * sizeof(BBM)
        );

        freelistdelete(
            bp->freelist,
            l,
            u
        );

        mmfree(bp->base, size);
        mmfree(bp, sizeof(BallocData));

        return NULL;
    }


    /*
     * Initially nothing is allocated.
     */
    for (size_t i = 0;
         i < bp->alloc_count;
         i++) {

        bp->alloc_sizes[i] = 0;
    }


    /*
     * Construct the initial free blocks.
     */
    unsigned int remaining = size;
    char *current = (char *)bp->base;

    while (remaining >= e2size(l)) {

        int e = u;

        while (e > l &&
               e2size(e) > remaining) {

            e--;
        }

        freelistfree(
            bp->freelist,
            bp->base,
            current,
            e,
            l
        );

        current += e2size(e);

        remaining -=
            (unsigned int)e2size(e);
    }

    return (Balloc)bp;
}


/*
 * ------------------------------------------------------------
 * balloc
 * ------------------------------------------------------------
 */

extern void *balloc(Balloc pool, unsigned int size)
{
    if (pool == NULL || size == 0)
        return NULL;

    BallocData *bp =
        (BallocData *)pool;


    /*
     * Determine requested exponent.
     */
    int e = size2e(size);

    if (e < bp->l)
        e = bp->l;

    if (e > bp->u)
        return NULL;


    /*
     * Search for a free block.
     */
    int found = e;
    void *mem = NULL;

    while (found <= bp->u) {

        mem = freelistalloc(
            bp->freelist,
            bp->base,
            found,
            bp->l
        );

        if (mem != NULL)
            break;

        found++;
    }


    if (mem == NULL)
        return NULL;


    /*
     * Split larger blocks until the requested
     * block size is reached.
     */
    while (found > e) {

        found--;

        void *buddy =
            baddrinv(
                bp->base,
                mem,
                found
            );

        freelistfree(
            bp->freelist,
            bp->base,
            buddy,
            found,
            bp->l
        );
    }


    /*
     * Mark this buddy pair as allocated.
     */
    bbmset(
        getmap(bp, e),
        bp->base,
        mem,
        e
    );


    /*
     * Remember the exact allocation size.
     */
    recordalloc(bp, mem, e);

    return mem;
}


/*
 * ------------------------------------------------------------
 * bsize
 * ------------------------------------------------------------
 */

extern unsigned int bsize(Balloc pool, void *mem)
{
    if (pool == NULL || mem == NULL)
        return 0;

    BallocData *bp =
        (BallocData *)pool;

    int e =
        getalloce(bp, mem);

    if (e < bp->l ||
        e > bp->u) {
        return 0;
    }

    return (unsigned int)e2size(e);
}


/*
 * ------------------------------------------------------------
 * bfree
 * ------------------------------------------------------------
 */

extern void bfree(Balloc pool, void *mem)
{
    if (pool == NULL || mem == NULL)
        return;

    BallocData *bp =
        (BallocData *)pool;


    /*
     * Determine exact allocation exponent.
     */
    int e =
        getalloce(bp, mem);

    if (e < bp->l ||
        e > bp->u) {
        return;
    }


    /*
     * The allocation no longer exists.
     */
    clearalloc(bp, mem);


    /*
     * Clear its buddy bitmap entry.
     */
    bbmclr(
        getmap(bp, e),
        bp->base,
        mem,
        e
    );


    /*
     * Merge with free buddies.
     */
    while (e < bp->u) {

        void *buddy =
            baddrinv(
                bp->base,
                mem,
                e
            );


        /*
         * Buddy must be inside the pool.
         */
        if ((char *)buddy <
                (char *)bp->base ||
            (char *)buddy >=
                (char *)bp->base +
                bp->size) {

            break;
        }


        /*
         * The buddy must currently exist as a free
         * block of exactly this size.
         */
        int buddy_size =
            freelistsize(
                bp->freelist,
                bp->base,
                buddy,
                bp->l,
                bp->u
            );

        if (buddy_size !=
            (int)e2size(e)) {

            break;
        }


        /*
         * Remove the free buddy so that the two
         * blocks can become one larger block.
         */
        if (!removefreeblock(
                bp,
                buddy,
                e)) {

            break;
        }


        /*
         * The merged block starts at the lower
         * buddy address.
         */
        mem =
            baddrclr(
                bp->base,
                mem,
                e
            );

        e++;
    }


    /*
     * Put the resulting block back onto the
     * appropriate free list.
     */
    freelistfree(
        bp->freelist,
        bp->base,
        mem,
        e,
        bp->l
    );
}


/*
 * ------------------------------------------------------------
 * bprint
 * ------------------------------------------------------------
 */

extern void bprint(Balloc pool)
{
    if (pool == NULL) {
        printf("allocator: NULL\n");
        return;
    }

    BallocData *bp =
        (BallocData *)pool;


    printf("Buddy Allocator\n");
    printf("----------------\n");

    printf("base: %p\n",
           bp->base);

    printf("size: %u\n",
           bp->size);

    printf("l:    %d\n",
           bp->l);

    printf("u:    %d\n",
           bp->u);


    /*
     * Print free lists.
     */
    printf("\nFree Lists\n");
    printf("----------\n");

    freelistprint(
        bp->freelist,
        bp->l,
        bp->u
    );


    /*
     * Print buddy maps.
     */
    printf("\nBitmaps\n");
    printf("-------\n");

    for (int e = bp->l;
         e <= bp->u;
         e++) {

        printf("e=%d: ", e);

        bbmprt(
            getmap(bp, e)
        );
    }
}


/*
 * ------------------------------------------------------------
 * bdelete
 * ------------------------------------------------------------
 */

extern void bdelete(Balloc pool)
{
    if (pool == NULL)
        return;

    BallocData *bp =
        (BallocData *)pool;

    int levels =
        bp->u - bp->l + 1;


    /*
     * Delete allocation metadata.
     */
    mmfree(
        bp->alloc_sizes,
        bp->alloc_count *
            sizeof(unsigned char)
    );


    /*
     * Delete buddy maps.
     */
    for (int e = bp->l;
         e <= bp->u;
         e++) {

        bbmdelete(
            bp->buddy_maps[
                e - bp->l
            ]
        );
    }


    mmfree(
        bp->buddy_maps,
        (size_t)levels *
            sizeof(BBM)
    );


    /*
     * Delete free lists.
     */
    freelistdelete(
        bp->freelist,
        bp->l,
        bp->u
    );


    /*
     * Delete managed memory.
     */
    mmfree(
        bp->base,
        bp->size
    );


    /*
     * Delete allocator structure.
     */
    mmfree(
        bp,
        sizeof(BallocData)
    );
}
