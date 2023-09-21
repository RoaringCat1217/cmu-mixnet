#include "datapacket.h"
#include "utils.h"
#include "attr.h"
#include "connection.h"
#include "logger.h"
#include "packet.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#define PING_REQUEST true
#define PING_RESPONSE false

int ping_send(bool type) {
    void *sendbuf;
    if (type == PING_REQUEST) {
        sendbuf = packet_recv_ptr;
        
    }
}