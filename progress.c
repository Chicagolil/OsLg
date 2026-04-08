#include <assert.h>
#include <stdio.h>
#include "binary_buddy.h"

extern Allocator a;

static void print_allocator_state(void) {
    printf("\n[Etat allocateur]\n");
    printf("base           = %p\n", a.base);
    printf("used_space     = %zu\n", a.used_space);
    printf("total_size     = %zu\n", a.total_size);
    printf("min_block_size = %zu\n", a.min_block_size);
    printf("level_count    = %zu\n", a.level_count);
    printf("leaf_count     = %zu\n", a.leaf_count);
    printf("node_count     = %zu\n", a.node_count);
    printf("tree           = %p\n", (void*)a.tree);
    printf("alloc_level    = %p\n", (void*)a.alloc_level);
}

int main(void) {
    void *base;
    void *p1;
    void *p2;
    void *p3;
    void *p4;


    base = init_buddy(MAX_ALLOC_SIZE);
    assert(base != NULL);
    assert(base == (void*)a.base);
    assert(get_used_space() == 0);

    assert(a.total_size == MAX_ALLOC_SIZE);
    assert(a.min_block_size == MIN_ALLOC_SIZE);
    assert(a.level_count == (MAX_ALLOC_SIZE_BITS - MIN_ALLOC_SIZE_BITS + 1));
    assert(a.leaf_count == (MAX_ALLOC_SIZE / MIN_ALLOC_SIZE));
    assert(a.node_count == (2 * a.leaf_count - 1));
    assert(a.tree != NULL);
    assert(a.alloc_level != NULL);

    for (size_t i = 0; i < a.node_count; i++) {
        assert(a.tree[i] == NODE_FREE);
    }

    for (size_t i = 0; i < a.leaf_count; i++) {
        assert(a.alloc_level[i] == -1);
    }

    print_allocator_state();
    printf("\n[Test init_structures OK]\n");



    p1 = balloc(4);
    assert(p1 == base);
    assert(get_used_space() == 4);
    
    p2 = balloc(8);
    assert(p2 == (void *)((char *)base + 8));
    assert(get_used_space() == 12);
    
    p3 = balloc(2);
    assert(p3 == (void *)((char *)base + 4));
    assert(get_used_space() == 14);
    
    bfree(p3);
    assert(get_used_space() == 12);
    
    bfree(p1);
    assert(get_used_space() == 8);
    
    p4 = balloc(8);
    assert(p4 == base);
    assert(get_used_space() == 16);
    
    printf("p1 = %p\n", p1);
    printf("p2 = %p\n", p2);
    printf("p3 = %p\n", p3);
    printf("p4 = %p\n", p4);
    printf("used_space = %zu\n", get_used_space());
    printf("[Test bfree OK]\n");



    assert(free_buddy() == 0);
    printf("[free_buddy OK]\n");

    return 0;
}