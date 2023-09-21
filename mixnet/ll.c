#include "ll.h"

#include <stdlib.h>

ll_node *create_ll_node(int cost, uint16_t node_addr);

ll_node *create_ll_node(int cost, uint16_t node_addr) {
    ll_node *new_ll_node = malloc(sizeof(ll_node));
    if (!new_ll_node) {
        return NULL;
    }
    new_ll_node->cost = cost;
    new_ll_node->node_addr = node_addr;
    new_ll_node->next = NULL;

    return new_ll_node;
}

list *init_list() {
    list *l = malloc(sizeof(list));
    if (!l) {
        return NULL;
    }
    l->head = NULL;

    return l;
}

void add_ll_node(int cost, uint16_t node_addr, list *l) {
    ll_node *current = NULL;
    if (l->head == NULL) {
        l->head = create_ll_node(cost, node_addr);
    } else {
        current = l->head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = create_ll_node(cost, node_addr);
    }
}

void free_ll(list *l) {
    ll_node *curr = l->head;
    ll_node *next = NULL;

    while (curr != NULL) {
        next = curr->next;
        free(curr);
        curr = next;
    }

    free(l);
}
