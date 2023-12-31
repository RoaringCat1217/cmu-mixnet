#ifndef UTILS_H_
#define UTILS_H_

#include "address.h"
#include "connection.h"
#include "ll.h"
#include "stdbool.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#define INT_MAX 0x70000000

#define MAX_PORTS (1 << 8)
#define MAX_NODES (1 << 16)

#define MICROSEC 0
#define MILLISEC 1

int mixnet_send_loop(void *handle, const uint8_t port, mixnet_packet *packet);
unsigned long get_timestamp(int unit);

typedef struct priority_queue_entry {
    uint16_t cost;
    mixnet_address from;
    mixnet_address to;
} priority_queue_entry;

typedef struct priority_queue {
    bool (*cmp)(priority_queue_entry, priority_queue_entry);
    uint32_t size;
    uint32_t capacity;
    priority_queue_entry *data;
} priority_queue;

priority_queue *pq_init(bool (*cmp)(priority_queue_entry,
                                    priority_queue_entry));
bool pq_empty(priority_queue *pq);
void pq_insert(priority_queue *pq, uint16_t cost, mixnet_address from,
               mixnet_address to);
priority_queue_entry pq_pop(priority_queue *pq);
void pq_free(priority_queue *pq);
bool less(priority_queue_entry x, priority_queue_entry y);

typedef struct node node;
typedef struct graph graph;
struct node {
    mixnet_address addr;
    uint32_t n_neighbors;
    mixnet_address neighbors[MAX_PORTS];
    uint16_t costs[MAX_PORTS];
};
struct graph {
    uint32_t n_nodes;
    node **nodes;
    int open_edges;
};
node *node_init(mixnet_address address);
graph *graph_init();
void graph_add_node(graph *g, mixnet_address addr);
int graph_add_edge(graph *g, mixnet_address from, mixnet_address to,
                   uint16_t cost);
node *graph_get_node(graph *g, mixnet_address addr);
void graph_free(graph *g);

typedef struct path {
    mixnet_address dest;
    uint8_t sendport;
    uint16_t total_cost;
    linkedlist *route; // include both the source and the destination
} path;

path *path_init(mixnet_address dest);
void path_free(path *p);

typedef struct port_and_packet {
    uint8_t port;
    mixnet_packet *packet;
} port_and_packet;

int find_port(mixnet_address target_addr, mixnet_address *neighbor_addrs,
              uint16_t num_neighbors);

#endif
