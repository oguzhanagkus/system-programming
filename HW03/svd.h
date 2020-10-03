/* This code is belong to Dianne Cook - dicook@iastate.edu
	 I just did some modifications run with my program. */ 

#ifndef SVD_H
#define SVD_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define PRECISION1 32768
#define PRECISION2 16384
#define PI 3.1415926535897932
#define MAXINT 2147483647
#define ASCII_TEXT_BORDER_WIDTH 4
#define MAXHIST 100
#define STEP0 0.01
#define FORWARD 1
#define BACKWARD -1
#define PROJ_DIM 5
#define TRUE 1
#define FALSE 0

#define MIN(x,y) ( (x) < (y) ? (x) : (y) )
#define MAX(x,y) ((x)>(y)?(x):(y))
#define SIGN(a, b) ((b) >= 0.0 ? fabs(a) : -fabs(a))

typedef struct {
	float x, y, z;
} fcoords;

typedef struct {
	long x, y, z;
} lcoords;

typedef struct {
	int x, y, z;
} icoords;

typedef struct {
	float min, max;
} lims;

typedef struct hist_rec {
  struct hist_rec *prev, *next;
  float *basis[3];
  int pos;
} hist_rec;

int svd(unsigned int **matrix, int m, int n, double *w);

#endif
