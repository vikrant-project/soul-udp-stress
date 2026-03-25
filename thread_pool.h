#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include "config.h"
#include "ring_buffer.h"

typedef struct thread_pool thread_pool_t;

thread_pool_t *thread_pool_create(const config_t *cfg, ring_buffer_t *rb);
void thread_pool_destroy(thread_pool_t *pool);

int thread_pool_start(thread_pool_t *pool);
void thread_pool_stop(thread_pool_t *pool);
void thread_pool_wait(thread_pool_t *pool);

#endif
