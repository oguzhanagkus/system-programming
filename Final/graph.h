#include "queue.h"

#ifndef GRAPH_H_
#define GRAPH_H_

#define VISITED   '1'
#define UNVISITED '0'

typedef struct graph_node {
  unsigned int id, adj_count, *adj_list;
} graph_node_t;

typedef struct graph {
  unsigned int node_count, edge_count;
  graph_node_t *nodes;
} graph_t;

typedef struct path {
  unsigned int from, to, distance, *nodes;
} path_t;

graph_t *create_graph(unsigned int node_count);
int extend_graph(graph_t *graph, unsigned int new_size);
int add_edge(graph_t *graph, unsigned int from, unsigned int to);
void delete_graph(graph_t *graph);

path_t *bfs(graph_t *graph, unsigned int from, unsigned int to);

void delete_path(path_t *path);

// void print_graph(graph_t *graph);
// void print_path(path_t *path);

#endif /* GRAPH_H_ */
