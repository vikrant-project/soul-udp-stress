#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <stdbool.h>

#define DEFAULT_THREADS 50
#define DEFAULT_IO_THREADS 4
#define MAX_THREADS 256
#define DEFAULT_PACKET_SIZE 1400
#define MAX_PACKET_SIZE 65507
#define MIN_PACKET_SIZE 64
#define TOTAL_BUFFER_SIZE (80 * 1024 * 1024)
#define RING_BUFFER_SIZE 8192
#define STATS_INTERVAL_US 1000000

typedef enum {
    PATTERN_RANDOM,
    PATTERN_SEQUENTIAL,
    PATTERN_FIXED
} pattern_type_t;

typedef struct {
    char target_ip[256];
    uint16_t target_port;
    uint32_t duration_seconds;
    uint32_t packet_size;
    uint32_t rate_limit;
    uint32_t num_threads;
    uint32_t num_io_threads;
    pattern_type_t pattern;
    bool local_only;
    bool verbose;
} config_t;

extern config_t g_config;
extern volatile bool g_running;

int config_parse(int argc, char *argv[], config_t *cfg);
void config_print(const config_t *cfg);
bool config_validate(const config_t *cfg);

#endif
