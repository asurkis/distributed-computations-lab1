#ifndef _MAIN_H_
#define _MAIN_H_

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

#define DEBUG                                                                  \
  do {                                                                         \
    fprintf(stderr, "Process %zu [%s:%d]\n", self->id, __FILE__, __LINE__);    \
    fflush(stderr);                                                            \
  } while (0)

#define CHK_RETCODE(code)                                                      \
  do {                                                                         \
    intmax_t result__ = code;                                                  \
    if (result__ < 0) {                                                        \
      fprintf(stderr, "[%s:%d] %s\n", __FILE__, __LINE__, #code);              \
      return result__;                                                         \
    }                                                                          \
  } while (0)

#define CHK_ERRNO(code)                                                        \
  do {                                                                         \
    intmax_t result__ = code;                                                  \
    if (result__ < 0) {                                                        \
      perror("[" __FILE__ "] " #code);                                         \
      fprintf(stderr, "[%s:%d] errno = %d\n", __FILE__, __LINE__, errno);      \
      return result__;                                                         \
    }                                                                          \
  } while (0)

struct Self {
  int *pipes;
  FILE *events_log;
  FILE *pipes_log;

  size_t id;
  size_t n_processes;

  timestamp_t local_time;
};

#endif
