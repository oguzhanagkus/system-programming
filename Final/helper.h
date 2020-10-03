#include "graph.h"
#include "common.h"
#include <pthread.h>

#ifndef HELPER_H_
#define HELPER_H_

typedef struct cache {
  int size, capacity;
  path_t **items;
} cache_t;

int become_daemon();
int create_lock();
void delete_lock();

graph_t *read_graph(char *input_path);

cache_t *create_cache(int capacity);
int extend_cache(cache_t *cache, int capacity);
int add_to_cache(cache_t *cache, path_t *path);
path_t *search_in_cache(cache_t *cache, request_t request);
void delete_cache(cache_t *cache);

#endif /* HELPER_H_ */
