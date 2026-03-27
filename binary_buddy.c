#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include "binary_buddy.h"

/*
* Initalize the structures needed for the buddy allocator.
* Returns 0 on success, -1 on failure.
*/
static int init_structures(const void* memory_base, size_t size);

/*
* Free the buddy allocator's internal data structures.
*/
static void free_structures();

/*
* Utils
*/
static size_t next_power_of_two(size_t x);                     // finds the next power of 2
static size_t get_required_block_size(size_t requested_size);
static size_t level_block_size(size_t level);
static size_t first_index_at_level(size_t level);
static size_t node_level(size_t node_index);
static size_t ptr_to_unit_index(void *ptr);

// ------------------- START PROTECTED CODE -------------------

Allocator a;

void* init_buddy(size_t size){
    void* memory_base = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
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

int free_buddy() {
    free_structures();
    return munmap((void*)a.base, a.total_size);
}

size_t get_used_space(){
    return a.used_space;
}

// ------------------- END PROTECTED CODE -------------------

void* balloc(size_t size) {
    // Implement this function to allocate a chunk of memory of given size using the buddy allocator
    return NULL;
}

void bfree(void* ptr) {
    // Implement this function to deallocate a previously allocated chunk of memory using the buddy allocator
}

static int init_structures(const void* memory_base, size_t size){
    // Implement this function to initialize your data structure used for the buddy allocator
    
    size_t level_count; 
    size_t block_size; 
    size_t leaf_count; 
    size_t node_count; 

    if(memory_base == NULL){ 
        return -1;
    }
    
    if( MIN_ALLOC_SIZE == 0 || MAX_ALLOC_SIZE == 0 ){
        return -1; 
    }

    if (MIN_ALLOC_SIZE > MAX_ALLOC_SIZE) {
        return -1;
    }

    if ((MIN_ALLOC_SIZE & (MIN_ALLOC_SIZE - 1)) != 0) {
        return -1;
    }


    if(MIN_ALLOC_SIZE > size || size == 0){
        return -1;
    }

    if(size % MIN_ALLOC_SIZE != 0){
        return -1;
    }

    if((size & (size -1)) !=0){
        return -1; 
    }

    a.base = memory_base; 
    a.used_space = 0; 
    a.total_size = size; 
    a.min_block_size = MIN_ALLOC_SIZE;
    a.max_block_size = size;

    a.tree = NULL; 
    a.alloc_level = NULL; 


    block_size = size; 
    level_count = 0;
    
    while(block_size >= MIN_ALLOC_SIZE){
        level_count++; 
        
        if(block_size == MIN_ALLOC_SIZE){
            break; 
        }

        block_size /=2;
    }

    if(block_size != MIN_ALLOC_SIZE){
        return -1; 
    }

    leaf_count = size / MIN_ALLOC_SIZE; 
    node_count = (1UL << level_count) - 1;

    a.level_count = level_count; 
    a.leaf_count = leaf_count; 
    a.node_count = node_count; 

    a.tree = malloc(node_count * sizeof(unsigned char)); 
    if(a.tree == NULL){
        free_structures();
        return -1; 
    }

    a.alloc_level = malloc(leaf_count * sizeof(int)); 
    if(a.alloc_level == NULL){
        free_structures(); 
        return -1;
    }

    memset(a.tree, NODE_FREE, node_count * sizeof(unsigned char));

    for(size_t i = 0; i< leaf_count; i++){
        a.alloc_level[i] = -1; 
    }

    return 0;
}

static void free_structures() {
    free(a.tree);
    free(a.alloc_level);

    a.tree = NULL;
    a.alloc_level = NULL;

    a.base = NULL;
    a.used_space = 0;
    a.total_size = 0;
    a.min_block_size = 0;
    a.max_block_size = 0;
    a.level_count = 0;
    a.leaf_count = 0;
    a.node_count = 0;
}



// ------------------- UTILS -------------------

static size_t next_power_of_two(size_t x){
    if(x == 0) return 1; 

    n--;
    n|= n >> 1;
    n |= n >>2; 
    n |= n >>4; 

}