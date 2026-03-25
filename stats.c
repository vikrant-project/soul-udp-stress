#include "stats.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

stats_t g_stats = {0};

static struct timeval start_time;
static uint64_t last_packets = 0;
static uint64_t last_bytes = 0;
static struct timeval last_time;

int stats_init(void) {
    atomic_init(&g_stats.packets_sent, 0);
    atomic_init(&g_stats.bytes_sent, 0);
    atomic_init(&g_stats.packets_dropped, 0);
    atomic_init(&g_stats.send_errors, 0);
    
    gettimeofday(&start_time, NULL);
    last_time = start_time;
    last_packets = 0;
    last_bytes = 0;
    
    return 0;
}

void stats_cleanup(void) {
}

void stats_record_packet(size_t bytes) {
    atomic_fetch_add_explicit(&g_stats.packets_sent, 1, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_stats.bytes_sent, bytes, memory_order_relaxed);
}

void stats_record_drop(void) {
    atomic_fetch_add_explicit(&g_stats.packets_dropped, 1, memory_order_relaxed);
}

void stats_record_error(void) {
    atomic_fetch_add_explicit(&g_stats.send_errors, 1, memory_order_relaxed);
}

static double timeval_diff_ms(struct timeval *end, struct timeval *start) {
    return (end->tv_sec - start->tv_sec) * 1000.0 + 
           (end->tv_usec - start->tv_usec) / 1000.0;
}

void stats_print(void) {
    struct timeval now;
    gettimeofday(&now, NULL);
    
    uint64_t packets = atomic_load_explicit(&g_stats.packets_sent, memory_order_relaxed);
    uint64_t bytes = atomic_load_explicit(&g_stats.bytes_sent, memory_order_relaxed);
    uint64_t drops = atomic_load_explicit(&g_stats.packets_dropped, memory_order_relaxed);
    uint64_t errors = atomic_load_explicit(&g_stats.send_errors, memory_order_relaxed);
    
    double interval_ms = timeval_diff_ms(&now, &last_time);
    double total_ms = timeval_diff_ms(&now, &start_time);
    
    if (interval_ms < 1.0) {
        interval_ms = 1.0;
    }
    
    uint64_t interval_packets = packets - last_packets;
    uint64_t interval_bytes = bytes - last_bytes;
    
    double pps = (interval_packets * 1000.0) / interval_ms;
    double mbps = (interval_bytes * 8.0 * 1000.0) / (interval_ms * 1000000.0);
    
    double avg_pps = (packets * 1000.0) / total_ms;
    double avg_mbps = (bytes * 8.0 * 1000.0) / (total_ms * 1000000.0);
    
    printf("\r[%.1fs] PPS: %.0f (avg: %.0f) | Mbps: %.2f (avg: %.2f) | "
           "Sent: %lu | Drops: %lu | Errors: %lu",
           total_ms / 1000.0, pps, avg_pps, mbps, avg_mbps,
           packets, drops, errors);
    fflush(stdout);
    
    last_packets = packets;
    last_bytes = bytes;
    last_time = now;
}

void stats_print_final(void) {
    printf("\n\n");
    printf("=== Final Statistics ===\n");
    
    struct timeval now;
    gettimeofday(&now, NULL);
    
    uint64_t packets = atomic_load_explicit(&g_stats.packets_sent, memory_order_relaxed);
    uint64_t bytes = atomic_load_explicit(&g_stats.bytes_sent, memory_order_relaxed);
    uint64_t drops = atomic_load_explicit(&g_stats.packets_dropped, memory_order_relaxed);
    uint64_t errors = atomic_load_explicit(&g_stats.send_errors, memory_order_relaxed);
    
    double total_ms = timeval_diff_ms(&now, &start_time);
    double total_sec = total_ms / 1000.0;
    
    double avg_pps = packets / total_sec;
    double avg_mbps = (bytes * 8.0) / (total_sec * 1000000.0);
    
    printf("Duration: %.2f seconds\n", total_sec);
    printf("Packets sent: %lu\n", packets);
    printf("Bytes sent: %lu (%.2f MB)\n", bytes, bytes / (1024.0 * 1024.0));
    printf("Average PPS: %.0f\n", avg_pps);
    printf("Average Mbps: %.2f\n", avg_mbps);
    printf("Packets dropped: %lu\n", drops);
    printf("Send errors: %lu\n", errors);
    
    if (packets > 0) {
        printf("Drop rate: %.2f%%\n", (drops * 100.0) / packets);
        printf("Error rate: %.2f%%\n", (errors * 100.0) / packets);
    }
}
