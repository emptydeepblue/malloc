#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdint.h>

void *malloc(size_t size);
void show_alloc_mem(void);

// Helper macros for readability
#define TEST_START(name) printf("\n\033[0;34m=== Test: %s ===\033[0m\n", name)
#define TEST_PASS(format, ...) printf("\033[0;32m✓ PASS: " format "\033[0m\n", ##__VA_ARGS__)
#define TEST_FAIL(format, ...) printf("\033[0;31m✗ FAIL: " format "\033[0m\n", ##__VA_ARGS__)
#define CHECK_ALLOC(ptr, desc) \
    if (!(ptr)) { \
        TEST_FAIL("%s allocation failed (NULL)", desc); \
        return; \
    } else { \
        TEST_PASS("%s allocated at %p", desc, ptr); \
    }

void test_basic_malloc() {
    TEST_START("Basic malloc");
    void *p1 = malloc(10);
    CHECK_ALLOC(p1, "malloc(10)");
    show_alloc_mem();
}

void test_alignment() {
    TEST_START("Alignment check");
    void *p = malloc(1);
    CHECK_ALLOC(p, "malloc(1)");
    if ((uintptr_t)p % 16 != 0) {
        TEST_FAIL("Pointer not aligned to 16 bytes: %p", p);
    } else {
        TEST_PASS("Pointer aligned to 16 bytes: %p", p);
    }
    show_alloc_mem();
}

void test_tiny_small_large() {
    TEST_START("TINY, SMALL, LARGE allocations");
    void *tiny = malloc(32);   // TINY ( <= 64)
    void *small = malloc(128); // SMALL (65-1024)
    void *large = malloc(2048); // LARGE (>1024)
    CHECK_ALLOC(tiny, "TINY malloc(32)");
    CHECK_ALLOC(small, "SMALL malloc(128)");
    CHECK_ALLOC(large, "LARGE malloc(2048)");
    TEST_PASS("All zone types allocated successfully");
    show_alloc_mem();
}

void test_show_alloc_mem() {
    TEST_START("show_alloc_mem with multiple allocations");
    void *p1 = malloc(42);
    void *p2 = malloc(84);
    void *p3 = malloc(1000);
    CHECK_ALLOC(p1, "malloc(42)");
    CHECK_ALLOC(p2, "malloc(84)");
    CHECK_ALLOC(p3, "malloc(1000)");
    TEST_PASS("Multiple allocations done");
    printf("Memory map:\n");
    show_alloc_mem();
}

int main() {
    printf("Starting malloc tests...\n");
    test_basic_malloc();
    test_alignment();
    test_tiny_small_large();
    test_show_alloc_mem();
    printf("\n=== All malloc tests completed ===\n");
    return 0;
}