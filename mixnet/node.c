#include "node.h"
#include "attr.h"
#include "connection.h"
#include "logger.h"
#include "lsa.h"
#include "packet.h"
#include "routing.h"
#include "stp.h"
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

    stp_curr_state =
        (mixnet_packet_stp){node_config.node_addr, 0, node_config.node_addr};
    stp_nexthop = PORT_NULL;
    curr_mixing_count = 0;
    pending_packets =
        malloc(node_config.mixing_factor * sizeof(port_and_packet *));

    stp_hello(true);
    timer = get_timestamp(MILLISEC);
    srand(time(NULL)); // randomize the seed

    lsa_init();
    logger_init(false, node_config.node_addr);
    print("node initialized, %d neighbors", node_config.num_neighbors);
}

// free arrays allocated to node and close log files
void free_node() {
    free(neighbor_addrs);
    free(port_open);
    free(dist_to_root);

    lsa_free();

    for (int i = 0; i < node_config.mixing_factor; ++i) {
        if (pending_packets[i] != NULL) {
            free(pending_packets[i]);
        }
    }
    free(pending_packets);

    logger_end();
}

void run_node(void *const handle, volatile bool *const keep_running,
              const struct mixnet_node_config c) {
    myhandle = handle;
    node_config = c;
    print("%d neighbors", node_config.num_neighbors);
    init_node();

    while (*keep_running) {
        port_recv = -1;
        packet_recv_ptr = NULL;
        int received = mixnet_recv(myhandle, &port_recv, &packet_recv_ptr);
        if (received > 0) {
            need_free = true;

            if (port_recv == node_config.num_neighbors) {
                // received from user
                switch (packet_recv_ptr->type) {
                case PACKET_TYPE_FLOOD:
                    print("received a FLOOD packet from user");
                    if (stp_flood() < 0) {
                        print_err("received from user, error in stp_flood");
                    }
                    break;
                case PACKET_TYPE_PING:
                    if (ping_recv((mixnet_packet_routing_header *)
                                      packet_recv_ptr->payload) < 0) {
                        print_err("received from neighbors, error in "
                                  "ping_recv");
                    }
                    break;
                case PACKET_TYPE_DATA:
                    print("received a DATA packet to %d from user",
                          ((mixnet_packet_routing_header *)
                               packet_recv_ptr->payload)
                              ->dst_address);
                    data_recv((mixnet_packet_routing_header *)
                                  packet_recv_ptr->payload);
                    break;
                default:
                    print_err("wrong packet type received");
                }
            } else {
                // received from network
                switch (packet_recv_ptr->type) {
                case PACKET_TYPE_STP:
                    print("received a STP packet from port %d (node %d) with "
                          "root %d",
                          port_recv, neighbor_addrs[port_recv],
                          ((mixnet_packet_stp *)packet_recv_ptr->payload)
                              ->root_address);
                    if (stp_recv(
                            (mixnet_packet_stp *)packet_recv_ptr->payload) <
                        0) {
                        print_err("error in stp_recv");
                    }
                    break;
                case PACKET_TYPE_LSA:
                    if (port_open[port_recv]) {
                        print("received a LSA packet from port %d (node %d)",
                              port_recv, neighbor_addrs[port_recv]);

                        if (lsa_update(
                                (mixnet_packet_lsa *)packet_recv_ptr->payload) <
                            0) {
                            print_err(
                                "received from neighbors, error in lsa_update");
                        }

                        if (lsa_broadcast() < 0) {
                            print_err("received from neighbors, error in "
                                      "lsa_broadcast");
                        }
                    } else {
                        print("received a LSA packet from port %d (node %d), "
                              "ignored",
                              port_recv, neighbor_addrs[port_recv]);
                    }
                    break;
                case PACKET_TYPE_FLOOD:
                    if (port_open[port_recv]) {
                        print("node %d received a FLOOD packet from port %d "
                              "(node %d)",
                              node_config.node_addr, port_recv,
                              neighbor_addrs[port_recv]);
                        if (send_to_user() < 0) {
                            print_err("error in send_to_user");
                        }
                        if (stp_flood() < 0) {
                            print_err(
                                "received from neighbors, error in stp_flood");
                        }
                    } else {
                        print("received a FLOOD packet from port %d (node %d), "
                              "ignored",
                              port_recv, neighbor_addrs[port_recv]);
                    }
                    break;
                case PACKET_TYPE_PING:
                    if (((mixnet_packet_routing_header *)
                             packet_recv_ptr->payload)
                            ->dst_address == node_config.node_addr) {

                        if (ping_recv((mixnet_packet_routing_header *)
                                          packet_recv_ptr->payload) < 0) {
                            print_err("error in ping_recv");
                        }
                        if (send_to_user() < 0) {
                            print_err("error in send_to_user");
                        }
                    } else {
                        if (routing_forward(packet_recv_ptr->payload) < 0) {
                            print_err("received from neighbors, error in "
                                      "routing_forward");
                        }
                    }
                    break;
                case PACKET_TYPE_DATA:
                    print("received a DATA packet from %d to %d from port %d "
                          "(node %d)",
                          ((mixnet_packet_routing_header *)
                               packet_recv_ptr->payload)
                              ->src_address,
                          ((mixnet_packet_routing_header *)
                               packet_recv_ptr->payload)
                              ->dst_address,
                          port_recv, neighbor_addrs[port_recv]);
                    if (((mixnet_packet_routing_header *)
                             packet_recv_ptr->payload)
                            ->dst_address == node_config.node_addr) {
                        if (send_to_user() < 0) {
                            print_err("error in send_to_user");
                        }
                        print("sent DATA packet to user");
                    } else {
                        if (routing_forward(packet_recv_ptr->payload) < 0) {
                            print_err("received from neighbors, error in "
                                      "routing_forward");
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

        if (curr_mixing_count == node_config.mixing_factor) {
            print("curr_mixing_count=node_config.mixing_factor=%d, sending "
                  "pending packets",
                  curr_mixing_count, node_config.mixing_factor);
            if (send_all_pending_packets() < 0) {
                print_err("failed to send all pending packets");
            }
            curr_mixing_count = 0;
        }

        if (stp_check_timer() < 0) {
            print_err("error in stp_check_timer");
        }

        lsa_update_status();
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

int send_all_pending_packets() {
    for (int i = 0; i < node_config.mixing_factor; ++i) {
        if (mixnet_send_loop(myhandle, pending_packets[i]->port,
                             pending_packets[i]->packet) < 0) {
            return -1;
        }
        free(pending_packets[i]);
        pending_packets[i] = NULL;
    }

    return 0;
}
