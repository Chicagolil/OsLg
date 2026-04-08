/*
* this file is a simple test file for your buddy allocator implementation. it is the first part
* based on questions 3 of the assignment.
*
* compile and run it with: gcc binary_buddy.c main3.c -wall -wextra -pedantic -o main -lm && ./main
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "binary_buddy.h"

int test_question3(const void* base_addr);


int main() {

    printf("- MIN_ALLOC_SIZE: %lu bytes (n_bits: %u)\n", MIN_ALLOC_SIZE, MIN_ALLOC_SIZE_BITS);
    printf("- MAX_ALLOC_SIZE: %lu bytes (n_bits: %u)\n", MAX_ALLOC_SIZE, MAX_ALLOC_SIZE_BITS);

    void *base = init_buddy(MAX_ALLOC_SIZE);
    if (base == NULL) {
        fprintf(stderr,"Failed to initialize buddy allocator\n");
        return 1;
    }
    printf("Memory base allocated at: %p (size: %lu bytes)\n\n", base, MAX_ALLOC_SIZE);

    test_question3(base);

    if (free_buddy() != 0) {
        fprintf(stderr, "Failed to free buddy allocator\n");
        return 1;
    }

    return 0;
}


int test_question3(const void* base_addr) {

    printf("\n------------------------------------------------------------\n");
    printf("[Test question 3 start]\n");

    char* ptrs[] = {NULL, NULL, NULL, NULL, NULL};

    // Allocate 4 bytes
    ptrs[0] = balloc(4);
    assert(ptrs[0] != NULL);
    printf(" * Allocated 4 bytes at address %p, total used space: %lu bytes\n", (void*)ptrs[0], get_used_space());
    assert(get_used_space() == 4);
    // [0-3] allocated, [4-7] free, [8-15] free
    // Write some data to the allocated block to check that it is correctly allocated
    for (size_t i = 0; i < 4; i++) {
        ptrs[0][i] = (char)(i + 1);
        assert(ptrs[0][i] == (char)(i + 1));
    }

    // Allocate 8 bytes
    ptrs[1] = balloc(8);
    assert(ptrs[1] != NULL);
    printf(" * Allocated 8 bytes at address %p, total used space: %lu bytes\n", (void*)ptrs[1], get_used_space());
    assert(get_used_space() == 12);
    // [0-3] allocated, [4-7] free, [8-15] allocated
    // Write some data to the allocated block to check that it is correctly allocated
    for (size_t i = 0; i < 8; i++) {
        ptrs[1][i] = (char)(i + 1);
        assert(ptrs[1][i] == (char)(i + 1));
    }

    // Allocate 2 bytes
    ptrs[2] = balloc(2);
    assert(ptrs[2] != NULL);
    printf(" * Allocated 2 bytes at address %p, total used space: %lu bytes\n", (void*)ptrs[2], get_used_space());
    assert(get_used_space() == 14);
    // [0-3] allocated, [4-5] allocated, [6-7] free, [8-15] allocated
    // Write some data to the allocated block to check that it is correctly allocated
    for (size_t i = 0; i < 2; i++) {
        ptrs[2][i] = (char)(i + 1);
        assert(ptrs[2][i] == (char)(i + 1));
    }

    puts("\n============================================================");
    display_mem();
    puts("============================================================\n");

    // Allocate 2 bytes, should succeed because there is enough space in the free block [6-7]
    ptrs[3] = balloc(2);
    assert(ptrs[3] != NULL);
    printf(" * Allocated 2 bytes at address %p, total used space: %lu bytes\n", (void*)ptrs[3], get_used_space());
    assert(get_used_space() == MAX_ALLOC_SIZE);
    // [0-3] allocated, [4-5] allocated, [6-7] allocated, [8-11] allocated, [12-15] allocated -> All space is allocated
    // Write some data to the allocated block to check that it is correctly allocated
    for (size_t i = 0; i < 2; i++) {
        ptrs[3][i] = (char)(i + 1);
        assert(ptrs[3][i] == (char)(i + 1));
    }

    puts("\n============================================================");
    display_mem();
    puts("============================================================\n");

    // Free all
    for (size_t i = 0; i < 4; i++) {
        bfree(ptrs[i]);
    }
    assert(get_used_space() == 0);
    printf("[Test question 3 asserts passed, but you have to verify yourself if the output is the expected one]\n");
    return 1;
}
