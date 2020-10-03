#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#ifndef COMMON_H_
#define COMMON_H_

typedef struct request {
	unsigned int from, to;
} request_t;

void error_exit(char *message, int error);
void write_log(int fd, char *message);
int get_timestamp(struct timeval timestamp, char *buffer);
double get_duration(struct timeval start, struct timeval end);

#endif /* COMMON_H_ */
