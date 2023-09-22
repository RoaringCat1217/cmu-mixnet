#include "lsa.h"
#include "attr.h"
#include "logger.h"
#include "packet.h"
#include "utils.h"

#include <stdbool.h>
#include <string.h>

void lsa_send();

// init LSA states and structures ONLY AFTER neighbor discovery
void lsa_init() {
    shortest_paths = malloc(sizeof(path *) * MAX_NODES);
    memset(shortest_paths, 0, sizeof(path *) * MAX_NODES);
    g = graph_init();
    graph_add_node(g, node_config.node_addr);
    for (uint8_t port = 0; port < node_config.num_neighbors; port++)
        graph_add_edge(g, node_config.node_addr, neighbor_addrs[port],
                       node_config.link_costs[port]);
}

void lsa_free() {
    for (int i = 0; i < MAX_NODES; i++) {
        if (shortest_paths[i] != NULL) {
            path_free(shortest_paths[i]);
        }
    }
    free(shortest_paths);
    graph_free(g);
}

void lsa_send() {}

int lsa_update(mixnet_packet_lsa *lsa_packet) {
    int n = lsa_packet->neighbor_count;

    graph_add_node(g, lsa_packet->node_address);

    for (int i = 0; i < n; ++i) {
        mixnet_lsa_link_params link = lsa_packet->links[i];
        graph_add_edge(g, lsa_packet->node_address, link.neighbor_mixaddr,
                       link.cost);
    }

    if (g->open_edges == 0 && lsa_status == LSA_CONSTRUCT) {
        // graph is complete, run dijkstra and complete shortest_paths
        priority_queue *pq = pq_init(less);
        shortest_paths[node_config.node_addr] =
            path_init(node_config.node_addr);
        shortest_paths[node_config.node_addr]->total_cost = 0;
        shortest_paths[node_config.node_addr]->sendport = PORT_NULL;
        ll_append(shortest_paths[node_config.node_addr]->route,
                  node_config.node_addr);
        node *n = graph_get_node(g, node_config.node_addr);
        for (uint32_t i = 0; i < n->n_neighbors; i++)
            pq_insert(pq, n->costs[i], node_config.node_addr, n->neighbors[i]);
        while (!pq_empty(pq)) {
            priority_queue_entry front = pq_pop(pq);
            uint16_t cost = front.cost;
            mixnet_address from = front.from;
            mixnet_address to = front.to;
            if (shortest_paths[to] != NULL)
                continue;
            shortest_paths[to] = path_init(to);
            shortest_paths[to]->total_cost = cost;
            if (shortest_paths[from]->sendport == PORT_NULL) {
                // my direct neighbors
                for (uint8_t port = 0; port < node_config.num_neighbors;
                     port++) {
                    if (neighbor_addrs[port] == to) {
                        shortest_paths[to]->sendport = port;
                        break;
                    }
                }
            } else {
                shortest_paths[to]->sendport = shortest_paths[from]->sendport;
            }
            ll_copy(shortest_paths[to]->route, shortest_paths[from]->route);
            ll_append(shortest_paths[to]->route, to);
            // add new edges to priority queue
            node *added = graph_get_node(g, to);
            for (uint32_t i = 0; i < added->n_neighbors; i++) {
                if (shortest_paths[added->neighbors[i]] == NULL) {
                    pq_insert(pq, cost + added->costs[i], to,
                              added->neighbors[i]);
                }
            }
        }
        pq_free(pq);

        lsa_status = LSA_RUN;
    }

    return 0;
}

// send LSA packets along the spanning tree
int lsa_flood() {
    int ret = 0;

    for (uint8_t port_num = 0; port_num < node_config.num_neighbors;
         ++port_num) {
        if (port_num != port_recv && port_open[port_num]) {
            print("send lsa update to port %d(node %d)", port_num,
                  neighbor_addrs[port_num]);

            int n = node_config.num_neighbors;
            int header_size = sizeof(mixnet_packet);
            int lsa_size = sizeof(mixnet_packet_lsa);
            int lsa_link_size = sizeof(mixnet_lsa_link_params);
            int packet_size = header_size + lsa_size + n * lsa_link_size;

            void *sendbuf = malloc(packet_size);

            mixnet_packet *headerp = (mixnet_packet *)sendbuf;
            headerp->total_size = packet_size;
            headerp->type = PACKET_TYPE_LSA;

            mixnet_packet_lsa *payloadp =
                (mixnet_packet_lsa *)((char *)sendbuf + header_size);

            mixnet_packet_lsa *payload = malloc(lsa_size + n * lsa_link_size);

            payload->node_address = node_config.node_addr;
            payload->neighbor_count = node_config.num_neighbors;
            for (int i = 0; i < n; ++i) {
                payload->links[i].neighbor_mixaddr = neighbor_addrs[i];
                payload->links[i].cost =
                    graph_get_node(g, node_config.node_addr)
                        ->neighbors[neighbor_addrs[i]];
            }

            memcpy(payloadp, payload, lsa_size + n * lsa_link_size);

            free(payload);

            ret = mixnet_send_loop(myhandle, port_num, sendbuf);
            if (ret < 0) {
                return -1;
            }
        }
    }

    return 0;
}

// broadcast along the spanning tree
int lsa_broadcast() {
    int ret = 0;

    for (uint8_t port_num = 0; port_num < node_config.num_neighbors;
         ++port_num) {
        if (port_num != port_recv && port_open[port_num]) {
            print("forward lsa update to port %d(node %d)", port_num,
                  neighbor_addrs[port_num]);

            ret = mixnet_send_loop(myhandle, port_num, packet_recv_ptr);
            if (ret < 0) {
                return -1;
            }
        }
    }

    need_free = false;

    return 0;
}

void lsa_update_status() {
    if (lsa_status == LSA_NEIGHBOR_DISCOVERY) {
        bool flag = true;
        for (int i = 0; i < node_config.num_neighbors; ++i) {
            if (neighbor_addrs[i] == ADDR_UNK) {
                flag = false;
                break;
            }
        }

        if (flag) {
            lsa_status = LSA_INIT_AND_SEND;
        }
    }

    if (lsa_status == LSA_INIT_AND_SEND) {
        lsa_init();
        lsa_send();
        lsa_status = LSA_CONSTRUCT;
    }
}
