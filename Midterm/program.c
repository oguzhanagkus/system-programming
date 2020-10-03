/* CSE344 - Systems Programming Course - Midterm Project - Mess Hall Simulation

  Sync with POSIX Unnamed Semaphores and Shared Memory

  Oğuzhan Agkuş - 161044003 */

#include "helper.h"

static volatile int live_children_count = 0;
static void sigchld_handler(int signal);

int main(int argc, char *argv[]) {
  int i, opt, count, input_fd;
  int kitchen_size, counter_size, times, taken, eaten, total_delivery;
  int undergraduate_count, graduate_count, student_count, cook_count, table_count;
  char buffer[256], read_buffer[1], food_types[3][15] = {"soup", "main course", "dessert"};
  char *input_path;

  /* -------------------- */ // Command line arguments checking

  if (argc != 15)
    error_exit("Given less/more arguments!\nUsage: ./program -N 3 -T 4 -S 4 -L 13 -U 8 -G 2 -F input_path");

  /* -------------------- */ // Getopt parsing

  while ((opt = getopt(argc, argv, "N:T:S:L:U:G:F:")) != -1) {
    switch (opt) {
      case 'N':
        cook_count = atoi(optarg); break;
      case 'T':
        table_count = atoi(optarg); break;
      case 'S':
        counter_size = atoi(optarg); break;
      case 'L':
        times = atoi(optarg); break;
      case 'U':
        undergraduate_count = atoi(optarg); break;
      case 'G':
        graduate_count = atoi(optarg); break;
      case 'F':
        input_path = optarg; break;
      default:
        error_exit("Invalid options!\nUsage: ./program -N 3 -T 4 -S 4 -L 13 -U 8 -G 2 -F input_path");
    }
  }

  /* -------------------- */ // Input checking

  if (input_path == NULL)
    error_exit("Input path missing.");

  if (cook_count <= 2)
    error_exit("N must be bigger than 2.");

  if (table_count < 1)
    error_exit("T must be bigger than 0.");

  if (graduate_count < 1)
    error_exit("G must be bigger than 1.");

  if (undergraduate_count < graduate_count)
    error_exit("U must be bigger than G.");

  student_count = undergraduate_count + graduate_count;

  if (student_count < cook_count || student_count < table_count)
    error_exit("G+U must be bigger than N and T.");

  if (counter_size <= 3)
    error_exit("S must be bigger than 3.");

  if (times < 3)
    error_exit("L must be bigger than 2.");

  if ((input_fd = open(input_path, O_RDONLY)) == -1)
    error_exit("Failed to open input path");

  if (check_input(input_fd, student_count*times) == -1)
    error_exit("Insufficient supplies in the file.");

  kitchen_size = 2 * student_count * times + 1;
  total_delivery = 3 * student_count * times;

  /* -------------------- */ // Signal handling and masking

  struct sigaction sa;
  sigset_t block_mask, empty_mask;

  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sa.sa_handler = sigchld_handler;

  if (sigaction(SIGCHLD, &sa, NULL) == -1)
    error_exit("Cannot set new signal handler");

  sigemptyset(&block_mask);
  sigaddset(&block_mask, SIGCHLD);

  if (sigprocmask(SIG_SETMASK, &block_mask, NULL) == -1)
    error_exit("Cannot set signal mask");

  /* -------------------- */ // Creating shared memory

  int flags, shm_fd;
  size_t size;
  mode_t perms;
  struct hall *my_hall;

  size = sizeof(*my_hall);
  flags = O_RDWR | O_CREAT;
  perms = S_IRUSR | S_IWUSR;
  shm_fd = shm_open("/shm_hall", flags, perms);

  if (shm_fd == -1)
    error_exit("shm_open() error");

  if (ftruncate(shm_fd, size) == -1)
    error_exit("ftruncate() error");

  my_hall = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

  if (my_hall == MAP_FAILED)
    error_exit("mmap() error");

  init_hall(my_hall, total_delivery, kitchen_size, counter_size, table_count);

  /* -------------------- */ // Forking supplier

  switch (fork()) {
    case -1: // Error
      error_exit("fork() error");

    case 0: // Child -> Supplier
      while (read(input_fd, read_buffer, 1) == 1) {
        sem_wait(&my_hall->my_kitchen.empty);
        sem_wait(&my_hall->my_kitchen.busy);

        // Entering the kitchen
        count = sprintf(buffer, "The supplier is going to the kitchen to deliver %s - kitchen items: P:%d,C:%d,D:%d = %d\n",
                                food_types[food_type(read_buffer[0])],
                                my_hall->my_kitchen.p, my_hall->my_kitchen.c, my_hall->my_kitchen.d, my_hall->my_kitchen.count);
        write(STDOUT_FILENO, buffer, count);

        my_hall->my_kitchen.count += 1;

        if (read_buffer[0] == 'P') {
          my_hall->my_kitchen.p += 1;
          sem_post(&my_hall->my_kitchen.p_sem);
        }
        else if (read_buffer[0] == 'C') {
          my_hall->my_kitchen.c += 1;
          sem_post(&my_hall->my_kitchen.c_sem);
        }
        else if (read_buffer[0] == 'D')  {
          my_hall->my_kitchen.d += 1;
          sem_post(&my_hall->my_kitchen.d_sem);
        }

        // After delivery
        count = sprintf(buffer, "The supplier delivered %s - after delivery - kitchen items: P:%d,C:%d,D:%d = %d\n",
                                food_types[food_type(read_buffer[0])],
                                my_hall->my_kitchen.p, my_hall->my_kitchen.c, my_hall->my_kitchen.d, my_hall->my_kitchen.count);
        write(STDOUT_FILENO, buffer, count);

        sem_post(&my_hall->my_kitchen.busy);
        sem_post(&my_hall->my_kitchen.full);
      }

      // Done delivering
      count = sprintf(buffer, "The supplier finished supplying - goodbye!\n");
      write(STDOUT_FILENO, buffer, count);

      exit(EXIT_SUCCESS);

    default: // Parent
      live_children_count++;
      break;
  }

  /* -------------------- */ // Forking cooks

  for (i = 1; i <= cook_count; i++) {
    switch (fork()) {
      case -1: // Error
        error_exit("fork() error");

      case 0: // Child -> Cook
        while (1) {
          if (sem_trywait(&my_hall->my_kitchen.delivered) == -1) break;;

          /* ---------- */ // Choosing meal

          sem_wait(&my_hall->my_counter.empty_);
          sem_wait(&my_hall->my_counter.busy_);

          switch (min(my_hall->my_counter.p_, my_hall->my_counter.c_, my_hall->my_counter.d_)) {
            case 0:
              taken = 0;
              my_hall->my_counter.p_ += 1; break;
            case 1:
              taken = 1;
              my_hall->my_counter.c_ += 1; break;
            case 2:
              taken = 2;
              my_hall->my_counter.d_ += 1; break;
            default:
              break;
          }

          sem_post(&my_hall->my_counter.busy_);
          sem_post(&my_hall->my_counter.full_);

          /* ---------- */ // Getting from kitchen

          // Waiting for/getting deliveries
          count = sprintf(buffer, "Cook %d is going to the kitchen to wait for/get a plate - kitchen items: P:%d,C:%d,D:%d = %d\n",
                                  i, my_hall->my_kitchen.p, my_hall->my_kitchen.c, my_hall->my_kitchen.d, my_hall->my_kitchen.count);
          write(STDOUT_FILENO, buffer, count);

          if (taken == 0)
            sem_wait(&my_hall->my_kitchen.p_sem);
          else if (taken == 1)
            sem_wait(&my_hall->my_kitchen.c_sem);
          else
            sem_wait(&my_hall->my_kitchen.d_sem);

          sem_wait(&my_hall->my_kitchen.full);
          sem_wait(&my_hall->my_kitchen.busy);

          if (taken == 0)
            my_hall->my_kitchen.p -= 1;
          else if (taken == 1)
            my_hall->my_kitchen.c -= 1;
          else
            my_hall->my_kitchen.d -= 1;
          my_hall->my_kitchen.count -= 1;

          sem_post(&my_hall->my_kitchen.busy);
          sem_post(&my_hall->my_kitchen.empty);

          /* ---------- */ // Going to counter

          // Going to counter
          count = sprintf(buffer, "Cook %d is going to the counter to deliver %s - counter items: P:%d,C:%d,D:%d = %d\n",
                                  i, food_types[taken], my_hall->my_counter.p, my_hall->my_counter.c, my_hall->my_counter.d, my_hall->my_counter.count);
          write(STDOUT_FILENO, buffer, count);

          sem_wait(&my_hall->my_counter.empty);
          sem_wait(&my_hall->my_counter.busy);

          if (taken == 0) {
            my_hall->my_counter.p += 1;
            if (my_hall->my_counter.c - my_hall->my_counter.p >= 0 &&
                my_hall->my_counter.d - my_hall->my_counter.p >= 0)
                sem_post(&my_hall->my_counter.available);
          }
          else if (taken == 1) {
            my_hall->my_counter.c += 1;
            if (my_hall->my_counter.p - my_hall->my_counter.c >= 0 &&
                my_hall->my_counter.d - my_hall->my_counter.c >= 0)
                sem_post(&my_hall->my_counter.available);
          }
          else {
            my_hall->my_counter.d += 1;
            if (my_hall->my_counter.p - my_hall->my_counter.d >= 0 &&
                my_hall->my_counter.c - my_hall->my_counter.d >= 0)
                sem_post(&my_hall->my_counter.available);
          }

          my_hall->my_counter.count += 1;

          // After delivery to counter
          count = sprintf(buffer, "Cook %d placed %s on the counter - counter items: P:%d,C:%d,D:%d = %d\n",
                                  i, food_types[taken], my_hall->my_counter.p, my_hall->my_counter.c,
                                  my_hall->my_counter.d, my_hall->my_counter.count);
          write(STDOUT_FILENO, buffer, count);

          sem_post(&my_hall->my_counter.busy);
          sem_post(&my_hall->my_counter.full);
        }

        // Done serving
        count = sprintf(buffer, "Cook %d finished serving - items at kitchen %d - going home - goodbye!\n", i, my_hall->my_kitchen.count);
        write(STDOUT_FILENO, buffer, count);

        exit(EXIT_SUCCESS);

      default: // Parent
        live_children_count++;
        break;
    }
  }

  /* -------------------- */ // Forking students

  for (i = 1; i <= student_count; i++) {
    switch (fork()) {
      case -1: // Error
        error_exit("fork() error");

      case 0: // Child -> Student
        eaten = 0;
        my_hall->my_counter.student_count += 1;

        /* ---------- */ // Graduate priority

        if (i > graduate_count) { // First n student is graduate student where n = graduate_count
          sem_wait(&my_hall->my_counter.priority);
          sem_post(&my_hall->my_counter.priority);
        }

        while (eaten < times) {
          /* ---------- */ // Waiting at counter

          // Arriving at the counter/waiting for food
          count = sprintf(buffer, "Student %d is going to the counter (round %d) - students at the counter: %d - counter items: P:%d,C:%d,D:%d = %d\n",
                          i, eaten + 1, my_hall->my_counter.student_count,  my_hall->my_counter.p, my_hall->my_counter.c,
                          my_hall->my_counter.d, my_hall->my_counter.count);
          write(STDOUT_FILENO, buffer, count);

          sem_wait(&my_hall->my_counter.available);
          sem_wait(&my_hall->my_counter.busy);
          sem_wait(&my_hall->my_counter.full);
          sem_wait(&my_hall->my_counter.full);
          sem_wait(&my_hall->my_counter.full);

          my_hall->my_counter.p -= 1;
          my_hall->my_counter.c -= 1;
          my_hall->my_counter.d -= 1;
          my_hall->my_counter.count -= 3;

          /* ----- */
          sem_wait(&my_hall->my_counter.busy_);
          sem_wait(&my_hall->my_counter.full_);
          sem_wait(&my_hall->my_counter.full_);
          sem_wait(&my_hall->my_counter.full_);
          my_hall->my_counter.p_ -= 1;
          my_hall->my_counter.c_ -= 1;
          my_hall->my_counter.d_ -= 1;
          sem_post(&my_hall->my_counter.empty_);
          sem_post(&my_hall->my_counter.empty_);
          sem_post(&my_hall->my_counter.empty_);
          sem_post(&my_hall->my_counter.busy_);
          /* ----- */

          sem_post(&my_hall->my_counter.empty);
          sem_post(&my_hall->my_counter.empty);
          sem_post(&my_hall->my_counter.empty);
          sem_post(&my_hall->my_counter.busy);

          my_hall->my_counter.student_count -= 1;

          /* ---------- */ // Going to table

          // Waiting for/getting a table
          count = sprintf(buffer, "Student %d got food and is going to get a table (round %d) - empyty tables: %d\n", i, eaten + 1, my_hall->my_tables.count);
          write(STDOUT_FILENO, buffer, count);

          sem_wait(&my_hall->my_tables.empty);
          my_hall->my_tables.count -= 1;

          // Eating
          count = sprintf(buffer, "Student %d sat at table to eat (round %d) - empyty tables: %d\n", i, eaten + 1, my_hall->my_tables.count);
          write(STDOUT_FILENO, buffer, count);

          eaten += 1;
          my_hall->my_tables.count += 1;
          sem_post(&my_hall->my_tables.empty);

          // Going to counter again
          if (eaten < times) {
            count = sprintf(buffer, "Student %d left table to eat again (round %d) - empyty tables: %d\n", i, eaten + 1, my_hall->my_tables.count);
            my_hall->my_counter.student_count += 1;
          }
          else // Last round
            count = sprintf(buffer, "Student %d left table - empyty tables: %d\n", i, my_hall->my_tables.count);

          write(STDOUT_FILENO, buffer, count);
        }

        // Done eating L times
        count = sprintf(buffer, "Student %d is done eating %d times - going home - goodbye!\n", i, eaten);
        write(STDOUT_FILENO, buffer, count);

        if (i <= graduate_count)
          my_hall->my_counter.finished += 1;

        if (my_hall->my_counter.finished == graduate_count)
          sem_post(&my_hall->my_counter.priority); // Let undergraduate students to get meal

        exit(EXIT_SUCCESS);

      default: // Parent
        live_children_count++;
        break;
    }
  }

  /* -------------------- */ // Waiting for children to exit

  sigemptyset(&empty_mask);

  while (live_children_count > 0) {
    if (sigsuspend(&empty_mask) == -1 && errno != EINTR)
      error_exit("Sigsuspend error");
  }

  /* -------------------- */

  if (close(input_fd) == -1)
    error_exit("Failed to close input path");

  if (shm_unlink("/shm_hall") == -1)
    error_exit("Failed to unlink /shm_hall");

  exit(EXIT_SUCCESS);
}

/* ---------------------------------------- */

static void sigchld_handler(int signal) {
  int status, saved_errno;
  pid_t child_pid;

  saved_errno = errno;

  while ((child_pid = waitpid(-1, &status, WNOHANG)) > 0)
    live_children_count--;

  if (child_pid == -1 && errno != ECHILD)
    error_exit("No child processes!");

  errno = saved_errno;
}
