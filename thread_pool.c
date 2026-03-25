#define _GNU_SOURCE
#include "thread_pool.h"
#include "packet_builder.h"
#include "socket_wrapper.h"
#include "stats.h"
#include "memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

#ifdef __linux__
#include <sched.h>
#endif

typedef struct {
    uint32_t thread_id;
    const config_t *config;
    ring_buffer_t *ring_buffer;
    uint64_t packets_generated;
} worker_context_t;

typedef struct {
    uint32_t thread_id;
    const config_t *config;
    ring_buffer_t *ring_buffer;
    socket_t sock;
    uint64_t packets_sent;
} io_context_t;

struct thread_pool {
    const config_t *config;
    ring_buffer_t *ring_buffer;
    
    pthread_t *worker_threads;
    worker_context_t *worker_contexts;
    uint32_t num_workers;
    
    pthread_t *io_threads;
    io_context_t *io_contexts;
    uint32_t num_io_threads;
    
    volatile bool running;
};

static void set_cpu_affinity(uint32_t thread_id) {
#ifdef __linux__
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(thread_id % sysconf(_SC_NPROCESSORS_ONLN), &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
#else
    (void)thread_id;
#endif
}

static void *worker_thread_func(void *arg) {
    worker_context_t *ctx = (worker_context_t *)arg;
    set_cpu_affinity(ctx->thread_id);
    
    uint64_t sequence = ctx->thread_id;
    const uint32_t batch_size = 32;
    
    while (g_running) {
        for (uint32_t i = 0; i < batch_size && g_running; i++) {
            packet_t pkt;
            memset(&pkt, 0, sizeof(pkt));
            
            if (packet_build(&pkt, sequence, ctx->thread_id,
                           ctx->config->packet_size, ctx->config->pattern) == 0) {
                while (!ring_buffer_push(ctx->ring_buffer, &pkt) && g_running) {
                    usleep(1);
                }
                ctx->packets_generated++;
                sequence += ctx->config->num_threads;
            }
        }
        
        if (ctx->config->rate_limit > 0) {
            usleep(1000);
        }
    }
    
    return NULL;
}

static void *io_thread_func(void *arg) {
    io_context_t *ctx = (io_context_t *)arg;
    set_cpu_affinity(ctx->thread_id + 128);
    
    packet_t pkt;
    const uint32_t batch_size = 64;
    
    while (g_running) {
        uint32_t sent_in_batch = 0;
        
        for (uint32_t i = 0; i < batch_size && g_running; i++) {
            memset(&pkt, 0, sizeof(pkt));
            
            if (!ring_buffer_pop(ctx->ring_buffer, &pkt)) {
                break;
            }
            
            ssize_t sent = my_socket_send(ctx->sock, pkt.data, pkt.size);
            
            if (sent > 0) {
                stats_record_packet((size_t)sent);
                ctx->packets_sent++;
                sent_in_batch++;
            } else if (sent < 0) {
                if (errno != EAGAIN && errno != EWOULDBLOCK && errno != ECONNREFUSED) {
                    stats_record_error();
                } else {
                    stats_record_drop();
                }
            }
            
            packet_free(&pkt);
        }
        
        if (sent_in_batch == 0) {
            usleep(100);
        }
    }
    
    return NULL;
}

thread_pool_t *thread_pool_create(const config_t *cfg, ring_buffer_t *rb) {
    thread_pool_t *pool = calloc(1, sizeof(thread_pool_t));
    if (!pool) {
        return NULL;
    }
    
    pool->config = cfg;
    pool->ring_buffer = rb;
    pool->num_workers = cfg->num_threads;
    pool->num_io_threads = cfg->num_io_threads;
    pool->running = false;
    
    pool->worker_threads = calloc(pool->num_workers, sizeof(pthread_t));
    pool->worker_contexts = calloc(pool->num_workers, sizeof(worker_context_t));
    pool->io_threads = calloc(pool->num_io_threads, sizeof(pthread_t));
    pool->io_contexts = calloc(pool->num_io_threads, sizeof(io_context_t));
    
    if (!pool->worker_threads || !pool->worker_contexts ||
        !pool->io_threads || !pool->io_contexts) {
        thread_pool_destroy(pool);
        return NULL;
    }
    
    for (uint32_t i = 0; i < pool->num_workers; i++) {
        pool->worker_contexts[i].thread_id = i;
        pool->worker_contexts[i].config = cfg;
        pool->worker_contexts[i].ring_buffer = rb;
        pool->worker_contexts[i].packets_generated = 0;
    }
    
    for (uint32_t i = 0; i < pool->num_io_threads; i++) {
        pool->io_contexts[i].thread_id = i;
        pool->io_contexts[i].config = cfg;
        pool->io_contexts[i].ring_buffer = rb;
        pool->io_contexts[i].sock = INVALID_SOCKET;
        pool->io_contexts[i].packets_sent = 0;
    }
    
    return pool;
}

void thread_pool_destroy(thread_pool_t *pool) {
    if (!pool) {
        return;
    }
    
    if (pool->io_contexts) {
        for (uint32_t i = 0; i < pool->num_io_threads; i++) {
            if (pool->io_contexts[i].sock != INVALID_SOCKET) {
                my_socket_close(pool->io_contexts[i].sock);
            }
        }
    }
    
    free(pool->worker_threads);
    free(pool->worker_contexts);
    free(pool->io_threads);
    free(pool->io_contexts);
    free(pool);
}

int thread_pool_start(thread_pool_t *pool) {
    if (!pool) {
        return -1;
    }
    
    for (uint32_t i = 0; i < pool->num_io_threads; i++) {
        pool->io_contexts[i].sock = my_socket_create();
        if (pool->io_contexts[i].sock == INVALID_SOCKET) {
            fprintf(stderr, "Failed to create socket for I/O thread %u\n", i);
            return -1;
        }
        
        if (my_socket_configure(pool->io_contexts[i].sock) < 0) {
            fprintf(stderr, "Failed to configure socket for I/O thread %u\n", i);
            return -1;
        }
        
        if (my_socket_set_nonblock(pool->io_contexts[i].sock) < 0) {
            fprintf(stderr, "Failed to set non-blocking for I/O thread %u\n", i);
            return -1;
        }
        
        if (my_socket_connect(pool->io_contexts[i].sock, 
                             pool->config->target_ip,
                             pool->config->target_port) < 0) {
            fprintf(stderr, "Failed to connect socket for I/O thread %u\n", i);
            return -1;
        }
    }
    
    pool->running = true;
    
    for (uint32_t i = 0; i < pool->num_workers; i++) {
        if (pthread_create(&pool->worker_threads[i], NULL, 
                          worker_thread_func, &pool->worker_contexts[i]) != 0) {
            fprintf(stderr, "Failed to create worker thread %u\n", i);
            pool->running = false;
            return -1;
        }
    }
    
    for (uint32_t i = 0; i < pool->num_io_threads; i++) {
        if (pthread_create(&pool->io_threads[i], NULL,
                          io_thread_func, &pool->io_contexts[i]) != 0) {
            fprintf(stderr, "Failed to create I/O thread %u\n", i);
            pool->running = false;
            return -1;
        }
    }
    
    return 0;
}

void thread_pool_stop(thread_pool_t *pool) {
    if (pool) {
        pool->running = false;
    }
}

void thread_pool_wait(thread_pool_t *pool) {
    if (!pool) {
        return;
    }
    
    for (uint32_t i = 0; i < pool->num_workers; i++) {
        pthread_join(pool->worker_threads[i], NULL);
    }
    
    for (uint32_t i = 0; i < pool->num_io_threads; i++) {
        pthread_join(pool->io_threads[i], NULL);
    }
}
