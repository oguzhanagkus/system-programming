/* CSE344 - Systems Programming Course - HW04 - Threads and Sync with Semaphores

  Oğuzhan Agkuş - 161044003 */

#include "helper.h"

int *shared_memory, sem_id, finished = 0; // Global variables

void *chef_function(void *arg); // Thread function
void sigint_handler(int signal); // Signal handler

int main(int argc, char *argv[]) {
  int input_fd, i, opt, count, result, error = 0;
  char *input_path, read_buffer[3], write_buffer[256];
  pthread_t chefs[CHEF_COUNT];
  struct argument arguments[CHEF_COUNT];
  struct sigaction sa;
  struct sembuf sop;

  srand(time(0)); // For random preparation time

  /* -------------------- */ // Command line arguments checking

  if (argc != 3)
    error_exit("Less/more arguments!\nUsage: ./program -i input_path", -1, NULL);

  /* -------------------- */ // Getopt parsing

  while ((opt = getopt(argc, argv, "i:")) != -1) {
    switch (opt) {
      case 'i':
        input_path = optarg; break;
      default:
        error_exit("Invalid options!\nUsage: ./program -i input_path", -1, NULL);
    }
  }

  /* -------------------- */ // Input checking

  if ((input_fd = open(input_path, O_RDONLY)) == -1)
    error_exit("Failed to open input path", errno, NULL);

  if (check_input(input_fd) == -1)
    error_exit("Invalid input file.", -1, NULL);

  /* -------------------- */ // Singal handling

  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sa.sa_handler = &sigint_handler;

  if (sigaction(SIGINT, &sa, NULL) == -1)
    error_exit("Cannot set new signal handler", errno, NULL);

  /* -------------------- */ // Initialization & creating threads

  shared_memory = init_shared_memory(); // Shared memory block between threads places in the heap

  if (init_sem(&sem_id) == -1)
    error_exit("init_sem() error", errno, shared_memory);

  init_arguments(arguments);

  for (i = 0; i < CHEF_COUNT; i++) {
    result = pthread_create(&chefs[i], NULL, chef_function, &arguments[i]);

    if(result != 0)
      error_exit("pthread_create() error", result, shared_memory);
  }

  /* -------------------- */ // Main thread

  while (read(input_fd, read_buffer, 3) >= 2) {
    count = sprintf(write_buffer, "The wholesaler delivers %s and %s.\n",
                    ingr_types[food_type(read_buffer[0])], ingr_types[food_type(read_buffer[1])]);
    write(STDOUT_FILENO, write_buffer, count);

    // Putting ingredients to shared_memory
    if (wait(sem_id, busy) == -1) { error = 1; break; }
    shared_memory[ingr_1] = food_type(read_buffer[0]);
    shared_memory[ingr_2] = food_type(read_buffer[1]);
    if (post(sem_id, food_type(read_buffer[0])) == -1) { error = 1; break; }
    if (post(sem_id, food_type(read_buffer[1])) == -1) { error = 1; break; }
    if (post(sem_id, busy) == -1) { error = 1; break; }

    // Waiting for the dessert
    count = sprintf(write_buffer, "The wholesaler waiting for the dessert.\n");
    write(STDOUT_FILENO, write_buffer, count);
    if (wait(sem_id, dessert) == -1) { error = 1; break; }

    // Getting the dessert from shared_memory
    if (wait(sem_id, busy) == -1) { error = 1; break; }
    shared_memory[product] -= 1;
    if (post(sem_id, busy) == -1) { error = 1; break; }

    count = sprintf(write_buffer, "The wholesaler has obtained the dessert and left to sell it.\n");
    write(STDOUT_FILENO, write_buffer, count);
  }

  if (error == 1)
    error_exit("semop() error in wholesaler", errno, shared_memory);

  count = sprintf(write_buffer, "The wholesaler stopped delivering.\n");
  write(STDOUT_FILENO, write_buffer, count);

  finished = 1;

  for (i = 0; i < 4; i++) {
    sop.sem_num = i;
    sop.sem_op = 3;
    sop.sem_flg = 0;

    if (semop(sem_id, &sop, 1) == -1)
      error_exit("semop() error", errno, shared_memory);
  }

  /* -------------------- */ // Joining threads

  for (i = 0; i < CHEF_COUNT; i++) {
    result = pthread_join(chefs[i], NULL);

    if(result != 0)
      error_exit("pthread_join() error", result, shared_memory);
  }

  /* -------------------- */ // Clean-up, close input file, remove semaphore set, free allocated memory

  if (close(input_fd) == -1)
    error_exit("Error occured while closing the input file", errno, shared_memory);

  if (semctl(sem_id, 0, IPC_RMID) == -1)
    error_exit("The semaphore set cannot removed", errno, shared_memory);

  free(shared_memory);
  exit(EXIT_SUCCESS);
}

/* ---------------------------------------- */ // Thread function for chefs

void *chef_function(void *arg) {
  int count, error = 0, delivered = 0;
  char write_buffer[256];

  struct argument *arguments = (struct argument *) arg;
  struct sembuf sops[2];

  fill_sops(arguments, sops);

  while (1) {
    count = sprintf(write_buffer, "Chef %d is waiting for %s and %s.\n", arguments->chef_id,
                    ingr_types[sops[0].sem_num], ingr_types[sops[1].sem_num]);
    write(STDOUT_FILENO, write_buffer, count);

    // Waiting for ingredients
    if (semop(sem_id, sops, 2) == -1) { error = 1; break; }
    if (finished == 1) break;

    // Getting ingredients from shared_memory
    if (wait(sem_id, busy) == -1) { error = 1; break; }
    count = sprintf(write_buffer, "Chef %d has taken the %s.\n", arguments->chef_id, ingr_types[shared_memory[ingr_1]]);
    write(STDOUT_FILENO, write_buffer, count);
    shared_memory[ingr_1] = empty;
    count = sprintf(write_buffer, "Chef %d has taken the %s.\n", arguments->chef_id, ingr_types[shared_memory[ingr_2]]);
    write(STDOUT_FILENO, write_buffer, count);
    shared_memory[ingr_2] = empty;
    if (post(sem_id, busy) == -1) { error = 1; break; }

    // Preparing
    count = sprintf(write_buffer, "Chef %d is preparing the dessert.\n", arguments->chef_id);
    write(STDOUT_FILENO, write_buffer, count);
    sleep(rand() % 5 + 1);

    // Putting the dessert to shared_memory
    if (wait(sem_id, busy) == -1){ error = 1; break; }
    shared_memory[product] += 1;
    if (post(sem_id, dessert) == -1) { error = 1; break; }
    if (post(sem_id, busy) == -1) { error = 1; break; }

    count = sprintf(write_buffer, "Chef %d has delivered the dessert to the wholesaler.\n", arguments->chef_id);
    write(STDOUT_FILENO, write_buffer, count);

    delivered += 1;
  }

  if (error == 1)
    error_exit("Error occured in thread", errno, shared_memory);

  count = sprintf(write_buffer, "Chef %d delivered %d dessert/s and stopped production!\n", arguments->chef_id, delivered);
  write(STDOUT_FILENO, write_buffer, count);

  pthread_exit(0);
}

/* ---------------------------------------- */ // Signal handler

void sigint_handler(int signal) {
  int count;
  char write_buffer[256];

  free(shared_memory);
  count = sprintf(write_buffer, "\n%s signal caught! Terminated with CTRL-C.\nShared memory has been freed.\n", sys_siglist[signal]);
  write(STDOUT_FILENO, write_buffer, count);

  exit(EXIT_FAILURE);
}
