#include "node.h"
#include "stp.h"
#include "lsa.h"
#include "attr.h"
#include "connection.h"
#include "logger.h"
#include "packet.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

// initialize the node, allocate memory and initialize values for arrays,
// and configure logger
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

    g = graph_init();
    shortest_paths = malloc(sizeof(path *) * MAX_NODES);
    memset(shortest_paths, 0, sizeof(path *) * MAX_NODES);
    stp_curr_state =
        (mixnet_packet_stp){node_config.node_addr, 0, node_config.node_addr};
    stp_nexthop = NO_NEXTHOP;
    timer = 0;

    logger_init(false, node_config.node_addr);
    print("node initialized, %d neighbors", node_config.num_neighbors);
}

// free arrays allocated to node and close log files
void free_node() {
    free(neighbor_addrs);
    free(port_open);
    free(dist_to_root);
    graph_free(g);
    for (int i = 0; i < MAX_NODES; ++i) {
        if (shortest_paths[i] != NULL) {
            free_path(shortest_paths[i]);
        }
    }
    free(shortest_paths);
    logger_end();
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
                    if (port_open[port_recv]) {
                        print("received a LSA packet from port %d(node %d)",
                              port_recv, neighbor_addrs[port_recv]);
        
                        if (lsa_update(
                                (mixnet_packet_lsa *)packet_recv_ptr->payload) <
                                0)
                            print_err(
                                "received from neighbors, error in lsa_update");
                    
                        if (lsa_flood() < 0)
                            print_err(
                                "received from neighbors, error in lsa_flood");
                    } else {
                        print("received a LSA packet from port %d(node %d), "
                              "ignored",
                              port_recv, neighbor_addrs[port_recv]);
                    }
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
