#ifndef ROUTING_H_
#define ROUTING_H_

#include "address.h"
#include "attr.h"
#include "ll.h"
#include "packet.h"
#include "stdbool.h"
#include "utils.h"
#include <stdlib.h>

#define PING_REQUEST true
#define PING_RESPONSE false

int routing_forward(char *payload);
int ping_send(mixnet_packet_routing_header *header, bool type);
int ping_recv(mixnet_packet_routing_header *header);
int data_send();

#endif // ROUTING_H_
