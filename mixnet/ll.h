#include <stdint.h>

typedef struct ll_node {
    int cost;
    uint16_t node_addr;
    ll_node *next;
} ll_node;

typedef struct list {
    ll_node *head;
} list;

list *init_list();
void add_ll_node(int cost, uint16_t node_addr, list *l);
void free_ll(list *l);
