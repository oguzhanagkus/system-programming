#include "helper.h"
#include "queue.h"

int count = 0, finished = 0;
data_t *data = NULL;
order_t *temp = NULL;
queue_t **order_queues = NULL;
pthread_mutex_t *mutexes = NULL;
pthread_cond_t *cvs = NULL;
argument_t *arguments = NULL;
pthread_t *florists = NULL;

void free_all();
void *florist_function(void *arg); // Thread function

int main(int argc, char *argv[]) {
  char *input_path, line[128];
  int opt, input_fd, i, j, id, result;
  double distance, temp_distance;

  /* -------------------- */ // Command line arguments checking

  if (argc != 3)
    error_exit("Less/more arguments!\nUsage: ./program -i input_path", -1);

  /* -------------------- */ // Getopt parsing

  while ((opt = getopt(argc, argv, "i:")) != -1) {
    switch (opt) {
      case 'i':
        input_path = optarg; break;
      default:
        error_exit("Invalid options!\nUsage: ./program -i input_path", -1);
    }
  }

  printf("%s\n", input_path);

  /* -------------------- */ // Reading file

  if ((input_fd = open(input_path, O_RDONLY)) == -1)
    error_exit("Failed to open input path", errno);

  /* -------------------- */ // Reading data

  data = read_data(input_fd);
  if (data == NULL)
    error_exit("Allocation error!", -1);

  /* -------------------- */ // Creating queues and threads

  count = data->florist_count;
  order_queues = (queue_t **) malloc(count * sizeof(queue_t *));
  if (order_queues == NULL) {
    free_all();
    error_exit("Allocation error!", -1);
  }

  mutexes = (pthread_mutex_t *) malloc(count * sizeof(pthread_mutex_t));
  if (mutexes == NULL) {
    free_all();
    error_exit("Allocation error!", -1);
  }

  cvs = (pthread_cond_t *) malloc(count * sizeof(pthread_cond_t));
  if (cvs == NULL) {
    free_all();
    error_exit("Allocation error!", -1);
  }

  arguments = (argument_t *) malloc(count * sizeof(argument_t));
  if (arguments == NULL) {
    free_all();
    error_exit("Allocation error!", -1);
  }

  florists = (pthread_t *) malloc(count * sizeof(pthread_t));
  if (florists == NULL) {
    free_all();
    error_exit("Allocation error!", -1);
  }

  /* ----- */

  for (i = 0; i < count; i++) {
    order_queues[i] = create_queue();
    if (order_queues[i] == NULL) {
      free_all();
      error_exit("Allocation error!", -1);
    }
  }

  for (i = 0; i < count; i++) {
    if (pthread_mutex_init(&mutexes[i], NULL) != 0) {
      free_all();
      error_exit("pthread_mutex_init() error", -1);
    }
  }

  for (i = 0; i < count; i++) {
    if (pthread_cond_init(&cvs[i], NULL) != 0) {
      free_all();
      error_exit("pthread_cond_init() error", -1);
    }
  }

  for (i = 0; i < count; i++)
    arguments[i].id = i;

  for (i = 0; i < count; i++) {
    result = pthread_create(&florists[i], NULL, florist_function, &arguments[i]);
    if (result != 0) {
      free_all();
      error_exit("pthread_create() error", result);
    }
  }

  /* -------------------- */ // Adding to queues

  while ((read_line(input_fd, line)) != 0) {
    temp = read_order(data, line);
    distance = -1;

    for (j = 0; j < count; j++) {
      if (data->stock_table[j][temp->request] == 1) {
        temp_distance = cheb_distance(data->florists[j].location, temp->location);
        if (distance == -1 || temp_distance < distance) {
          distance = temp_distance;
          id = j;
        }
      }
    }

    pthread_mutex_lock (&mutexes[id]);
    enqueue(order_queues[id], temp);
    pthread_cond_signal (&cvs[id]);
    pthread_mutex_unlock (&mutexes[id]);
  }

  /* -------------------- */ // Joining

  for (i = 0; i < count; i++) {
    result = pthread_join(florists[i], NULL);
    if(result != 0)
      error_exit("pthread_join() error", result);
  }

  close(input_fd);
  free_all();
  return 0;
}

/* ---------------------------------------- */ // Free allocated blocks

void free_all() {
  if (florists != NULL)
    free(florists);
  if (arguments != NULL)
    free(arguments);
  if (cvs != NULL)
    free(cvs);
  if (mutexes != NULL)
    free(mutexes);
  if (order_queues != NULL)
    free_queues(order_queues, count);
  if (data != NULL)
    free_data(data);
}

/* ---------------------------------------- */ // Thread function

void *florist_function(void *arg) {
  argument_t *arguments  = (argument_t *) arg;
  int id = arguments->id;

  printf("Thread %d created.\n", id);
  return NULL;
}

/* ---------------------------------------- */

/*
  for (i = 0; i < count; i++) {
    printf("\n");
    while (order_queues[i]->count != 0) {
      temp = dequeue(order_queues[i]);
      printf("%s - (%.2lf,%.2lf) - %d\n", temp->client, temp->location.x, temp->location.y, temp->request);
      free(temp);
    }
  }
*/
