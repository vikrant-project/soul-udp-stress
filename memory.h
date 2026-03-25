#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>
#include <stdint.h>

#define CACHE_LINE_SIZE 64

int memory_init(size_t total_size);
void memory_cleanup(void);

void *memory_alloc_aligned(size_t size);
void memory_free_aligned(void *ptr);

typedef struct memory_pool memory_pool_t;

memory_pool_t *memory_pool_create(size_t item_size, size_t item_count);
void memory_pool_destroy(memory_pool_t *pool);
void *memory_pool_alloc(memory_pool_t *pool);
void memory_pool_free(memory_pool_t *pool, void *ptr);

#endif
