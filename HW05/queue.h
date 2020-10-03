/* CSE344 - Systems Programming Course - HW05 - Threads and Mutexes

  Oğuzhan Agkuş - 161044003 */

#include "helper.h"

#ifndef QUEUE
#define QUEUE

typedef struct node {
  order_t *order;
  struct node *next;
} node_t;

typedef struct queue {
  size_t count;
  node_t *front, *rear;
} queue_t;

node_t *create_node(order_t *order);
queue_t *create_queue();
int enqueue(queue_t *q, order_t *order);
order_t *dequeue(queue_t *queue);
int delete_queue(queue_t *queue);
void free_queues(queue_t **queues, size_t count);

#endif /* QUEUE */
