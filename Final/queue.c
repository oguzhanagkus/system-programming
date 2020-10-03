#include "queue.h"

/* ---------------------------------------- */

queue_node_t *create_node(unsigned int id) {
  queue_node_t *temp;

  temp = malloc(sizeof(queue_node_t));
  if (temp == NULL)
    return NULL;

  temp->id = id;
  temp->next = NULL;

  return temp;
}

/* ---------------------------------------- */

queue_t *create_queue() {
  queue_t *queue;

  queue = malloc(sizeof(queue_t));
  if (queue == NULL)
    return NULL;

  queue->size = 0;
  queue->front = NULL;
  queue->rear = NULL;

  return queue;
}

/* ---------------------------------------- */

int delete_queue(queue_t *queue) {
  unsigned int temp;

  if (queue == NULL)
    return -1;

  while (queue->size != 0) {
    if (dequeue(queue, &temp) == -1)
      break;
  }

  free(queue);

  return 0;
}

/* ---------------------------------------- */

int enqueue(queue_t *queue, unsigned int id) {
  queue_node_t *temp;

  temp = create_node(id);
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

  queue->size += 1;

  return 0;
}

/* ---------------------------------------- */

int dequeue(queue_t *queue, unsigned int *id) {
  queue_node_t *temp;

  if (queue->front == NULL)
    return -1;

  temp = queue->front;
  queue->front = queue->front->next;

  if (queue->front == NULL)
    queue->rear = NULL;

  queue->size -= 1;
  (*id) = temp->id;

  free(temp);

  return 0;
}

/* ---------------------------------------- */

// This function only used for testing

/*

void print_queue(queue_t *queue) {
  unsigned int i;
  queue_node_t *temp;

  temp = queue->front;

  for (i = 0; i < queue->size; i++) {
    printf("%d - ", temp->id);
    temp = temp->next;
  }

  printf("\n");
}

*/

/* ---------------------------------------- */
