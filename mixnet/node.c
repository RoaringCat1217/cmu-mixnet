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
#include "packet.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#define UNKNOWN_ADDR (-1)
#define HEADER_SIZE (12)

void run_node(void *const handle,
              volatile bool *const keep_running,
              const struct mixnet_node_config c) {

    mixnet_address *neighbor_addrs = malloc(c.num_neighbors * sizeof(mixnet_address));
    bool *port_status = malloc(c.num_neighbors * sizeof(bool));

    mixnet_packet_stp config;
    mixnet_address mixnet_addr;
    struct timeval tval_before, tval_after, tval_result;


    (void) c;
    (void) handle;

    init_node(c, &mixnet_addr, &config, neighbor_addrs, port_status);

    gettimeofday(&tval_before, NULL);

    while(*keep_running) {
        uint8_t port;
        mixnet_packet **packets = NULL;

        gettimeofday(&tval_after, NULL);

        timersub(&tval_after, &tval_before, &tval_result);

        // TODO: use tval_result


        int packet_count = mixnet_recv(handle, &port, packets);

        if (port == c.num_neighbors) {
            for (int i = 0; i < packet_count; ++i) {
                if (packets[i]->type == PACKET_TYPE_FLOOD) {
                    // broadcast along the spanning tree

                } else if (packets[i]->type == PACKET_TYPE_DATA || packets[i]->type == PACKET_TYPE_PING) {
                    // TODO: compute route to destination using FIB
                }
            }
        } else {
            for (int i = 0; i < packet_count; ++i) {
                if (packets[i]->type == PACKET_TYPE_STP) {
                    // do neighbor discovery, update root node data, reset reelection timer, and broadcast packet
                    run_stp(&config, packets[i], port_status, i);
                    gettimeofday(&tval_before, NULL);
                } else if (packets[i]->type == PACKET_TYPE_LSA) {
                    // TODO: update local link state, compute shortest paths, and broadcast along the spanning tree
                } else {
                    // TODO: node is either a forwarding node or destination
                }
            }
        }
    }
}

void init_node(struct mixnet_node_config c, mixnet_address* mixnet_addr, mixnet_packet_stp *config,
        mixnet_address *neighbor_addrs, bool *port_status) {
    *mixnet_addr = c.node_addr;

    config->root_address = c.node_addr;
    config->path_length = 0;
    config->node_address = c.node_addr;

    for (int i = 0; i < c.num_neighbors; i++) {
        neighbor_addrs[i] = UNKNOWN_ADDR;
        port_status[i] = true;
    }
}

void run_stp(mixnet_packet_stp *config, mixnet_packet *packet, bool *port_status, int port_id) {
    /**
      * 1. If their ID number is smaller, replace root/path with their root/path, incrementing PathLength by one
      * 2. If their ID number is the same but their path length is shorter, replace your root/path with theirs,
      *        incrementing PathLength by 1
      * 3. If neighbor A and neighbor B both tell you the same ID and path length, choose to route through A
        since A is lower than B.
     */

    uint16_t root_addr, path_length, node_addr;

    memcpy(&root_addr, packet->payload, sizeof root_addr);
    memcpy(&path_length, (packet->payload) + 2, sizeof root_addr);
    memcpy(&node_addr, (packet->payload) + 4, sizeof root_addr);

    if (root_addr < config->root_address) {
        config->root_address = root_addr;
        config->path_length = path_length + 1;
    } else if (root_addr == config->root_address && path_length < config->path_length) {
        config->path_length = path_length + 1;
    }

    if (path_length <= config->path_length && root_addr > config->root_address) {
        port_status[port_id] = false;
    }
}
