#include "packet_builder.h"
#include "memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

static uint8_t *random_template = NULL;
static size_t random_template_size = 0;

int packet_builder_init(void) {
    random_template_size = MAX_PACKET_SIZE;
    random_template = memory_alloc_aligned(random_template_size);
    if (!random_template) {
        fprintf(stderr, "Failed to allocate random template\n");
        return -1;
    }
    
    FILE *urandom = fopen("/dev/urandom", "rb");
    if (urandom) {
        size_t read_bytes = fread(random_template, 1, random_template_size, urandom);
        if (read_bytes != random_template_size) {
            fprintf(stderr, "Warning: partial random data read\n");
        }
        fclose(urandom);
    } else {
        for (size_t i = 0; i < random_template_size; i++) {
            random_template[i] = (uint8_t)(rand() & 0xFF);
        }
    }
    
    return 0;
}

void packet_builder_cleanup(void) {
    if (random_template) {
        memory_free_aligned(random_template);
        random_template = NULL;
    }
}

static uint32_t get_timestamp_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint32_t)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

int packet_build(packet_t *pkt, uint64_t seq, uint32_t thread_id,
                 size_t size, pattern_type_t pattern) {
    if (size < sizeof(packet_header_t)) {
        return -1;
    }
    
    pkt->data = malloc(size);
    if (!pkt->data) {
        return -1;
    }
    
    pkt->size = size;
    pkt->sequence = seq;
    
    packet_header_t *hdr = (packet_header_t *)pkt->data;
    hdr->sequence = seq;
    hdr->thread_id = thread_id;
    hdr->timestamp_ms = get_timestamp_ms();
    
    uint8_t *payload = pkt->data + sizeof(packet_header_t);
    size_t payload_size = size - sizeof(packet_header_t);
    
    switch (pattern) {
        case PATTERN_RANDOM:
            if (payload_size <= random_template_size - sizeof(packet_header_t)) {
                memcpy(payload, random_template + sizeof(packet_header_t), payload_size);
            } else {
                for (size_t i = 0; i < payload_size; i++) {
                    payload[i] = random_template[(i + sizeof(packet_header_t)) % random_template_size];
                }
            }
            break;
            
        case PATTERN_SEQUENTIAL:
            for (size_t i = 0; i < payload_size; i++) {
                payload[i] = (uint8_t)((seq + i) & 0xFF);
            }
            break;
            
        case PATTERN_FIXED:
            memset(payload, 0xFF, payload_size);
            break;
    }
    
    return 0;
}

void packet_free(packet_t *pkt) {
    if (pkt && pkt->data) {
        free(pkt->data);
        pkt->data = NULL;
    }
}
