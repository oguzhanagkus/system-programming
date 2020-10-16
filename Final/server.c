#include "helper.h"

#define INITIAL_CACHE_SIZE 50

float load;
int working_count, thread_count, maximum_count, exit_flag;
int active_readers, active_writers, waiting_readers, waiting_writers;
int *thread_arguments = NULL;
queue_t *request_queue = NULL;
pthread_t *thread_pool = NULL;
pthread_t resizer_thread;
pthread_mutex_t load_mutex, queue_mutex, cache_mutex;
pthread_cond_t waiting_connection, available_thread, resize_condition;
pthread_cond_t ready_to_write, ready_to_read;

int lock_fd, log_fd, server_fd, temp_fd;
graph_t *graph = NULL;
cache_t *cache = NULL;

void *resizer_function(void *arg);
void *handler_function(void *arg);
void error_handler(char *message, int error);
void signal_handler(int signal);
void exit_handler();
void thread_handler();
void path_to_string(path_t *path, char *buffer);

int main(int argc, char *argv[]) {
  int port, start, maximum;
  int opt, optval, address_len, result, i;
  char *input_path, *log_path, buffer[4096];
  sigset_t block_set, previous_mask;

  struct sockaddr_in address;
  struct timeval start_time, end_time;

  exit_flag = 0;

  /* -------------------- */ // Command line arguments checking

  if (argc != 11)
    error_exit("Less/more arguments!\nUsage: sudo ./server -i input.txt -o log.txt -p 8080 -s 4 -x 24", -1);

  /* -------------------- */ // Getopt parsing

  while ((opt = getopt(argc, argv, "i:o:p:s:x:")) != -1) {
    switch (opt) {
      case 'i':
        input_path = optarg; break;
      case 'o':
        log_path = optarg; break;
      case 'p':
        port = atoi(optarg); break;
      case 's':
        start = atoi(optarg); break;
      case 'x':
        maximum = atoi(optarg); break;
      default:
        error_exit("Invalid options!\nUsage: sudo ./server -i input.txt -o log.txt -p 8080 -s 4 -x 24", -1);
    }
  }

  /* -------------------- */ // Argument checking

  if (port < 0 || port > 65535)
  error_exit("Port number should be in [0, 65535] interval.", -1);

  if (start < 2)
    error_exit("Number of threads in the pool at startup should be at least 2!", -1);

  if (start > maximum)
    error_exit("Number of threads in the pool at startup should be smaller than maximum number of the threads!", -1);

  if ((log_fd = open(log_path, O_WRONLY | O_CREAT, 0600)) < 0)
    error_exit("Failed while opening/creating log file", errno);

  close(log_fd);

  /* ------------------------------------------------------------ */ // Becoming daemon

  lock_fd = become_daemon();
  if (lock_fd == -1)
    error_exit("Error occured!", errno);

  /* -------------------- */ // Setting handler and disabling interrupt signal for a while

  signal(SIGINT, signal_handler);

  sigemptyset(&block_set);
  sigaddset(&block_set, SIGINT);

  if (sigprocmask(SIG_BLOCK, &block_set, &previous_mask) == -1)
    exit(EXIT_FAILURE);

  /* -------------------- */ // Preparing needed stuff

  if ((log_fd = open(log_path, O_WRONLY)) < 0)
    exit(EXIT_FAILURE);

  if (lseek(log_fd, 0, SEEK_END) < 0)
    exit(EXIT_FAILURE);

  write_log(log_fd, "---------------> Server started! <---------------");
  write_log(log_fd, "Executing with parameters:");
  sprintf(buffer, "-i %s", input_path);
  write_log(log_fd, buffer);
  sprintf(buffer, "-p %d", port);
  write_log(log_fd, buffer);
  sprintf(buffer, "-o %s", log_path);
  write_log(log_fd, buffer);
  sprintf(buffer, "-s %d", start);
  write_log(log_fd, buffer);
  sprintf(buffer, "-x %d", maximum);
  write_log(log_fd, buffer);
  write_log(log_fd, "Loading graph...");

  request_queue = create_queue();
  if (request_queue == NULL)
    error_handler("Request queue cannot created!", -1);

  gettimeofday(&start_time, NULL);

  if ((graph = read_graph(input_path)) == NULL)
    error_handler("Graph cannot be readed!", -1);

  gettimeofday(&end_time, NULL);
  sprintf(buffer, "Graph loaded in %lf seconds with %d nodes and %d edges.", get_duration(start_time, end_time), graph->node_count, graph->edge_count);
  write_log(log_fd, buffer);

  if ((cache = create_cache(INITIAL_CACHE_SIZE)) == NULL)
    error_handler("Cache cannot be created!", -1);

  /* -------------------- */ // Preparing needed syncronization items

  active_readers = 0;
  active_writers = 0;
  waiting_readers = 0;
  waiting_writers = 0;
  working_count = 0;
  thread_count = start;
  maximum_count = maximum;

  /* ---------- */ // Mutex initialization

  result = pthread_mutex_init(&load_mutex, NULL);
  if (result != 0)
  error_handler("pthread_mutex_init() error", result);

  result = pthread_mutex_init(&queue_mutex, NULL);
  if (result != 0)
    error_handler("pthread_mutex_init() error", result);

  result = pthread_mutex_init(&cache_mutex, NULL);
  if (result != 0)
    error_handler("pthread_mutex_init() error", result);

  /* ---------- */ // Condition variable initialization

  result = pthread_cond_init(&waiting_connection, NULL);
  if (result != 0)
  error_handler("pthread_cond_init() error", result);

  result = pthread_cond_init(&available_thread, NULL);
  if (result != 0)
  error_handler("pthread_cond_init() error", result);

  result = pthread_cond_init(&resize_condition, NULL);
  if (result != 0)
  error_handler("pthread_cond_init() error", result);

  result = pthread_cond_init(&ready_to_write, NULL);
  if (result != 0)
    error_handler("pthread_cond_init() error", result);

  result = pthread_cond_init(&ready_to_read, NULL);
  if (result != 0)
    error_handler("pthread_cond_init() error", result);

  /* ---------- */ // Thread pool initialization

  thread_arguments = malloc(maximum_count * sizeof(int));
  if (thread_arguments == NULL)
    error_handler("malloc() failed!", -1);

  for (i = 0; i < maximum_count; i++)
    thread_arguments[i] = i;

  thread_pool = malloc(maximum_count * sizeof(pthread_t));
  if (thread_pool == NULL)
    error_handler("Thread pool cannot created!", -1);

  for (i = 0; i < thread_count; i++) {
    result = pthread_create(&thread_pool[i], NULL, handler_function, &thread_arguments[i]);
    if (result != 0)
      error_handler("Thread pool cannot created", result);
  }

  /* ---------- */ // Resizer thread initialization

  result = pthread_create(&resizer_thread, NULL, resizer_function, NULL);
  if (result != 0)
    error_handler("Resizer thread cannot created", result);

  /* -------------------- */ // Preparing socket

  optval = 1;
  address_len = sizeof(address);
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port);

  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    error_handler("socket() failed", errno);

  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &optval, sizeof(optval)) == -1)
    error_handler("setsockopt() failed", errno);

  if (bind(server_fd, (struct sockaddr *) &address, sizeof(address)) == -1)
    error_handler("bind() failed", errno);

  if (listen(server_fd, 5) == -1)
    error_handler("listen() failed", errno);

  /* -------------------- */ // Enabling interrupt signal

  if (sigprocmask(SIG_SETMASK, &previous_mask, NULL) == -1)
    error_handler("sigprocmask() failed", errno);

  /* -------------------- */ // Accepting incoming connections

  while (1) {
    if ((temp_fd = accept(server_fd, (struct sockaddr *) &address, (socklen_t *) &address_len)) == -1)
      error_handler("accept() failed", errno);

    pthread_mutex_lock(&load_mutex);
    while (working_count == thread_count) {
      write_log(log_fd, "No thread is available! Waiting for one.");
      pthread_cond_signal(&resize_condition);
      pthread_cond_wait(&available_thread, &load_mutex);
    }
    pthread_mutex_unlock(&load_mutex);

    pthread_mutex_lock(&queue_mutex);
    enqueue(request_queue, temp_fd);
    pthread_cond_signal(&waiting_connection);
    pthread_mutex_unlock(&queue_mutex);
  }

  /* -------------------- */

  exit_handler();
  exit(EXIT_SUCCESS);
}

/* ---------------------------------------- */

void *resizer_function(void *arg) {
  char buffer[256];
  int i, new_size;
  void *stuff;

  stuff = arg;
  arg = stuff;

  while (1) {
    pthread_mutex_lock(&load_mutex);
    while (load < 75 && thread_count < maximum_count && exit_flag == 0)
      pthread_cond_wait(&resize_condition, &load_mutex);

    if (exit_flag == 1) {
      pthread_mutex_unlock(&load_mutex);
      break;
    }

    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

    new_size = round(thread_count * 1.25);
    if (new_size > maximum_count)
      new_size = maximum_count;

    /* ---------- */

    for (i = thread_count; i < new_size; i++)
      pthread_create(&thread_pool[i], NULL, handler_function, &thread_arguments[i]);

    /* ---------- */

    sprintf(buffer, "System load %.2f%%, pool extended to %d threads", load, new_size);
    write_log(log_fd, buffer);

    thread_count = new_size;
    load = (float) working_count / (float) thread_count * 100;

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

    pthread_cond_signal(&available_thread);
    pthread_mutex_unlock(&load_mutex);

    if (thread_count == maximum_count)
      break;
  }

  return NULL;
}

/* ---------------------------------------- */

void *handler_function(void *arg) {
  path_t *path;
  request_t request;
  int id, flag, fd;
  char buffer[4096], path_buffer[4096];

  flag = 1;

  if (arg == NULL)
    id = 0;
  else
    memcpy(&id, arg, sizeof(int));

  sprintf(buffer, "Thread #%d: Waiting for connection...", id);
  write_log(log_fd, buffer);

  while (1) {
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

    /* -------------------- */ // Getting a request from queue

    pthread_mutex_lock(&queue_mutex);
    while (request_queue->size == 0  && exit_flag == 0)
      pthread_cond_wait(&waiting_connection, &queue_mutex);

    if (exit_flag == 1) {
      pthread_mutex_unlock(&queue_mutex);
      break;
    }

    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

    /* ---------- */ // Updating system load

    pthread_mutex_lock(&load_mutex);
    working_count += 1;
    load = (float) working_count / (float) thread_count * 100;

    sprintf(buffer, "A connection has been delegated to thread id #%d, system load: %.2f%%", id, load);
    write_log(log_fd, buffer);

    pthread_cond_signal(&resize_condition);
    pthread_mutex_unlock(&load_mutex);

    /* ---------- */ // Dequeuing

    dequeue(request_queue, (unsigned int *) &fd);
    pthread_mutex_unlock(&queue_mutex);

    /* -------------------- */ // Reading request

    if (read(fd, (void *) &request, sizeof(request_t)) == -1) {
      close(fd);
      continue;
    }

    flag = 1;

    /* -------------------- */ // Accessing cache as a reader

    pthread_mutex_lock(&cache_mutex);
    while ((active_writers + waiting_readers) > 0) {
      waiting_readers += 1;
      pthread_cond_wait(&ready_to_read, &cache_mutex);
      waiting_readers -= 1;
    }
    active_readers += 1;
    pthread_mutex_unlock(&cache_mutex);

    sprintf(buffer, "Thread #%d: Searching database for a path from node %d to node %d", id, request.from, request.to);
    write_log(log_fd, buffer);
    path = search_in_cache(cache, request);

    pthread_mutex_lock(&cache_mutex);
    active_readers -= 1;
    if (active_readers == 0 && waiting_writers > 0)
      pthread_cond_signal(&ready_to_write);
    pthread_mutex_unlock(&cache_mutex);

    /* -------------------- */ // Performing breadth first search

    if (path == NULL) {
      sprintf(buffer, "Thread #%d: No path in database, calculating %d->%d", id, request.from, request.to);
      write_log(log_fd, buffer);

      path = bfs(graph, request.from, request.to);
      flag = 0;
    }

    /* -------------------- */ // Replying to client

    if (path->distance != 0) {
      path_to_string(path, path_buffer);

      if (flag == 1)
        sprintf(buffer, "Thread #%d: Path found in database: %s", id, path_buffer);
      else
        sprintf(buffer, "Thread #%d: Path calculated: %s", id, path_buffer);
    }
    else {
      sprintf(buffer, "Thread #%d: Path not possible from node %d to %d", id, request.from, request.to);
    }

    write_log(log_fd, buffer);

    /* ---------- */ // Send path size to client

    if (send(fd, (void *) &path->distance, sizeof(int), 0) == -1) {
      close(fd);
      continue;
    }

    sprintf(buffer, "Thread #%d: Responding to client", id);
    write_log(log_fd, buffer);

    /* ---------- */ // If there is path, send it

    if (path->distance != 0) {
      if (send(fd, (void *) path->nodes, (path->distance + 1) * sizeof(unsigned int), 0) == -1) {
        close(fd);
        continue;
      }
    }

    /* -------------------- */ // Accessing cache as a writer

    if (flag == 0) {
      pthread_mutex_lock(&cache_mutex);
      while ((active_writers + active_readers) > 0) {
        waiting_writers += 1;
        pthread_cond_wait(&ready_to_write, &cache_mutex);
        waiting_writers -= 1;
      }
      active_writers += 1;
      pthread_mutex_unlock(&cache_mutex);

      sprintf(buffer, "Thread #%d: Adding path to database", id);
      write_log(log_fd, buffer);
      add_to_cache(cache, path);

      pthread_mutex_lock(&cache_mutex);
      active_writers -= 1;
      if (waiting_writers > 0)
        pthread_cond_signal(&ready_to_write);
      else if (waiting_readers > 0)
        pthread_cond_signal(&ready_to_read);
      pthread_mutex_unlock(&cache_mutex);
    }

    // sleep(3);

    /* -------------------- */ // Updating system load

    pthread_mutex_lock(&load_mutex);
    working_count -= 1;
    load = (float) working_count / (float) thread_count * 100;
    pthread_cond_signal(&available_thread);
    pthread_mutex_unlock(&load_mutex);
  }

  return NULL;
}

/* ---------------------------------------- */

void error_handler(char *message, int error) {
  char buffer[256];

  if (error == -1)
    sprintf(buffer, "Error occurred: %s", message);
  else
    sprintf(buffer, "Error occurred: %s: %s", message, strerror(error));

  write_log(log_fd, buffer);
  write_log(log_fd, "Exited with failure.");

  exit_handler();
  exit(EXIT_FAILURE);
}

/* ---------------------------------------- */

void signal_handler(int signal) {
  char buffer[256];

  sprintf(buffer, "%s signal received.", sys_siglist[signal]);
  write_log(log_fd, buffer);

  exit_handler();
  exit(EXIT_SUCCESS);
}

/* ---------------------------------------- */

void exit_handler() {
  thread_handler();

  free(thread_arguments);
  free(thread_pool);
  delete_queue(request_queue);
  delete_cache(cache);
  delete_graph(graph);

  write_log(log_fd, "All heap blocks were freed.");
  write_log(log_fd, "Server shutting down.");

  close(server_fd);
  close(log_fd);
  close(lock_fd);
  delete_lock();
}

/* ---------------------------------------- */

void thread_handler() {
  int i;

  exit_flag = 1;

  write_log(log_fd, "Waiting for ongoing threads to complete.");

  if (thread_pool != NULL) {
    pthread_cond_broadcast(&waiting_connection);

    for (i = 0; i < thread_count; i++)
      pthread_join(thread_pool[i], NULL);
  }

  pthread_cond_signal(&resize_condition);
  pthread_join(resizer_thread, NULL);

  write_log(log_fd, "All threads have terminated.");

/*
  pthread_mutex_destroy(&load_mutex);
  pthread_mutex_destroy(&queue_mutex);
  pthread_mutex_destroy(&cache_mutex);

  pthread_cond_destroy(&waiting_connection);
  pthread_cond_destroy(&available_thread);
  pthread_cond_destroy(&resize_condition);
  pthread_cond_destroy(&ready_to_write);
  pthread_cond_destroy(&ready_to_read);
*/
}

/* ---------------------------------------- */

void path_to_string(path_t *path, char *buffer) {
  unsigned int i;

  sprintf(buffer, "%d->", path->nodes[0]);
  for (i = 1; i < path->distance; i++)
    sprintf(buffer, "%s%d->", buffer, path->nodes[i]);
  sprintf(buffer, "%s%d", buffer, path->nodes[i]);
}

/* ---------------------------------------- */
