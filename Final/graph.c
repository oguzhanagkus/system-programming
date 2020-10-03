#include "graph.h"

/* ---------------------------------------- */

graph_t *create_graph(unsigned int node_count) {
  unsigned int i;
  graph_t *graph;

  graph = malloc(sizeof(graph_t));
  if (graph == NULL)
    return NULL;

  graph->node_count = node_count;
  graph->edge_count = 0;
  graph->nodes = malloc(node_count * sizeof(graph_node_t));
  if (graph->nodes == NULL)
    return NULL;

  for (i = 0; i < node_count; i++) {
    graph->nodes[i].id = i;
    graph->nodes[i].adj_count = 0;
    graph->nodes[i].adj_list = NULL;
  }

  return graph;
}

/* ---------------------------------------- */

int extend_graph(graph_t *graph, unsigned int new_size) {
  unsigned int current_size, i;
  graph_node_t *temp;

  current_size = graph->node_count;

  if (new_size < current_size) {
    return 0;
  }
  else {
    temp = realloc(graph->nodes, new_size * sizeof(graph_node_t));
    if (temp == NULL)
      return -1;

    for (i = current_size; i < new_size; i++) {
      temp[i].id = i;
      temp[i].adj_count = 0;
      temp[i].adj_list = NULL;
    }

    graph->nodes = temp;
    graph->node_count = new_size;
  }

  return 0;
}

/* ---------------------------------------- */

int add_edge(graph_t *graph, unsigned int from, unsigned int to) {
  unsigned int current_size, new_size, adj_count, *temp;

  current_size = graph->node_count;

  if (current_size <= from || current_size <= to) {
    if (from < to)
      new_size = to + 1;
    else
      new_size = from + 1;

    if (extend_graph(graph, new_size) == -1)
      return -1;
  }

  adj_count = graph->nodes[from].adj_count;
  if (adj_count == 0) {
    graph->nodes[from].adj_list = malloc(sizeof(unsigned int));
    if (graph->nodes[from].adj_list == NULL)
      return -1;
  }
  else {
    temp = realloc(graph->nodes[from].adj_list, (adj_count + 1) * sizeof(unsigned int));
    if (temp == NULL)
      return -1;

    graph->nodes[from].adj_list = temp;
  }

  graph->nodes[from].adj_list[adj_count++] = to;
  graph->nodes[from].adj_count = adj_count;
  graph->edge_count += 1;

  return 0;
}

/* ---------------------------------------- */

// This function only used for testing

/*

void print_graph(graph_t *graph) {
  unsigned int i, j;

  if (graph == NULL || graph->node_count == 0)
    return;

  printf("Node count: %d\nEdge Count: %d\n", graph->node_count, graph->edge_count);

  for (i = 0; i < graph->node_count; i++) {
    printf("Node %d - %d: ", i, graph->nodes[i].adj_count);
    for (j = 0; j < graph->nodes[i].adj_count; j++)
      printf("%d, ", graph->nodes[i].adj_list[j]);
    printf("\n");
  }
}

*/

/* ---------------------------------------- */

void delete_graph(graph_t *graph) {
  unsigned int i;

  if (graph == NULL)
    return;

  if (graph->nodes == NULL) {
    free(graph);
    return;
  }

  for (i = 0; i < graph->node_count; i++) {
    if (graph->nodes[i].adj_count > 0)
      free(graph->nodes[i].adj_list);
  }

  free(graph->nodes);
  free(graph);
}

/* ---------------------------------------- */

path_t *bfs(graph_t *graph, unsigned int from, unsigned int to) {
  if (graph == NULL)
    return NULL;

  char *visited;
  path_t *path;
  queue_t *queue;
  unsigned int i, j, temp, found, node_count, *parent, *distance;

  found = 0;
  node_count = graph->node_count;

  path = malloc(sizeof(path_t));
  path->from = from;
  path->to = to;
  path->distance = 0;
  path->nodes = NULL;

  if (from > node_count || to > node_count)
    return path;

  parent = malloc(node_count * sizeof(unsigned int));
  distance = malloc(node_count * sizeof(unsigned int));
  visited = malloc(node_count * sizeof(char));
  queue = create_queue();

  for (i = 0; i < node_count; i++) {
    distance[i] = 0;
    visited[i] = UNVISITED;
  }

  enqueue(queue, from);
  parent[from] = from;
  visited[from] = VISITED;

  while (queue->size > 0 && !found) {
    dequeue(queue, &temp);

    for (i = 0; i < graph->nodes[temp].adj_count; i++) {
      j = graph->nodes[temp].adj_list[i];

      if (visited[j] == UNVISITED) {
        enqueue(queue, j);
        parent[j] = temp;
        distance[j] = distance[temp] + 1;
        visited[j] = VISITED;

        if (j == to) {
          found = 1;
          break;
        }
      }
    }
  }

  i = distance[to];

  if (found == 1) {
    path->distance = i;
    path->nodes = malloc((i + 1) * sizeof(unsigned int));

    temp = parent[to];
    path->nodes[i--] = to;

    while (temp != from) {
      path->nodes[i--] = temp;
      temp = parent[temp];
    }

    path->nodes[i] = temp;
  }

  free(parent);
  free(distance);
  free(visited);
  delete_queue(queue);

  return path;
}

/* ---------------------------------------- */

// This function only used for testing

/*

void print_path(path_t *path) {
  unsigned int i;

  if (path == NULL || path->distance == 0) {
    printf("No path!\n");
    return;
  }

  printf("From %d to %d: ", path->from, path->to);

  for (i = 0; i < path->distance; i++)
    printf("%d -> ", path->nodes[i]);

  printf("%d\n", path->nodes[i]);
}

*/

/* ---------------------------------------- */

void delete_path(path_t *path) {
  if (path == NULL)
    return;

  if (path->nodes != NULL)
    free(path->nodes);

  free(path);
}

/* ---------------------------------------- */
