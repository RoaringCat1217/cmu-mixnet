#include "utils.h"

#define INIT_CAPACITY 16

// repeatedly send a packet until success or error
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

// get current timestamp in milliseconds or microseconds
unsigned long get_timestamp(int unit) {
    struct timeval te;
    gettimeofday(&te, NULL);
    unsigned long ms = te.tv_sec * 1000UL + te.tv_usec / 1000UL;
    unsigned long us = te.tv_sec * 1000000UL + te.tv_usec;
    if (unit == MILLISEC)
        return ms;
    return us;
}

// priority queue

priority_queue *pq_init(bool (*f)(priority_queue_entry, priority_queue_entry)) {
    priority_queue *pq = (priority_queue *)malloc(sizeof(priority_queue));
    pq->cmp = f;
    pq->size = 0;
    pq->capacity = INIT_CAPACITY;
    pq->data = (priority_queue_entry *)malloc(sizeof(priority_queue_entry) *
                                              pq->capacity);
    return pq;
}

bool pq_empty(priority_queue *pq) {
    return pq->size > 0;
}

void pq_insert(priority_queue *pq, uint16_t cost, mixnet_address from,
               mixnet_address to) {
    if (pq->size + 1 > pq->capacity) {
        pq->capacity *= 2;
        priority_queue_entry *new_data = (priority_queue_entry *)malloc(
            sizeof(priority_queue_entry) * pq->capacity);
        memcpy(new_data, pq->data, pq->size * sizeof(priority_queue_entry));
        free(pq->data);
        pq->data = new_data;
    }
    pq->data[pq->size++] = (priority_queue_entry){cost, from, to};
    uint32_t i = pq->size - 1, parent;
    while (i > 0) {
        parent = (i - 1) / 2;
        if (pq->cmp(pq->data[i], pq->data[parent])) {
            priority_queue_entry tmp = pq->data[i];
            pq->data[i] = pq->data[parent];
            pq->data[parent] = tmp;
            i = parent;
        } else
            break;
    }
}

priority_queue_entry pq_pop(priority_queue *pq) {
    priority_queue_entry popped = pq->data[0];
    pq->data[0] = pq->data[--pq->size];
    uint32_t i = 0, left, right;
    while (true) {
        left = i * 2 + 1, right = i * 2 + 2;
        if (left >= pq->size)
            break;
        if (right >= pq->size) {
            if (pq->cmp(pq->data[left], pq->data[i])) {
                priority_queue_entry tmp = pq->data[left];
                pq->data[left] = pq->data[i];
                pq->data[i] = tmp;
                break;
            }
        }
        if (pq->cmp(pq->data[left], pq->data[right])) {
            if (pq->cmp(pq->data[i], pq->data[left]))
                break;
            else {
                priority_queue_entry tmp = pq->data[left];
                pq->data[left] = pq->data[i];
                pq->data[i] = tmp;
                i = left;
            }
        } else {
            if (pq->cmp(pq->data[i], pq->data[right]))
                break;
            else {
                priority_queue_entry tmp = pq->data[right];
                pq->data[right] = pq->data[i];
                pq->data[i] = tmp;
                i = right;
            }
        }
    }
    return popped;
}

void pq_free(priority_queue *pq) {
    free(pq->data);
    free(pq);
}

bool less(priority_queue_entry x, priority_queue_entry y) {
    if (x.cost != y.cost)
        return x.cost < y.cost;
    if (x.from != y.from)
        return x.from < y.from;
    return x.to < y.to;
}

// node and graph

node *node_init(mixnet_address address) {
    node *n = (node *)malloc(sizeof(node));
    n->addr = address;
    n->n_neighbors = 0;
    return n;
}

graph *graph_init() {
    graph *g = (graph *)malloc(sizeof(graph));
    g->n_nodes = 0;
    g->open_edges = 0;
    g->nodes = (node **)malloc(MAX_NODES * sizeof(node *));
    memset(g->nodes, 0, MAX_NODES * sizeof(node *));
    return g;
}

void graph_add_node(graph *g, mixnet_address addr) {
    if (g->nodes[addr] == NULL) {
        node *n = node_init(addr);
        g->nodes[addr] = n;
        g->n_nodes++;
    }
}

int graph_add_edge(graph *g, mixnet_address from, mixnet_address to,
                   uint16_t cost) {
    if (g->nodes[from] == NULL)
        return -1;
    node *n = g->nodes[from];
    n->neighbors[n->n_neighbors] = to;
    n->costs[n->n_neighbors] = cost;
    n->n_neighbors++;
    if (g->nodes[to] == NULL)
        g->open_edges++;
    else
        g->open_edges--;
    return 0;
}

node *graph_get_node(graph *g, mixnet_address addr) {
    return g->nodes[addr];
}

void graph_free(graph *g) {
    for (int i = 0; i < MAX_NODES; i++) {
        if (g->nodes[i] != NULL)
            free(g->nodes[i]);
    }
    free(g);
}

// path and link
path *path_init(mixnet_address dest) {
    path *p = malloc(sizeof(path));
    p->dest = dest;
    p->total_cost = UINT16_MAX;
    p->route = ll_init();
    p->sendport = -1;
    return p;
}

void path_free(path *p) {
    ll_free(p->route);
    free(p);
}
