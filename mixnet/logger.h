#include "address.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

void logger_init(bool turn_on, mixnet_address node_addr);
void print(const char *format, ...);
void print_err(const char *format, ...);
void logger_end();