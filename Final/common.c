#include "common.h"

/* ---------------------------------------- */

void error_exit(char *message, int error) {
  int count;
  char buffer[256];

  if (error == -1)
    count = sprintf(buffer, "%s\n", message);
  else
    count = sprintf(buffer, "%s: %s\n", message, strerror(error));

  write(STDERR_FILENO, buffer, count);
  exit(EXIT_FAILURE);
}

/* ---------------------------------------- */

void write_log(int fd, char *message) {
  int count;
  char buffer[4096];
  struct timeval temp_time;

  gettimeofday(&temp_time, NULL);
  get_timestamp(temp_time, buffer);

  count = sprintf(buffer, "%s\t| %s\n", buffer, message);

  write(fd, buffer, count);
}

/* ---------------------------------------- */

int get_timestamp(struct timeval timestamp, char *buffer) {
  struct tm time_struct;
  int year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0, milisecond = 0;

  memset(&time_struct, 0, sizeof (struct tm));
  localtime_r(&timestamp.tv_sec, &time_struct);

  year = time_struct.tm_year + 1900;
  month = time_struct.tm_mon + 1;
  day = time_struct.tm_mday;
  hour = time_struct.tm_hour;
  minute = time_struct.tm_min;
  second = time_struct.tm_sec;
  milisecond = timestamp.tv_usec / 1000;

  return sprintf(buffer, "%02d-%02d-%04d %02d:%02d:%02d.%03d", day, month, year, hour, minute, second, milisecond);
}

/* ---------------------------------------- */

double get_duration(struct timeval start, struct timeval end) {
  long seconds, microseconds;
  double duration;

  if (start.tv_sec > end.tv_sec) {
    return 0;
  }
  else if (start.tv_sec == end.tv_sec) {
    if (start.tv_usec > end.tv_usec)
      return 0;
    else if (start.tv_usec == end.tv_usec)
      return 0;
  }

  seconds = end.tv_sec - start.tv_sec;
  microseconds = end.tv_usec - start.tv_usec;
  duration = seconds + ((double) microseconds / 1000000);

  return duration;
}

/* ---------------------------------------- */
