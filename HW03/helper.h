/* CSE344 - Systems Programming Course HW03 - Pipes

  Oğuzhan Agkuş - 161044003 */
  
#ifndef HELPER
#define HELPER

#include <math.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/wait.h>
#include <sys/types.h>

#define READ_END 0
#define WRITE_END 1

typedef struct bidirect_pipe {
  int going[2];
  int coming[2];
} bidirect_pipe_t;

unsigned int **create_matrix(size_t m, size_t n);
unsigned int **multiplication(unsigned int ** matrix_1, unsigned int ** matrix_2, size_t m, size_t n, size_t p);
int fill_square_matrix(char *path, unsigned int ***matrix, size_t n);
void free_matrix(unsigned int **matrix, size_t n);
void print_matrix(unsigned int **matrix, size_t m, size_t n);
void print_array(double *array, size_t n);

#endif
