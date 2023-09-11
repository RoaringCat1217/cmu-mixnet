/**
 * Copyright (C) 2023 Carnegie Mellon University
 *
 * This file is part of the Mixnet course project developed for
 * the Computer Networks course (15-441/641) taught at Carnegie
 * Mellon University.
 *
 * No part of the Mixnet project may be copied and/or distributed
 * without the express permission of the 15-441/641 course staff.
 */
#include "node.h"

#include "connection.h"
#include "packet.h"

#include <stdio.h>
#include <stdlib.h>

#define ADDR_UNK 0
#define NO_NEXTHOP -1

/* states of the node should be initialized as global variables */

/* basic states */
void *myhandle;

struct mixnet_node_config node_config;

// STP
mixnet_address *neighbor_addrs;
mixnet_packet_stp stp_curr_state;
uint8_t stp_nexthop = NO_NEXTHOP; 
bool *port_open;

// receive packets
uint8_t port_recv;
mixnet_packet *packet_recv_ptr;

void init_node() {
    neighbor_addrs = (mixnet_address *)malloc(node_config.num_neighbors * sizeof(mixnet_address));
    for (int i = 0; i < node_config.num_neighbors; i++)
        neighbor_addrs[i] = ADDR_UNK;
    
    stp_curr_state = (mixnet_packet_stp){node_config.node_addr, 0, node_config.node_addr};
    stp_nexthop = -1;

    port_open = (bool *)malloc(node_config.num_neighbors * sizeof(bool));
    for (int i = 0; i < node_config.num_neighbors; i++)
        port_open[i] = false;

}

// send STP packets to all neighbors
int stp_send() {
    int nsent = 0;
    for (uint8_t port = 0; port < node_config.num_neighbors; port++) {
        void *sendbuf = malloc(sizeof(mixnet_packet) + sizeof(mixnet_packet_stp));
        mixnet_packet *headerp = (mixnet_packet *)sendbuf;
        headerp->total_size = sizeof(mixnet_packet) + sizeof(mixnet_packet_stp);
        headerp->type = PACKET_TYPE_STP;
        mixnet_packet_stp *payloadp = (mixnet_packet_stp *)(sendbuf + sizeof(mixnet_packet));
        *payloadp = stp_curr_state;
        int ret;
        while (true) {
            ret = mixnet_send(myhandle, port, headerp);
            if (ret == -1)
                return -1;
            if (ret == 1) {
                nsent++;
                break;
            }
        }
    }
    return nsent;
}

// received an STP packet
void stp_recv(mixnet_packet_stp *stp_packet) {
    neighbor_addrs[port_recv] = stp_packet->node_address;

    bool stp_changed = false;
    if (stp_packet->root_address < stp_curr_state.root_address) {
        stp_curr_state.root_address = stp_packet->root_address;
        stp_curr_state.path_length = stp_packet->path_length + 1;
        stp_nexthop = port_recv;
        stp_changed = true;
    } else if (stp_packet->root_address == stp_curr_state.root_address) {
        if (stp_packet->path_length + 1 < stp_curr_state.path_length) {
            stp_curr_state.path_length = stp_packet->path_length + 1;
            stp_nexthop = port_recv;
            stp_changed = true;
        } else if (stp_packet->path_length + 1 == stp_curr_state.path_length && stp_packet->node_address < neighbor_addrs[stp_nexthop]) {
            stp_nexthop = port_recv;
            stp_changed = true;
        }
    }
    if (stp_changed) {
        stp_send();
    }
}

void run_node(void *const handle,
              volatile bool *const keep_running,
              const struct mixnet_node_config c) {
    
    myhandle = handle;
    node_config = c;
    init_node();

    while(*keep_running) {
        int received = mixnet_recv(myhandle, &port_recv, &packet_recv_ptr);
        if (received) {

        }
    }
}
