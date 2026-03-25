#include "utils.h"
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

void utils_sleep_ms(unsigned int ms) {
    usleep(ms * 1000);
}

unsigned long utils_get_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}
