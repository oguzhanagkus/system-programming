/* CSE344 - Systems Programming Course - Midterm Project - Mess Hall Simulation

  Sync with POSIX Unnamed Semaphores and Shared Memory

  Oğuzhan Agkuş - 161044003 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <semaphore.h>

#ifndef HELPER
#define HELPER

struct kitchen {
  sem_t delivered, empty, full, busy;
  sem_t p_sem, c_sem, d_sem;
  int p, c, d, count;
};

struct counter {
  sem_t priority, available, empty, full, busy;
  sem_t empty_, full_, busy_;
  int student_count, count, finished;
  int p, c, d;
  int p_, c_, d_;
};

struct tables {
  sem_t empty;
  int count;
};

struct hall {
  struct kitchen my_kitchen;
  struct counter my_counter;
  struct tables my_tables;
};

void error_exit(char *error_message);
void init_hall(struct hall *my_hall, int total_delivery, int kitchen_size, int counter_size, int table_count);
int init_kitchen(struct hall *my_hall, int total_delivery, int kitchen_size);
int init_counter(struct hall *my_hall, int counter_size);
int init_tables(struct hall *my_hall, int table_count);
int check_input(int fd, int count);
int min(int p, int c, int d);
int food_type(char type);

#endif
