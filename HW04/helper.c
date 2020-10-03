/* CSE344 - Systems Programming Course - HW04 - Threads and Sync with Semaphores

  Oğuzhan Agkuş - 161044003 */

#include "helper.h"

/* ---------------------------------------- */

const char *ingr_types[] = { "flour", "milk", "sugar", "walnuts" };

/* ---------------------------------------- */

void error_exit(char *message, int error, int *to_free) {
  char buffer[256];
  int count;

  if (to_free != NULL) // Free shared_memory
    free(to_free);

  if (error == -1)
    count = sprintf(buffer, "%s\n", message);
  else
    count = sprintf(buffer, "%s: %s\n", message, strerror(error));

  write(STDERR_FILENO, buffer, count);
  exit(EXIT_FAILURE);
}

/* ---------------------------------------- */

int check_input(int fd) {
  int row = 0, count = 0;
  char previous = '-', buffer[1];

  while (read(fd, buffer, 1) == 1) {
    switch (buffer[0]) {
      case 'F':
      case 'M':
      case 'S':
      case 'W':
        if (previous == buffer[0])
          return -1;

        previous = buffer[0];
        count += 1;
        break;

      case '\n':
        if (count == 2) {
          row += 1;
          count = 0;
          previous = '-';
        }
        else
          return -1;
        break;

      default:
        return -1;
    }
  }

  if (row < 10) {
    close(fd);
    return -1;
  }
  else {
    lseek(fd, 0, SEEK_SET);
    return 0;
  }
}

/* ---------------------------------------- */

int food_type(char type) {
  if (type == 'F')
    return 0;
  else if (type == 'M')
    return 1;
  else if (type == 'S')
    return 2;
  else
    return 3;
}

/* ---------------------------------------- */

int *init_shared_memory() {
  int *shared_memory;

  shared_memory = (int *) malloc(3 * sizeof(int));
  if (shared_memory == NULL)
    error_exit("malloc() error", -1, NULL);

  shared_memory[ingr_1] = empty;
  shared_memory[ingr_2] = empty;
  shared_memory[product] = 0;

  return shared_memory;
}

/* ---------------------------------------- */

int init_sem(int *sem_id) {
  int error = 0;
  union semun arg;

  *sem_id = semget(IPC_PRIVATE, 6, IPC_CREAT | IPC_EXCL | 0600);
  if (*sem_id == -1)
    return -1;

  arg.array = (unsigned short *) malloc(6 * sizeof(unsigned short));
  if (arg.array == NULL)
    return -1;

  arg.array[flour] = 0;
  arg.array[milk] = 0;
  arg.array[sugar] = 0;
  arg.array[walnuts] = 0;
  arg.array[dessert] = 0;
  arg.array[busy] = 1;

  if (semctl(*sem_id, 0, SETALL, arg) == -1)
    error = 1;

  free(arg.array);

  if (error == 1)
    return -1;
  else
    return 0;
}

/* ---------------------------------------- */

void init_arguments(struct argument arguments[CHEF_COUNT]) {
  int i;

  for (i = 0; i < CHEF_COUNT; i++)
    arguments[i].chef_id = i + 1;

  arguments[0].ingredients[flour] = endless;
  arguments[0].ingredients[milk] = endless;
  arguments[0].ingredients[sugar] = lack;
  arguments[0].ingredients[walnuts] = lack;

  arguments[1].ingredients[flour] = endless;
  arguments[1].ingredients[milk] = lack;
  arguments[1].ingredients[sugar] = endless;
  arguments[1].ingredients[walnuts] = lack;

  arguments[2].ingredients[flour] = endless;
  arguments[2].ingredients[milk] = lack;
  arguments[2].ingredients[sugar] = lack;
  arguments[2].ingredients[walnuts] = endless;

  arguments[3].ingredients[flour] = lack;
  arguments[3].ingredients[milk] = endless;
  arguments[3].ingredients[sugar] = endless;
  arguments[3].ingredients[walnuts] = lack;

  arguments[4].ingredients[flour] = lack;
  arguments[4].ingredients[milk] = endless;
  arguments[4].ingredients[sugar] = lack;
  arguments[4].ingredients[walnuts] = endless;

  arguments[5].ingredients[flour] = lack;
  arguments[5].ingredients[milk] = lack;
  arguments[5].ingredients[sugar] = endless;
  arguments[5].ingredients[walnuts] = endless;
}

/* ---------------------------------------- */

void fill_sops(struct argument *arguments, struct sembuf sops[2]) {
  if (arguments->ingredients[flour] == lack) {
    sops[0].sem_num = flour;

    if (arguments->ingredients[milk] == lack)
      sops[1].sem_num = milk;
    else if (arguments->ingredients[sugar] == lack)
      sops[1].sem_num = sugar;
    else
      sops[1].sem_num = walnuts;
  }
  else if (arguments->ingredients[milk] == lack) {
    sops[0].sem_num = milk;

    if (arguments->ingredients[sugar] == lack)
      sops[1].sem_num = sugar;
    else
      sops[1].sem_num = walnuts;
  }
  else {
    sops[0].sem_num = sugar;
    sops[1].sem_num = walnuts;
  }

  sops[0].sem_op = -1;
  sops[0].sem_flg = 0;
  sops[1].sem_op = -1;
  sops[1].sem_flg = 0;
}

/* ---------------------------------------- */

int wait(int sem_id, int semaphore) {
  struct sembuf sop;

  sop.sem_num = semaphore;
  sop.sem_op = -1;
  sop.sem_flg = 0;

  if (semop(sem_id, &sop, 1) == -1)
    return -1;

  return 0;
}

/* ---------------------------------------- */

int post(int sem_id, int semaphore) {
  struct sembuf sop;

  sop.sem_num = semaphore;
  sop.sem_op = 1;
  sop.sem_flg = 0;

  if (semop(sem_id, &sop, 1) == -1)
    return -1;

  return 0;
}
