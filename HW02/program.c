/*
 * CSE344 - Systems Programming - HW02
 *
 *  Oguzhan Agkus - 16104403
 *
 */

#include "helper.h"

// Global variables for signal handler
sig_atomic_t sigint_count = 0;
sig_atomic_t sigusr1_count = 0;
sig_atomic_t sigusr2_count = 0;

// Function declarations
void process_1(char *input_path, char *temp_path, int fd_temp, pid_t child_pid);
void process_2(char *input_path, char *output_path, char *temp_path, int fd_temp);
void delete_line(char *temp_path, int fd_temp, int line_length);
void handler(int sig_number);

int main(int argc, char *argv[]) {
  if(argc != 5) {
    printf("Given less/more arguments!\n");
    exit(FAIL);
  }

  /* -------------------- */

  char *input_path = NULL, *output_path = NULL;
  int opt;

  while((opt = getopt(argc, argv, "i:o:")) != -1) {
    switch(opt) {
      case 'i':
        input_path = optarg; break;
      case 'o':
        output_path = optarg; break;
      default:
        printf("Provide valid arguments!\n");
        exit(FAIL);
    }
  }

  if(input_path == NULL || output_path == NULL) {
    printf("Missing arguements!\n");
    exit(FAIL);
  }

  /* -------------------- */

  int fd_temp, fd_temp_child;
  char template[] = "./tmp_XXXXXX";

  if((fd_temp = mkstemp(template)) < 0) {
    perror("Temporary file");
    exit(FAIL);
  }

  /* -------------------- */

  pid_t child_pid;
  child_pid = fork();

  if(child_pid != 0)
    process_1(input_path, template, fd_temp, child_pid);     // Run process_1

  else {
    if((fd_temp_child = open(template, O_RDWR)) < 0) {
      perror("Temporary file (child)");
      exit(FAIL);
    }

    process_2(input_path, output_path, template, fd_temp_child);     // Run process_2
  }

  return 0;
}

/* ---------------------------------------------------------------------------------------------------- */

void process_1(char *input_path, char *temp_path, int fd_temp, pid_t child_pid) {
  int fd_input;

  if((fd_input = open(input_path, O_RDONLY)) < 0) {
    if(close(fd_temp) < 0) {
      perror("Temp file");
      exit(FAIL);
    }

    if(unlink(temp_path) < 0) {
      perror("Temp path");
      exit(FAIL);
    }

    kill(child_pid, SIGUSR1);     // process_1 done with input file, say it to process_2
    perror("Input file");
    exit(FAIL);
  }

  /* -------------------- */

  // Signal operations
  struct sigaction usr1_action, critical_action, critical_action_backup;

  memset(&usr1_action, 0, sizeof(usr1_action));
  memset(&critical_action, 0, sizeof(critical_action));
  memset(&critical_action_backup, 0, sizeof(critical_action_backup));

  usr1_action.sa_handler = &handler;      // Set new handler
  critical_action.sa_handler = &handler;  // Set new handler

  sigaction(SIGUSR1, &usr1_action, NULL); // Replace handler because SIGUSR1's default action is terminate

  /* -------------------- */

  point_t temp[POINT_COUNT];  // Points
  line_t eq;                  // Line equation

  int i = 0, total_byte_read = 0, total_eq_estimated = 0;
  unsigned char read_buffer[POINT_COUNT * 2];
  char write_buffer[32];
  size_t bytes_read, bytes_write;

  struct flock lock;
  memset(&lock, 0, sizeof(lock));
  lock.l_pid = getpid();

  while(1) {
    fcntl(fd_temp, F_GETLK, &lock);

    if(lock.l_type != F_UNLCK)
      continue;

    else {
      lock.l_type = F_WRLCK;
      fcntl(fd_temp, F_SETLKW, &lock);    // Set file lock to temp_file

      if((bytes_read = read(fd_input, read_buffer, sizeof(read_buffer))) == sizeof(read_buffer)) {
        total_byte_read += bytes_read;

        if(lseek(fd_temp, 0, SEEK_END) < 0) {
          perror("Cannot move cursor in input file");
          exit(FAIL);
        }

        for(i = 0; i < POINT_COUNT; i++) {
          temp[i].x = (uint8_t) read_buffer[2*i];
          temp[i].y = (uint8_t) read_buffer[2*i + 1];

          bytes_write = sprintf(write_buffer, "(%d, %d), ", read_buffer[2*i], read_buffer[2*i + 1]);

          if(write(fd_temp, write_buffer, bytes_write) < 0) {
            perror("Failed while writing to temp file");
            exit(FAIL);
          }
        }

        sigaction(SIGINT, &critical_action, &critical_action_backup);     // Critacal section start
        eq = least_squares_method(temp, POINT_COUNT);                     // Calculation in the critical section
        sigaction(SIGINT, &critical_action_backup, NULL);                 // Critacal section end

        total_eq_estimated++;

        if(eq.b == 0)
          bytes_write = sprintf(write_buffer, "%.3lfx\n", eq.a);
        else if(eq.b < 0)
          bytes_write = sprintf(write_buffer, "%.3lfx%.3lf\n", eq.a, eq.b);
        else
          bytes_write = sprintf(write_buffer, "%.3lfx+%.3lf\n", eq.a, eq.b);

        if(write(fd_temp, write_buffer, bytes_write) < 0) {
          perror("Failed while writing to temp file");
          exit(FAIL);
        }

        lock.l_type = F_UNLCK;
        fcntl(fd_temp, F_SETLKW, &lock);     // Unlock temp_file's lock

        if(sigusr1_count != 0){         // If process_2 is on waiting with sigsuspend, process_1 gets SIGUSR1 signal to know that
          sigusr1_count = 0;            // Reset
          kill(child_pid, SIGUSR2);     // Data available in temp file, say it to process_2
        }
      }

      else
        break;
    }
  }

  kill(child_pid, SIGUSR2);     // Data available in temp file, say it to process_2

  if(close(fd_input) < 0) {
    perror("Input file");
    exit(FAIL);
  }

  if(close(fd_temp) < 0) {
    perror("Temp file");
    exit(FAIL);
  }

  kill(child_pid, SIGUSR1);     // process_1 done with input file, say it to process_2

  printf("Total bytes read: %d\n",total_byte_read);
  printf("Total equation estimated: %d\n", total_eq_estimated);
  printf("Incoming signals in critical section: SIGINT = %d\n", (int) sigint_count);
}

/* ---------------------------------------- */

void process_2(char *input_path, char *output_path, char *temp_path, int fd_temp) {
  int fd_output;

  if((fd_output = open(output_path, O_WRONLY | O_CREAT, 0666)) < 0) {
    if(close(fd_temp) < 0) {
      perror("Temp file");
      exit(FAIL);
    }

    if(unlink(temp_path) < 0) {
      perror("Temp path");
      exit(FAIL);
    }

    perror("Output file");
    exit(FAIL);
  }

  /* -------------------- */

  // Signal operations
  struct sigaction usr1_action, usr2_action, critical_action, critical_action_backup;

  memset(&usr1_action, 0, sizeof(usr1_action));
  memset(&usr2_action, 0, sizeof(usr2_action));
  memset(&critical_action, 0, sizeof(critical_action));
	memset(&critical_action_backup, 0, sizeof(critical_action_backup));

  usr1_action.sa_handler = &handler;        // Set new handler
  usr2_action.sa_handler = &handler;        // Set new handler
  critical_action.sa_handler = &handler;    // Set new handler

  sigaction(SIGUSR1, &usr1_action, NULL);   // Replace handler because SIGUSR1's default action is terminate
  sigaction(SIGUSR2, &usr2_action, NULL);   // Replace handler because SIGUSR1's default action is terminate

  sigset_t suspend_mask;
  sigfillset(&suspend_mask);
  sigdelset(&suspend_mask, SIGUSR2);        // It is used for sigsuspend

  /* -------------------- */

  point_t points[POINT_COUNT];     // Points
  int index = 0;                   // Index of current point
  line_t eq;                       // Line equation

  int current_size = 32;                                               // Size of error arrays, starts with 32
  double mae, mse, rmse;                                               // Error variables for current line
  double *mae_array = NULL, *mse_array = NULL, *rmse_array = NULL;     // Store all error variables
  double mae_mean, mse_mean, rmse_mean;                                // Means of error variables
  double mae_md, mse_md, rmse_md;                                      // Mean deviations of error variables
  double mae_std, mse_std, rmse_std;                                   // Standard deviations of error variables

  char temp[16], read_buffer[1], write_buffer[32];                     // temp for atoi and atof, read_buffer for reading byte by byte
  int i = 0, j = 0, k = 0;                                             // i for temp array, j for file operations, k for structs
  int line_length = 0, line_count = 0;
  size_t bytes_read, bytes_write;

  // Dynamic allocation for this array, so because we do not know there are how many lines
  mae_array = (double *) malloc(current_size * sizeof(double));
  mse_array = (double *) malloc(current_size * sizeof(double));
  rmse_array = (double *) malloc(current_size * sizeof(double));

  clean(temp, sizeof(temp));

  struct flock lock;
  memset(&lock, 0, sizeof(lock));
  lock.l_pid = getpid();

  while(1) {
    fcntl(fd_temp, F_GETLK, &lock);

    if(lock.l_type != F_UNLCK)
      continue;

    else {
      lock.l_type = F_WRLCK;
      fcntl(fd_temp, F_SETLKW, &lock);     // Set file lock to temp_file

      do {
        bytes_read = read(fd_temp, read_buffer, 1);

        if(bytes_read == 0)
          break;

        line_length += 1;

        if(index < POINT_COUNT) {
          if(read_buffer[0] == ',' || read_buffer[0] == '\n') {
            if(k == 0)
              points[index].x = atoi(temp);

            else if(k == 1)
              points[index++].y = atoi(temp);

            k = 1 - k;
            i = 0;
            clean(temp, sizeof(temp));
          }

          else {
            if(read_buffer[0] != ' ' && read_buffer[0] != '(' && read_buffer[0] != ')')
              temp[i++] = read_buffer[0];
          }
        }

        else {
          if(read_buffer[0] == 'x' || read_buffer[0] == '\n') {
            if(k == 0)
              eq.a = atof(temp);

            else if(k == 1)
              eq.b = atof(temp);

            k = 1 - k;
            i = 0;
            clean(temp, sizeof(temp));
            }

          else {
            temp[i++] = read_buffer[0];
          }
        }
      } while(read_buffer[0] != '\n');

      if(bytes_read == 0 && sigusr1_count != 0)    // If there is no byte to read and process_1 done with input file
        break;

      if(bytes_read == 0 && sigusr1_count == 0){   // There is no byte to read and there will be new input to temp file
        lock.l_type = F_UNLCK;
        fcntl(fd_temp, F_SETLKW, &lock);           // Unlock the file because process_1 will write to it
        kill(getppid(), SIGUSR1);                  // Say process_2 is waiting for process_1 to fill temp file
        sigsuspend(&suspend_mask);                 // Suspend, until SIGUSR2 arrive
        continue;
      }

      sigaction(SIGINT, &critical_action, &critical_action_backup);     // Critacal section start
      mae = mean_absolute_error(points, POINT_COUNT, eq);
      mse = mean_squared_error(points, POINT_COUNT, eq);
      rmse = root_mean_squared_error(points, POINT_COUNT, eq);
      sigaction(SIGINT, &critical_action_backup, NULL);                 // Critacal section end

      // Data storing for later calculation
      mae_array[line_count] = mae;
      mse_array[line_count] = mse;
      rmse_array[line_count] = rmse;

      line_count++;

      if(line_count == current_size) {
        current_size += INCREASE;
        mae_array = realloc(mae_array, current_size * sizeof(double));
        mse_array = realloc(mse_array, current_size * sizeof(double));
        rmse_array = realloc(rmse_array, current_size * sizeof(double));
      }

      // Printing line
      for(j = 0; j < POINT_COUNT; j++) {
        bytes_write = sprintf(write_buffer, "(%d, %d), ", points[j].x, points[j].y);

        if(write(fd_output, write_buffer, bytes_write) < 0) {
          perror("Failed while writing to temp file");
          exit(FAIL);
        }
      }

      if(eq.b == 0)
        bytes_write = sprintf(write_buffer, "%.3lfx, ", eq.a);
      else if(eq.b < 0)
        bytes_write = sprintf(write_buffer, "%.3lfx%.3lf, ", eq.a, eq.b);
      else
        bytes_write = sprintf(write_buffer, "%.3lfx+%.3lf, ", eq.a, eq.b);

      if(write(fd_output, write_buffer, bytes_write) < 0) {
        perror("Failed while writing to temp file");
        exit(FAIL);
      }

      bytes_write = sprintf(write_buffer, "%.3lf, %.3lf, %.3lf\n", mae, mse, rmse);

      if(write(fd_output, write_buffer, bytes_write) < 0) {
        perror("Failed while writing to temp file");
        exit(FAIL);
      }

      delete_line(temp_path, fd_temp, line_length);     // Deleting line from temp file

      line_length = 0;
      index = 0;

      lock.l_type = F_UNLCK;
      fcntl(fd_temp, F_SETLKW, &lock);                  // Unlock temp_file's lock
    }
  }

  /* -------------------- */

  if(line_count > 0) {
    mae_mean = mean(mae_array, line_count);
    mse_mean = mean(mse_array, line_count);
    rmse_mean = mean(rmse_array, line_count);

    mae_md = mean_deviation(mae_array, line_count, mae_mean);
    mse_md = mean_deviation(mse_array, line_count, mse_mean);
    rmse_md = mean_deviation(rmse_array, line_count, rmse_mean);

    mae_std = standard_deviation(mae_array, line_count, mae_mean);
    mse_std = standard_deviation(mse_array, line_count, mse_mean);
    rmse_std = standard_deviation(rmse_array, line_count, rmse_mean);

    printf("\n");
    printf("MAE->\tMean: %.3lf, Mean Deviation: %.3lf, Standard Deviation: %.3lf\n", mae_mean, mae_md, mae_std);
    printf("MSE->\tMean: %.3lf, Mean Deviation: %.3lf, Standard Deviation: %.3lf\n", mse_mean, mse_md, mse_std);
    printf("RMSE->\tMean: %.3lf, Mean Deviation: %.3lf, Standard Deviation: %.3lf\n", rmse_mean, rmse_md, rmse_std);
  }

  else
    printf("No input given!\n");

  /* -------------------- */

  free(mae_array);
  free(mse_array);
  free(rmse_array);

  if(close(fd_output) < 0) {
    perror("Output file");
    exit(FAIL);
  }

  if(close(fd_temp) < 0) {
    perror("Temp file");
    exit(FAIL);
  }

  if(unlink(temp_path) < 0) {
    perror("Temp path");
    exit(FAIL);
  }

  if(unlink(input_path) < 0) {
    perror("Input path");
    exit(FAIL);
  }
}

/* ---------------------------------------- */

// It read the left characters to buffer, then write over the file and decrease the size
void delete_line(char *temp_path, int fd_temp, int line_length) {
  struct stat file_stat;
  char *replace_buffer;
  int replace_size, bytes_read;

  stat(temp_path, &file_stat);
  replace_size = file_stat.st_size - line_length;
  replace_buffer = (char *) malloc(replace_size * sizeof(char));
  bytes_read = read(fd_temp, replace_buffer, replace_size);

  lseek(fd_temp, 0, SEEK_SET);
  write(fd_temp, replace_buffer, bytes_read);
  ftruncate(fd_temp, replace_size);
  lseek(fd_temp, 0, SEEK_SET);
  free(replace_buffer);
}

/* ---------------------------------------- */

// Signal handler
void handler(int sig_number) {
  if(sig_number == SIGINT)
    ++sigint_count;
  else if(sig_number == SIGUSR1)
    ++sigusr1_count;
  else if(sig_number == SIGUSR2)
    ++sigusr2_count;
}
