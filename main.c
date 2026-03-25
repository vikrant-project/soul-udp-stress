#include "config.h"
#include "socket_wrapper.h"
#include "packet_builder.h"
#include "memory.h"
#include "ring_buffer.h"
#include "thread_pool.h"
#include "stats.h"
#include "signals.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

static void print_banner(void) {
    printf("\n");
    printf("╔═══════════════════════════════════════════════╗\n");
    printf("║         SOUL - UDP Stress Testing Tool       ║\n");
    printf("║              High Performance Edition         ║\n");
    printf("╚═══════════════════════════════════════════════╝\n");
    printf("\n");
}

int main(int argc, char *argv[]) {
    int exit_code = EXIT_FAILURE;
    ring_buffer_t *ring_buffer = NULL;
    thread_pool_t *thread_pool = NULL;
    
    print_banner();
    
    if (config_parse(argc, argv, &g_config) < 0) {
        return EXIT_FAILURE;
    }
    
    if (!config_validate(&g_config)) {
        return EXIT_FAILURE;
    }
    
    config_print(&g_config);
    printf("\n");
    
    if (signals_init() < 0) {
        fprintf(stderr, "Failed to initialize signal handlers\n");
        goto cleanup;
    }
    
    if (socket_wrapper_init() < 0) {
        fprintf(stderr, "Failed to initialize socket wrapper\n");
        goto cleanup;
    }
    
    if (memory_init(TOTAL_BUFFER_SIZE) < 0) {
        fprintf(stderr, "Failed to initialize memory system\n");
        goto cleanup;
    }
    
    if (packet_builder_init() < 0) {
        fprintf(stderr, "Failed to initialize packet builder\n");
        goto cleanup;
    }
    
    if (stats_init() < 0) {
        fprintf(stderr, "Failed to initialize statistics\n");
        goto cleanup;
    }
    
    ring_buffer = ring_buffer_create(RING_BUFFER_SIZE);
    if (!ring_buffer) {
        fprintf(stderr, "Failed to create ring buffer\n");
        goto cleanup;
    }
    
    thread_pool = thread_pool_create(&g_config, ring_buffer);
    if (!thread_pool) {
        fprintf(stderr, "Failed to create thread pool\n");
        goto cleanup;
    }
    
    printf("Starting test...\n\n");
    
    if (thread_pool_start(thread_pool) < 0) {
        fprintf(stderr, "Failed to start thread pool\n");
        goto cleanup;
    }
    
    unsigned long start_time_ms = utils_get_time_ms();
    unsigned long end_time_ms = start_time_ms + (g_config.duration_seconds * 1000);
    unsigned long last_stats_ms = start_time_ms;
    
    while (g_running) {
        unsigned long now_ms = utils_get_time_ms();
        
        if (now_ms >= end_time_ms) {
            break;
        }
        
        if (now_ms - last_stats_ms >= 1000) {
            stats_print();
            last_stats_ms = now_ms;
        }
        
        utils_sleep_ms(100);
    }
    
    printf("\n\nStopping test...\n");
    g_running = false;
    
    thread_pool_stop(thread_pool);
    thread_pool_wait(thread_pool);
    
    stats_print_final();
    
    exit_code = EXIT_SUCCESS;
    
cleanup:
    if (thread_pool) {
        thread_pool_destroy(thread_pool);
    }
    
    if (ring_buffer) {
        ring_buffer_destroy(ring_buffer);
    }
    
    stats_cleanup();
    packet_builder_cleanup();
    memory_cleanup();
    socket_wrapper_cleanup();
    signals_cleanup();
    
    if (exit_code == EXIT_SUCCESS) {
        printf("\nTest completed successfully.\n");
    } else {
        printf("\nTest failed.\n");
    }
    
    return exit_code;
}
