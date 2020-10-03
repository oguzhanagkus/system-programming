/*
 * CSE344 - Systems Programming - HW02
 *
 *  Oguzhan Agkus - 16104403
 *
 */

#ifndef HELPER
#define HELPER

#include <math.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/stat.h>
#include <sys/types.h>

#define POINT_COUNT 10
#define INCREASE 32

#define FAIL -1
#define SUCCESS 0

typedef struct point {
  uint8_t x;
  uint8_t y;
} point_t;

typedef struct line {
  double a;
  double b;
} line_t;

void clean(char *array, size_t size);
void print(point_t *points, size_t count);

double f(double x, line_t eq);
double mean(double *array, size_t size);
double mean_deviation(double *array, size_t size, double mean);
double standard_deviation(double *array, size_t size, double mean);
double mean_squared_error(point_t *points, size_t count, line_t eq);
double mean_absolute_error(point_t *points, size_t count, line_t eq);
double root_mean_squared_error(point_t *points, size_t count, line_t eq);
line_t least_squares_method(point_t *points, size_t count);

#endif
