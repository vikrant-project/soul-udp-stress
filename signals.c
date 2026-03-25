#include "signals.h"
#include "config.h"
#include <stdio.h>
#include <signal.h>
#include <string.h>

static void signal_handler(int signum) {
    (void)signum;
    g_running = false;
}

int signals_init(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    if (sigaction(SIGINT, &sa, NULL) < 0) {
        perror("sigaction SIGINT failed");
        return -1;
    }
    
    if (sigaction(SIGTERM, &sa, NULL) < 0) {
        perror("sigaction SIGTERM failed");
        return -1;
    }
    
    signal(SIGPIPE, SIG_IGN);
    
    return 0;
}

void signals_cleanup(void) {
}
