#ifndef LL_H_
#define LL_H_

#include "address.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct ll_node {
    mixnet_address node_addr;
    struct ll_node *next;
} ll_node;

typedef struct linkedlist {
    ll_node *head, *tail;
    uint32_t size;
} linkedlist;

linkedlist *ll_init();
void ll_copy(linkedlist *dst, linkedlist *src);
void ll_append(linkedlist *ll, mixnet_address addr);
void ll_free(linkedlist *ll);
void ll_print(char *dst, ll_node *begin, ll_node *end);

#endif
