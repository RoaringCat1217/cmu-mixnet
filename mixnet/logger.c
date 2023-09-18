#include "logger.h"

mixnet_address node;
bool on;
FILE *out;

unsigned long timestamp() {
    struct timeval te;
    gettimeofday(&te, NULL);
    unsigned long us = te.tv_sec * 1000000UL + te.tv_usec;
    return us;
}

void logger_init(bool turn_on, mixnet_address node_addr) {
    on = turn_on;
    node = node_addr;
    char filename[64];
    if (on) {
        sprintf(filename, "./logs/node_%d.csv", node);
        out = fopen(filename, "w+");
        fprintf(out, "node address|timestamp|msg\n");
    }
}

void print(const char *format, ...) {
    if (on) {
        va_list args;
        va_start(args, format);
        unsigned long t = timestamp();
        fprintf(out, "%d|%ld|", node, t);
        vfprintf(out, format, args);
        fprintf(out, "\n");
        va_end(args);
    }
}

void print_err(const char *format, ...) {
    if (on) {
        va_list args;
        va_start(args, format);
        unsigned long t = timestamp();
        fprintf(out, "%d|%ld|ERROR: ", node, t);
        vfprintf(out, format, args);
        fprintf(out, "\n");
        va_end(args);
    }
}

void logger_end() {
    if (on) {
        fclose(out);
    }
}