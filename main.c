#include "main.h"

static size_t pipe_pos(size_t i, size_t j) { return i * (i - 1) + 2 * j; }

static int init_process(struct Self *self) {
  char buf[256];
  sprintf(buf, "debug_%zu.log", self->id);
  self->debug_log = fopen(buf, "w");

  int *pipes_old = self->pipes;
  for (size_t i = 0; i < self->n_processes; ++i) {
    for (size_t j = 0; j < i; ++j) {
      if (i != self->id && j != self->id) {
        int *to_close = &pipes_old[pipe_pos(i, j)];
        fprintf(self->debug_log,
                "i=%zu, j=%zu, pipe_pos=%zu, closing %d and %d\n", i, j,
                pipe_pos(i, j), to_close[0], to_close[1]);
        CHK_ERRNO(close(to_close[0]));
        CHK_ERRNO(close(to_close[1]));
      }
    }
  }

  self->pipes = malloc(2 * sizeof(int) * self->n_processes);
  for (size_t k = 0; k < 2; ++k) {
    for (size_t i = 0; i < self->id; ++i)
      self->pipes[2 * i + k] = pipes_old[pipe_pos(self->id, i) + k];
    self->pipes[2 * self->id + k] = -1;
    for (size_t i = self->id + 1; i < self->n_processes; ++i)
      self->pipes[2 * i + k] = pipes_old[pipe_pos(i, self->id) + k];
  }
  fprintf(self->debug_log, "Pipes for process:\n");
  for (size_t i = 0; i < self->n_processes; ++i)
    fprintf(self->debug_log, "with process %zu: rfd=%d wfd=%d\n", i,
            self->pipes[2 * i + 0], self->pipes[2 * i + 1]);
  free(pipes_old);
  return 0;
}

static int deinit_process(struct Self *self) {
  for (size_t i = 0; i < self->n_processes; ++i) {
    if (i != self->id) {
      int *to_close = &self->pipes[2 * i];
      fprintf(self->debug_log, "to process %zu, closing %d and %d\n", i,
              to_close[0], to_close[1]);
      CHK_ERRNO(close(to_close[0]));
      CHK_ERRNO(close(to_close[1]));
    }
  }

  fclose(self->pipes_log);
  fclose(self->events_log);
  fclose(self->debug_log);
  return 0;
}

static int run_child(struct Self *self) {
  CHK_RETCODE(init_process(self));

  CHK_RETCODE(deinit_process(self));
  return 0;
}

static int run_parent(struct Self *self) {
  CHK_RETCODE(init_process(self));

  DEBUG
  for (size_t i = 1; i < self->n_processes; ++i)
    wait(NULL);
  DEBUG

  CHK_RETCODE(deinit_process(self));
  return 0;
}

struct Self self;

int main(int argc, char **argv) {
  size_t n_children;
  if (argc < 3 || sscanf(argv[2], "%zu", &n_children) != 1) {
    printf("Usage: %s -p <number of processes>\n", argv[0]);
    return 0;
  }

  self.pipes_log = fopen(pipes_log, "w");
  self.events_log = fopen(events_log, "w");

  self.n_processes = n_children + 1;
  self.pipes = malloc(sizeof(int) * self.n_processes * n_children);
  for (size_t i = 0; 2 * i < self.n_processes * n_children; ++i) {
    CHK_ERRNO(pipe(&self.pipes[2 * i]));
    fprintf(self.pipes_log, "Open pipe %zu: rfd=%d, wfd=%d\n", i,
            self.pipes[2 * i + 0], self.pipes[2 * i + 1]);
  }
  fflush(self.pipes_log);

  self.local_time = 0;

  for (size_t id = 1; id < self.n_processes; ++id) {
    pid_t pid = fork();
    CHK_ERRNO(pid);
    if (!pid) {
      self.id = id;
      return run_child(&self);
    }
  }

  self.id = 0;
  return run_parent(&self);
}
