/* CSE344 - Systems Programming Course - HW04 - Threads and Sync with Semaphores

  Oğuzhan Agkuş - 161044003 */

#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifndef HELPER
#define HELPER

#define CHEF_COUNT 6

enum stock { empty = -1, lack = 0, endless = 1 };
enum index { ingr_1 = 0, ingr_2 = 1, product = 2 };
enum semaphore { flour = 0, milk = 1, sugar = 2, walnuts = 3, dessert = 4, busy = 5 };

extern const char *ingr_types[];

struct argument {
  int chef_id;
  int ingredients[4];
};

union semun {
  int val;
  struct semid_ds *buf;
  unsigned short *array;
};

void error_exit(char *error_message, int error, int *to_free);

int check_input(int fd);
int food_type(char type);

int *init_shared_memory();
int init_sem(int *sem_id);
void init_arguments(struct argument arguments[CHEF_COUNT]);
void fill_sops(struct argument *arguments, struct sembuf sops[2]);

int wait(int sem_id, int semaphore);
int post(int sem_id, int semaphore);

#endif
