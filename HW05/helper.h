/* CSE344 - Systems Programming Course - HW05 - Threads and Mutexes

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

#define STR_LEN 16

typedef struct coordinate {
  double x;
  double y;
} coordinate_t;

typedef struct florist {
  char name[16];
  coordinate_t location;
  double speed;
  size_t stock;
  int *flowers;
} florist_t;

typedef struct data {
  size_t florist_count;
  florist_t *florists;
  size_t flower_count;
  char (*flowers)[STR_LEN];
  int **stock_table;
} data_t;

typedef struct order {
  char client[STR_LEN];
  coordinate_t location;
  int request;
} order_t;

typedef struct argument {
  int id;
} argument_t;

typedef struct report {
  char name[STR_LEN];
  int delivered;
  double time;
} report_t;

void error_exit(char *message, int error);
double absolute(double x);
double cheb_distance(coordinate_t u, coordinate_t v);
char *flower_type(const data_t *data, size_t i);
int flower_index(const data_t *data, const char *flower);
size_t read_line(int fd, char *line);
data_t *read_data(int fd);
void free_data(data_t *data);
void show_data(const data_t *data);
order_t *read_order(const data_t *data, char *line);
void print_reports(report_t *reports, size_t count);

#endif /* HELPER */
