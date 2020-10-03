#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>

void print_array(int *array, int size);

int main(int argc, char *argv[]) {

  // If user write less or more than 3 argument
  if(argc != 7) {
    printf("Less or more arguments!\n");
    return -1;
  }

  /* ---------------------------------------- */

  char *input_path = NULL, *output_path = NULL;
  int time = 0, opt;

  // Read the parameters
  while((opt = getopt(argc, argv, "i:o:t:")) != -1) {
    switch(opt) {
      case 'i':
        input_path = optarg; break;

      case 'o':
        output_path = optarg; break;

      case 't':
        time = atoi(optarg);break;
    }
  }

  // If
  if(input_path == NULL || output_path == NULL) {
    printf("Invalid path/s!\n");
    return -1;
  }

  // Check time value, it should be 1 <= time <= 50. If it is invalid, do not exit, just update it.
  if(time < 1)
    time = 1;

  else if(time > 50)
    time = 50;

  /* ---------------------------------------- */

  // File descriptors
  int fd_input;

  if((fd_input = open(input_path, O_RDWR)) < 0) {
    printf("Input file cannot be opened.\n");
    return -1;
  }

  /* ---------------------------------------- */

  char buffer[1], temp[3];
  size_t bytes_read;
  int temp_number, i = 0, j = 0;
  int array[32];

  struct flock lock;
  lock.l_type = F_WRLCK;
  lock.l_whence = SEEK_SET;
  lock.l_start = 0;
  lock.l_len = 0;
  lock.l_pid = getpid();

  while(1) {
    fcntl(fd_input, F_GETLK, &lock);

    if(lock.l_type != F_UNLCK)
      continue;

    else {
      lock.l_type = F_WRLCK;

      if((fcntl(fd_input, F_SETLK, &lock)) < 0)
        continue;

      else {
        do {
          bytes_read = read(fd_input, buffer, 1);

          if(bytes_read == 0)
            break;

          if(buffer[0] == '+' || buffer[0] == ',' || buffer[0] == '\n') {
            temp_number = atoi(temp);
            array[j++] = temp_number;

            i = 0;
            temp[0] = '-';
            temp[1] = '-';
            temp[2] = '-';
          }

          else {
            if(buffer[0] != 'i')
              temp[i++] = buffer[0];
          }
        } while(buffer[0] != '\n');

        if(bytes_read == 0)
          break;

        j = 0;

        print_array(array, 32);

        lock.l_type = F_UNLCK;
        fcntl(fd_input, F_SETLK, &lock);

        usleep(time);
      }
    }
  }

  close(fd_input);

  return 0;
}

void print_array(int *array, int size) {
  int i;

  for(i = 0; i < size; i++)
    printf("%d ",array[i]);

  printf("\n");
}
