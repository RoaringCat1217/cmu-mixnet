#ifndef ATTR_H_
#define ATTR_H_

#include "connection.h"
#include "logger.h"
#include "config.h"
#include "packet.h"

#define ADDR_UNK -1
#define NO_NEXTHOP -1
#define INT_MAX 0x7FFFFFFF

// states of the node should be initialized as global variables

// basic states
extern void *myhandle;
extern struct mixnet_node_config node_config;

// stp
extern mixnet_address *neighbor_addrs;
extern mixnet_packet_stp stp_curr_state;
extern uint8_t stp_nexthop;
extern bool *port_open;
extern int *dist_to_root;
extern unsigned long timer;

// lsa

// receive packets
extern uint8_t port_recv;
extern mixnet_packet *packet_recv_ptr;
extern bool need_free;

#endif // ATTR_H_
