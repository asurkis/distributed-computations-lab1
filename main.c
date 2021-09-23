#include "main.h"

static size_t pipe_pos(size_t i, size_t j) { return i * (i - 1) + 2 * j; }

static int init_process(struct Self *self) {
  int *pipes_old = self->pipes;
  self->pipes = malloc(sizeof(int) * 2 * self->n_processes);

  for (size_t i = 0; i < self->n_processes; ++i) {
    for (size_t j = 0; j < i; ++j) {
      if (i != self->id && j != self->id) {
        CHK_ERRNO(close(pipes_old[pipe_pos(i, j) + 0]));
        CHK_ERRNO(close(pipes_old[pipe_pos(i, j) + 1]));
        fprintf(self->pipes_log, "Process %zu closed pipe %d\n", self->id,
                pipes_old[pipe_pos(i, j) + 0]);
        fprintf(self->pipes_log, "Process %zu closed pipe %d\n", self->id,
                pipes_old[pipe_pos(i, j) + 1]);
      }
    }
  }
  fflush(self->pipes_log);

  for (size_t i = 0; i < self->id; ++i) {
    self->pipes[2 * i + 0] = pipes_old[pipe_pos(self->id, i) + 0];
    self->pipes[2 * i + 1] = pipes_old[pipe_pos(self->id, i) + 1];
  }

  for (size_t i = self->id + 1; i < self->n_processes; ++i) {
    self->pipes[2 * i + 0] = pipes_old[pipe_pos(i, self->id) + 0];
    self->pipes[2 * i + 1] = pipes_old[pipe_pos(i, self->id) + 1];
  }

  free(pipes_old);
  self->pipes[2 * self->id + 0] = -1;
  self->pipes[2 * self->id + 1] = -1;
  return 0;
}

static int deinit_process(struct Self *self) {
  for (size_t i = 0; i < 2 * self->n_processes; ++i) {
    int p = self->pipes[i];
    if (p != -1) {
      CHK_ERRNO(close(p));
      fprintf(self->pipes_log, "Process %zu closed pipe %d\n", self->id, p);
    }
  }
  fflush(self->pipes_log);
  free(self->pipes);
  fclose(self->pipes_log);
  fclose(self->events_log);
  return 0;
}

static void log_event(struct Self *self, char *buf, size_t len) {
  fwrite(buf, sizeof(char), len, self->events_log);
  fflush(self->events_log);
  fwrite(buf, sizeof(char), len, stdout);
  fflush(stdout);
}

static int wait_for_msg_from_children(struct Self *self, Message *msg,
                                      int16_t type) {
  for (size_t i = 1; i < self->n_processes; ++i) {
    if (i != self->id) {
      int received_start = 0;
      while (!received_start) {
        int receive_result = receive(self, (local_id)i, msg);
        CHK_RETCODE(receive_result);
        received_start = msg->s_header.s_type == type;
      }
    }
  }
  return 0;
}

#define DEBUG fprintf(stderr, "%zu: [%s:%d]\n", self->id, __FILE__, __LINE__);

static int run_child(struct Self *self) {
  init_process(self);
  Message msg;
  size_t len;
  msg.s_header.s_magic = MESSAGE_MAGIC;

  len = snprintf(msg.s_payload, sizeof(msg.s_payload), log_started_fmt,
                 (int)self->id, (int)getpid(), (int)getppid());
  log_event(self, msg.s_payload, len);

  msg.s_header.s_local_time = self->local_time;
  msg.s_header.s_type = STARTED;
  msg.s_header.s_payload_len = len;
  CHK_RETCODE(send_multicast(self, &msg));

  CHK_RETCODE(wait_for_msg_from_children(self, &msg, STARTED));

  len = snprintf(msg.s_payload, sizeof(msg.s_payload),
                 log_received_all_started_fmt, (int)self->id);
  log_event(self, msg.s_payload, len);

  len = snprintf(msg.s_payload, sizeof(msg.s_payload), log_done_fmt,
                 (int)self->id);
  log_event(self, msg.s_payload, len);

  msg.s_header.s_local_time = self->local_time;
  msg.s_header.s_type = DONE;
  msg.s_header.s_payload_len = len;
  CHK_RETCODE(send_multicast(self, &msg));

  CHK_RETCODE(wait_for_msg_from_children(self, &msg, DONE));

  len = snprintf(msg.s_payload, sizeof(msg.s_payload),
                 log_received_all_done_fmt, (int)self->id);
  log_event(self, msg.s_payload, len);

  deinit_process(self);
  return 0;
}

static int run_parent(struct Self *self) {
  init_process(self);
  Message msg;
  size_t len;
  msg.s_header.s_magic = MESSAGE_MAGIC;

  CHK_RETCODE(wait_for_msg_from_children(self, &msg, STARTED));
  len = snprintf(msg.s_payload, sizeof(msg.s_payload),
                 log_received_all_started_fmt, (int)self->id);
  log_event(self, msg.s_payload, len);

  CHK_RETCODE(wait_for_msg_from_children(self, &msg, DONE));
  len = snprintf(msg.s_payload, sizeof(msg.s_payload),
                 log_received_all_done_fmt, (int)self->id);
  log_event(self, msg.s_payload, len);

  for (size_t i = 1; i < self->n_processes; ++i) {
    pid_t pid = wait(NULL);
    CHK_ERRNO(pid);
  }
  deinit_process(self);
  return 0;
}

struct Self self;

int main(int argc, char **argv) {
  if (argc < 3 || sscanf(argv[2], "%zu", &self.n_processes) != 1 ||
      self.n_processes < 1) {
    printf("Usage: %s -p <number of processes>\n", argv[0]);
    return 0;
  }

  self.pipes_log = fopen(pipes_log, "w");
  self.events_log = fopen(events_log, "w");

  self.pipes = malloc(sizeof(int) * self.n_processes * (self.n_processes - 1));
  for (size_t i = 0; 2 * i < self.n_processes * (self.n_processes - 1); ++i) {
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
