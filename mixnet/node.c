#include "node.h"
#include "stp.h"
#include "lsa.h"
#include "attr.h"
#include "connection.h"
#include "logger.h"
#include "packet.h"
#include "utils.h"
#include "routing.h"

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

    lsa_init();

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
   
    lsa_free();
   
    logger_end();
}

void run_node(void *const handle, volatile bool *const keep_running,
              const struct mixnet_node_config c) {

    myhandle = handle;
    node_config = c;
    print("%d neighbors", node_config.num_neighbors);
    init_node();

    // TODO: send lsa after neighbor discovering

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
                if (routing_mix() < 0) {
                    print_err("received from user, error in routing_mix");
                }
                if (routing_forward() < 0) {
                    print_err("received from user, error in routing_forward");
                }
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
                                0) {
                            print_err(
                                "received from neighbors, error in lsa_update");
                        }
                    
                        if (lsa_broadcast() < 0) {
                            print_err(
                                "received from neighbors, error in lsa_broadcast");
                        }
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
                        if (send_to_user() < 0) {
                            print_err("error in send_to_user");
                        }
                        if (stp_flood() < 0) {
                            print_err(
                                "received from neighbors, error in stp_flood");
                        }
                    } else {
                        print("received a FLOOD packet from port %d(node %d), "
                              "ignored",
                              port_recv, neighbor_addrs[port_recv]);
                    }
                    break;
                case PACKET_TYPE_PING:
                case PACKET_TYPE_DATA:
                uint16_t dest = 0;
                memset(dest, packet_recv_ptr->payload + 2, 2);
                
                if (dest == node_config.node_addr) {
                    send_to_user();
                } else {
                    if (routing_mix() < 0) {
                            print_err("received from neighbors, error in routing_mix");
                    }
                    if (routing_forward() < 0) {
                        print_err(
                            "received from neighbors, error in routing_forward");
                    }
                }
                    break;
                default:
                    print_err("wrong packet type received");
                }
            }

            if (need_free) {
                free(packet_recv_ptr);
            }
        }

        if (stp_check_timer() < 0)
            print_err("error in check_timer");
    }

    free_node();
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
