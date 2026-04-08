/*
* This file is a simple test file for your buddy allocator implementation. It is the first part
* based on questions 1 of the assignment.
*
* Compile and run it with: gcc binary_buddy.c main1.c -Wall -Wextra -pedantic -o main -lm && ./main
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "binary_buddy.h"

int test_question1(const void* base_addr);
void randomize_memory(void* base, size_t size);


int main() {

    printf("- MIN_ALLOC_SIZE: %lu bytes (n_bits: %u)\n", MIN_ALLOC_SIZE, MIN_ALLOC_SIZE_BITS);
    printf("- MAX_ALLOC_SIZE: %lu bytes (n_bits: %u)\n", MAX_ALLOC_SIZE, MAX_ALLOC_SIZE_BITS);

    void *base = init_buddy(MAX_ALLOC_SIZE);
    if (base == NULL) {
        fprintf(stderr,"Failed to initialize buddy allocator\n");
        return 1;
    }
    randomize_memory(base, MAX_ALLOC_SIZE);
    printf("Memory base allocated at: %p (size: %lu bytes)\n\n", base, MAX_ALLOC_SIZE);

    test_question1(base);

    if (free_buddy() != 0) {
        fprintf(stderr, "Failed to free buddy allocator\n");
        return 1;
    }

    return 0;
}


int test_question1(const void* base_addr) {

    printf("------------------------------------------------------------\n");
    printf("[Test question 1 start]\n");

    char* ptrs[] = {NULL, NULL, NULL};

    // Allocate 8 bytes
    size_t a1 = 8;
    ptrs[0] = balloc(a1);
    assert(ptrs[0] != NULL);
    printf(" * Allocated 8 bytes at address %p, total used space: %lu bytes\n", (void*)ptrs[0], get_used_space());
    assert(get_used_space() == 8);
    assert(ptrs[0] == (char*)base_addr);
    for (size_t i = 0; i < a1; i++) {
        // Check if all it zeroes out
        assert(ptrs[0][i] == 0);
        // Write some data to the allocated block to check that it is correctly allocated
        ptrs[0][i] = (char)(i + 1);
        assert(ptrs[0][i] == (char)(i + 1));
    }

    // Allocate 4 bytes
    size_t a2 = 4;
    ptrs[1] = balloc(a2);
    assert(ptrs[1] != NULL);
    printf(" * Allocated 4 bytes at address %p, total used space: %lu bytes\n", (void*)ptrs[1], get_used_space());
    assert(get_used_space() == 12);
    assert(ptrs[1] == (char*)((size_t)base_addr + 8));
    for (size_t i = 0; i < a2; i++) {
        // Check if all it zeroes out
        assert(ptrs[1][i] == 0);
        // Write some data to the allocated block to check that it is correctly allocated
        ptrs[1][i] = (char)(i + 1);
        assert(ptrs[1][i] == (char)(i + 1));
    }

    // Allocate 4 bytes    
    size_t a3 = 4;
    ptrs[2] = balloc(a3);
    assert(ptrs[2] != NULL);
    printf(" * Allocated 4 bytes at address %p, total used space: %lu bytes\n", (void*)ptrs[2], get_used_space());
    assert(get_used_space() == 16);
    assert(ptrs[2] == (char*)((size_t)base_addr + 12));
    for (size_t i = 0; i < a3; i++) {
        // Check if all it zeroes out
        assert(ptrs[2][i] == 0);
        // Write some data to the allocated block to check that it is correctly allocated
        ptrs[2][i] = (char)(i + 1);
        assert(ptrs[2][i] == (char)(i + 1));
    }

    // Free allocated blocks
    for (size_t i = 0; i < 3; i++) {
        bfree(ptrs[i]);
        for (size_t j = 0; j < (i == 0 ? a1 : (i == 1 ? a2 : a3)); j++) {
            // Check if all it zeroes out after free
            assert(ptrs[i][j] == 0);
        }
    }
    printf(" * Freed allocated blocks, total used space: %lu bytes\n", get_used_space());
    assert(get_used_space() == 0);

    // Check if all is zeroed out after free
    for (size_t i = 0; i < MAX_ALLOC_SIZE; i++) {
        assert(((char*)base_addr)[i] == 0);
    }

    printf("[Test question 1 passed]\n");
    return 1;
}


void randomize_memory(void* base, size_t size) {
    srand(42);
    unsigned char* ptr = (unsigned char*)base;
    for (size_t i = 0; i < size; i++) {
        ptr[i] = rand() % 256;
    }
}
