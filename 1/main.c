#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#define MAX_FILE_SIZE 9999999999

char *lcg(const int *const x0, const int *const a, const int *const c,
          const int *const m, const size_t *const size) {
  // Линейный конруэнтный генератор псевдослучайных чисел
  char *ptr = (char *)malloc(*size + 1);

  ptr[0] = *x0;
  for (int i = 1; i < *size; i++) {
    ptr[i] = (*a * ptr[0] + *c) % *m;
  }

  return ptr;
}

void otp(const char *const x, const char *const key, const size_t *const size,
         char *ptr) {
  // Шифр Вернама
  // x - A или C
  // key - ключ

  for (int i = 0; i < *size; i++) {
    ptr[i] = x[i] ^ key[i];
  }
}

typedef struct otp_params {
  char *x;
  char *key;
  size_t size;
  char *output;
  pthread_barrier_t *bar;

} otp_params;

void *thread_otp(void *params) {
  otp(((otp_params *)params)->x, ((otp_params *)params)->key,
      &((otp_params *)params)->size, ((otp_params *)params)->output);
  pthread_barrier_wait(((otp_params *)params)->bar);
  pthread_exit(0);
}

typedef struct lcg_params {
  int x0;
  int a;
  int c;
  int m;
  size_t size;
} lcg_params;

void *thread_lcg(void *params) {
  // Обертка
  char *ptr = lcg(&((lcg_params *)params)->x0, &((lcg_params *)params)->a,
                  &((lcg_params *)params)->c, &((lcg_params *)params)->m,
                  &((lcg_params *)params)->size);

  pthread_exit(ptr);
}

int main(int argc, char *argv[]) {
  struct timeval start, end;
  gettimeofday(&start, NULL);
  // НАЧАЛО ЭТАПА 1
  // Прочитать параметры командной строки, распаковать их в структуру.
  int opt;
  char *input_file = NULL;
  char *output_file = NULL;
  lcg_params lcg_struct;

  while ((opt = getopt(argc, argv, "i:o:x:a:c:m:")) != -1) {
    switch (opt) {
    case 'i':
      input_file = optarg;
      break;
    case 'o':
      output_file = optarg;
      break;
    case 'x': {
      // x0 = atoi(optarg);
      // sscanf(optarg, "%d", &x0);
      lcg_struct.x0 = atoi(&optarg[1]);
      break;
    }
    case 'a':
      lcg_struct.a = atoi(optarg);
      break;
    case 'c':
      lcg_struct.c = atoi(optarg);
      break;
    case 'm':
      lcg_struct.m = atoi(optarg);
      break;
    case '?':
      if (optopt == 'i' || optopt == 'o' || optopt == 'x' || optopt == 'a' ||
          optopt == 'c' || optopt == 'm')
        fprintf(stderr, "Option -%c requires an argument.\n", optopt);
      else
        fprintf(stderr, "Unknown option `-%c'.\n", optopt);
      exit(EXIT_FAILURE);
    default:
      abort();
    }
  }

  printf("input_file: %s\n", input_file);
  printf("output_file: %s\n", output_file);
  printf("X0: %d\n", lcg_struct.x0);
  printf("A: %d\n", lcg_struct.a);
  printf("C: %d\n", lcg_struct.c);
  printf("M: %d\n", lcg_struct.m);

  // КОНЕЦ ЭТАПА 1
  gettimeofday(&end, NULL);
  long int diff =
      (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
  printf("Этап 1:%lo мс.\n", diff);

  // НАЧАЛО ЭТАПА 2
  // Прочитать файл c открытым текстом в бинарном виде, отобразить в оперативную
  // память. Получить размер файла. Предусмотреть ограничение на размер файла.
  gettimeofday(&start, NULL);

  FILE *fd_i;
  char *file_data;
  size_t file_size;

  fd_i = fopen(input_file, "rb");
  if (fd_i == NULL) {
    fprintf(stderr, "Error opening file.\n");
    return 1;
  }

  fseek(fd_i, 0L, SEEK_END);
  lcg_struct.size = ftell(fd_i);
  fseek(fd_i, 0L, SEEK_SET);

  file_data = (char *)malloc(lcg_struct.size);
  if (file_data == NULL) {
    fprintf(stderr, "Error allocating memory.\n");
    return 1;
  }

  file_size = fread(file_data, sizeof(char), lcg_struct.size, fd_i);
  if (file_size != lcg_struct.size) {
    fprintf(stderr, "Error reading file.\n");
    return 1;
  }

  // КОНЕЦ ЭТАПА 2
  gettimeofday(&end, NULL);
  diff = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
  printf("Этап 2:%lo мс.\n", diff);
  // НАЧАЛО ЭТАПА 3
  // Создать ПСП по прочитанным параметрам с помощью ЛКГ в отдельном потоке
  // pthread API.
  gettimeofday(&start, NULL);

  long threads_num = sysconf(_SC_NPROCESSORS_ONLN);

  char *output_buffer = (char *)malloc(lcg_struct.size + 1);
  output_buffer[lcg_struct.size] = '\0';

  pthread_t lcg_thread, ptp_threads[threads_num];
  otp_params otp_struct[threads_num];

  if (pthread_create(&lcg_thread, NULL, &thread_lcg, (void *)&lcg_struct) < 0) {
    fprintf(stderr, "Thread creation error.\n");
    return 1;
  }

  // КОНЕЦ ЭТАПА 3
  gettimeofday(&end, NULL);
  diff = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
  printf("Этап 3:%lo мс.\n", diff);
  // НАЧАЛО ЭТАПА 4
  // Синхронизировать основной поток с рабочим с помощью присоединения.
  gettimeofday(&start, NULL);

  char *key;
  if (pthread_join(lcg_thread, (void **)&key) < 0) {
    fprintf(stderr, "Thread join error.\n");
    return 1;
  }

  // КОНЕЦ ЭТАПА 4
  gettimeofday(&end, NULL);
  diff = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
  printf("Этап 4:%lo мс.\n", diff);
  // НАЧАЛО ЭТАПА 5
  // Создать барьер
  gettimeofday(&start, NULL);

  pthread_barrier_t bar;
  pthread_barrier_init(&bar, NULL, threads_num + 1);
  // КОНЕЦ ЭТАПА 5
  gettimeofday(&end, NULL);
  diff = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
  printf("Этап 5:%lo мс.\n", diff);
  // НАЧАЛО ЭТАПА 6
  // Задекларировать структуру — контекст, содержащую барьер, входные данные для
  // каждого воркера (фрагменты блокнота и открытого текста), а также
  // предусматривающую получение выходных данных от воркера.
  gettimeofday(&start, NULL);

  size_t block_size = lcg_struct.size / threads_num;
  size_t addition_to_last_block = lcg_struct.size - (threads_num * block_size);
  for (size_t i = 0; i < threads_num; i++) {
    otp_struct[i].output = output_buffer + block_size * i;
    otp_struct[i].key = key + block_size * i;
    otp_struct[i].size = block_size;
    otp_struct[i].x = file_data + block_size * i;
    otp_struct[i].bar = &bar;
    if (i == (threads_num - 1)) {
      otp_struct[i].size += addition_to_last_block;
    }
  }

  // КОНЕЦ ЭТАПА 6
  gettimeofday(&end, NULL);
  diff = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
  printf("Этап 6:%lo мс.\n", diff);
  // НАЧАЛО ЭТАПА 7
  //  Создать N воркеров с помощью функции pthread_create(), передав каждому
  //  экземпляр контекста.
  gettimeofday(&start, NULL);

  for (size_t i = 0; i < threads_num; i++) {
    if (pthread_create(&ptp_threads[i], NULL, &thread_otp, &otp_struct[i]) <
        0) {
      fprintf(stderr, "Thread creation error.\n");
      return 1;
    }
  }
  // КОНЕЦ ЭТАПА 7
  gettimeofday(&end, NULL);
  diff = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
  printf("Этап 7:%lo мс.\n", diff);
  // НАЧАЛО ЭТАПА 8
  // Главный поток блокируется по ожиданию барьера.
  gettimeofday(&start, NULL);

  pthread_barrier_wait(&bar);

  // КОНЕЦ ЭТАПА 8
  gettimeofday(&end, NULL);
  diff = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
  printf("Этап 7:%lo мс.\n", diff);
  // НАЧАЛО ЭТАПА 10
  //  Когда счётчик n барьера становится равен N + 1 (n = N + 1), главный поток
  //  разблокируется и продолжает работу.
  gettimeofday(&start, NULL);

  fclose(fd_i);
  free(file_data);

  FILE *fd_o;
  fd_o = fopen(output_file, "w");
  if (fd_o == NULL) {
    fprintf(stderr, "File descriptor open error.\n");
    return 1;
  }

  // Главный поток объединяет и сохраняет данные в выходной файл.

  size_t written = fwrite(output_buffer, sizeof(char), lcg_struct.size, fd_o);

  // КОНЕЦ ЭТАПА 10
  gettimeofday(&end, NULL);
  diff = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
  printf("Этап 7:%lo мс.\n", diff);
  // НАЧАЛО ЭТАПА 11
  //  Когда счётчик n барьера становится равен N + 1 (n = N + 1), главный поток
  //  разблокируется и продолжает работу.
  gettimeofday(&start, NULL);

  pthread_barrier_destroy(&bar);

  fclose(fd_o);
  free(output_buffer);
  free(key);

  printf("\n\n");
  return 0;
}
