#include "utils.h"

#define INIT_CAPACITY 16
#define MAX_PORTS 256
#define MAX_NODES (1 << 16)

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

// get current timestamp in milliseconds
unsigned long get_timestamp() {
    struct timeval te;
    gettimeofday(&te, NULL);
    unsigned long ms = te.tv_sec * 1000UL + te.tv_usec / 1000UL;

    return ms;
}

// priority queue

struct priority_queue_entry{
    int key;
    int value;
} ;

struct priority_queue{
    bool (*cmp)(int, int);
    uint32_t size;
    uint32_t capacity;
    priority_queue_entry *data;
} ;

priority_queue *pq_init(bool (*f)(int, int)) {
    priority_queue *pq = (priority_queue *)malloc(sizeof(priority_queue));
    pq->cmp = f;
    pq->size = 0;
    pq->capacity = INIT_CAPACITY;
    pq->data = (priority_queue_entry *)malloc(sizeof(priority_queue_entry) * pq->capacity);
    return pq;
}

bool pq_empty(priority_queue *pq) {
    return pq->size > 0;
}

void pq_insert(priority_queue *pq, int key, int value) {
    if (pq->size + 1 > pq->capacity) {
        pq->capacity *= 2;
        priority_queue_entry *new_data = (priority_queue_entry *)malloc(sizeof(priority_queue_entry) * pq->capacity);
        memcpy(new_data, pq->data, pq->size * sizeof(priority_queue_entry));
        free(pq->data);
        pq->data = new_data;
    }
    pq->data[pq->size++] = (priority_queue_entry){key, value};
    uint32_t i = pq->size - 1, parent;
    while (i > 0) {
        parent = (i - 1) / 2;
        if (pq->cmp(pq->data[i].key, pq->data[parent].key)) {
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
            if (pq->cmp(pq->data[left].key, pq->data[i].key)) {
                priority_queue_entry tmp = pq->data[left];
                pq->data[left] = pq->data[i];
                pq->data[i] = tmp;
                break;
            }
        }
        if (pq->cmp(pq->data[left].key, pq->data[right].key)) {
            if (pq->cmp(pq->data[i].key, pq->data[left].key))
                break;
            else {
                priority_queue_entry tmp = pq->data[left];
                pq->data[left] = pq->data[i];
                pq->data[i] = tmp;
                i = left;
            }
        } else {
            if (pq->cmp(pq->data[i].key, pq->data[right].key))
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

bool less(int x, int y) {
    return x < y;
}

bool greater(int x, int y) {
    return x > y;
}

// node and graph
struct node {
    mixnet_address addr;
    uint32_t n_neighbors;
    mixnet_address neighbors[MAX_PORTS];
    int costs[MAX_PORTS];
};

struct graph {
    uint32_t n_nodes;
    node **nodes;
};

node *node_init(mixnet_address address) {
    node *n = (node *)malloc(sizeof(node));
    n->addr = address;
    n->n_neighbors = 0;
    return n;
}

void node_add_neighbor(node *n, mixnet_address neighbor_addr, int neighbor_cost) {
    n->neighbors[n->n_neighbors] = neighbor_addr;
    n->costs[n->n_neighbors] = neighbor_cost;
    n->n_neighbors++;
}

graph *graph_init() {
    graph *g = (graph *)malloc(sizeof(graph));
    g->n_nodes = 0;
    g->nodes = (node **)malloc(MAX_NODES * sizeof(node *));
    memset(g->nodes, 0, MAX_NODES * sizeof(node *));
    return g;
}

void graph_insert(graph *g, node *n) {
    g->nodes[n->addr] = n;
    g->n_nodes++;
}

node *graph_get(graph *g, mixnet_address addr) {
    return g->nodes[addr];
}

void graph_free(graph *g) {
    for (int i = 0; i < MAX_NODES; i++) {
        if (g->nodes[i] != NULL)
            free(g->nodes[i]);
    }
    free(g);
}