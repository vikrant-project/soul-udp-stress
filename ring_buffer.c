#include "ring_buffer.h"
#include "memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

struct ring_buffer {
    packet_t *buffer;
    size_t capacity;
    size_t head;
    size_t tail;
    pthread_mutex_t lock;
    char padding[CACHE_LINE_SIZE];
};

ring_buffer_t *ring_buffer_create(size_t capacity) {
    ring_buffer_t *rb = memory_alloc_aligned(sizeof(ring_buffer_t));
    if (!rb) {
        return NULL;
    }
    
    memset(rb, 0, sizeof(ring_buffer_t));
    rb->capacity = capacity;
    rb->head = 0;
    rb->tail = 0;
    
    if (pthread_mutex_init(&rb->lock, NULL) != 0) {
        memory_free_aligned(rb);
        return NULL;
    }
    
    rb->buffer = calloc(capacity, sizeof(packet_t));
    if (!rb->buffer) {
        pthread_mutex_destroy(&rb->lock);
        memory_free_aligned(rb);
        return NULL;
    }
    
    return rb;
}

void ring_buffer_destroy(ring_buffer_t *rb) {
    if (rb) {
        if (rb->buffer) {
            free(rb->buffer);
        }
        pthread_mutex_destroy(&rb->lock);
        memory_free_aligned(rb);
    }
}

bool ring_buffer_push(ring_buffer_t *rb, packet_t *pkt) {
    if (!rb || !pkt || !pkt->data) {
        return false;
    }
    
    pthread_mutex_lock(&rb->lock);
    
    size_t next_head = (rb->head + 1) % rb->capacity;
    
    if (next_head == rb->tail) {
        pthread_mutex_unlock(&rb->lock);
        return false;
    }
    
    rb->buffer[rb->head] = *pkt;
    rb->head = next_head;
    
    pthread_mutex_unlock(&rb->lock);
    
    return true;
}

bool ring_buffer_pop(ring_buffer_t *rb, packet_t *pkt) {
    if (!rb || !pkt) {
        return false;
    }
    
    pthread_mutex_lock(&rb->lock);
    
    if (rb->tail == rb->head) {
        pthread_mutex_unlock(&rb->lock);
        return false;
    }
    
    *pkt = rb->buffer[rb->tail];
    rb->tail = (rb->tail + 1) % rb->capacity;
    
    pthread_mutex_unlock(&rb->lock);
    
    return true;
}

size_t ring_buffer_size(ring_buffer_t *rb) {
    if (!rb) {
        return 0;
    }
    
    pthread_mutex_lock(&rb->lock);
    
    size_t size;
    if (rb->head >= rb->tail) {
        size = rb->head - rb->tail;
    } else {
        size = rb->capacity - rb->tail + rb->head;
    }
    
    pthread_mutex_unlock(&rb->lock);
    
    return size;
}

bool ring_buffer_is_empty(ring_buffer_t *rb) {
    if (!rb) {
        return true;
    }
    
    pthread_mutex_lock(&rb->lock);
    bool empty = (rb->head == rb->tail);
    pthread_mutex_unlock(&rb->lock);
    
    return empty;
}

bool ring_buffer_is_full(ring_buffer_t *rb) {
    if (!rb) {
        return true;
    }
    
    pthread_mutex_lock(&rb->lock);
    size_t next_head = (rb->head + 1) % rb->capacity;
    bool full = (next_head == rb->tail);
    pthread_mutex_unlock(&rb->lock);
    
    return full;
}
