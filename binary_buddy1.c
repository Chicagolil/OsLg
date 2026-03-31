#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include "binary_buddy1.h"

/*
 * Initialize the structures needed for the buddy allocator.
 * Returns 0 on success, -1 on failure.
 */
static int init_structures(const void *memory_base, size_t size);

/*
 * Free the buddy allocator's internal data structures.
 */
static void free_structures();

/*
 * Utils
 */
static size_t next_power_of_two(size_t x);
static size_t get_required_block_size(size_t requested_size);
static size_t level_block_size(size_t level);
static size_t first_index_at_level(size_t level);
static size_t block_size_to_level(size_t block_size);

/*
 * Free-list helpers
 */
static void   fl_push(size_t level, size_t node_index);
static long   fl_pop(size_t level);
static void   fl_remove(size_t level, size_t node_index);

/*
 * Tree helpers
 */
static size_t left_child(size_t index);
static size_t right_child(size_t index);
static size_t parent_index(size_t index);
static size_t buddy_index(size_t index);

/*
 * bfree() helpers
 */
static void try_merge_upward(size_t index, size_t level);

enum { ALLOC_LEVEL_FREE = -1 };


// ------------------- START PROTECTED CODE -------------------

Allocator a;

void *init_buddy(size_t size)
{
    void *memory_base = mmap(NULL, size, PROT_READ | PROT_WRITE,
                             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (memory_base == MAP_FAILED) {
        perror("mmap");
        return NULL;
    }

    if (init_structures(memory_base, size) != 0) {
        munmap(memory_base, size);
        return NULL;
    }

    return memory_base;
}

int free_buddy()
{
    free_structures();
    return munmap((void *)a.base, a.total_size);
}

size_t get_used_space()
{
    return a.used_space;
}

// ------------------- END PROTECTED CODE -------------------

void *balloc(size_t size)
{
    size_t required_block_size;
    size_t target_level;
    long   node_index;
    size_t level;
    size_t first_index;
    size_t position_in_level;
    size_t offset;
    size_t start_unit;

    required_block_size = get_required_block_size(size);
    if (required_block_size == 0)
        return NULL;

    target_level = block_size_to_level(required_block_size);

    /*
     * Find the smallest level >= target_level that has a free block.
     * We search up the tree (larger blocks) only if target_level is empty.
     */
    node_index = -1;
    for (level = target_level; ; level--) {
        node_index = fl_pop(level);
        if (node_index != -1)
            break;
        if (level == 0)
            return NULL;   /* no memory available at all */
    }

    /*
     * We got a free block at `level`, which is coarser than needed.
     * Split it down until we reach target_level, pushing the right
     * half of each split onto the free-list.
     */
    while (level < target_level) {
        size_t left  = left_child((size_t)node_index);
        size_t right = right_child((size_t)node_index);

        a.tree[(size_t)node_index] = NODE_SPLIT;
        level++;

        /* right buddy goes onto the free-list for this new level */
        a.tree[right] = NODE_FREE;
        fl_push(level, right);

        /* continue splitting the left child */
        a.tree[left] = NODE_FREE;
        node_index   = (long)left;
    }

    /* Mark the chosen node as fully allocated */
    a.tree[(size_t)node_index] = NODE_FULL;

    /* Compute the memory address */
    first_index      = first_index_at_level(target_level);
    position_in_level = (size_t)node_index - first_index;
    offset           = position_in_level * level_block_size(target_level);

    /* Update accounting */
    a.used_space += required_block_size;

    start_unit = offset / a.min_block_size;
    a.alloc_level[start_unit] = (int)target_level;

    return (void *)((char *)a.base + offset);
}

void bfree(void *ptr)
{
    size_t offset;
    size_t unit_index;
    int    level_int;
    size_t level;
    size_t block_size;
    size_t position_in_level;
    size_t node_index;
    size_t first_index;

    if (ptr == NULL)
        return;
    if (a.base == NULL)
        return;

    offset = (size_t)((char *)ptr - (char *)a.base);

    if (offset >= a.total_size)
        return;
    if (offset % a.min_block_size != 0)
        return;

    unit_index = offset / a.min_block_size;

    if (unit_index >= a.leaf_count)
        return;

    level_int = a.alloc_level[unit_index];
    if (level_int == ALLOC_LEVEL_FREE)
        return;

    level      = (size_t)level_int;
    block_size = level_block_size(level);

    if (offset % block_size != 0)
        return;

    position_in_level = offset / block_size;
    first_index       = first_index_at_level(level);
    node_index        = first_index + position_in_level;

    if (node_index >= a.node_count)
        return;
    if (a.tree[node_index] != NODE_FULL)
        return;

    /* Mark free and update accounting */
    a.tree[node_index] = NODE_FREE;
    a.used_space      -= block_size;

    a.alloc_level[unit_index] = ALLOC_LEVEL_FREE;

    try_merge_upward(node_index, level);
}


// ------------------- INIT / FREE STRUCTURES -------------------

static int init_structures(const void *memory_base, size_t size)
{
    size_t level_count;
    size_t block_size;
    size_t leaf_count;
    size_t node_count;

    if (memory_base == NULL)
        return -1;
    if (MIN_ALLOC_SIZE == 0 || MAX_ALLOC_SIZE == 0)
        return -1;
    if (MIN_ALLOC_SIZE > MAX_ALLOC_SIZE)
        return -1;
    if ((MIN_ALLOC_SIZE & (MIN_ALLOC_SIZE - 1)) != 0)
        return -1;
    if (MIN_ALLOC_SIZE > size || size == 0)
        return -1;
    if (size % MIN_ALLOC_SIZE != 0)
        return -1;
    if ((size & (size - 1)) != 0)
        return -1;

    a.base          = memory_base;
    a.used_space    = 0;
    a.total_size    = size;
    a.min_block_size = MIN_ALLOC_SIZE;
    a.max_block_size = size;

    a.tree        = NULL;
    a.alloc_level = NULL;
    a.free_lists  = NULL;

    /* Count levels */
    block_size  = size;
    level_count = 0;
    while (block_size >= MIN_ALLOC_SIZE) {
        level_count++;
        if (block_size == MIN_ALLOC_SIZE)
            break;
        block_size /= 2;
    }
    if (block_size != MIN_ALLOC_SIZE)
        return -1;

    leaf_count = size / MIN_ALLOC_SIZE;
    node_count = (1UL << level_count) - 1;

    a.level_count = level_count;
    a.leaf_count  = leaf_count;
    a.node_count  = node_count;

    /* Allocate the binary-state tree */
    a.tree = malloc(node_count * sizeof(unsigned char));
    if (a.tree == NULL) {
        free_structures();
        return -1;
    }

    /* Allocate the per-leaf allocation-level map */
    a.alloc_level = malloc(leaf_count * sizeof(int));
    if (a.alloc_level == NULL) {
        free_structures();
        return -1;
    }

    /*
     * Allocate the free-list heads: one FreeNode* per level.
     * Each entry is either NULL (empty) or the head of a singly-linked
     * list of free blocks at that level.  The FreeNode structs are
     * written directly into the managed memory region, so no extra heap
     * allocation is needed beyond these pointer-sized heads.
     */
    a.free_lists = malloc(level_count * sizeof(FreeNode *));
    if (a.free_lists == NULL) {
        free_structures();
        return -1;
    }

    /* Initialise tree and alloc_level */
    memset(a.tree, NODE_FREE, node_count * sizeof(unsigned char));
    for (size_t i = 0; i < leaf_count; i++)
        a.alloc_level[i] = ALLOC_LEVEL_FREE;

    /* Initialise free-list heads: all NULL */
    for (size_t i = 0; i < level_count; i++)
        a.free_lists[i] = NULL;

    /*
     * The entire managed region is one free block at level 0.
     * Write the FreeNode header directly at the base of the region.
     */
    fl_push(0, 0);

    return 0;
}

static void free_structures()
{
    free(a.tree);
    free(a.alloc_level);
    free(a.free_lists);

    a.tree        = NULL;
    a.alloc_level = NULL;
    a.free_lists  = NULL;

    a.used_space    = 0;
    a.min_block_size = 0;
    a.max_block_size = 0;
    a.level_count   = 0;
    a.leaf_count    = 0;
    a.node_count    = 0;
}


// ------------------- UTILS -------------------

static size_t next_power_of_two(size_t n)
{
    if (n == 0) return 1;
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    if (sizeof(size_t) == 8)
        n |= n >> 32;
    return n + 1;
}

static size_t get_required_block_size(size_t requested_size)
{
    size_t block_size;

    if (requested_size == 0)
        return 0;
    if (requested_size > a.total_size)
        return 0;
    if (requested_size <= a.min_block_size)
        return a.min_block_size;

    block_size = next_power_of_two(requested_size);
    if (block_size > a.total_size)
        return 0;

    return block_size;
}

static size_t level_block_size(size_t level)
{
    return a.total_size >> level;
}

static size_t first_index_at_level(size_t level)
{
    return ((size_t)1 << level) - 1;
}

static size_t block_size_to_level(size_t block_size)
{
    size_t level       = 0;
    size_t current_size = a.total_size;
    while (current_size > block_size) {
        current_size /= 2;
        level++;
    }
    return level;
}

// ------------------- TREE HELPERS -------------------

static size_t left_child(size_t index)  { return 2 * index + 1; }
static size_t right_child(size_t index) { return 2 * index + 2; }
static size_t parent_index(size_t index){ return (index - 1) / 2; }

static size_t buddy_index(size_t index)
{
    /* Left child has odd index, right child has even index (> 0).
     * buddy of left  = right = index + 1
     * buddy of right = left  = index - 1  */
    return (index % 2 == 1) ? index + 1 : index - 1;
}

// ------------------- FREE-LIST -------------------

/*
 * A FreeNode is a tiny header written at the start of the free block
 * itself (in-place intrusive linked list).  It stores the tree node
 * index so we can update a.tree[] without recomputing it, and a
 * pointer to the next free block at the same level.
 */

/*
 * Convert a tree node index + level to the corresponding memory address
 * inside the managed region.
 */
static void *node_to_ptr(size_t node_index, size_t level)
{
    size_t first    = first_index_at_level(level);
    size_t pos      = node_index - first;
    size_t offset   = pos * level_block_size(level);
    return (void *)((char *)a.base + offset);
}

static void fl_push(size_t level, size_t node_index)
{
    FreeNode *fn = (FreeNode *)node_to_ptr(node_index, level);
    fn->node_index = node_index;
    fn->next       = a.free_lists[level];
    a.free_lists[level] = fn;
}

/*
 * Pop the list head at `level` in O(1).
 * Returns the tree node index, or -1 if empty.
 */
static long fl_pop(size_t level)
{
    FreeNode *head;

    if (a.free_lists[level] == NULL)
        return -1;

    head = a.free_lists[level];
    a.free_lists[level] = head->next;
    return (long)head->node_index;
}

/*
 * Remove a specific node from the free-list at `level`.
 * Used when merging: the buddy is already in the free-list and must be
 * pulled out before the merged parent is inserted at the level above.
 */
static void fl_remove(size_t level, size_t node_index)
{
    FreeNode *prev = NULL;
    FreeNode *cur  = a.free_lists[level];
    void     *target = node_to_ptr(node_index, level);

    while (cur != NULL) {
        if ((void *)cur == target) {
            if (prev == NULL)
                a.free_lists[level] = cur->next;
            else
                prev->next = cur->next;
            return;
        }
        prev = cur;
        cur  = cur->next;
    }
}


// ------------------- BFREE HELPER -------------------

static void try_merge_upward(size_t index, size_t level)
{
    size_t current = index;
    size_t cur_level = level;

    /* Add the freed node to the free-list for its level */
    fl_push(cur_level, current);

    /* Walk up, merging with buddy as long as the buddy is free */
    while (current != 0) {
        size_t bud    = buddy_index(current);
        size_t parent = parent_index(current);

        if (a.tree[bud] != NODE_FREE)
            break;   /* buddy is split or full — cannot merge */

        /* Both buddies are free: remove both from their level's list,
         * mark the parent free, and continue merging upward */
        fl_remove(cur_level, bud);
        fl_remove(cur_level, current);

        a.tree[current] = NODE_FREE;   /* redundant but explicit */
        a.tree[bud]     = NODE_FREE;
        a.tree[parent]  = NODE_FREE;

        cur_level--;
        current = parent;
        fl_push(cur_level, current);
    }

    /* If we stopped before the root, the parent must be NODE_SPLIT */
    if (current != 0) {
        size_t parent = parent_index(current);
        a.tree[parent] = NODE_SPLIT;
    }
}
