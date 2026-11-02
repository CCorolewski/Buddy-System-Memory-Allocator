#include <stdio.h>

#include "freelist.h"
#include "utils.h"
#include "balloc.h"


static int tests = 0;
static int passed = 0;


/*
 * Record the result of one test.
 */
static void check(int condition, const char *name)
{
    tests++;

    if (condition) {
        printf("[PASS] %s\n", name);
        passed++;
    }
    else {
        printf("[FAIL] %s\n", name);
    }
}


/*
 * ------------------------------------------------------------
 * Free-list tests
 * ------------------------------------------------------------
 */
static void test_freelist(void)
{
    printf("\n========================================\n");
    printf("FREE LIST TESTS\n");
    printf("========================================\n");

    int l = 4;
    int u = 8;
    size_t size = 256;

    void *base = mmalloc(size);

    check((long)base != -1,
          "mmalloc creates test memory");

    if ((long)base == -1)
        return;

    FreeList f = freelistcreate(size, l, u);

    check(f != NULL,
          "freelistcreate creates free lists");

    if (f == NULL) {
        mmfree(base, size);
        return;
    }


    /*
     * Add one 256-byte block.
     */
    printf("\n--- Add 256-byte block ---\n");

    freelistfree(f, base, base, 8, l);

    int blocksize =
        freelistsize(f, base, base, l, u);

    check(blocksize == 256,
          "freelistsize returns 256");


    /*
     * Allocate the block again.
     */
    void *block =
        freelistalloc(f, base, 8, l);

    check(block == base,
          "freelistalloc returns the 256-byte block");


    /*
     * List should now be empty.
     */
    void *empty =
        freelistalloc(f, base, 8, l);

    check(empty == NULL,
          "freelistalloc returns NULL for empty list");


    /*
     * Add four 64-byte blocks.
     */
    printf("\n--- Four 64-byte blocks ---\n");

    void *block0 = base;
    void *block1 = (char *)base + 64;
    void *block2 = (char *)base + 128;
    void *block3 = (char *)base + 192;

    freelistfree(f, base, block0, 6, l);
    freelistfree(f, base, block1, 6, l);
    freelistfree(f, base, block2, 6, l);
    freelistfree(f, base, block3, 6, l);

    /*
     * Since insertion happens at the front,
     * allocation should happen in reverse order.
     */
    void *p0 = freelistalloc(f, base, 6, l);
    void *p1 = freelistalloc(f, base, 6, l);
    void *p2 = freelistalloc(f, base, 6, l);
    void *p3 = freelistalloc(f, base, 6, l);

    check(p0 == block3,
          "64-byte allocation #1 returns block3");

    check(p1 == block2,
          "64-byte allocation #2 returns block2");

    check(p2 == block1,
          "64-byte allocation #3 returns block1");

    check(p3 == block0,
          "64-byte allocation #4 returns block0");

    empty = freelistalloc(f, base, 6, l);

    check(empty == NULL,
          "64-byte free list is empty afterward");


    freelistdelete(f, l, u);
    mmfree(base, size);
}


/*
 * ------------------------------------------------------------
 * Basic buddy allocator tests
 * ------------------------------------------------------------
 */
static void test_balloc_basic(void)
{
    printf("\n========================================\n");
    printf("BASIC BALLOC TESTS\n");
    printf("========================================\n");

    Balloc pool = bcreate(256, 4, 8);

    check(pool != NULL,
          "bcreate creates allocator");

    if (pool == NULL)
        return;


    /*
     * 10 bytes should require a 16-byte block.
     */
    void *a = balloc(pool, 10);

    check(a != NULL,
          "balloc(10) succeeds");

    check(bsize(pool, a) == 16,
          "balloc(10) creates 16-byte allocation");


    /*
     * 20 bytes should require 32 bytes.
     */
    void *b = balloc(pool, 20);

    check(b != NULL,
          "balloc(20) succeeds");

    check(b != a,
          "second allocation has different address");

    check(bsize(pool, b) == 32,
          "balloc(20) creates 32-byte allocation");


    /*
     * 50 bytes should require 64 bytes.
     */
    void *c = balloc(pool, 50);

    check(c != NULL,
          "balloc(50) succeeds");

    check(c != a && c != b,
          "third allocation has different address");

    check(bsize(pool, c) == 64,
          "balloc(50) creates 64-byte allocation");


    /*
     * Maximum block is 256 bytes.
     */
    void *too_big = balloc(pool, 300);

    check(too_big == NULL,
          "balloc(300) fails");


    /*
     * Free b and make sure its block can be reused.
     */
    bfree(pool, b);

    void *d = balloc(pool, 20);

    check(d != NULL,
          "allocation succeeds after free");

    check(d == b,
          "recently freed block is reused");


    /*
     * Free everything.
     */
    bfree(pool, a);
    bfree(pool, c);
    bfree(pool, d);


    /*
     * Everything should coalesce back into one
     * 256-byte block.
     */
    void *whole = balloc(pool, 256);

    check(whole != NULL,
          "entire pool available after freeing everything");

    if (whole != NULL) {
        check(bsize(pool, whole) == 256,
              "whole-pool allocation has size 256");

        bfree(pool, whole);
    }

    bdelete(pool);
}


/*
 * ------------------------------------------------------------
 * Allocation-size tests
 * ------------------------------------------------------------
 */
static void test_bsize(void)
{
    printf("\n========================================\n");
    printf("BSIZE TESTS\n");
    printf("========================================\n");

    Balloc pool = bcreate(256, 4, 8);

    check(pool != NULL,
          "create allocator for bsize tests");

    if (pool == NULL)
        return;

    /*
     * These four allocations exactly fill 240 bytes:
     *
     * 1  -> 16
     * 17 -> 32
     * 63 -> 64
     * 65 -> 128
     *
     * Total = 240
     */
    void *a = balloc(pool, 1);
    void *b = balloc(pool, 17);
    void *c = balloc(pool, 63);
    void *d = balloc(pool, 65);

    check(a != NULL,
          "1-byte request succeeds");

    check(b != NULL,
          "17-byte request succeeds");

    check(c != NULL,
          "63-byte request succeeds");

    check(d != NULL,
          "65-byte request succeeds");


    if (a != NULL)
        check(bsize(pool, a) == 16,
              "bsize(1-byte request) == 16");

    if (b != NULL)
        check(bsize(pool, b) == 32,
              "bsize(17-byte request) == 32");

    if (c != NULL)
        check(bsize(pool, c) == 64,
              "bsize(63-byte request) == 64");

    if (d != NULL)
        check(bsize(pool, d) == 128,
              "bsize(65-byte request) == 128");


    if (a != NULL)
        bfree(pool, a);

    if (b != NULL)
        bfree(pool, b);

    if (c != NULL)
        bfree(pool, c);

    if (d != NULL)
        bfree(pool, d);

    bdelete(pool);
}


/*
 * ------------------------------------------------------------
 * Pool exhaustion / coalescing test
 * ------------------------------------------------------------
 */
static void test_exhaustion(void)
{
    printf("\n========================================\n");
    printf("POOL EXHAUSTION TESTS\n");
    printf("========================================\n");

    Balloc pool = bcreate(256, 4, 8);

    check(pool != NULL,
          "create allocator for exhaustion test");

    if (pool == NULL)
        return;

    void *blocks[16];

    /*
     * 256 / 16 = 16 allocations.
     */
    for (int i = 0; i < 16; i++) {

        blocks[i] = balloc(pool, 16);

        char name[80];

        snprintf(name,
                 sizeof(name),
                 "16-byte allocation %d succeeds",
                 i + 1);

        check(blocks[i] != NULL, name);
    }

    printf("\n--- State after allocating all 16 blocks ---\n");
    bprint(pool);

    /*
     * Pool should now be completely full.
     */
    void *extra = balloc(pool, 16);

    check(extra == NULL,
          "allocation fails when pool is exhausted");


    /*
     * Free all sixteen blocks.
     */
    for (int i = 0; i < 16; i++) {
        if (blocks[i] != NULL)
            bfree(pool, blocks[i]);
    }


printf("\n--- State after freeing all 16 blocks ---\n");
bprint(pool);

    /*
     * If coalescing worked, the entire pool should
     * once again be available as one block.
     */
    void *whole = balloc(pool, 256);

    check(whole != NULL,
          "pool completely coalesces after exhaustion");

    if (whole != NULL) {
        check(bsize(pool, whole) == 256,
              "coalesced block has size 256");

        bfree(pool, whole);
    }

    bdelete(pool);
}


/*
 * ------------------------------------------------------------
 * Main
 * ------------------------------------------------------------
 */
int main(void)
{
    printf("========================================\n");
    printf("BUDDY MEMORY ALLOCATOR TEST SUITE\n");
    printf("========================================\n");

    test_freelist();
    test_balloc_basic();
    test_bsize();
    test_exhaustion();


    printf("\n========================================\n");
    printf("TEST SUMMARY\n");
    printf("========================================\n");

    printf("Passed: %d\n", passed);
    printf("Failed: %d\n", tests - passed);
    printf("Total:  %d\n", tests);

    printf("========================================\n");

    if (passed == tests) {
        printf("ALL TESTS PASSED\n");
        return 0;
    }

    printf("SOME TESTS FAILED\n");
    return 1;
}
