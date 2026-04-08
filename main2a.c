/*
* This file is a simple test file for your buddy allocator implementation. It is the first part
* based on questions 2 (a) of the assignment.
*
* Compile and run it with: gcc binary_buddy.c main2a.c -Wall -Wextra -pedantic -o main -lm && ./main
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <time.h>
#include "binary_buddy.h"

int test_question2a(const void* base_addr);


int main() {

    printf("- MIN_ALLOC_SIZE: %lu bytes (n_bits: %u)\n", MIN_ALLOC_SIZE, MIN_ALLOC_SIZE_BITS);
    printf("- MAX_ALLOC_SIZE: %lu bytes (n_bits: %u)\n", MAX_ALLOC_SIZE, MAX_ALLOC_SIZE_BITS);

    void *base = init_buddy(MAX_ALLOC_SIZE);
    if (base == NULL) {
        fprintf(stderr,"Failed to initialize buddy allocator\n");
        return 1;
    }
    printf("Memory base allocated at: %p (size: %lu bytes)\n\n", base, MAX_ALLOC_SIZE);

    test_question2a(base);

    if (free_buddy() != 0) {
        fprintf(stderr, "Failed to free buddy allocator\n");
        return 1;
    }

    return 0;
}


int test_question2a(const void* base_addr) {
    printf("\n------------------------------------------------------------\n");
    printf("[Test question 2 (a) start]\n");

    char* ptrs[] = {NULL, NULL, NULL};
    time_t timestamps[3];  // Store timestamps for each allocation

    // Allocate 8 bytes
    size_t a1 = 8;
    time_t t_before_a1 = time(NULL);
    ptrs[0] = balloc(a1);
    time_t t_after_a1 = time(NULL);
    assert(ptrs[0] != NULL);
    printf(" * Allocated 8 bytes at address %p, total used space: %lu bytes\n", (void*)ptrs[0], get_used_space());
    assert(get_used_space() == 8);
    assert(ptrs[0] == (char*)base_addr);
    
    // Get and verify timestamp for first allocation
    timestamps[0] = get_timestamp(ptrs[0]);
    printf("   - Timestamp: %ld (allocation time range: %ld to %ld)\n", timestamps[0], t_before_a1, t_after_a1);
    assert(timestamps[0] >= t_before_a1 && timestamps[0] <= t_after_a1);
    
    for (size_t i = 0; i < a1; i++) {
        // Check if all it zeroes out
        assert(ptrs[0][i] == 0);
        // Write some data to the allocated block to check that it is correctly allocated
        ptrs[0][i] = (char)(i + 1);
        assert(ptrs[0][i] == (char)(i + 1));
    }

    // Allocate 4 bytes
    size_t a2 = 4;
    time_t t_before_a2 = time(NULL);
    ptrs[1] = balloc(a2);
    time_t t_after_a2 = time(NULL);
    assert(ptrs[1] != NULL);
    printf(" * Allocated 4 bytes at address %p, total used space: %lu bytes\n", (void*)ptrs[1], get_used_space());
    assert(get_used_space() == 12);
    assert(ptrs[1] == (char*)((size_t)base_addr + 8));
    
    // Get and verify timestamp for second allocation
    timestamps[1] = get_timestamp(ptrs[1]);
    printf("   - Timestamp: %ld (allocation time range: %ld to %ld)\n", timestamps[1], t_before_a2, t_after_a2);
    assert(timestamps[1] >= t_before_a2 && timestamps[1] <= t_after_a2);
    assert(timestamps[1] >= timestamps[0]);  // Should be allocated after or at same time as first
    
    for (size_t i = 0; i < a2; i++) {
        // Check if all it zeroes out
        assert(ptrs[1][i] == 0);
        // Write some data to the allocated block to check that it is correctly allocated
        ptrs[1][i] = (char)(i + 1);
        assert(ptrs[1][i] == (char)(i + 1));
    }

    // Allocate 4 bytes    
    size_t a3 = 4;
    time_t t_before_a3 = time(NULL);
    ptrs[2] = balloc(a3);
    time_t t_after_a3 = time(NULL);
    assert(ptrs[2] != NULL);
    printf(" * Allocated 4 bytes at address %p, total used space: %lu bytes\n", (void*)ptrs[2], get_used_space());
    assert(get_used_space() == 16);
    assert(ptrs[2] == (char*)((size_t)base_addr + 12));
    
    // Get and verify timestamp for third allocation
    timestamps[2] = get_timestamp(ptrs[2]);
    printf("   - Timestamp: %ld (allocation time range: %ld to %ld)\n", timestamps[2], t_before_a3, t_after_a3);
    assert(timestamps[2] >= t_before_a3 && timestamps[2] <= t_after_a3);
    assert(timestamps[2] >= timestamps[1]);  // Should be allocated after or at same time as second
    
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
    }
    printf(" * Freed allocated blocks, total used space: %lu bytes\n", get_used_space());
    assert(get_used_space() == 0);

    printf("[Test question 2 (a) passed]\n");
    return 1;
}
