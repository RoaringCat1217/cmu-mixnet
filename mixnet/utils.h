#include "address.h"
#include "stdbool.h"
#include <stdlib.h>
#include <string.h>

typedef struct priority_queue_entry priority_queue_entry;
typedef struct priority_queue priority_queue;
priority_queue *pq_init(bool (*cmp)(int, int));
bool pq_empty(priority_queue *pq);
void pq_insert(priority_queue *pq, int key, int value);
priority_queue_entry pq_pop(priority_queue *pq);
void pq_free(priority_queue *pq);
bool less(int x, int y);
bool greater(int x, int y);