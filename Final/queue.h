#include <stdio.h>
#include <stdlib.h>

#ifndef QUEUE_H_
#define QUEUE_H_

typedef struct queue_node {
  unsigned int id;
  struct queue_node *next;
} queue_node_t;

typedef struct queue {
  unsigned int size;
  queue_node_t *front, *rear;
} queue_t;

queue_node_t *create_node(unsigned int id);
queue_t *create_queue();
int delete_queue(queue_t *queue);
int enqueue(queue_t *queue, unsigned int id);
int dequeue(queue_t *queue, unsigned int *id);

// void print_queue(queue_t *queue);

#endif /* QUEUE_H_ */
