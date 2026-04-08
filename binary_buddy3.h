#ifndef BINARY_BUDDY_H
#define BINARY_BUDDY_H

#include <stddef.h>
#include "buddy_size.h"

// You can add code/struct here if needed

typedef enum {
    NODE_FREE = 0,
    NODE_SPLIT = 1,
    NODE_FULL = 2
} NodeState;

// question 3
void display_mem(void);


// ------------------- START PROTECTED CODE -------------------

/* 
* Initialize the buddy allocator with a memory region of given size.
* Returns a pointer to the base of the memory region on success, NULL on failure.
*/
void* init_buddy(size_t size);

/*
* Release the memory region used by the buddy allocator.
* Returns 0 on success, -1 on failure.
*/
int free_buddy();

/* 
* Allocate a memory chunk of given size.
* Returns a pointer to the allocated chunk, NULL on failure.
*/
void* balloc(size_t size);

/* 
* Deallocate a previously allocated memory chunk.
*/
void bfree(void* ptr);

/*
* Get the total used space in bytes in the allocator.
*/
size_t get_used_space();

typedef struct {
    // The 3 fields below are protected
    const void* base; // Base address of the memory region
    size_t used_space; // Total used space in the allocator
    size_t total_size; // Total size of the memory region

// ------------------- END PROTECTED CODE -------------------

    size_t min_block_size;
    size_t level_count;
    size_t leaf_count;
    size_t node_count;
    unsigned char *tree;
    unsigned char *has_free;
    int *alloc_level;

    // Question 3
    int *ptrs;
    int alloc_counter;


} Allocator; // Cannot modify the name of this struct, but you can add fields to it



#endif // BINARY_BUDDY_H