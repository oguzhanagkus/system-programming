/* CSE344 - Systems Programming Course - HW05 - Threads and Mutexes

  Oğuzhan Agkuş - 161044003 */

#include "queue.h"

/* ---------------------------------------- */

node_t *create_node(order_t *order) {
  node_t *temp;

  temp = (node_t *) malloc(sizeof(node_t));
  if (temp == NULL)
    return NULL;

  temp->order = order;
  temp->next = NULL;

  return temp;
}

/* ---------------------------------------- */

queue_t *create_queue() {
  queue_t *queue;

  queue = (queue_t *) malloc(sizeof(queue_t));
  if (queue == NULL)
    return NULL;

  queue->count = 0;
  queue->front = NULL;
  queue->rear = NULL;

  return queue;
}

/* ---------------------------------------- */

int enqueue(queue_t *queue, order_t *order) {
  node_t *temp;

  temp = create_node(order);
  if (temp == NULL)
    return -1;

  if (queue->rear == NULL) {
    queue->front = temp;
    queue->rear = temp;
  }
  else {
    queue->rear->next = temp;
    queue->rear = temp;
  }

  queue->count += 1;

  return 0;
}

/* ---------------------------------------- */

order_t *dequeue(queue_t *queue) {
  node_t *temp;
  order_t *order;

  if (queue->front == NULL)
    return NULL;

  temp = queue->front;
  queue->front = queue->front->next;

  if (queue->front == NULL)
    queue->rear = NULL;

  queue->count -= 1;
  order = temp->order;

  free(temp);
  return order;
}

/* ---------------------------------------- */

int delete_queue(queue_t *queue) {
  order_t *temp;

  if (queue == NULL)
    return -1;

  while (queue->count != 0) {
    temp = dequeue(queue);
    free(temp);
  }

  free(queue);

  return 0;
}

/* ---------------------------------------- */

void free_queues(queue_t **queues, size_t count) {
  size_t i;

  for (i = 0; i < count; i++)
    delete_queue(queues[i]);
  free(queues);
}

/* ---------------------------------------- */
