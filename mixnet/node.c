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
#include <string.h>
#include <sys/time.h>

#define ADDR_UNK 0
#define NO_NEXTHOP -1
#define INT_MAX 0x7FFFFFFF

/* states of the node should be initialized as global variables */

/* basic states */
void *myhandle;

struct mixnet_node_config node_config;

// STP
mixnet_address *neighbor_addrs;
mixnet_packet_stp stp_curr_state;
uint8_t stp_nexthop = NO_NEXTHOP; 
bool *port_open;
int *dist_to_root;
unsigned long timer;

// receive packets
uint8_t port_recv;
mixnet_packet *packet_recv_ptr;
bool need_free;

// get current timestamp in milliseconds
unsigned long get_timestamp() {
    struct timeval te; 
    gettimeofday(&te, NULL); 
    unsigned long ms = te.tv_sec * 1000UL + te.tv_usec / 1000UL;
    return ms;
}

void init_node() {
    neighbor_addrs = (mixnet_address *)malloc(node_config.num_neighbors * sizeof(mixnet_address));
    for (uint8_t port = 0; port < node_config.num_neighbors; port++)
        neighbor_addrs[port] = ADDR_UNK;
    
    stp_curr_state = (mixnet_packet_stp){node_config.node_addr, 0, node_config.node_addr};
    stp_nexthop = NO_NEXTHOP;
    port_open = (bool *)malloc(node_config.num_neighbors * sizeof(bool));
    for (uint8_t port = 0; port < node_config.num_neighbors; port++)
        port_open[port] = true;
    dist_to_root = (int *)malloc(sizeof(int) * node_config.num_neighbors);
    for (uint8_t port = 0; port < node_config.num_neighbors; port++)
        dist_to_root[port] = INT_MAX;
    timer = 0;
}

void free_node() {
    free(neighbor_addrs);
    free(port_open);
    free(dist_to_root);

}

// send STP packets to all neighbors
int stp_send() {
    int nsent = 0;
    for (uint8_t port = 0; port < node_config.num_neighbors; port++) {
        void *sendbuf = malloc(sizeof(mixnet_packet) + sizeof(mixnet_packet_stp));
        mixnet_packet *headerp = (mixnet_packet *)sendbuf;
        headerp->total_size = sizeof(mixnet_packet) + sizeof(mixnet_packet_stp);
        headerp->type = PACKET_TYPE_STP;

        mixnet_packet_stp *payloadp = (mixnet_packet_stp *)((char *)sendbuf + sizeof(mixnet_packet));
        *payloadp = stp_curr_state;
        int ret;
        while (true) {
            ret = mixnet_send(myhandle, port, headerp);
            if (ret < 0)
                return -1;
            if (ret == 1) {
                nsent++;
                break;
            }
        }
    }
    return nsent;
}

// received an STP packet, returns 0 on success
int stp_recv(mixnet_packet_stp *stp_packet) {
    // update the neighbor's address and distance to root
    neighbor_addrs[port_recv] = stp_packet->node_address;
    dist_to_root[port_recv] = stp_packet->path_length;

    // change my STP state
    bool stp_changed = false;
    bool notify = false;
    if (stp_packet->root_address < stp_curr_state.root_address) {
        stp_curr_state.root_address = stp_packet->root_address;
        stp_curr_state.path_length = stp_packet->path_length + 1;
        stp_nexthop = port_recv;
        stp_changed = true;
        notify = true;
    } else if (stp_packet->root_address == stp_curr_state.root_address) {
        if (stp_packet->path_length + 1 < stp_curr_state.path_length) {
            stp_curr_state.path_length = stp_packet->path_length + 1;
            stp_nexthop = port_recv;
            stp_changed = true;
            notify = true;
        } else if (stp_packet->path_length + 1 == stp_curr_state.path_length && stp_packet->node_address < neighbor_addrs[stp_nexthop]) {
            stp_nexthop = port_recv;
            stp_changed = true;
        }
    }

    // change port blocking state
    if (stp_changed) {
        for (uint8_t port = 0; port < node_config.num_neighbors; port++) {
            if (port == stp_nexthop)
                port_open[port] = true;
            else if (dist_to_root[port] > stp_curr_state.path_length)
                port_open[port] = true;
            else
                port_open[port] = false;
        }
    } else {
        if (port_recv == stp_nexthop)
            port_open[port_recv] = true;
        else if (dist_to_root[port_recv] > stp_curr_state.path_length)
            port_open[port_recv] = true;
        else
            port_open[port_recv] = false;
    }

    // copy the root's "hello" message to all other neighbors
    if (stp_curr_state.root_address != node_config.node_addr && stp_packet->node_address == stp_curr_state.root_address) {
        for (uint8_t port = 0; port < node_config.num_neighbors; port++)
            if (port != port_recv) {
                void *sendbuf = malloc(sizeof(mixnet_packet) + sizeof(mixnet_packet_stp));
                memcpy(sendbuf, stp_packet, sizeof(mixnet_packet) + sizeof(mixnet_packet_stp));
                int ret;
                while (true) {
                    ret = mixnet_send(myhandle, port, sendbuf);
                    if (ret < 0)
                        return -1;
                    if (ret == 1) {
                        break;
                    }
                }
            }
        // reset timer
        timer = get_timestamp();
    }

    // notify neighbors if anything changes
    if (notify && stp_send() < 0)
        return -1;
    
    return 0;
    
}

// send hello message if this is a root, otherwise decide if a reelection is needed
int stp_hello() {
    unsigned long now = get_timestamp();
    unsigned long interval = now - timer;
    if (stp_curr_state.root_address == node_config.node_addr && interval >= node_config.root_hello_interval_ms) {
        if (stp_send() < 0)
            return -1;
        timer = now;
    } else if (stp_curr_state.root_address != node_config.node_addr && interval >= node_config.reelection_interval_ms) {
        // reelect
        stp_curr_state = (mixnet_packet_stp){node_config.node_addr, 0, node_config.node_addr};
        stp_nexthop = NO_NEXTHOP;
        for (uint8_t port = 0; port < node_config.num_neighbors; port++)
            port_open[port] = true;
        for (uint8_t port = 0; port < node_config.num_neighbors; port++)
            dist_to_root[port] = INT_MAX;
        timer = 0;
    }

    return 0;
}

void run_node(void *const handle,
              volatile bool *const keep_running,
              const struct mixnet_node_config c) {
    
    myhandle = handle;
    node_config = c;
    init_node();

    while(*keep_running) {
        int received = mixnet_recv(myhandle, &port_recv, &packet_recv_ptr);
        if (received > 0) {
            need_free = true;
            switch (packet_recv_ptr->type) {
                case PACKET_TYPE_STP:
                    if (stp_recv((mixnet_packet_stp *) packet_recv_ptr->payload) < 0)
                        fprintf(stderr, "error in stp_recv()");
                    break;
                default:
                    fprintf(stderr, "received unsupported packet, dropped");
            }
            if (need_free)
                free(packet_recv_ptr);
        }

        if (stp_hello() < 0)
            fprintf(stderr, "error in stp_hello()");
    }

    free_node();
}
