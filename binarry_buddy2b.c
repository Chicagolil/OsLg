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
static size_t block_size_to_level(size_t block_size);


/*
* Tree navigation helpers
*/
static size_t left_child(size_t index); 
static size_t right_child(size_t index);
static size_t parent_index(size_t index);
static int is_leaf_level(size_t level); 
static void mark_children_free(size_t index); 
static long allocate_node(size_t node_index, size_t current_level, size_t target_level);
static unsigned char recompute_has_free(size_t index);
static void refresh_has_free_upward(size_t index);



/*
* bfree() helpers
*/
static int children_are_free(size_t index); 
static void try_merge_upward(size_t index);


enum { ALLOC_LEVEL_FREE = -1 };



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
    size_t required_block_size;
    size_t target_level; 
    long allocated_node; 
    size_t level; 
    size_t first_index; 
    size_t position_in_level; 
    size_t offset; 
    void *ptr; 
    size_t start_unit;

    required_block_size = get_required_block_size(size);

    if(required_block_size == 0){
        return NULL;
    }

    target_level = block_size_to_level(required_block_size); 
    
    allocated_node = allocate_node(0,0, target_level); 
    if(allocated_node == -1){ 
        return NULL; 
    }

    // récupérer l'addresse mémoire du noeud alloué
    level = target_level;
    first_index = first_index_at_level(level); 
    position_in_level = (size_t) allocated_node - first_index; 
    offset = position_in_level * level_block_size(level); 

    ptr = (void *)((char * )a.base + offset);

    // mise à jour de l'espace utilisé
    a.used_space += required_block_size; 

    start_unit = offset / a.min_block_size;
    a.alloc_level[start_unit] = (int)level;


    return ptr;
}

void bfree(void* ptr) {
    size_t offset; 
    size_t unit_index; 
    int level_int; 
    size_t level; 
    size_t block_size; 
    size_t position_in_level; 
    size_t node_index; 
    size_t first_index;

    if(ptr == NULL){
        return; 
    }

    if(a.base == NULL){
        return;
    }

    offset = (size_t)((char*)ptr - (char*)a.base); 

    if(offset >= a.total_size){
        return ; 
    }

    if(offset % a.min_block_size !=0){
        return;
    }

    unit_index = offset/a.min_block_size; 

    if(unit_index >= a.leaf_count){
        return; 
    }

    level_int = a.alloc_level[unit_index]; 
    if(level_int == ALLOC_LEVEL_FREE){
        return;
    }

    level = (size_t)level_int; 
    block_size= level_block_size(level);

    if(offset % block_size != 0){
        return ;
    }

    position_in_level = offset/block_size; 
    first_index = first_index_at_level(level); 
    node_index = first_index + position_in_level; 

    if(node_index >= a.node_count){
        return; 
    }

    if(a.tree[node_index]!= NODE_FULL ){
        return;
    }


    a.tree[node_index] = NODE_FREE;
    a.has_free[node_index] = 1;
    a.used_space -= block_size; 

    a.alloc_level[unit_index] = ALLOC_LEVEL_FREE;
    try_merge_upward(node_index);
    refresh_has_free_upward(node_index);
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

    a.tree = NULL; 
    a.has_free = NULL; 
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

    a.has_free = malloc(node_count * sizeof(unsigned char)); 
    if(a.has_free == NULL){
        free_structures();
        return -1;
    }

    a.alloc_level = malloc(leaf_count * sizeof(int)); 
    if(a.alloc_level == NULL){
        free_structures(); 
        return -1;
    }

    memset(a.tree, NODE_FREE, node_count * sizeof(unsigned char));
    memset(a.has_free, 1, node_count * sizeof(unsigned char));

    for(size_t i = 0; i< leaf_count; i++){
        a.alloc_level[i] = ALLOC_LEVEL_FREE;
    }

    return 0;
}

static void free_structures() {
    free(a.tree);
    free(a.has_free);
    free(a.alloc_level);

    a.tree = NULL;
    a.has_free = NULL;
    a.alloc_level = NULL;

    // a.base = NULL;           free_buddy() en a besoin 
    // a.total_size = 0;

    a.used_space = 0;
    a.min_block_size = 0;
    a.level_count = 0;
    a.leaf_count = 0;
    a.node_count = 0;
}



// ------------------- UTILS -------------------

static size_t next_power_of_two(size_t n){
    if(n == 0) return 1; 

    n--;
    n|= n >> 1;
    n |= n >>2; 
    n |= n >>4; 
    n |= n>> 8; 
    n |= n >> 16; 
    if(sizeof(size_t) == 8){
        n |= n >> 32;
    }
    return n+1;

}

static size_t get_required_block_size(size_t requested_size){
    size_t block_size; 

    if(requested_size == 0){
        return 0;
    }

    if(requested_size > a.total_size){
        return 0;
    }


    if(requested_size <= a.min_block_size){
        return a.min_block_size;
    }

    block_size = next_power_of_two(requested_size);

    if(block_size > a.total_size){
        return 0;
    }

    return block_size;
}

static size_t level_block_size(size_t level){
    return a.total_size >> level;
}

static size_t first_index_at_level(size_t level){
    return ((size_t)1 << level) -1;
}


static size_t block_size_to_level(size_t block_size){
    size_t level = 0; 
    size_t current_size = a.total_size;

    while(current_size > block_size){
        current_size /=2;
        level++;
    }
    return level;
}


// ------------------- TREE NAVIGATION HELPERS -------------------

static size_t left_child(size_t index){
    return (2 * index) + 1;
}

static size_t right_child(size_t index){
    return (2 * index) + 2;
}

static size_t parent_index(size_t index){ 
    return (index - 1)/2; 
}

static int is_leaf_level(size_t level){
    return level == (a.level_count - 1 );
}

static void mark_children_free(size_t index){
    size_t left = left_child(index); 
    size_t right = right_child(index); 

    if(left < a.node_count){
        a.tree[left] = NODE_FREE; 
        a.has_free[left] = 1; 
    }

    if(right < a.node_count){
        a.tree[right] = NODE_FREE; 
        a.has_free[right] = 1; 
    }
}


static long allocate_node(size_t node_index, size_t current_level, size_t target_level){
    size_t left; 
    size_t right; 
    long result; 

    if(node_index >= a.node_count){     // est ce que le noeud existe ? 
        return -1; 
    }

    if(a.has_free[node_index] == 0){
        return -1; 
    }

    if(a.tree[node_index] == NODE_FULL){   // est ce que le noeud est plein ? 
        return -1; 
    } 

    if(current_level == target_level){    // est ce qu'on est arrivé au niveau voulu ?
        if(a.tree[node_index] == NODE_FREE){
            a.tree[node_index] = NODE_FULL;
            a.has_free[node_index] = 0;
            refresh_has_free_upward(node_index);
            return (long)node_index;
        }
        return -1; 
    }

    if(is_leaf_level(current_level)){   // est ce qu'on est au bout de l'arbre - pas possible d'aller plus bas 
        return -1; 
    }

    if(a.tree[node_index] == NODE_FREE){ // si le noeud courant est libre, et qu'on veut aller plus bas, il faut le découper
        a.tree[node_index]= NODE_SPLIT; 
        mark_children_free(node_index);
    }

    if(a.tree[node_index] != NODE_SPLIT){
        return -1; 
    }

    left = left_child(node_index);
    right = right_child(node_index);

    // Question 2b - inversion des bloc conditionels pour changer l'ordre des appels récursifs

    if(right < a.node_count && a.has_free[right]){
        result = allocate_node(right, current_level + 1, target_level);
        if(result != -1){
            return result;
        }
    }

    if(left < a.node_count && a.has_free[left]){
        result = allocate_node(left,current_level +1 , target_level);
        if(result !=-1){
            return result;
        }
    }


    a.has_free[node_index] = recompute_has_free(node_index);
    refresh_has_free_upward(node_index); 
    return -1;
}    

static unsigned char recompute_has_free(size_t index){
    size_t left;
    size_t right; 

    if(index >= a.node_count){
        return 0;
    }

    if(a.tree[index] == NODE_FREE){ 
        return 1;
    }
    
    if(a.tree[index] == NODE_FULL){
        return 0;
    }

    left = left_child(index);
    right = right_child(index);

    return (unsigned char)(
        (left < a.node_count && a.has_free[left] != 0) || 
        (right < a.node_count && a.has_free[right] != 0)
    );
}

static void refresh_has_free_upward(size_t index){
    size_t current = index; 

    while(1){ 
        a.has_free[current] = recompute_has_free(current); 
        if (current==0){
            break;
        }
        current = parent_index(current);
    }
}


// ------------------- BFREE HELPERS -------------------


static int children_are_free(size_t index){
    size_t left = left_child(index); 
    size_t right = right_child(index);

    if(left >= a.node_count || right >= a.node_count){
        return 0;
    }


    return a.tree[left] == NODE_FREE && a.tree[right] == NODE_FREE;
}

static void try_merge_upward(size_t index){ 
    size_t current = index;

    while(current != 0){
        size_t parent = parent_index(current);

        if(!children_are_free(parent)){
            a.tree[parent] = NODE_SPLIT; 
            a.has_free[parent] = recompute_has_free(parent);
            return;
        }
        a.tree[parent] = NODE_FREE; 
        a.has_free[parent] = 1; 
        current = parent;
    }
}