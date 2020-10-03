#include "helper.h"

/* ---------------------------------------- */

int become_daemon() {
  int lock_fd, temp_fd, max_fd;

  switch (fork()) {                                       // Become background process
    case -1: return -1;                                   // Error
    case  0: break;                                       // Child continues
    default: exit(EXIT_SUCCESS);                          // Parent terminates
  }

  if (setsid() == -1)                                     // Become leader of new session
    return -1;

  switch (fork()) {                                       // Ensure we are not session leader
    case -1: return -1;                                   // Error
    case  0: break;                                       // Child continues
    default: exit(EXIT_SUCCESS);                          // Parent terminates
  }

  max_fd = sysconf(_SC_OPEN_MAX);                         // Maximum number of open file descriptors
  if (max_fd == -1)
      max_fd = 8192;

  for (temp_fd = 0; temp_fd < max_fd; temp_fd++)          // Close all open file descriptors
    close(temp_fd);

  umask(0);                                               // Clear file mode creation mask
  // chdir("/");                                          // Change to root directory
  lock_fd = create_lock();                                // Create lock to ensure running single instance

  return lock_fd;
}

/* ---------------------------------------- */

int create_lock() {
  int fd, count;
  char buffer[16];
  char *file_directory = "/.config/server.pid";
  char *home_directory = getenv("HOME");
  char *lock_directory = malloc(strlen(home_directory) + strlen(file_directory) + 1);

  strncpy(lock_directory, home_directory, strlen(home_directory) + 1);
  strncat(lock_directory, file_directory, strlen(file_directory) + 1);

  fd = open(lock_directory, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
  if (fd == -1)
    return -1;

  free(lock_directory);

  struct flock lock;
  lock.l_type = F_WRLCK;
  lock.l_whence = SEEK_SET;
  lock.l_start = 0;
  lock.l_len = 0;

  if (fcntl(fd, F_SETLK, &lock) == -1) {
    close(fd);
    return -1;
  }

  if (ftruncate(fd, 0) == -1) {
    close(fd);
    return -1;
  }

  count = sprintf(buffer, "%d", getpid());
  write(fd, buffer, count);

  return fd;
}

/* ---------------------------------------- */

void delete_lock() {
  char *file_directory = "/.config/server.pid";
  char *home_directory = getenv("HOME");
  char *lock_directory = malloc(strlen(home_directory) + strlen(file_directory) + 1);

  strncpy(lock_directory, home_directory, strlen(home_directory) + 1);
  strncat(lock_directory, file_directory, strlen(file_directory) + 1);

  unlink(lock_directory);
  free(lock_directory);
}

/* ---------------------------------------- */

graph_t *read_graph(char *input_path) {
  int fd, flag, index, ignore, from, to;
  char read_buffer, buffer[16];
  graph_t *graph;

  flag = 0;
  index = 0;
  ignore = 0;
  memset(buffer, 0, 16);

  if ((fd = open(input_path, O_RDONLY)) == -1)
    exit(1);

  graph = create_graph(10);

  if (graph == NULL)
    return NULL;

  while (read(fd, &read_buffer, 1) == 1) {
    if (ignore == 1) {
      if (read_buffer == '\n')  // Ignore until \n
        ignore = 0;
      continue;
    }

    if (read_buffer == '#') {   // If there is a # ignore until \n
      ignore = 1;
      continue;
    }

    if (read_buffer == ' ' || read_buffer == '\t' || read_buffer == '\n') {
      if (flag == 0) {
        from = atoi(buffer);
        flag = 1;
      }
      else {
        to = atoi(buffer);
        flag = 0;

        if (add_edge(graph, from, to) == -1) {
          delete_graph(graph);
          return NULL;
        }
      }

      memset(buffer, 0, 16);
      index = 0;
    }
    else
      buffer[index++] = read_buffer;
  }

  close(fd);

  return graph;
}

/* ---------------------------------------- */

cache_t *create_cache(int capacity) {
  int i;
  cache_t *cache;

  if (capacity < 1)
    return NULL;

  cache = malloc(sizeof(cache_t));
  if (cache == NULL)
    return NULL;

  cache->items = malloc(capacity * sizeof(path_t *));
  if (cache->items == NULL) {
    free(cache);
    return NULL;
  }

  for (i = 0; i < capacity; i++)
    cache->items[i] = NULL;

  cache->size = 0;
  cache->capacity = capacity;

  return cache;
}

/* ---------------------------------------- */

int extend_cache(cache_t *cache, int capacity) {
  int i;
  path_t **temp;

  if (cache == NULL)
    return -1;

  if (capacity <= cache->capacity)
    return -1;

  temp = realloc(cache->items, capacity * sizeof(path_t *));
  if (temp == NULL)
    return -1;

  cache->items = temp;
  cache->capacity = capacity;

  for (i = cache->size; i < capacity; i++)
    cache->items[i] = NULL;


  return 0;
}

/* ---------------------------------------- */

int add_to_cache(cache_t *cache, path_t *path) {
  if (cache == NULL)
    return -1;

  if (cache->size == cache->capacity) {
    if (extend_cache(cache, 2 * cache->capacity) == -1)
      return -1;
  }

  cache->items[cache->size] = path;
  cache->size += 1;

  return 0;
}

/* ---------------------------------------- */

path_t *search_in_cache(cache_t *cache, request_t request) {
  int i;

  if (cache == NULL)
    return NULL;

  for (i = 0; i < cache->size; i++) {
    if (cache->items[i]->from == request.from && cache->items[i]->to == request.to)
      return cache->items[i];
  }

  return NULL;
}

/* ---------------------------------------- */

void delete_cache(cache_t *cache) {
  int i;

  if (cache == NULL)
    return;

  if (cache->items == NULL) {
    free(cache);
    return;
  }

  for (i = 0; i < cache->size; i++)
    delete_path(cache->items[i]);

  free(cache->items);
  free(cache);
}

/* ---------------------------------------- */
