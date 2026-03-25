#ifndef STATS_H
#define STATS_H

#include <stdint.h>
#include <stddef.h>
#include <stdatomic.h>

typedef struct {
    atomic_uint_fast64_t packets_sent;
    atomic_uint_fast64_t bytes_sent;
    atomic_uint_fast64_t packets_dropped;
    atomic_uint_fast64_t send_errors;
} stats_t;

extern stats_t g_stats;

int stats_init(void);
void stats_cleanup(void);

void stats_record_packet(size_t bytes);
void stats_record_drop(void);
void stats_record_error(void);

void stats_print(void);
void stats_print_final(void);

#endif
