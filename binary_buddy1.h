#ifndef BINARY_BUDDY_H
#define BINARY_BUDDY_H

#include <stddef.h>
#include "buddy_size.h"

/*
 * Intrusive singly-linked list node written directly into the free
 * memory block it represents.  No heap allocation required.
 */
typedef struct FreeNode {
    size_t           node_index;  /* index in a.tree[]            */
    struct FreeNode *next;        /* next free block at this level */
} FreeNode;

typedef enum {
    NODE_FREE  = 0,
    NODE_SPLIT = 1,
    NODE_FULL  = 2
} NodeState;

// ------------------- START PROTECTED CODE -------------------

/*
 * Initialize the buddy allocator with a memory region of given size.
 * Returns a pointer to the base of the memory region on success, NULL on failure.
 */
void *init_buddy(size_t size);

/*
 * Release the memory region used by the buddy allocator.
 * Returns 0 on success, -1 on failure.
 */
int free_buddy();

/*
 * Allocate a memory chunk of given size.
 * Returns a pointer to the allocated chunk, NULL on failure.
 */
void *balloc(size_t size);

/*
 * Deallocate a previously allocated memory chunk.
 */
void bfree(void *ptr);

/*
 * Get the total used space in bytes in the allocator.
 */
size_t get_used_space();

typedef struct {
    /* The 3 fields below are protected */
    const void *base;        /* Base address of the memory region  */
    size_t      used_space;  /* Total used space in the allocator  */
    size_t      total_size;  /* Total size of the memory region    */

// ------------------- END PROTECTED CODE -------------------

    size_t      min_block_size;
    size_t      max_block_size;
    size_t      level_count;
    size_t      leaf_count;
    size_t      node_count;
    unsigned char *tree;        /* per-node state: FREE / SPLIT / FULL */
    int           *alloc_level; /* per-leaf-unit: level at which it was allocated, or -1 */
    FreeNode     **free_lists;  /* free_lists[level] = head of free-block list at that level */

} Allocator;

#endif /* BINARY_BUDDY_H */
