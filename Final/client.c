#include "common.h"

unsigned int *path = NULL;

void report(unsigned int *path, int distance, double duration);
void signal_handler(int signal);
void cleanup();

int main(int argc, char *argv[]) {
  int port, start, destination, opt, client_fd, distance;
  char *ip_address, buffer[1024];
  request_t request;

  struct sockaddr_in server_address;
  struct timeval start_time, end_time;

  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  /* -------------------- */ // Command line arguments checking

  if (argc != 9)
    error_exit("Less/more arguments!\nUsage: sudo ./client -a 127.0.0.1 -p 8080 -s 768 -d 979", -1);

  /* -------------------- */ // Getopt parsing

  while ((opt = getopt(argc, argv, "a:p:s:d:")) != -1) {
    switch (opt) {
      case 'a':
        ip_address = optarg; break;
      case 'p':
        port = atoi(optarg); break;
      case 's':
        start = atoi(optarg); break;
      case 'd':
        destination = atoi(optarg); break;
      default:
        error_exit("Invalid options!\nUsage: sudo ./client -a 127.0.0.1 -p 8080 -s 768 -d 979", -1);
    }
  }

  /* -------------------- */ // Argument checking

  if (start < 0)
    error_exit("Start node must be positive integer.", -1);

  if (destination < 0)
    error_exit("Destination node must be positive integer.", -1);

  if (port < 0 || port > 65535)
    error_exit("Port number should be in [0, 65535] interval.", -1);

  /* -------------------- */

  request.from = start;
  request.to = destination;

  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(port);

  if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    error_exit("socket() failed", errno);

  if (inet_pton(AF_INET, ip_address, &server_address.sin_addr) != 1)
    error_exit("inet_pton() failed", errno);

  sprintf(buffer, "Client (%d) connecting to %s:%d", getpid(), ip_address, port);
  write_log(STDOUT_FILENO, buffer);

  if (connect(client_fd, (struct sockaddr *) &server_address, sizeof(server_address)) == -1)
    error_exit("connect() failed", errno);

  sprintf(buffer, "Client (%d) connected and requesting a path from node %d to node %d", getpid(), request.from, request.to);
  write_log(STDOUT_FILENO, buffer);

  /* -------------------- */

  if (send(client_fd, (void *) &request, sizeof(request_t), 0) == -1)
    error_exit("send() failed", errno);

  gettimeofday(&start_time, NULL);

  if (read(client_fd, (void *) &distance, sizeof(int)) == -1)
    error_exit("read() failed", errno);

  if (distance == 0) {
    gettimeofday(&end_time, NULL);
  }
  else {
    path = malloc((distance + 1) * sizeof(unsigned int));

    if (read(client_fd, (void *) path, (distance + 1) * sizeof(unsigned int)) == -1) {
      cleanup();
      error_exit("read() failed", errno);
    }

    gettimeofday(&end_time, NULL);
  }

  report(path, distance, get_duration(start_time, end_time));

  close(client_fd);
  cleanup();

  return 0;
}

/* ---------------------------------------- */

void report(unsigned int *path, int distance, double duration) {
  int i;
  char buffer[4096];

  sprintf(buffer, "Server's response to (%d): ", getpid());

  if (distance == 0) {
    sprintf(buffer, "%s No path", buffer);
  }
  else {
    for (i = 0; i < distance; i++)
      sprintf(buffer, "%s%d->", buffer, path[i]);
    sprintf(buffer, "%s%d", buffer, path[i]);
  }

  sprintf(buffer, "%s, arrived in %.6lf seconds, shutting down.", buffer, duration);

  write_log(STDOUT_FILENO, buffer);
}

/* ---------------------------------------- */

void signal_handler(int signal) {
  char buffer[1024];

  sprintf(buffer, "\n%s signal received! Terminated with CTRL-C / SIGINT.", sys_siglist[signal]);
  write_log(STDOUT_FILENO, buffer);

  cleanup();
  exit(EXIT_SUCCESS);
}

/* ---------------------------------------- */

void cleanup() {
  if (path != NULL)
    free(path);
}

/* ---------------------------------------- */
