#include "node.h"

#include "connection.h"
#include "logger.h"
#include "packet.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#define ADDR_UNK -1
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

void init_node() {
    neighbor_addrs = malloc(node_config.num_neighbors * sizeof(mixnet_address));
    for (uint8_t port = 0; port < node_config.num_neighbors; port++) {
        neighbor_addrs[port] = ADDR_UNK;
    }

    port_open = malloc(node_config.num_neighbors * sizeof(bool));
    for (uint8_t port = 0; port < node_config.num_neighbors; port++) {
        port_open[port] = true;
    }

    dist_to_root = malloc(sizeof(int) * node_config.num_neighbors);
    for (uint8_t port = 0; port < node_config.num_neighbors; port++) {
        dist_to_root[port] = INT_MAX;
    }

    stp_curr_state =
        (mixnet_packet_stp){node_config.node_addr, 0, node_config.node_addr};
    stp_nexthop = NO_NEXTHOP;
    timer = 0;

    logger_init(false, node_config.node_addr);
    print("node initialized, %d neighbors", node_config.num_neighbors);
}

void free_node() {
    free(neighbor_addrs);
    free(port_open);
    free(dist_to_root);
    logger_end();
}

// send STP packets to all neighbors
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
            nsent++;
        }
    }

    return nsent;
}

// received an STP packet, returns 0 on success
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
        if (stp_hello() < 0) {
            fprintf(stderr, "stp_recv's hello relay error\n");
            return -1;
        }

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

// send hello message if this is a root, otherwise decide if a reelection is
// needed
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

void run_node(void *const handle, volatile bool *const keep_running,
              const struct mixnet_node_config c) {

    myhandle = handle;
    node_config = c;
    print("%d neighbors", node_config.num_neighbors);
    init_node();

    while (*keep_running) {
        int received = mixnet_recv(myhandle, &port_recv, &packet_recv_ptr);
        if (received > 0) {
            need_free = true;

            if (port_recv == node_config.num_neighbors) {
                switch (packet_recv_ptr->type) {
                case PACKET_TYPE_FLOOD:
                    print("received a FLOOD packet from user", port_recv);
                    if (stp_flood() < 0)
                        print_err("received from user, error in stp_flood");

                    break;
                case PACKET_TYPE_PING:
                case PACKET_TYPE_DATA:
                    break;
                default:
                    print_err("wrong packet type received");
                }
            } else {
                switch (packet_recv_ptr->type) {
                case PACKET_TYPE_STP:
                    print("received a STP packet from port %d(node %d)",
                          port_recv, neighbor_addrs[port_recv]);
                    if (stp_recv(
                            (mixnet_packet_stp *)packet_recv_ptr->payload) < 0)
                        print_err("error in stp_recv");
                    break;
                case PACKET_TYPE_LSA:
                    // TODO: update local link state, compute shortest paths,
                    //       and broadcast along the spanning tree
                    break;
                case PACKET_TYPE_FLOOD:
                    if (port_open[port_recv]) {
                        print("received a FLOOD packet from port %d(node %d)",
                              port_recv, neighbor_addrs[port_recv]);
                        if (send_to_user() < 0)
                            print_err("error in send_to_user");
                        if (stp_flood() < 0)
                            print_err(
                                "received from neighbors, error in stp_flood");
                    } else {
                        print("received a FLOOD packet from port %d(node %d), "
                              "ignored",
                              port_recv, neighbor_addrs[port_recv]);
                    }

                    break;
                case PACKET_TYPE_PING:
                case PACKET_TYPE_DATA:
                    break;
                default:
                    print_err("wrong packet type received");
                }
            }

            if (need_free)
                free(packet_recv_ptr);
        }

        if (stp_check_timer() < 0)
            print_err("error in check_timer");
    }

    free_node();
}
