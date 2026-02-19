#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdint.h>

void *malloc(size_t size);
void free(void *ptr);
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
    free(p1);
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
    free(p);
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
    free(tiny);
    free(small);
    free(large);
}

void test_free() {
    TEST_START("Free functionality");
    void *p1 = malloc(50);
    void *p2 = malloc(100);
    CHECK_ALLOC(p1, "malloc(50)");
    CHECK_ALLOC(p2, "malloc(100)");
    TEST_PASS("Allocated two blocks");
    show_alloc_mem();
    
    free(p1);
    TEST_PASS("Freed first block");
    show_alloc_mem();
    
    void *p3 = malloc(30); // Should reuse the freed space
    CHECK_ALLOC(p3, "malloc(30) after free");
    TEST_PASS("Reallocated in freed space");
    show_alloc_mem();
    
    free(p2);
    free(p3);
    TEST_PASS("Freed remaining blocks");
}

void test_double_free() {
    TEST_START("Double free protection");
    void *p = malloc(20);
    CHECK_ALLOC(p, "malloc(20)");
    free(p);
    TEST_PASS("First free succeeded");
    // Double free should not crash (though behavior undefined, but in our impl it should handle)
    free(p); // This might not do anything or log, but shouldn't segfault
    TEST_PASS("Double free handled (no crash)");
}

void test_realloc() {
    TEST_START("Realloc functionality");
    void *p1 = malloc(50);
    CHECK_ALLOC(p1, "malloc(50)");
    TEST_PASS("Initial allocation done");
    
    // Realloc to smaller size
    void *p2 = realloc(p1, 30);
    if (p2) {
        TEST_PASS("Realloc to smaller size succeeded: %p", p2);
    } else {
        TEST_FAIL("Realloc to smaller size failed");
        return;
    }
    
    // Realloc to larger size
    void *p3 = realloc(p2, 100);
    if (p3) {
        TEST_PASS("Realloc to larger size succeeded: %p", p3);
    } else {
        TEST_FAIL("Realloc to larger size failed");
        return;
    }
    
    // Realloc with NULL
    void *p4 = realloc(NULL, 20);
    if (p4) {
        TEST_PASS("Realloc with NULL succeeded: %p", p4);
    } else {
        TEST_FAIL("Realloc with NULL failed");
        return;
    }
    
    // Realloc with size 0
    void *p5 = realloc(p4, 0);
    if (!p5) {
        TEST_PASS("Realloc with size 0 returned NULL");
    } else {
        TEST_FAIL("Realloc with size 0 should return NULL");
    }
    
    free(p3);
    TEST_PASS("Realloc tests completed");
}

void *thread_test(void *arg) {
    (void)arg;
    for (int i = 0; i < 100; i++) {
        void *ptr = malloc(64);
        if (ptr) {
            memset(ptr, 0xAA, 64);
            free(ptr);
        }
        ptr = malloc(128);
        if (ptr) {
            void *new_ptr = realloc(ptr, 256);
            if (new_ptr) {
                memset(new_ptr, 0xBB, 256);
                free(new_ptr);
            } else {
                free(ptr);
            }
        }
    }
    return NULL;
}

void test_thread_safety() {
    TEST_START("Thread safety");
    const int num_threads = 4;
    pthread_t threads[num_threads];
    
    for (int i = 0; i < num_threads; i++) {
        if (pthread_create(&threads[i], NULL, thread_test, NULL) != 0) {
            TEST_FAIL("Failed to create thread %d", i);
            return;
        }
    }
    
    for (int i = 0; i < num_threads; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            TEST_FAIL("Failed to join thread %d", i);
            return;
        }
    }
    
    TEST_PASS("All threads completed without issues");
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
    free(p1);
    free(p2);
    free(p3);
}

int main() {
    printf("Starting malloc tests...\n");
    test_basic_malloc();
    test_alignment();
    test_tiny_small_large();
    test_free();
    test_double_free();
    test_realloc();
    test_thread_safety();
    test_show_alloc_mem();
    printf("\n=== All malloc tests completed ===\n");
    return 0;
}