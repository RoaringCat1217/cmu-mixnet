#include "logger.h"

mixnet_address node;
bool on;
char timebuf[9];

void logger_init(bool turn_on, mixnet_address node_addr) {
    on = turn_on;
    node = node_addr;
}

unsigned long timestamp() {
    struct timeval te; 
    gettimeofday(&te, NULL); 
    unsigned long us = te.tv_sec * 1000000UL + te.tv_usec;
    return us % 1000000000UL;
}

void print(const char *format, ...) {
    if (on) {
        va_list args;
        va_start(args, format);
        unsigned long t = timestamp();
        printf("[node %d](%ld): ", node, t);
        vprintf(format, args);
        printf("\n");
        va_end(args);
    }
}

void print_err(const char *format, ...) {
    if (on) {
        va_list args;
        va_start(args, format);
        unsigned long t = timestamp();
        fprintf(stderr, "[node %d](%ld): ", node, t);
        vfprintf(stderr, format, args);
        fprintf(stderr, "\n");
        va_end(args);
    }
}