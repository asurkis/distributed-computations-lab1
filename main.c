#include "common.h"
#include "ipc.h"
#include "pa1.h"
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define CHK_RETCODE(code)                                                      \
  do {                                                                         \
    intmax_t result__ = code;                                                  \
    if (result__ < 0)                                                          \
      return result__;                                                         \
  } while (0)

#define CHK_ERRNO(code)                                                        \
  do {                                                                         \
    intmax_t result__ = code;                                                  \
    if (result__ < 0) {                                                        \
      perror("[" __FILE__ "] " #code);                                         \
      fprintf(stderr, "[%s:%d] errno = %d", __FILE__, __LINE__, errno);        \
      return result__;                                                         \
    }                                                                          \
  } while (0)

size_t n_processes;
pid_t *children_pidx;

/* K-th row contains pipes for processes K and from 0 to (K - 1)
   Therefore it starts at index (K * (K - 1) / 2)
   (0 => no pipes
    1 => 0
    2 => 1, 2
    3 => 3, 4, 5
    4 => 6, 7, 8, 9
    ...)
   Times 2 for pipes of both types

   Pipe between I and J where I < J is at (J * (J - 1) / 2 + I) */
int *pipes;

int run_child(size_t my_id) {
  free(children_pidx);
  return 0;
}

int run_parent(size_t my_id) {
  for (size_t i = 1; i < n_processes; ++i) {
    pid_t pid = wait(NULL);
    CHK_ERRNO(pid);
    printf("Child (pid %d) terminated\n", pid);
  }
  return 0;
}

int main(int argc, char **argv) {
  if (argc < 3 || sscanf(argv[2], "%zu", &n_processes) != 1 ||
      n_processes < 1) {
    printf("Usage: %s -p <number of processes>\n", argv[0]);
    return 0;
  }

  pipes = malloc(sizeof(int) * n_processes * (n_processes - 1));
  for (size_t i = 0; 2 * i < n_processes * (n_processes - 1); ++i) {
    CHK_ERRNO(pipe(&pipes[2 * i]));
  }

  children_pidx = malloc(sizeof(pid_t) * (n_processes - 1));
  for (size_t id = 1; id < n_processes; ++id) {
    pid_t pid = children_pidx[id - 1] = fork();
    CHK_ERRNO(pid);
    if (!pid)
      return run_child(id);
    printf("Child (pid %d) started\n", pid);
  }

  return run_parent(0);
}
