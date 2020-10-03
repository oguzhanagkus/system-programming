/* CSE344 - Systems Programming Course - Midterm Project - Mess Hall Simulation

  Sync with POSIX Unnamed Semaphores and Shared Memory

  Oğuzhan Agkuş - 161044003 */

#include "helper.h"

/* ---------------------------------------- */

void error_exit(char *message) {
  char buffer[256];
  int count;

  if (errno == 0)
    count = sprintf(buffer, "%s\n", message);
  else
    count = sprintf(buffer, "%s: %s\n", message, strerror(errno));

  write(STDERR_FILENO, buffer, count);
  exit(EXIT_FAILURE);
}

/* ---------------------------------------- */

void init_hall(struct hall *my_hall, int total_delivery, int kitchen_size, int counter_size, int table_count) {
  if (init_kitchen(my_hall, total_delivery, kitchen_size) == -1)
    error_exit("sem_init() error in kitchen");

  if (init_counter(my_hall, counter_size) == -1)
    error_exit("sem_init() error in counter");

  if (init_tables(my_hall, table_count) == -1)
    error_exit("sem_init() error in tables");
}

/* ---------------------------------------- */

int init_kitchen(struct hall *my_hall, int total_delivery, int kitchen_size) {
  if (sem_init(&my_hall->my_kitchen.delivered, 1, total_delivery) == -1)
    return -1;

  if (sem_init(&my_hall->my_kitchen.empty, 1, kitchen_size) == -1)
    return -1;

  if (sem_init(&my_hall->my_kitchen.full, 1, 0) == -1)
    return -1;

  if (sem_init(&my_hall->my_kitchen.busy, 1, 1) == -1)
    return -1;

  if (sem_init(&my_hall->my_kitchen.p_sem, 1, 0) == -1)
    return -1;

  if (sem_init(&my_hall->my_kitchen.c_sem, 1, 0) == -1)
    return -1;

  if (sem_init(&my_hall->my_kitchen.d_sem, 1, 0) == -1)
    return -1;

  my_hall->my_kitchen.p = 0;
  my_hall->my_kitchen.c = 0;
  my_hall->my_kitchen.d = 0;
  my_hall->my_kitchen.count = 0;

  return 0;
}

/* ---------------------------------------- */

int init_counter(struct hall *my_hall, int counter_size) {
  if (sem_init(&my_hall->my_counter.priority, 1, 0) == -1)
    return -1;

  if (sem_init(&my_hall->my_counter.available, 1, 0) == -1)
    return -1;

  if (sem_init(&my_hall->my_counter.empty, 1, counter_size) == -1)
    return -1;

  if (sem_init(&my_hall->my_counter.full, 1, 0) == -1)
    return -1;

  if (sem_init(&my_hall->my_counter.busy, 1, 1) == -1)
    return -1;

  if (sem_init(&my_hall->my_counter.empty_, 1, counter_size) == -1)
    return -1;

  if (sem_init(&my_hall->my_counter.full_, 1, 0) == -1)
    return -1;

  if (sem_init(&my_hall->my_counter.busy_, 1, 1) == -1)
    return -1;

  my_hall->my_counter.student_count = 0;
  my_hall->my_counter.count = 0;
  my_hall->my_counter.finished = 0;
  my_hall->my_counter.p = 0;
  my_hall->my_counter.c = 0;
  my_hall->my_counter.d = 0;
  my_hall->my_counter.p_ = 0;
  my_hall->my_counter.c_ = 0;
  my_hall->my_counter.d_ = 0;

  return 0;
}

/* ---------------------------------------- */

int init_tables(struct hall *my_hall, int table_count) {
  if (sem_init(&my_hall->my_tables.empty, 1, table_count) == -1)
    return -1;

  my_hall->my_tables.count = table_count;

  return 0;
}

/* ---------------------------------------- */

int check_input(int fd, int count) {
  int p = 0, c = 0, d = 0;
  char buffer[1];

  while (read(fd, buffer, 1) == 1) {
    if (buffer[0] == 'P') p += 1;
    else if (buffer[0] == 'C') c += 1;
    else if (buffer[0] == 'D') d += 1;
  }

  if (p == count && c == count && d == count) {
    lseek(fd, 0, SEEK_SET);
    return 0;
  }
  else {
    close(fd);
    return -1;
  }
}
/* ---------------------------------------- */

int min(int p, int c, int d) {
  if (p < c && p < d)
    return 0;
  else if (c < d)
    return 1;
  else
    return 2;
}

/* ---------------------------------------- */

int food_type(char type) {
  if (type == 'P')
    return 0;
  else if (type == 'C')
    return 1;
  else
    return 2;
}
