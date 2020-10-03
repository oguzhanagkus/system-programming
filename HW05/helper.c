/* CSE344 - Systems Programming Course - HW05 - Threads and Mutexes

  Oğuzhan Agkuş - 161044003 */

#include "helper.h"

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

double absolute(double x) {
  if (x < 0)
    return x * -1;
  else
    return x;
}

/* ---------------------------------------- */

double cheb_distance(coordinate_t u, coordinate_t v) {
  double temp_x, temp_y;

  temp_x = abs(u.x - v.x);
  temp_y = abs(u.y - v.y);

  if (temp_x > temp_y)
    return temp_x;
  else
    return temp_y;
}

/* ---------------------------------------- */

char *flower_type(const data_t *data, size_t i) {
  return data->flowers[i];
}

/* ---------------------------------------- */

int flower_index(const data_t *data, const char *flower) {
  size_t i;

  if (data->flower_count == 0)
    return -1;

  for (i = 0; i < data->flower_count; i++) {
    if (strcmp(data->flowers[i], flower) == 0)
     return i;
  }

  return -1;
}

/* ---------------------------------------- */

size_t read_line(int fd, char *buffer) {
  size_t count = 0;
  char read_buffer = '\0';

  while (read(fd, &read_buffer, 1) == 1) {
    if (read_buffer != '\n')
      buffer[count++] = read_buffer;
    else
      break;
  }
  buffer[count] = '\0';

  return count;
};

/* ---------------------------------------- */

data_t *read_data(int fd) {
  void *temp;
  data_t *data;
  size_t florist_size = 4, flower_size = 4, stock_size = 4, bytes_read = 0, info = 0, i, j;
  char line[128], *token, *saved, *delimeters = " ,;:()";
  int type;

  /* ---------- */

  data = (data_t *) malloc(sizeof(data_t));
  if (data == NULL)
    return NULL;

  data->florist_count = 0;
  data->flower_count = 0;
  data->stock_table = NULL;

  data->florists = (florist_t *) malloc(florist_size * sizeof(florist_t));
  if (data->florists == NULL) {
    free(data);
    return NULL;
  }

  data->flowers = (char (*)[]) malloc(sizeof(char[flower_size][STR_LEN]));
  if (data->flowers == NULL) {
    free(data->florists);
    free(data);
    return NULL;
  }

  /* ---------- */

  while ((bytes_read = read_line(fd, line)) != 0) {
    if (data->florist_count == florist_size) {
      florist_size += florist_size;
      temp = realloc(data->florists, florist_size * sizeof(florist_t));
      if (temp == NULL) {
        free(data->florists);
        free(data->flowers);
        free(data);
        return NULL;
      }
      else
        data->florists = (florist_t *) temp;
    }

    /* ----- */

    saved = line;
    data->florists[data->florist_count].stock = 0;
    data->florists[data->florist_count].flowers = (int *) malloc(stock_size * sizeof(int));
    if (data->florists[data->florist_count].flowers == NULL)
      free_data(data);

    while ((token = strtok_r(saved, delimeters, &saved)) != NULL) {
      switch (info) {
        case 0:
          strcpy(data->florists[data->florist_count].name, token); break;
        case 1:
          data->florists[data->florist_count].location.x = atof(token); break;
        case 2:
          data->florists[data->florist_count].location.y = atof(token); break;
        case 3:
          data->florists[data->florist_count].speed = atof(token); break;
        default:
          type = flower_index(data, token);
          if (type == -1) {
            if (data->flower_count == flower_size) {
              flower_size += flower_size;
              temp = realloc(data->flowers, sizeof(char[flower_size][STR_LEN]));
              if (temp == NULL) {
                free_data(data);
                return NULL;
              }
              else
                data->flowers = (char (*)[]) temp;
            }

            type = data->flower_count;
            strcpy(data->flowers[data->flower_count], token);
            data->flower_count += 1;
          }

          if (data->florists[data->florist_count].stock == stock_size) {
            stock_size += stock_size;
            temp = realloc(data->florists[data->florist_count].flowers, stock_size * sizeof(int));
            if (temp == NULL)
              free_data(data);
            else
              data->florists[data->florist_count].flowers = (int *) temp;
          }

          data->florists[data->florist_count].flowers[data->florists[data->florist_count].stock] = type;
          data->florists[data->florist_count].stock += 1;
          break;
      }
      info += 1;
    }
    if (data->florists[data->florist_count].stock < stock_size) {
      temp = realloc(data->florists[data->florist_count].flowers, data->florists[data->florist_count].stock * sizeof(int));
      if (temp == NULL)
        free_data(data);
      else
        data->florists[data->florist_count].flowers = (int *) temp;
    }

    data->florist_count += 1;
    stock_size = 4;
    info = 0;
  }

  /* ---------- */

  if (data->florist_count < florist_size) {
    temp = realloc(data->florists, florist_size * sizeof(florist_t));
    if (temp == NULL) {
      free_data(data);
      return NULL;
    }
    else
      data->florists = (florist_t *) temp;
  }

  if (data->flower_count < flower_size) {
    temp = realloc(data->flowers, sizeof(char[data->flower_count][STR_LEN]));
    if (temp == NULL) {
      free_data(data);
      return NULL;
    }
    else
      data->flowers = (char (*)[]) temp;
  }

  /* ---------- */

  data->stock_table = (int **) malloc(data->florist_count * sizeof(int *));
  if (data->stock_table == NULL) {
    free_data(data);
    return NULL;
  }

  for (i = 0; i < data->florist_count; i++) {
    data->stock_table[i] = (int *) malloc(data->flower_count * sizeof(int));
    if (data->stock_table[i] == NULL) {
      for (j = 0; j < i; j++)
        free(data->stock_table[j]);
      free(data->stock_table);

      data->stock_table = NULL;
      free_data(data);
    }
  }

  for (i = 0; i < data->florist_count; i++) {
    for (j = 0; j < data->flower_count; j++)
      data->stock_table[i][j] = 0;
  }

  for (i = 0; i < data->florist_count; i++) {
    for (j = 0; j < data->florists[i].stock; j++)
      data->stock_table[i][data->florists[i].flowers[j]] = 1;
  }

  return data;
}

/* ---------------------------------------- */

void free_data(data_t *data) {
  size_t i;

  if (data->stock_table != NULL) {
    for (i = 0; i < data->florist_count; i++)
        free(data->stock_table[i]);
    free(data->stock_table);
  }

  for (i = 0; i < data->florist_count; i++)
    free(data->florists[i].flowers);
  free(data->florists);
  free(data->flowers);
  free(data);
}

/* ---------------------------------------- */

void show_data(const data_t *data) {
  size_t i, j;
  int temp;

  printf("Florists:\n");
  for (i = 0; i < data->florist_count; i++) {
    printf("   -> %s, (%.2lf,%.2lf), %.2lf: ", data->florists[i].name, data->florists[i].location.x,
                                      data->florists[i].location.y, data->florists[i].speed);
    for (j = 0; j < data->florists[i].stock - 1; j++)
      printf("%s, ", flower_type(data, data->florists[i].flowers[j]));
    printf("%s\n", flower_type(data, data->florists[i].flowers[j]));
  }

  printf("\nFlower types:\n");
  for (i = 0; i < data->flower_count; i++)
  printf("   -> %s\n", data->flowers[i]);

  printf("\nStock table:\n");
  for (i = 0; i < data->florist_count; i++) {
    printf("   ");
    for (j = 0; j < data->flower_count; j++) {
      temp = data->stock_table[i][j];
      printf("%d ", temp);
    }
    printf("\n");
  }
}

/* ---------------------------------------- */

order_t *read_order(const data_t *data, char *line) {
  order_t *order;
  char *token, *saved, *delimeters = " ,:()";
  size_t info = 0;

  order = (order_t *) malloc(sizeof(order_t));
  if (order == NULL)
    return NULL;

  saved = line;

  while((token = strtok_r(saved, delimeters, &saved)) != NULL) {
    switch (info) {
      case 0:
        strcpy(order->client, token); break;
      case 1:
        order->location.x = atof(token); break;
      case 2:
        order->location.y = atof(token); break;
      case 3:
        order->request = flower_index(data, token); break;
      default:
        break;
    }
    info += 1;
  }

  return order;
}

/* ---------------------------------------- */

void print_reports(report_t *reports, size_t count) {
  size_t i;
  char *line = "------------------------------------------------------------";

  printf("\n%s\n", line);
  printf("%-16s\t%-16s\t%-16s\n", "Florists", "# of sales", "Total time");
  printf("%s\n", line);
  for (i = 0; i < count; i++)
    printf("%-16s\t%-16d\t%.0lfms\n", reports[i].name, reports[i].delivered, reports[i].time);
  printf("%s\n", line);
}
