#ifndef UTILS_H_
#define UTILS_H_

#include <stddef.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>
#include "stdbool.h"
#include "address.h"
#include "connection.h"

int mixnet_send_loop(void *handle, const uint8_t port, mixnet_packet *packet);
unsigned long get_timestamp();

typedef struct priority_queue_entry priority_queue_entry;
typedef struct priority_queue priority_queue;
priority_queue *pq_init(bool (*cmp)(int, int));
bool pq_empty(priority_queue *pq);
void pq_insert(priority_queue *pq, int key, int value);
priority_queue_entry pq_pop(priority_queue *pq);
void pq_free(priority_queue *pq);
bool less(int x, int y);
bool greater(int x, int y);

typedef struct node node;
typedef struct graph graph;
node *node_init(mixnet_address address);
void node_add_neighbor(node *n, mixnet_address neighbor_addr, int neighbor_cost);
graph *graph_init();
void graph_insert(graph *g, node *n);
node *graph_get(graph *g, mixnet_address addr);
void graph_free(graph *g);

#endif