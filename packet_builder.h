#ifndef PACKET_BUILDER_H
#define PACKET_BUILDER_H

#include <stdint.h>
#include <stddef.h>
#include "config.h"

#pragma pack(push, 1)
typedef struct {
    uint64_t sequence;
    uint32_t thread_id;
    uint32_t timestamp_ms;
} packet_header_t;
#pragma pack(pop)

typedef struct {
    uint8_t *data;
    size_t size;
    uint64_t sequence;
} packet_t;

int packet_builder_init(void);
void packet_builder_cleanup(void);

int packet_build(packet_t *pkt, uint64_t seq, uint32_t thread_id, 
                 size_t size, pattern_type_t pattern);

void packet_free(packet_t *pkt);

#endif
