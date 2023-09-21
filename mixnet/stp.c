#include "stp.h"
#include "utils.h"
#include "attr.h"
#include "connection.h"
#include "logger.h"
#include "packet.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

// send STP update packets to all neighbors, triggered by stp_recv
// returns the number of packets sent, or -1 for error
int stp_send() {
    int nsent = 0;

    for (uint8_t port = 0; port < node_config.num_neighbors; port++) {
        if (port == port_recv) {
            continue;
        }

        void *sendbuf =
            malloc(sizeof(mixnet_packet) + sizeof(mixnet_packet_stp));

        mixnet_packet *headerp = (mixnet_packet *)sendbuf;
        headerp->total_size = sizeof(mixnet_packet) + sizeof(mixnet_packet_stp);
        headerp->type = PACKET_TYPE_STP;

        mixnet_packet_stp *payloadp =
            (mixnet_packet_stp *)((char *)sendbuf + sizeof(mixnet_packet));
        *payloadp = stp_curr_state;

        int ret = mixnet_send_loop(myhandle, port, headerp);
        if (ret < 0) {
            return -1;
        } else {
            print("sent STP packet to port %d(node %d)", port,
                  neighbor_addrs[port]);
            nsent++;
        }
    }
    return nsent;
}

// send STP hello messages to all open ports
// return the number of packets send, or -1 for error
int stp_hello() {
    int nsent = 0;

    for (uint8_t port = 0; port < node_config.num_neighbors; port++) {
        if (!port_open[port]) {
            continue;
        }

        void *sendbuf =
            malloc(sizeof(mixnet_packet) + sizeof(mixnet_packet_stp));

        mixnet_packet *headerp = (mixnet_packet *)sendbuf;
        headerp->total_size = sizeof(mixnet_packet) + sizeof(mixnet_packet_stp);
        headerp->type = PACKET_TYPE_STP;

        mixnet_packet_stp *payloadp =
            (mixnet_packet_stp *)((char *)sendbuf + sizeof(mixnet_packet));
        *payloadp = stp_curr_state;

        int ret = mixnet_send_loop(myhandle, port, headerp);
        if (ret < 0) {
            return -1;
        } else {
            print("sent STP Hello packet to port %d(node %d)", port,
                  neighbor_addrs[port]);
            nsent++;
        }
    }

    return nsent;
}

// received an STP packet, send STP updates if my STP state changes
// returns 0 on success
int stp_recv(mixnet_packet_stp *stp_packet) {
    // update the neighbor's address and distance to root
    neighbor_addrs[port_recv] = stp_packet->node_address;

    if (stp_packet->root_address == stp_curr_state.root_address) {
        dist_to_root[port_recv] = stp_packet->path_length;
    } else {
        dist_to_root[port_recv] = INT_MAX;
    }

    // change my STP state
    bool stp_changed = false;
    bool notify = false;
    if (stp_packet->root_address < stp_curr_state.root_address) {
        stp_curr_state.root_address = stp_packet->root_address;
        stp_curr_state.path_length = stp_packet->path_length + 1;
        dist_to_root[port_recv] = stp_packet->path_length;
        stp_nexthop = port_recv;
        stp_changed = true;
        notify = true;
    } else if (stp_packet->root_address == stp_curr_state.root_address) {
        if (stp_packet->path_length + 1 < stp_curr_state.path_length) {
            stp_curr_state.path_length = stp_packet->path_length + 1;
            stp_nexthop = port_recv;
            stp_changed = true;
            notify = true;
        } else if (stp_packet->path_length + 1 == stp_curr_state.path_length &&
                   stp_packet->node_address < neighbor_addrs[stp_nexthop]) {
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
    if (stp_curr_state.root_address != node_config.node_addr &&
        stp_packet->root_address == stp_curr_state.root_address &&
        port_recv == stp_nexthop) {
        if (stp_hello() < 0)
            return -1;

        // reset timer
        timer = get_timestamp();
    }

    // notify neighbors if anything changes
    if (notify && stp_send() < 0) {
        print_err("stp_recv's notify error");
        return -1;
    }

    return 0;
}

// sends hello message if this is a root, otherwise decides if a reelection
// is needed
int stp_check_timer() {
    unsigned long now = get_timestamp();
    unsigned long interval = now - timer;

    if (stp_curr_state.root_address == node_config.node_addr &&
        interval >= node_config.root_hello_interval_ms) {
        if (stp_hello() < 0) {
            return -1;
        }
        timer = get_timestamp();
    } else if (stp_curr_state.root_address != node_config.node_addr &&
               interval >= node_config.reelection_interval_ms) {
        // reelect
        stp_curr_state = (mixnet_packet_stp){node_config.node_addr, 0,
                                             node_config.node_addr};
        stp_nexthop = NO_NEXTHOP;
        for (uint8_t port = 0; port < node_config.num_neighbors; port++) {
            port_open[port] = true;
        }
        for (uint8_t port = 0; port < node_config.num_neighbors; port++) {
            dist_to_root[port] = INT_MAX;
        }

        if (stp_hello() < 0) {
            return -1;
        }

        timer = get_timestamp();
    }

    return 0;
}

// send FLOOD packets via spanning tree
int stp_flood() {
    int ret = 0;

    for (uint8_t port_num = 0; port_num < node_config.num_neighbors;
         ++port_num) {
        if (port_num != port_recv && port_open[port_num]) {
            print("flood to port %d(node %d)", port_num,
                  neighbor_addrs[port_num]);

            void *sendbuf = malloc(sizeof(mixnet_packet));
            mixnet_packet *headerp = (mixnet_packet *)sendbuf;
            headerp->total_size = sizeof(mixnet_packet);
            headerp->type = PACKET_TYPE_FLOOD;

            ret = mixnet_send_loop(myhandle, port_num, sendbuf);
            if (ret < 0) {
                return -1;
            }
        }
    }

    return 0;
}

// send the current received packet to user
int send_to_user() {
    need_free = false;
    if (mixnet_send_loop(myhandle, node_config.num_neighbors, packet_recv_ptr) <
        0) {
        return -1;
    }

    return 0;
}
