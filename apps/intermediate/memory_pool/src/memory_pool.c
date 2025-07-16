/**
 * @file memory_pool.c
 * @brief Memory pool allocator demonstrating advanced memory management
 * @author dunamismax
 * @date 2025
 * 
 * This program demonstrates:
 * - Custom memory allocator implementation
 * - Memory pool design patterns
 * - Block allocation and deallocation
 * - Memory fragmentation handling
 * - Performance optimization techniques
 * - Memory alignment considerations
 * - Debugging and leak detection
 * - Memory usage statistics
 * - Arena-based allocation
 * - Object pool management
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>

// Include our utility libraries
#include "utils.h"
#include "dynamic_array.h"

/**
 * @brief Default memory alignment (8 bytes for 64-bit systems)
 */
#define DEFAULT_ALIGNMENT 8

/**
 * @brief Minimum block size
 */
#define MIN_BLOCK_SIZE 16

/**
 * @brief Memory pool signature for corruption detection
 */
#define POOL_SIGNATURE 0xDEADBEEF

/**
 * @brief Block header signature
 */
#define BLOCK_SIGNATURE 0xCAFEBABE

/**
 * @brief Maximum number of free lists for different sizes
 */
#define MAX_FREE_LISTS 32

/**
 * @brief Memory block header
 * 
 * Demonstrates: Block metadata management,
 * memory tracking, corruption detection
 */
typedef struct BlockHeader {
    uint32_t signature;        // Corruption detection
    size_t size;              // Block size (excluding header)
    bool is_free;             // Free/allocated status
    struct BlockHeader* next; // Next block in free list
    struct BlockHeader* prev; // Previous block in memory
    uint32_t magic;           // End corruption detection
} BlockHeader;

/**
 * @brief Free list entry for different block sizes
 * 
 * Demonstrates: Size-based free list management,
 * efficient allocation strategies
 */
typedef struct FreeList {
    size_t block_size;
    BlockHeader* head;
    size_t count;
} FreeList;

/**
 * @brief Memory pool structure
 * 
 * Demonstrates: Pool management, statistics tracking,
 * allocation strategy configuration
 */
typedef struct {
    uint32_t signature;           // Pool corruption detection
    void* memory;                 // Base memory address
    size_t total_size;           // Total pool size
    size_t used_size;            // Currently allocated size
    size_t peak_usage;           // Peak memory usage
    size_t alignment;            // Memory alignment requirement
    size_t num_allocations;      // Total allocation count
    size_t num_deallocations;    // Total deallocation count
    size_t num_blocks;           // Current number of blocks
    size_t fragmentation_count;  // Fragmentation events
    FreeList free_lists[MAX_FREE_LISTS]; // Size-based free lists
    BlockHeader* first_block;    // First block in pool
    bool enable_debugging;       // Debug mode flag
    char name[64];              // Pool name for debugging
} MemoryPool;

/**
 * @brief Memory pool statistics
 * 
 * Demonstrates: Performance monitoring, usage analysis
 */
typedef struct {
    size_t total_allocated;
    size_t total_freed;
    size_t current_usage;
    size_t peak_usage;
    size_t fragmentation_ratio;
    size_t allocation_count;
    size_t deallocation_count;
    size_t free_blocks;
    size_t largest_free_block;
} PoolStats;

/**
 * @brief Align size to boundary
 * @param size Size to align
 * @param alignment Alignment boundary
 * @return Aligned size
 * 
 * Demonstrates: Memory alignment calculations,
 * platform considerations
 */
size_t align_size(size_t size, size_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

/**
 * @brief Get free list index for block size
 * @param size Block size
 * @return Free list index
 * 
 * Demonstrates: Size-based indexing strategy,
 * allocation optimization
 */
size_t get_free_list_index(size_t size) {
    if (size <= 16) return 0;
    if (size <= 32) return 1;
    if (size <= 64) return 2;
    if (size <= 128) return 3;
    if (size <= 256) return 4;
    if (size <= 512) return 5;
    if (size <= 1024) return 6;
    if (size <= 2048) return 7;
    if (size <= 4096) return 8;
    
    // For larger sizes, use log-based indexing
    size_t index = 9;
    size_t threshold = 8192;
    
    while (index < MAX_FREE_LISTS - 1 && size > threshold) {
        threshold *= 2;
        index++;
    }
    
    return index;
}

/**
 * @brief Initialize memory pool
 * @param pool Pointer to pool structure
 * @param size Total pool size
 * @param alignment Memory alignment requirement
 * @param name Pool name for debugging
 * @return true if initialization was successful
 * 
 * Demonstrates: Pool initialization, memory mapping,
 * data structure setup
 */
bool memory_pool_init(MemoryPool* pool, size_t size, size_t alignment, const char* name) {
    if (!pool || size == 0) return false;
    
    // Align pool size
    size = align_size(size, getpagesize());
    
    // Allocate memory using mmap for better control
    void* memory = mmap(NULL, size, PROT_READ | PROT_WRITE, 
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    
    if (memory == MAP_FAILED) {
        log_message("ERROR", "Failed to allocate memory pool: %s", strerror(errno));
        return false;
    }
    
    // Initialize pool structure
    memset(pool, 0, sizeof(MemoryPool));
    pool->signature = POOL_SIGNATURE;
    pool->memory = memory;
    pool->total_size = size;
    pool->alignment = alignment > 0 ? alignment : DEFAULT_ALIGNMENT;
    pool->enable_debugging = false;
    
    if (name) {
        strncpy(pool->name, name, sizeof(pool->name) - 1);
        pool->name[sizeof(pool->name) - 1] = '\0';
    } else {
        strcpy(pool->name, "unnamed");
    }
    
    // Initialize free lists
    for (size_t i = 0; i < MAX_FREE_LISTS; i++) {
        pool->free_lists[i].block_size = 0;
        pool->free_lists[i].head = NULL;
        pool->free_lists[i].count = 0;
    }
    
    // Create initial free block
    BlockHeader* initial_block = (BlockHeader*)memory;
    initial_block->signature = BLOCK_SIGNATURE;
    initial_block->size = size - sizeof(BlockHeader);
    initial_block->is_free = true;
    initial_block->next = NULL;
    initial_block->prev = NULL;
    initial_block->magic = BLOCK_SIGNATURE;
    
    pool->first_block = initial_block;
    pool->num_blocks = 1;
    
    // Add to appropriate free list
    size_t list_index = get_free_list_index(initial_block->size);
    pool->free_lists[list_index].head = initial_block;
    pool->free_lists[list_index].count = 1;
    
    log_message("INFO", "Memory pool '%s' initialized: %zu bytes", pool->name, size);
    return true;
}

/**
 * @brief Validate block header integrity
 * @param block Pointer to block header
 * @return true if block is valid
 * 
 * Demonstrates: Memory corruption detection,
 * defensive programming
 */
bool validate_block(BlockHeader* block) {
    if (!block) return false;
    
    return (block->signature == BLOCK_SIGNATURE && 
            block->magic == BLOCK_SIGNATURE &&
            block->size > 0);
}

/**
 * @brief Split a large block into smaller blocks
 * @param pool Pointer to memory pool
 * @param block Block to split
 * @param requested_size Size for first block
 * @return Pointer to second block, or NULL if no split occurred
 * 
 * Demonstrates: Block splitting strategy,
 * memory fragmentation management
 */
BlockHeader* split_block(MemoryPool* pool, BlockHeader* block, size_t requested_size) {
    if (!pool || !block || !validate_block(block)) return NULL;
    
    size_t aligned_size = align_size(requested_size, pool->alignment);
    size_t remaining_size = block->size - aligned_size;
    
    // Only split if remaining size is large enough for a new block
    if (remaining_size < sizeof(BlockHeader) + MIN_BLOCK_SIZE) {
        return NULL;
    }
    
    // Create new block for remaining space
    BlockHeader* new_block = (BlockHeader*)((uint8_t*)block + sizeof(BlockHeader) + aligned_size);
    new_block->signature = BLOCK_SIGNATURE;
    new_block->size = remaining_size - sizeof(BlockHeader);
    new_block->is_free = true;
    new_block->next = NULL;
    new_block->prev = block;
    new_block->magic = BLOCK_SIGNATURE;
    
    // Update original block
    block->size = aligned_size;
    
    // Update block linking
    if (block->next) {
        block->next->prev = new_block;
        new_block->next = block->next;
    }
    block->next = new_block;
    
    pool->num_blocks++;
    
    if (pool->enable_debugging) {
        log_message("DEBUG", "Split block: %zu -> %zu + %zu", 
                   aligned_size + new_block->size, aligned_size, new_block->size);
    }
    
    return new_block;
}

/**
 * @brief Coalesce adjacent free blocks
 * @param pool Pointer to memory pool
 * @param block Starting block for coalescing
 * @return Pointer to coalesced block
 * 
 * Demonstrates: Memory defragmentation,
 * adjacent block merging
 */
BlockHeader* coalesce_blocks(MemoryPool* pool, BlockHeader* block) {
    if (!pool || !block || !validate_block(block) || !block->is_free) {
        return block;
    }
    
    // Coalesce with next block
    while (block->next && block->next->is_free && validate_block(block->next)) {
        BlockHeader* next_block = block->next;
        
        // Check if blocks are adjacent
        uint8_t* block_end = (uint8_t*)block + sizeof(BlockHeader) + block->size;
        if (block_end == (uint8_t*)next_block) {
            // Merge blocks
            block->size += sizeof(BlockHeader) + next_block->size;
            block->next = next_block->next;
            
            if (next_block->next) {
                next_block->next->prev = block;
            }
            
            pool->num_blocks--;
            
            if (pool->enable_debugging) {
                log_message("DEBUG", "Coalesced blocks: new size %zu", block->size);
            }
        } else {
            break;
        }
    }
    
    // Coalesce with previous block
    while (block->prev && block->prev->is_free && validate_block(block->prev)) {
        BlockHeader* prev_block = block->prev;
        
        // Check if blocks are adjacent
        uint8_t* prev_end = (uint8_t*)prev_block + sizeof(BlockHeader) + prev_block->size;
        if (prev_end == (uint8_t*)block) {
            // Merge blocks
            prev_block->size += sizeof(BlockHeader) + block->size;
            prev_block->next = block->next;
            
            if (block->next) {
                block->next->prev = prev_block;
            }
            
            pool->num_blocks--;
            block = prev_block;
            
            if (pool->enable_debugging) {
                log_message("DEBUG", "Coalesced with previous: new size %zu", block->size);
            }
        } else {
            break;
        }
    }
    
    return block;
}

/**
 * @brief Remove block from free list
 * @param pool Pointer to memory pool
 * @param block Block to remove
 * @param list_index Free list index
 * 
 * Demonstrates: Free list management,
 * linked list operations
 */
void remove_from_free_list(MemoryPool* pool, BlockHeader* block, size_t list_index) {
    if (!pool || !block || list_index >= MAX_FREE_LISTS) return;
    
    FreeList* list = &pool->free_lists[list_index];
    
    if (list->head == block) {
        list->head = block->next;
    }
    
    if (block->next) {
        block->next->prev = block->prev;
    }
    
    if (block->prev) {
        block->prev->next = block->next;
    }
    
    list->count--;
    block->next = NULL;
    block->prev = NULL;
}

/**
 * @brief Add block to free list
 * @param pool Pointer to memory pool
 * @param block Block to add
 * 
 * Demonstrates: Free list insertion,
 * size-based organization
 */
void add_to_free_list(MemoryPool* pool, BlockHeader* block) {
    if (!pool || !block || !validate_block(block)) return;
    
    size_t list_index = get_free_list_index(block->size);
    FreeList* list = &pool->free_lists[list_index];
    
    // Insert at head of free list
    block->next = list->head;
    block->prev = NULL;
    
    if (list->head) {
        list->head->prev = block;
    }
    
    list->head = block;
    list->count++;
}

/**
 * @brief Allocate memory from pool
 * @param pool Pointer to memory pool
 * @param size Size to allocate
 * @return Pointer to allocated memory, or NULL on failure
 * 
 * Demonstrates: Memory allocation algorithm,
 * best-fit strategy, block management
 */
void* memory_pool_alloc(MemoryPool* pool, size_t size) {
    if (!pool || pool->signature != POOL_SIGNATURE || size == 0) {
        return NULL;
    }
    
    size_t aligned_size = align_size(size, pool->alignment);
    size_t total_size = aligned_size + sizeof(BlockHeader);
    
    // Check if request is too large
    if (total_size > pool->total_size - pool->used_size) {
        log_message("ERROR", "Pool '%s': Allocation too large: %zu bytes", 
                   pool->name, size);
        return NULL;
    }
    
    BlockHeader* best_block = NULL;
    size_t best_list_index = 0;
    
    // Search free lists for suitable block
    for (size_t i = get_free_list_index(aligned_size); i < MAX_FREE_LISTS; i++) {
        FreeList* list = &pool->free_lists[i];
        
        for (BlockHeader* block = list->head; block; block = block->next) {
            if (validate_block(block) && block->is_free && block->size >= aligned_size) {
                if (!best_block || block->size < best_block->size) {
                    best_block = block;
                    best_list_index = i;
                    
                    // Perfect fit found
                    if (block->size == aligned_size) {
                        break;
                    }
                }
            }
        }
        
        if (best_block && best_block->size == aligned_size) {
            break;
        }
    }
    
    if (!best_block) {
        log_message("ERROR", "Pool '%s': No suitable block found for %zu bytes", 
                   pool->name, size);
        return NULL;
    }
    
    // Remove from free list
    remove_from_free_list(pool, best_block, best_list_index);
    
    // Split block if necessary
    BlockHeader* split_result = split_block(pool, best_block, aligned_size);
    if (split_result) {
        add_to_free_list(pool, split_result);
    }
    
    // Mark block as allocated
    best_block->is_free = false;
    
    // Update pool statistics
    pool->used_size += best_block->size;
    pool->num_allocations++;
    
    if (pool->used_size > pool->peak_usage) {
        pool->peak_usage = pool->used_size;
    }
    
    if (pool->enable_debugging) {
        log_message("DEBUG", "Pool '%s': Allocated %zu bytes (requested %zu)", 
                   pool->name, best_block->size, size);
    }
    
    // Return pointer to user data (after header)
    return (uint8_t*)best_block + sizeof(BlockHeader);
}

/**
 * @brief Free memory back to pool
 * @param pool Pointer to memory pool
 * @param ptr Pointer to memory to free
 * 
 * Demonstrates: Memory deallocation, block coalescing,
 * double-free protection
 */
void memory_pool_free(MemoryPool* pool, void* ptr) {
    if (!pool || pool->signature != POOL_SIGNATURE || !ptr) {
        return;
    }
    
    // Get block header
    BlockHeader* block = (BlockHeader*)((uint8_t*)ptr - sizeof(BlockHeader));
    
    if (!validate_block(block)) {
        log_message("ERROR", "Pool '%s': Invalid block header for free", pool->name);
        return;
    }
    
    if (block->is_free) {
        log_message("ERROR", "Pool '%s': Double free detected", pool->name);
        return;
    }
    
    // Update statistics
    pool->used_size -= block->size;
    pool->num_deallocations++;
    
    // Mark as free
    block->is_free = true;
    
    if (pool->enable_debugging) {
        log_message("DEBUG", "Pool '%s': Freed %zu bytes", pool->name, block->size);
    }
    
    // Coalesce with adjacent free blocks
    block = coalesce_blocks(pool, block);
    
    // Add to free list
    add_to_free_list(pool, block);
}

/**
 * @brief Get memory pool statistics
 * @param pool Pointer to memory pool
 * @param stats Pointer to store statistics
 * @return true if statistics were retrieved successfully
 * 
 * Demonstrates: Statistics collection, performance monitoring
 */
bool memory_pool_get_stats(MemoryPool* pool, PoolStats* stats) {
    if (!pool || !stats || pool->signature != POOL_SIGNATURE) {
        return false;
    }
    
    memset(stats, 0, sizeof(PoolStats));
    
    stats->total_allocated = pool->num_allocations;
    stats->total_freed = pool->num_deallocations;
    stats->current_usage = pool->used_size;
    stats->peak_usage = pool->peak_usage;
    stats->allocation_count = pool->num_allocations;
    stats->deallocation_count = pool->num_deallocations;
    
    // Calculate fragmentation ratio
    if (pool->total_size > 0) {
        stats->fragmentation_ratio = (pool->num_blocks * 100) / 
                                   (pool->total_size / sizeof(BlockHeader));
    }
    
    // Count free blocks and find largest
    for (size_t i = 0; i < MAX_FREE_LISTS; i++) {
        stats->free_blocks += pool->free_lists[i].count;
        
        for (BlockHeader* block = pool->free_lists[i].head; block; block = block->next) {
            if (block->size > stats->largest_free_block) {
                stats->largest_free_block = block->size;
            }
        }
    }
    
    return true;
}

/**
 * @brief Display pool statistics
 * @param pool Pointer to memory pool
 * 
 * Demonstrates: Statistics presentation, performance analysis
 */
void display_pool_stats(MemoryPool* pool) {
    if (!pool || pool->signature != POOL_SIGNATURE) return;
    
    PoolStats stats;
    if (!memory_pool_get_stats(pool, &stats)) return;
    
    printf("\n=== Memory Pool Statistics: %s ===\n", pool->name);
    printf("Total size: %zu bytes\n", pool->total_size);
    printf("Used size: %zu bytes (%.1f%%)\n", 
           pool->used_size, (double)pool->used_size * 100.0 / pool->total_size);
    printf("Peak usage: %zu bytes (%.1f%%)\n", 
           pool->peak_usage, (double)pool->peak_usage * 100.0 / pool->total_size);
    printf("Free size: %zu bytes\n", pool->total_size - pool->used_size);
    printf("Total blocks: %zu\n", pool->num_blocks);
    printf("Free blocks: %zu\n", stats.free_blocks);
    printf("Largest free: %zu bytes\n", stats.largest_free_block);
    printf("Allocations: %zu\n", pool->num_allocations);
    printf("Deallocations: %zu\n", pool->num_deallocations);
    printf("Active allocations: %zu\n", pool->num_allocations - pool->num_deallocations);
    printf("Fragmentation ratio: %zu%%\n", stats.fragmentation_ratio);
    printf("Alignment: %zu bytes\n", pool->alignment);
    printf("Debug mode: %s\n", pool->enable_debugging ? "enabled" : "disabled");
    printf("==============================\n");
}

/**
 * @brief Dump pool memory layout for debugging
 * @param pool Pointer to memory pool
 * 
 * Demonstrates: Memory layout visualization,
 * debugging techniques
 */
void dump_pool_layout(MemoryPool* pool) {
    if (!pool || pool->signature != POOL_SIGNATURE) return;
    
    printf("\n=== Memory Pool Layout: %s ===\n", pool->name);
    
    BlockHeader* current = pool->first_block;
    size_t block_num = 0;
    
    while (current && validate_block(current)) {
        printf("Block %zu: %s, Size: %zu bytes, Address: %p\n",
               block_num++,
               current->is_free ? "FREE" : "USED",
               current->size,
               (void*)current);
        
        current = current->next;
        
        // Safety check to prevent infinite loops
        if (block_num > 1000) {
            printf("... (truncated after 1000 blocks)\n");
            break;
        }
    }
    
    printf("============================\n");
}

/**
 * @brief Validate pool integrity
 * @param pool Pointer to memory pool
 * @return true if pool is valid
 * 
 * Demonstrates: Data structure validation,
 * corruption detection
 */
bool validate_pool(MemoryPool* pool) {
    if (!pool || pool->signature != POOL_SIGNATURE) {
        log_message("ERROR", "Invalid pool signature");
        return false;
    }
    
    if (!pool->memory || pool->total_size == 0) {
        log_message("ERROR", "Invalid pool memory configuration");
        return false;
    }
    
    // Validate all blocks
    BlockHeader* current = pool->first_block;
    size_t total_blocks = 0;
    size_t calculated_used = 0;
    
    while (current) {
        if (!validate_block(current)) {
            log_message("ERROR", "Invalid block header at %p", (void*)current);
            return false;
        }
        
        if (!current->is_free) {
            calculated_used += current->size;
        }
        
        total_blocks++;
        current = current->next;
        
        // Safety check
        if (total_blocks > 10000) {
            log_message("ERROR", "Too many blocks detected (possible corruption)");
            return false;
        }
    }
    
    if (calculated_used != pool->used_size) {
        log_message("ERROR", "Used size mismatch: calculated %zu, tracked %zu",
                   calculated_used, pool->used_size);
        return false;
    }
    
    return true;
}

/**
 * @brief Destroy memory pool
 * @param pool Pointer to memory pool
 * 
 * Demonstrates: Resource cleanup, memory unmapping
 */
void memory_pool_destroy(MemoryPool* pool) {
    if (!pool || pool->signature != POOL_SIGNATURE) return;
    
    log_message("INFO", "Destroying pool '%s'", pool->name);
    
    if (pool->num_allocations != pool->num_deallocations) {
        log_message("WARN", "Pool '%s': Memory leak detected - %zu allocations, %zu deallocations",
                   pool->name, pool->num_allocations, pool->num_deallocations);
    }
    
    // Unmap memory
    if (pool->memory && munmap(pool->memory, pool->total_size) != 0) {
        log_message("ERROR", "Failed to unmap pool memory: %s", strerror(errno));
    }
    
    // Clear pool structure
    memset(pool, 0, sizeof(MemoryPool));
}

/**
 * @brief Test memory pool functionality
 * @param pool Pointer to memory pool
 * 
 * Demonstrates: Testing strategies, validation procedures
 */
void test_memory_pool(MemoryPool* pool) {
    if (!pool) return;
    
    printf("\n=== Testing Memory Pool ===\n");
    
    const size_t test_sizes[] = {16, 32, 64, 128, 256, 512, 1024, 2048};
    const size_t num_tests = sizeof(test_sizes) / sizeof(test_sizes[0]);
    void* ptrs[num_tests];
    
    // Test allocation
    printf("Testing allocation...\n");
    for (size_t i = 0; i < num_tests; i++) {
        ptrs[i] = memory_pool_alloc(pool, test_sizes[i]);
        if (ptrs[i]) {
            printf("  Allocated %zu bytes at %p\n", test_sizes[i], ptrs[i]);
            // Write test pattern
            memset(ptrs[i], (int)(i + 1), test_sizes[i]);
        } else {
            printf("  Failed to allocate %zu bytes\n", test_sizes[i]);
        }
    }
    
    display_pool_stats(pool);
    
    // Test data integrity
    printf("\nTesting data integrity...\n");
    for (size_t i = 0; i < num_tests; i++) {
        if (ptrs[i]) {
            uint8_t* data = (uint8_t*)ptrs[i];
            bool valid = true;
            
            for (size_t j = 0; j < test_sizes[i]; j++) {
                if (data[j] != (uint8_t)(i + 1)) {
                    valid = false;
                    break;
                }
            }
            
            printf("  Block %zu: %s\n", i, valid ? "valid" : "corrupted");
        }
    }
    
    // Test deallocation
    printf("\nTesting deallocation...\n");
    for (size_t i = 0; i < num_tests; i += 2) {
        if (ptrs[i]) {
            memory_pool_free(pool, ptrs[i]);
            printf("  Freed block %zu\n", i);
            ptrs[i] = NULL;
        }
    }
    
    display_pool_stats(pool);
    
    // Test reallocation
    printf("\nTesting reallocation...\n");
    for (size_t i = 0; i < num_tests; i += 2) {
        ptrs[i] = memory_pool_alloc(pool, test_sizes[i] * 2);
        if (ptrs[i]) {
            printf("  Reallocated %zu bytes at %p\n", test_sizes[i] * 2, ptrs[i]);
        }
    }
    
    display_pool_stats(pool);
    
    // Clean up remaining allocations
    printf("\nCleaning up...\n");
    for (size_t i = 0; i < num_tests; i++) {
        if (ptrs[i]) {
            memory_pool_free(pool, ptrs[i]);
        }
    }
    
    display_pool_stats(pool);
    printf("=========================\n");
}

/**
 * @brief Interactive memory pool interface
 * @param pool Pointer to memory pool
 * 
 * Demonstrates: Interactive testing, command processing
 */
void run_interactive_mode(MemoryPool* pool) {
    if (!pool) return;
    
    printf("\n=== Interactive Memory Pool ===\n");
    printf("Commands:\n");
    printf("  alloc <size>    - Allocate memory\n");
    printf("  free <address>  - Free memory\n");
    printf("  stats           - Show pool statistics\n");
    printf("  layout          - Dump memory layout\n");
    printf("  validate        - Validate pool integrity\n");
    printf("  debug           - Toggle debug mode\n");
    printf("  test            - Run automated tests\n");
    printf("  help            - Show this help\n");
    printf("  quit            - Exit\n");
    printf("===============================\n");
    
    char command[256];
    DynamicArray* allocations = darray_create(sizeof(void*), 16);
    
    while (true) {
        printf("\npool> ");
        fflush(stdout);
        
        if (!fgets(command, sizeof(command), stdin)) {
            break;
        }
        
        // Remove trailing newline
        char* newline = strchr(command, '\n');
        if (newline) *newline = '\0';
        
        // Parse command
        char* cmd = strtok(command, " \t");
        if (!cmd) continue;
        
        if (strcmp(cmd, "alloc") == 0) {
            char* size_str = strtok(NULL, " \t");
            if (!size_str) {
                printf("Usage: alloc <size>\n");
                continue;
            }
            
            int size;
            if (!str_to_int(size_str, &size) || size <= 0) {
                printf("Error: Invalid size\n");
                continue;
            }
            
            void* ptr = memory_pool_alloc(pool, size);
            if (ptr) {
                printf("Allocated %d bytes at %p\n", size, ptr);
                darray_push(allocations, &ptr);
            } else {
                printf("Error: Allocation failed\n");
            }
            
        } else if (strcmp(cmd, "free") == 0) {
            char* addr_str = strtok(NULL, " \t");
            if (!addr_str) {
                printf("Usage: free <address>\n");
                continue;
            }
            
            void* ptr = (void*)strtoul(addr_str, NULL, 16);
            if (ptr) {
                memory_pool_free(pool, ptr);
                printf("Freed memory at %p\n", ptr);
            } else {
                printf("Error: Invalid address\n");
            }
            
        } else if (strcmp(cmd, "stats") == 0) {
            display_pool_stats(pool);
            
        } else if (strcmp(cmd, "layout") == 0) {
            dump_pool_layout(pool);
            
        } else if (strcmp(cmd, "validate") == 0) {
            if (validate_pool(pool)) {
                printf("Pool validation: PASSED\n");
            } else {
                printf("Pool validation: FAILED\n");
            }
            
        } else if (strcmp(cmd, "debug") == 0) {
            pool->enable_debugging = !pool->enable_debugging;
            printf("Debug mode: %s\n", pool->enable_debugging ? "enabled" : "disabled");
            
        } else if (strcmp(cmd, "test") == 0) {
            test_memory_pool(pool);
            
        } else if (strcmp(cmd, "help") == 0) {
            printf("\nCommands:\n");
            printf("  alloc <size>    - Allocate memory\n");
            printf("  free <address>  - Free memory\n");
            printf("  stats           - Show pool statistics\n");
            printf("  layout          - Dump memory layout\n");
            printf("  validate        - Validate pool integrity\n");
            printf("  debug           - Toggle debug mode\n");
            printf("  test            - Run automated tests\n");
            printf("  help            - Show this help\n");
            printf("  quit            - Exit\n");
            
        } else if (strcmp(cmd, "quit") == 0 || strcmp(cmd, "exit") == 0) {
            break;
            
        } else {
            printf("Unknown command: %s\n", cmd);
            printf("Type 'help' for available commands\n");
        }
    }
    
    // Clean up allocations
    if (allocations) {
        darray_destroy(allocations);
    }
}

/**
 * @brief Display help information
 * @param program_name Program name from argv[0]
 */
void display_help(const char* program_name) {
    printf("Memory Pool Allocator - Advanced Memory Management\n");
    printf("Usage: %s [options]\n\n", program_name);
    printf("Options:\n");
    printf("  -s, --size <bytes>      Pool size (default: 1MB)\n");
    printf("  -a, --alignment <bytes> Memory alignment (default: 8)\n");
    printf("  -n, --name <name>       Pool name (default: 'default')\n");
    printf("  -i, --interactive       Run in interactive mode\n");
    printf("  -t, --test              Run automated tests\n");
    printf("  -d, --debug             Enable debug output\n");
    printf("  --help                  Show this help\n\n");
    printf("Features demonstrated:\n");
    printf("- Custom memory allocator implementation\n");
    printf("- Block allocation and coalescing\n");
    printf("- Memory alignment and fragmentation handling\n");
    printf("- Performance monitoring and statistics\n");
    printf("- Memory corruption detection\n");
    printf("- Interactive memory management interface\n");
}

/**
 * @brief Main function
 * @param argc Number of command line arguments
 * @param argv Array of command line arguments
 * @return Exit status
 * 
 * Demonstrates: Command line processing, pool management,
 * testing procedures
 */
int main(int argc, char* argv[]) {
    size_t pool_size = 1024 * 1024; // 1MB default
    size_t alignment = DEFAULT_ALIGNMENT;
    char pool_name[64] = "default";
    bool interactive_mode = false;
    bool run_tests = false;
    bool debug_mode = false;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            display_help(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--size") == 0) {
            if (++i >= argc) {
                printf("Error: Size value required\n");
                return 1;
            }
            int size;
            if (!str_to_int(argv[i], &size) || size <= 0) {
                printf("Error: Invalid pool size\n");
                return 1;
            }
            pool_size = size;
        } else if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--alignment") == 0) {
            if (++i >= argc) {
                printf("Error: Alignment value required\n");
                return 1;
            }
            int align;
            if (!str_to_int(argv[i], &align) || align <= 0 || (align & (align - 1)) != 0) {
                printf("Error: Invalid alignment (must be power of 2)\n");
                return 1;
            }
            alignment = align;
        } else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--name") == 0) {
            if (++i >= argc) {
                printf("Error: Pool name required\n");
                return 1;
            }
            strncpy(pool_name, argv[i], sizeof(pool_name) - 1);
            pool_name[sizeof(pool_name) - 1] = '\0';
        } else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--interactive") == 0) {
            interactive_mode = true;
        } else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--test") == 0) {
            run_tests = true;
        } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--debug") == 0) {
            debug_mode = true;
        } else {
            printf("Error: Unknown option: %s\n", argv[i]);
            display_help(argv[0]);
            return 1;
        }
    }
    
    // Initialize memory pool
    MemoryPool pool;
    if (!memory_pool_init(&pool, pool_size, alignment, pool_name)) {
        printf("Error: Failed to initialize memory pool\n");
        return 1;
    }
    
    pool.enable_debugging = debug_mode;
    
    printf("Memory pool initialized: %s (%zu bytes, %zu-byte alignment)\n",
           pool_name, pool_size, alignment);
    
    // Run requested mode
    if (interactive_mode) {
        run_interactive_mode(&pool);
    } else if (run_tests) {
        test_memory_pool(&pool);
    } else {
        // Default demonstration
        printf("\nRunning basic demonstration...\n");
        display_pool_stats(&pool);
        test_memory_pool(&pool);
    }
    
    // Final validation and cleanup
    if (!validate_pool(&pool)) {
        printf("Error: Pool validation failed\n");
    }
    
    memory_pool_destroy(&pool);
    
    log_message("INFO", "Memory pool application terminated");
    return 0;
}

/**
 * Educational Notes:
 * 
 * 1. Memory Management:
 *    - Custom allocator implementation
 *    - Block allocation and deallocation strategies
 *    - Memory alignment and padding
 * 
 * 2. Data Structures:
 *    - Linked list management for free blocks
 *    - Size-based free list organization
 *    - Block header design and validation
 * 
 * 3. Algorithm Implementation:
 *    - Best-fit allocation strategy
 *    - Block splitting and coalescing
 *    - Fragmentation reduction techniques
 * 
 * 4. Performance Optimization:
 *    - Fast allocation for common sizes
 *    - Memory access pattern optimization
 *    - Statistical monitoring
 * 
 * 5. Error Handling:
 *    - Corruption detection and validation
 *    - Double-free protection
 *    - Bounds checking and safety
 * 
 * 6. System Programming:
 *    - Memory mapping with mmap
 *    - Page-aligned allocation
 *    - Platform-specific optimizations
 * 
 * 7. Debugging Techniques:
 *    - Memory layout visualization
 *    - Statistics collection and analysis
 *    - Integrity validation procedures
 * 
 * 8. Testing Strategies:
 *    - Automated test suites
 *    - Interactive testing interfaces
 *    - Stress testing and validation
 */