/*
 * CSE344 - Systems Programming - HW02
 *
 *  Oguzhan Agkus - 16104403
 *
 */

#include "helper.h"

void clean(char *array, size_t size) {
  int i;

  for(i = 0; i < size; i++)
    array[i] = '!';
}

/* ---------------------------------------- */

void print(point_t *points, size_t count) {
  int i;

  for(i = 0; i < count; i++)
    printf("(%d,%d), ", points[i].x, points[i].y);
}

/* ---------------------------------------- */

double f(double x, line_t eq) {
  return (eq.a * x + eq.b);
}

/* ---------------------------------------- */

double mean(double *array, size_t size) {
  int i;
  double result = 0;

  for(i = 0; i < size; i++)
    result += array[i];

  result /= size;

  return result;
}

/* ---------------------------------------- */

double mean_deviation(double *array, size_t size, double mean) {
  int i;
  double result = 0;

  for(i = 0; i < size; i++)
    result += fabs(mean - array[i]);

  result /= size;

  return result;
}

/* ---------------------------------------- */

double standard_deviation(double *array, size_t size, double mean) {
  int i;
  double result = 0;

  for(i = 0; i < size; i++)
    result += pow((mean - array[i]), 2);

  result /= size;
  result = sqrt(result);

  return result;
}

/* ---------------------------------------- */

double mean_squared_error(point_t *points, size_t count, line_t eq) {
  int i;
  double result = 0;

  for(i = 0; i < count; i++)
  result += pow(points[i].y - f(points[i].x, eq), 2);

  result = result / count;

  return result;
}

/* ---------------------------------------- */

double mean_absolute_error(point_t *points, size_t count, line_t eq) {
  int i;
  double result = 0;

  for(i = 0; i < count; i++)
    result += fabs(points[i].y - f(points[i].x, eq));

  result /= count;

  return result;
}

/* ---------------------------------------- */

double root_mean_squared_error(point_t *points, size_t count, line_t eq) {
  int i;
  double result = 0;

  for(i = 0; i < count; i++)
    result += pow(points[i].y - f(points[i].x, eq), 2);

  result = result / count;
  result = sqrt(result);

  return result;
}

/* ---------------------------------------- */

line_t least_squares_method(point_t *points, size_t count) {
  int i;
  line_t eq;
  double m, temp;
  double x_sum = 0, y_sum = 0, x_sq_sum = 0, xy_sum = 0;

  for(i = 0; i < count; i++) {
    x_sum += points[i].x;
    y_sum += points[i].y;
    x_sq_sum += (points[i].x * points[i].x);
    xy_sum += (points[i].x * points[i].y);
  }

  m = (count * xy_sum - (x_sum * y_sum)) / (count * x_sq_sum - (x_sum * x_sum));
  temp = (y_sum - (m * x_sum)) / count;

  eq.a = m;
  eq.b = temp;

  return eq;
}
