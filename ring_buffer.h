#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "packet_builder.h"

typedef struct ring_buffer ring_buffer_t;

ring_buffer_t *ring_buffer_create(size_t capacity);
void ring_buffer_destroy(ring_buffer_t *rb);

bool ring_buffer_push(ring_buffer_t *rb, packet_t *pkt);
bool ring_buffer_pop(ring_buffer_t *rb, packet_t *pkt);

size_t ring_buffer_size(ring_buffer_t *rb);
bool ring_buffer_is_empty(ring_buffer_t *rb);
bool ring_buffer_is_full(ring_buffer_t *rb);

#endif
