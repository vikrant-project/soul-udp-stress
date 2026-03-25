#include "memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int memory_init(size_t total_size) {
    (void)total_size;
    return 0;
}

void memory_cleanup(void) {
}

void *memory_alloc_aligned(size_t size) {
    void *ptr = NULL;
    int ret = posix_memalign(&ptr, CACHE_LINE_SIZE, size);
    if (ret != 0) {
        errno = ret;
        perror("posix_memalign failed");
        return NULL;
    }
    return ptr;
}

void memory_free_aligned(void *ptr) {
    free(ptr);
}

typedef struct memory_pool_item {
    struct memory_pool_item *next;
} memory_pool_item_t;

struct memory_pool {
    void *base;
    size_t item_size;
    size_t item_count;
    memory_pool_item_t *free_list;
};

memory_pool_t *memory_pool_create(size_t item_size, size_t item_count) {
    memory_pool_t *pool = calloc(1, sizeof(memory_pool_t));
    if (!pool) {
        return NULL;
    }
    
    size_t aligned_size = (item_size + CACHE_LINE_SIZE - 1) & ~(CACHE_LINE_SIZE - 1);
    pool->item_size = aligned_size;
    pool->item_count = item_count;
    
    pool->base = memory_alloc_aligned(aligned_size * item_count);
    if (!pool->base) {
        free(pool);
        return NULL;
    }
    
    pool->free_list = NULL;
    for (size_t i = 0; i < item_count; i++) {
        memory_pool_item_t *item = (memory_pool_item_t *)((uint8_t *)pool->base + i * aligned_size);
        item->next = pool->free_list;
        pool->free_list = item;
    }
    
    return pool;
}

void memory_pool_destroy(memory_pool_t *pool) {
    if (pool) {
        if (pool->base) {
            memory_free_aligned(pool->base);
        }
        free(pool);
    }
}

void *memory_pool_alloc(memory_pool_t *pool) {
    if (!pool || !pool->free_list) {
        return NULL;
    }
    
    memory_pool_item_t *item = pool->free_list;
    pool->free_list = item->next;
    
    return item;
}

void memory_pool_free(memory_pool_t *pool, void *ptr) {
    if (!pool || !ptr) {
        return;
    }
    
    memory_pool_item_t *item = (memory_pool_item_t *)ptr;
    item->next = pool->free_list;
    pool->free_list = item;
}
