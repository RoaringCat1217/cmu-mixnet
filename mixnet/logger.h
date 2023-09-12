#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>
#include "address.h"

void logger_init(bool turn_on, mixnet_address node_addr);
void print(const char *format, ...);
void print_err(const char *format, ...);
