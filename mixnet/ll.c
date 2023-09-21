#include "ll.h"


linkedlist *ll_init(){
    linkedlist *ll = malloc(sizeof(linkedlist));
    ll->size = 0;
    ll->head = NULL;
    ll->tail = NULL;
    return ll;
}

void ll_append(linkedlist *ll, mixnet_address addr) {
    ll_node *n = malloc(sizeof(ll_node));
    n->node_addr = addr;
    n->next = NULL;
    if (ll->size == 0) {
        ll->head = ll->tail = n;
    } else {
        ll->tail->next = n;
        ll->tail = n;
    }
    ll->size++;
}

void ll_copy(linkedlist *dst, linkedlist *src) {
    if (dst->head != NULL) {
        ll_node *curr = dst->head;
        ll_node *next = NULL;

        while (curr != NULL) {
            next = curr->next;
            free(curr);
            curr = next;
        }
        dst->head = dst->tail = NULL;
    }
    dst->size = src->size;
    ll_node *curr = NULL;
    for (ll_node *n = src->head; n != NULL; n = n->next) {
        ll_node *copied = malloc(sizeof(ll_node));
        copied->node_addr = n->node_addr;
        copied->next = NULL;
        if (curr == NULL)
            dst->head = copied;
        else
            curr->next = copied;
        curr = copied;
    }
    dst->tail = curr;
}

void ll_free(linkedlist *ll){
    ll_node *curr = ll->head;
    ll_node *next = NULL;

    while (curr != NULL) {
        next = curr->next;
        free(curr);
        curr = next;
  }
  free(ll);
}
