/* CSE344 - Systems Programming Course HW03 - Pipes

  Oğuzhan Agkuş - 161044003 */

#include "svd.h"
#include "helper.h"

static volatile int live_children_count = 0;
static void sigchld_handler(int signal);

int main(int argc, char *argv[]) {
  int opt;
  size_t n;
  char *input_1 = NULL, *input_2 = NULL;

  int barrier[2];
  bidirect_pipe_t pipes[4];

  struct sigaction sa;
  sigset_t block_mask, empty_mask;

  size_t size;
  double *singular_values = NULL;
  unsigned int **matrix_1 = NULL, **matrix_2 = NULL, **result = NULL;

  int i, j, k, sync, error;

  /* -------------------- */ // Command line arguments checking

  if(argc != 7) {
    fprintf(stderr, "Given less/more arguments!\n-> Usage: ./program -i input_1 -j input_2 -n int_value\n");
    exit(EXIT_FAILURE);
  }

  /* -------------------- */ // Getopt parsing

  while((opt = getopt(argc, argv, "i:j:n:")) != -1) {
    switch(opt) {
      case 'i':
        input_1 = optarg; break;
      case 'j':
        input_2 = optarg; break;
      case 'n':
        n = (size_t) atoi(optarg); break;
      default:
        fprintf(stderr, "Invalid options!\n-> Usage: ./program -i input_1 -j input_2 -n int_value\n");
        exit(EXIT_FAILURE);
    }
  }

  /* -------------------- */ // Input checking

  if(input_1 == NULL || input_2 == NULL) {
    fprintf(stderr, "Missing input arguments.\n");
    exit(EXIT_FAILURE);
  }

  if((int) n < 1) {
    fprintf(stderr, "Invalid 'n' value! Provide positive integer.\n");
    exit(EXIT_FAILURE);
  }

  if((int) n > 10) {
    fprintf(stderr, "Big 'n' value! Memory may run out! Provide smaller integer.\n");
    exit(EXIT_FAILURE);
  }

  /* -------------------- */ // Creating pipes

  if(pipe(barrier) == -1) {
    fprintf(stderr, "Failed to create barrier pipe: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  for(i = 0; i < 4; i++) {
    if((pipe(pipes[i].going) == -1) || (pipe(pipes[i].coming) == -1)) {
      fprintf(stderr, "Failed to create data pipes: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
    }
  }

  /* -------------------- */ // Signal handling and masking

  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sa.sa_handler = sigchld_handler;

  if(sigaction(SIGCHLD, &sa, NULL) == -1) {
    fprintf(stderr, "Cannot set new signal handler: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  sigemptyset(&block_mask);
  sigaddset(&block_mask, SIGCHLD);

  if(sigprocmask(SIG_SETMASK, &block_mask, NULL) == -1) {
    fprintf(stderr, "Cannot set signal mask: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  /* -------------------- */ // Fork for 4 child

  for(i = 0; i < 4; i++) {
    live_children_count++;

    switch(fork()) {
      case -1:
        live_children_count--;
        fprintf(stderr, "Fork error!\n");
        exit(EXIT_FAILURE);

      case 0: // Child
        /* -------------------- */ // Closing unused file descriptors

        if(close(barrier[READ_END]) == -1) { // Closing unused file descriptors
          fprintf(stderr, "Failed to close unused file descriptors: %s\n", strerror(errno));
          exit(EXIT_FAILURE);
        }

        j = 0;
        while(j < 4) {
          if(j != i) {
            if((close(pipes[j].going[READ_END]) == -1) ||
               (close(pipes[j].going[WRITE_END]) == -1) ||
               (close(pipes[j].coming[READ_END]) == -1) ||
               (close(pipes[j].coming[WRITE_END]) == -1))
               {
                 fprintf(stderr, "Failed to close unused file descriptors: %s\n", strerror(errno));
                 exit(EXIT_FAILURE);
               }
          }
          else {
            if((close(pipes[j].going[WRITE_END]) == -1) ||
               (close(pipes[j].coming[READ_END]) == -1))
               {
                 fprintf(stderr, "Failed to close unused file descriptors: %s\n", strerror(errno));
                 exit(EXIT_FAILURE);
               }
          }

          j++;
        }

        /* -------------------- */ // Reading size

        switch(read(pipes[i].going[READ_END], &size, sizeof(size_t))) {
          case -1:
            fprintf(stderr, "Failed to read: %s\n", strerror(errno));

          case 0:
            exit(EXIT_FAILURE);

          default:
            break;
        }

        /* -------------------- */ // Dynamic allocation and error check

        if((matrix_1 = create_matrix(size / 2, size)) == NULL) {
          exit(EXIT_FAILURE);
        }

        if((matrix_2 = create_matrix(size, size / 2)) == NULL) {
          free_matrix(matrix_1, size / 2);
          exit(EXIT_FAILURE);
        }

        /* -------------------- */ // Reading data

        error = 0;

        for(k = 0; k < size / 2 && error == 0; k++) {
          if(read(pipes[i].going[READ_END], matrix_1[k], size * sizeof(unsigned int)) == -1) {
            fprintf(stderr, "Failed to read: %s\n", strerror(errno));
            error = 1; break;
          }
        }

        for(k = 0; k < size && error == 0; k++) {
          if(read(pipes[i].going[READ_END], matrix_2[k], (size / 2) * sizeof(unsigned int)) == -1) {
            fprintf(stderr, "Failed to read: %s\n", strerror(errno));
            error = 1; break;
          }
        }

        if(error == 1) {
          free_matrix(matrix_1, size / 2);
          free_matrix(matrix_2, size);
          exit(EXIT_FAILURE);
        }

        /* -------------------- */ // Calculation

        result = multiplication(matrix_1, matrix_2, size / 2, size, size / 2);

        if(result == NULL) {
          fprintf(stderr, "Memory allocation error!\n");
          free_matrix(matrix_1, size / 2);
          free_matrix(matrix_2, size);
          exit(EXIT_FAILURE);
        }

        /* -------------------- */ // Sync barrier

        error = 0;

        if(close(barrier[WRITE_END]) == -1) {
          fprintf(stderr, "Failed to close barrier: %s\n", strerror(errno));
          error = 1;
        }

        /* -------------------- */ // Sending data

        for(k = 0; k < size / 2 && error == 0; k++) {
          if(write(pipes[i].coming[WRITE_END], result[k], (size / 2) * sizeof(unsigned int)) == -1) {
            fprintf(stderr, "Failed to write: %s\n", strerror(errno));
            error = 1; break;
          }
        }

        /* -------------------- */ // Exiting with success

        free_matrix(matrix_1, size / 2);
        free_matrix(matrix_2, size);
        free_matrix(result, size / 2);

        if(error == 1)
          exit(EXIT_FAILURE);

        exit(EXIT_SUCCESS);

      default: // Parent
        break;
    }
  }

  /* -------------------- */ // Closing unused file descriptors

  if(close(barrier[WRITE_END]) == -1) {
    fprintf(stderr, "Failed to close unused file descriptors: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  for(i = 0; i < 4; i++) {
    if((close(pipes[i].going[READ_END]) == -1) ||
       (close(pipes[i].coming[WRITE_END]) == -1))
       {
         fprintf(stderr, "Failed to close unused file descriptors: %s\n", strerror(errno));
         exit(EXIT_FAILURE);
       }
  }

  /* -------------------- */ // Create data

  size = (size_t) pow(2, n);

  if(fill_square_matrix(input_1, &matrix_1, size) == -1){
    exit(EXIT_FAILURE);
  }

  if(fill_square_matrix(input_2, &matrix_2, size) == -1){
    free_matrix(matrix_1, size);
    exit(EXIT_FAILURE);
  }

  /* -------------------- */ // Send data

  error = 0;

  for(i = 0; i < 4 && error == 0; i++) {
    if(write(pipes[i].going[WRITE_END], &size, sizeof(size_t)) == -1) {
      fprintf(stderr, "Failed to write: %s\n", strerror(errno));
      error = 1; break;
    }
  }

  for(i = 0; i < 4 && error == 0; i++) {
    if(i == 0 || i == 1) {
      for(k = 0; k < size / 2 && error == 0 ; k++) {
        if(write(pipes[i].going[WRITE_END], matrix_1[k], size * sizeof(unsigned int)) == -1) {
          fprintf(stderr, "Failed to write: %s\n", strerror(errno));
          error = 1; break;
        }
      }
    }

    else if(i == 2 || i == 3) {
      for(k = size / 2; k < size && error == 0; k++) {
        if(write(pipes[i].going[WRITE_END], matrix_1[k], size * sizeof(unsigned int)) == -1) {
          fprintf(stderr, "Failed to write: %s\n", strerror(errno));
          error = 1; break;
        }
      }
    }

    if(i == 0 || i == 2) {
      for(k = 0; k < size && error == 0; k++) {
        if(write(pipes[i].going[WRITE_END], matrix_2[k], (size / 2) * sizeof(unsigned int)) == -1) {
          fprintf(stderr, "Failed to write: %s\n", strerror(errno));
          error = 1; break;
        }
      }
    }

    else if(i == 1 || i == 3) {
      for(k = 0; k < size && error == 0; k++) {
        if(write(pipes[i].going[WRITE_END], matrix_2[k] + (size / 2), (size / 2) * sizeof(unsigned int)) == -1) {
          fprintf(stderr, "Failed to write: %s\n", strerror(errno));
          error = 1; break;
        }
      }
    }
  }

  free_matrix(matrix_1, size);
  free_matrix(matrix_2, size);

  if(error == 1)
    exit(EXIT_FAILURE);

  /* -------------------- */ // Sync barrier

  if(read(barrier[READ_END], &sync, 1) != 0) {
    fprintf(stderr, "Parent did not get EOF: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  /* -------------------- */ // Reading from children

  if((result = create_matrix(size, size)) == NULL)
    exit(EXIT_FAILURE);

  error = 0;

  for(k = 0; k < size / 2 && error == 0; k++) {
    for(i = 0; i < 4 && error == 0; i++) {
      if(i == 0) {
        if(read(pipes[i].coming[READ_END], result[k], (size / 2) * sizeof(unsigned int)) == -1) {
          fprintf(stderr, "Failed to read: %s\n", strerror(errno));
          error = 1; break;
        }
      }
      else if(i == 1) {
        if(read(pipes[i].coming[READ_END], result[k] + (size / 2), (size / 2) * sizeof(unsigned int)) == -1) {
          fprintf(stderr, "Failed to read: %s\n", strerror(errno));
          error = 1; break;
        }
      }
      else if(i == 2) {
        if(read(pipes[i].coming[READ_END], result[k + (size / 2)], (size / 2) * sizeof(unsigned int)) == -1) {
          fprintf(stderr, "Failed to read: %s\n", strerror(errno));
          error = 1; break;
        }
      }
      else if(i == 3) {
        if(read(pipes[i].coming[READ_END], result[k + (size / 2)] + (size / 2), (size / 2) * sizeof(unsigned int)) == -1) {
          fprintf(stderr, "Failed to read: %s\n", strerror(errno));
          error = 1; break;
        }
      }
    }
  }

  if(error == 1) {
    free_matrix(result, size);
    exit(EXIT_FAILURE);
  }

  /* -------------------- */ // Singular value calculation

  singular_values = (double *) malloc(size * sizeof(double));

  if(singular_values == NULL) {
    fprintf(stderr, "Memory allocation error!\n");
    free_matrix(result, size);
    exit(EXIT_FAILURE);
  }

  if(svd(result, size, size, singular_values) == 0)
    error = 1;

  free_matrix(result, size);

  if(error == 1) {
    free(singular_values);
    exit(EXIT_FAILURE);
  }

  fprintf(stdout, "\n-> Singular values:\n\n");
  print_array(singular_values, size);
  free(singular_values);

  /* -------------------- */ // Waiting for children to exit

  sigemptyset(&empty_mask);

  while(live_children_count > 0) {
    if(sigsuspend(&empty_mask) == -1 && errno != EINTR) {
      fprintf(stderr, "Sigsuspend error: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
    }
  }

  /* -------------------- */ // Exiting with success

  // fprintf(stdout, "Succesfully finished!\n");
  exit(EXIT_SUCCESS);
}

/* ---------------------------------------- */

static void sigchld_handler(int signal) {
  int status, saved_errno;
  pid_t child_pid;

  saved_errno = errno;

  while((child_pid = waitpid(-1, &status, WNOHANG)) > 0) {
    // printf("Handler reaped child %d\n", child_pid);
    live_children_count--;
  }

  if(child_pid == -1 && errno != ECHILD) {
    fprintf(stdout, "No child processes!\n");
    exit(EXIT_FAILURE);
  }

  errno = saved_errno;
}
