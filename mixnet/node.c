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

struct mixnet_node_config node_config;

mixnet_address *neighbor_addrs;
mixnet_packet_stp stp_curr_state;
mixnet_address stp_parent; 
bool *port_open;

uint8_t port_recv;
mixnet_packet *packet_recv_ptr;

void init_node() {
    neighbor_addrs = (mixnet_address *)malloc(node_config.num_neighbors * sizeof(mixnet_address));
    for (int i = 0; i < node_config.num_neighbors; i++)
        neighbor_addrs[i] = ADDR_UNK;
    
    stp_curr_state = (mixnet_packet_stp){node_config.node_addr, 0, node_config.node_addr};
    stp_parent = node_config.node_addr;

    port_open = (bool *)malloc(node_config.num_neighbors * sizeof(bool));
    for (int i = 0; i < node_config.num_neighbors; i++)
        port_open[i] = false;

}

void stp_recv(mixnet_packet_stp *stp_packet) {
    neighbor_addrs[port_recv] = stp_packet->node_address;
    if (stp_packet->root_address < stp_curr_state.root_address) {
        stp_curr_state.root_address = stp_packet->root_address;
        
    }
}

void run_node(void *const handle,
              volatile bool *const keep_running,
              const struct mixnet_node_config c) {

    (void) c;
    (void) handle;

    node_config = c;
    init_node();

    while(*keep_running) {
        int received = mixnet_recv(handle, &port_recv, &packet_recv_ptr);
        if (received) {

        }
    }
}
