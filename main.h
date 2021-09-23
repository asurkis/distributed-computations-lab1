#ifndef _MAIN_H_
#define _MAIN_H_

#include "common.h"
#include "ipc.h"
#include "pa1.h"
#include <errno.h>
#include <inttypes.h>
#include <poll.h>
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

struct Self {
  /* K-th row contains pipes for processes K and from 0 to (K - 1)
    Therefore it starts at index (K * (K - 1) / 2)
        0 => no pipes
        1 => 0
        2 => 1, 2
        3 => 3, 4, 5
        4 => 6, 7, 8, 9
        ...
    Times 2 for pipes of both types

    Pipe between I and J where I < J is at (J * (J - 1) / 2 + I) */
  int *pipes;
  struct pollfd *polls;
  FILE *events_log;
  FILE *pipes_log;

  size_t id;
  size_t n_processes;

  timestamp_t local_time;
};

#endif
