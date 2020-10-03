/* CSE344 - Systems Programming Course - HW05 - Threads and Mutexes

  Oğuzhan Agkuş - 161044003 */

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
report_t *reports = NULL;

void free_all();
void *florist_function(void *arg); // Thread function
void sigint_handler(int signal); // Interrupt handler

int main(int argc, char *argv[]) {
  char *input_path, line[128];
  int opt, input_fd, i, j, id, result;
  double distance, temp_distance;

  srand(time(0)); // Random preparation time
  signal(SIGINT, sigint_handler); // Interrupt handler

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

  printf("Florist application initializing from file: %s\n", input_path);
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

  reports = (report_t *) malloc(count * sizeof(report_t));
  if (reports == NULL) {
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

    pthread_mutex_lock(&mutexes[i]);
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

  printf("%d florists have been created.\n", count);

  /* -------------------- */ // Adding to queues

  printf("Processing requests...\n\n");
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

    enqueue(order_queues[id], temp);
    if (order_queues[id]->count == 0)
      pthread_mutex_unlock(&mutexes[id]);
  }

  finished = 1;
  for (i = 0; i < count; i++)
    pthread_mutex_unlock(&mutexes[i]);

  /* -------------------- */ // Joining

  for (i = 0; i < count; i++) {
    result = pthread_join(florists[i], NULL);
    if(result != 0)
      error_exit("pthread_join() error", result);
  }

  printf("All requests proccessed.\n");
  print_reports(reports, count);

  free_all();
  close(input_fd);
  exit(EXIT_SUCCESS);
}

/* ---------------------------------------- */ // Free allocated blocks

void free_all() {
  if (reports != NULL)
    free(reports);
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
  order_t *temp;
  int id = arguments->id, count = 0;
  double distance, time, total_time = 0;

  while (finished == 0 || order_queues[id]->count > 0) {
    pthread_mutex_lock(&mutexes[id]);

    if (finished == 1 && order_queues[id]->count == 0)
      break;

    temp = dequeue(order_queues[id]);

    if (order_queues[id]->count > 0)
      pthread_mutex_unlock(&mutexes[id]);

    /* ----- */

    distance = cheb_distance(data->florists[id].location, temp->location);
    time = rand() % 250 + 1;
    time += distance / data->florists[id].speed;
    total_time += time;

    usleep((int) time * 1000);
    printf("Florist %s has delivered a %s to %s in %.0lfms.\n", data->florists[id].name, flower_type(data, temp->request), temp->client, time);
    free(temp);

    count += 1;
  }

  strcpy(reports[id].name, data->florists[id].name);
  reports[id].delivered = count;
  reports[id].time = total_time;

  printf("%s is closing shop!\n", reports[id].name);

  return NULL;
}

/* ---------------------------------------- */

void sigint_handler(int signal) {
  int i;

  printf("\n%s signal caught! Terminated with CTRL-C.\nSources released.\n", sys_siglist[signal]);

  if (mutexes != NULL) {
    for (i = 0; i < count; i++)
    pthread_mutex_unlock(&mutexes[i]);
  }

  if (florists != NULL) {
    for (i = 0; i < count; i++)
      pthread_cancel(florists[i]);

    for (i = 0; i < count; i++)
      pthread_join(florists[i], NULL);
  }

  free_all();
  exit(EXIT_SUCCESS);
}

/* ---------------------------------------- */
