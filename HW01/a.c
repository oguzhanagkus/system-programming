#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

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
  int fd_input, fd_output;

  if((fd_input = open(input_path, O_RDONLY)) < 0) {
    printf("Input file cannot be opened.\n");
    return -1;
  }

  if((fd_output = open(output_path, O_WRONLY | O_CREAT, 0666)) < 0) {
    printf("Output file cannot be opened/created.\n");
    return -1;
  }

  /* ---------------------------------------- */

  char buffer[32], temp[16];
  size_t bytes_read, bytes_written;
  int i;

  struct flock lock;
  lock.l_type = F_WRLCK;
  lock.l_whence = SEEK_SET;
  lock.l_start = 0;
  lock.l_len = 0;
  lock.l_pid = getpid();

  while(1) {
    fcntl(fd_output, F_GETLK, &lock);

    if(lock.l_type != F_UNLCK)
      continue;

    else {
      lock.l_type = F_WRLCK;

      if((fcntl(fd_output, F_SETLK, &lock)) < 0)
        continue;

      else {
        if((bytes_read = read(fd_input, buffer, sizeof(buffer))) == sizeof(buffer)) {
          lseek(fd_output, 0, SEEK_END);

          for(i = 0; i < bytes_read; i += 2) {
            bytes_written = sprintf(temp, "%d+i%d,", buffer[i], buffer[i+1]);
            write(fd_output, temp, bytes_written);
          }

          lseek(fd_output, -1, SEEK_CUR);
          write(fd_output, "\n", 1);

          lock.l_type = F_UNLCK;
          fcntl(fd_output, F_SETLK, &lock);

          usleep(time);
        }

        else
          break;
      }
    }
  }

  close(fd_input);
  close(fd_output);

  return 0;
}
