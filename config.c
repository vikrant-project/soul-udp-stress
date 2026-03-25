#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

config_t g_config = {0};
volatile bool g_running = true;

static void print_usage(const char *prog) {
    printf("Usage: %s <ip> <port> <duration> [options]\n", prog);
    printf("\nRequired arguments:\n");
    printf("  ip          Target IP address\n");
    printf("  port        Target port (1-65535)\n");
    printf("  duration    Test duration in seconds\n");
    printf("\nOptions:\n");
    printf("  --rate <pps>       Limit packets per second (0=unlimited)\n");
    printf("  --size <bytes>     Packet size (%d-%d, default: %d)\n",
           MIN_PACKET_SIZE, MAX_PACKET_SIZE, DEFAULT_PACKET_SIZE);
    printf("  --threads <N>      Number of threads (1-%d, default: %d)\n",
           MAX_THREADS, DEFAULT_THREADS);
    printf("  --pattern <type>   Payload pattern: random, sequential, fixed\n");
    printf("  --local-only       Restrict to localhost only\n");
    printf("  --verbose          Enable verbose output\n");
    printf("  --help             Show this help\n");
}

int config_parse(int argc, char *argv[], config_t *cfg) {
    if (argc < 4) {
        print_usage(argv[0]);
        return -1;
    }

    memset(cfg, 0, sizeof(config_t));
    
    strncpy(cfg->target_ip, argv[1], sizeof(cfg->target_ip) - 1);
    cfg->target_port = (uint16_t)atoi(argv[2]);
    cfg->duration_seconds = (uint32_t)atoi(argv[3]);
    
    cfg->packet_size = DEFAULT_PACKET_SIZE;
    cfg->rate_limit = 0;
    cfg->num_threads = DEFAULT_THREADS;
    cfg->num_io_threads = DEFAULT_IO_THREADS;
    cfg->pattern = PATTERN_RANDOM;
    cfg->local_only = false;
    cfg->verbose = false;

    for (int i = 4; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return -1;
        } else if (strcmp(argv[i], "--rate") == 0 && i + 1 < argc) {
            cfg->rate_limit = (uint32_t)atoi(argv[++i]);
        } else if (strcmp(argv[i], "--size") == 0 && i + 1 < argc) {
            cfg->packet_size = (uint32_t)atoi(argv[++i]);
        } else if (strcmp(argv[i], "--threads") == 0 && i + 1 < argc) {
            cfg->num_threads = (uint32_t)atoi(argv[++i]);
        } else if (strcmp(argv[i], "--pattern") == 0 && i + 1 < argc) {
            i++;
            if (strcmp(argv[i], "random") == 0) {
                cfg->pattern = PATTERN_RANDOM;
            } else if (strcmp(argv[i], "sequential") == 0) {
                cfg->pattern = PATTERN_SEQUENTIAL;
            } else if (strcmp(argv[i], "fixed") == 0) {
                cfg->pattern = PATTERN_FIXED;
            } else {
                fprintf(stderr, "Unknown pattern: %s\n", argv[i]);
                return -1;
            }
        } else if (strcmp(argv[i], "--local-only") == 0) {
            cfg->local_only = true;
        } else if (strcmp(argv[i], "--verbose") == 0) {
            cfg->verbose = true;
        }
    }

    return 0;
}

void config_print(const config_t *cfg) {
    const char *pattern_names[] = {"random", "sequential", "fixed"};
    printf("Configuration:\n");
    printf("  Target: %s:%u\n", cfg->target_ip, cfg->target_port);
    printf("  Duration: %u seconds\n", cfg->duration_seconds);
    printf("  Packet size: %u bytes\n", cfg->packet_size);
    printf("  Threads: %u workers, %u I/O\n", cfg->num_threads, cfg->num_io_threads);
    printf("  Pattern: %s\n", pattern_names[cfg->pattern]);
    printf("  Rate limit: %s\n", cfg->rate_limit ? "enabled" : "unlimited");
    printf("  Local only: %s\n", cfg->local_only ? "yes" : "no");
}

bool config_validate(const config_t *cfg) {
    struct sockaddr_in addr;
    
    if (inet_pton(AF_INET, cfg->target_ip, &addr.sin_addr) != 1) {
        fprintf(stderr, "Invalid IP address: %s\n", cfg->target_ip);
        return false;
    }
    
    if (cfg->local_only) {
        uint32_t ip = ntohl(addr.sin_addr.s_addr);
        if ((ip >> 24) != 127) {
            fprintf(stderr, "Local-only mode: IP must be 127.x.x.x\n");
            return false;
        }
    }
    
    if (cfg->target_port == 0) {
        fprintf(stderr, "Invalid port: %u\n", cfg->target_port);
        return false;
    }
    
    if (cfg->duration_seconds == 0 || cfg->duration_seconds > 86400) {
        fprintf(stderr, "Invalid duration: %u (must be 1-86400)\n", cfg->duration_seconds);
        return false;
    }
    
    if (cfg->packet_size < MIN_PACKET_SIZE || cfg->packet_size > MAX_PACKET_SIZE) {
        fprintf(stderr, "Invalid packet size: %u (must be %d-%d)\n",
                cfg->packet_size, MIN_PACKET_SIZE, MAX_PACKET_SIZE);
        return false;
    }
    
    if (cfg->num_threads == 0 || cfg->num_threads > MAX_THREADS) {
        fprintf(stderr, "Invalid thread count: %u (must be 1-%d)\n",
                cfg->num_threads, MAX_THREADS);
        return false;
    }
    
    return true;
}
