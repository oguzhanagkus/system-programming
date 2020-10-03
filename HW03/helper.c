/* CSE344 - Systems Programming Course HW03 - Pipes

  Oğuzhan Agkuş - 161044003 */

#include "helper.h"

// Creates a mxn matrix (two dimensional array)
// Return its address on success, NULL in case of allocation error
unsigned int **create_matrix(size_t m, size_t n) {
  int i, j;
  unsigned int **matrix;

  matrix = (unsigned int **) malloc(m * sizeof(unsigned int *));

  if(matrix == NULL) {
    fprintf(stderr, "Memory allocation error!\n");
    return NULL;
  }

  for(i = 0; i < m; i++) {
    matrix[i] = (unsigned int *) malloc(n * sizeof(unsigned int));

    if(matrix[i] == NULL) {
      for(j = 0; j < i; j++)
        free(matrix[j]);

      free(matrix);
      fprintf(stderr, "Memory allocation error!\n");
      return NULL;
    }
  }

  return matrix;
}

/* ---------------------------------------- */

// Performs matrix multiplication, gets 2 matrix: mxn and nxp
// Returns result matrix's address on success, NULL in case of allocation error
unsigned int **multiplication(unsigned int **matrix_1, unsigned int **matrix_2, size_t m, size_t n, size_t p) {
  int i, j, k, temp;
  unsigned int**result = NULL;

  result = create_matrix(m, p);

  if(result == NULL) {
    fprintf(stderr, "Memory allocation error!\n");
    return NULL;
  }

  temp = 0;

  for(i = 0; i < m; i++) {
    for(j = 0; j < p; j++) {
      for(k = 0; k < n; k++)
        temp += matrix_1[i][k] * matrix_2[k][j];

      result[i][j] = temp;
      temp = 0;
    }
  }

  return result;
}

/* ---------------------------------------- */

// Gets a path and nxn matrix (allocated) then fills the matrix with ASCII values of each character in the file
// Returns 0 on success, -1 in case of error
int fill_square_matrix(char *path, unsigned int ***matrix, size_t n) {
  char *read_buffer = NULL;
  int i, j, fd, bytes_read;

  if((fd = open(path, O_RDONLY)) == -1) {
    fprintf(stderr, "Failed to open %s: %s\n", path, strerror(errno));
    return -1;
  }

  if((read_buffer = (char *) malloc(n * sizeof(char))) == NULL) {
    fprintf(stderr, "Memory allocation error!\n");
    return -1;
  }

  (*matrix) = create_matrix(n, n);

  if(matrix == NULL) {
    fprintf(stderr, "Memory allocation error!\n");
    return -1;
  }

  for(i = 0; i < n; i++) {
    if((bytes_read = read(fd, read_buffer, n)) == n) {
      for(j = 0; j < n; j++) {
        if(((*matrix)[i][j] >= 0) && ((*matrix)[i][j] < 255))     // If it is an ASCII character
          (*matrix)[i][j] = (unsigned int) read_buffer[j];
        else                                                      // If it is not an ASCII character
          (*matrix)[i][j] = 0;
      }
    }
    else {
      fprintf(stderr, "There are not sufficient characters in %s\n", path);  // If there is no enough character to fill matrix
      return -1;
    }
  }

  free(read_buffer);

  if(close(fd) == -1) {
    fprintf(stderr, "Failed to close %s: %s\n", path, strerror(errno));
    return -1;
  }

  return 0;
}

/* ---------------------------------------- */

// Gets a dynamicallay allocated matrix and free it
void free_matrix(unsigned int **matrix, size_t n) {
  int i;

  for(i = 0; i < n; i++)
    free(matrix[i]);

  free(matrix);
}

/* ---------------------------------------- */

// Gets a mxn matrix and print it
void print_matrix(unsigned int **matrix, size_t m, size_t n) {
  int i, j;

  for(i = 0; i < m; i++) {
    for(j = 0; j < n; j++)
    fprintf(stdout, "%d\t", matrix[i][j]);

    fprintf(stdout, "\n");
  }

  fprintf(stdout, "\n");
}

/* ---------------------------------------- */

// Gets a n-sized array and print it
void print_array(double *array, size_t n) {
  int i;

  for(i = 0; i < n; i++)
    fprintf(stdout, "%.3lf\n", array[i]);

  fprintf(stdout, "\n");
}
