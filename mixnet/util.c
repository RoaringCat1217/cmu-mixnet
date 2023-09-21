#include "connection.h"

#include <stddef.h>
#include <sys/time.h>

// repeatedly send a packet until success or error
int mixnet_send_loop(void *handle, const uint8_t port, mixnet_packet *packet) {
    while (true) {
        int ret = mixnet_send(handle, port, packet);
        if (ret < 0)
            return -1;
        if (ret > 0)
            break;
    }

    return 1;
}

// get current timestamp in milliseconds
unsigned long get_timestamp() {
    struct timeval te;
    gettimeofday(&te, NULL);
    unsigned long ms = te.tv_sec * 1000UL + te.tv_usec / 1000UL;

    return ms;
}
