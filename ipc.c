#include "main.h"

static int write_repeat(int fd, char const *buf, size_t size) {
  while (size) {
    ssize_t shift = write(fd, buf, size);
    CHK_ERRNO(shift);
    buf += shift;
    size -= shift;
  }
  return 1;
}

static int read_repeat(int fd, char *buf, size_t size) {
  while (size) {
    ssize_t shift = read(fd, buf, size);
    CHK_ERRNO(shift);
    if (!shift)
      return 0;
    buf += shift;
    size -= shift;
  }
  return 1;
}

int send(void *self_, local_id dst_, Message const *msg) {
  struct Self *self = self_;
  write_repeat(self->pipes[2 * dst_ + 1], (char *)&msg->s_header,
               sizeof(MessageHeader));
  switch (msg->s_header.s_type) {
  case STARTED:
  case DONE:
    // CHK_RETCODE(write_repeat(self->pipes[2 * dst_ + 1], msg->s_payload,
    //                          msg->s_header.s_payload_len));
    break;

  case ACK:
  case STOP:
  case CS_REQUEST:
  case CS_REPLY:
  case CS_RELEASE:
    break;

  case TRANSFER:
  case BALANCE_HISTORY:
  default:
    return -1;
  }

  fprintf(self->debug_log, "Sent message of type %d to %d\n",
          msg->s_header.s_type, (int)dst_);
  fflush(self->debug_log);

  self->local_time++;
  return 0;
}

int send_multicast(void *self_, Message const *msg) {
  struct Self *self = self_;
  for (size_t i = 0; i < self->n_processes; ++i) {
    if (i != self->id) {
      send(self, (local_id)i, msg);
    }
  }
  return 0;
}

int receive(void *self_, local_id from, Message *msg) {
  struct Self *self = self_;
  int read_result = read_repeat(self->pipes[2 * from], (char *)&msg->s_header,
                                sizeof(MessageHeader));
  CHK_RETCODE(read_result);
  if (!read_result)
    return -1;

  fprintf(self->debug_log, "Received message of type %d from %d\n",
          msg->s_header.s_type, (int)from);
  fflush(self->debug_log);

  switch (msg->s_header.s_type) {
  case STARTED:
  case DONE:
    // read_result = read_repeat(self->pipes[2 * from], msg->s_payload,
    //                           msg->s_header.s_payload_len);
    // CHK_RETCODE(read_result);
    // if (!read_result)
    //   return -1;
    break;

  case ACK:
  case STOP:
  case CS_REQUEST:
  case CS_REPLY:
  case CS_RELEASE:
    break;

  case TRANSFER:
  case BALANCE_HISTORY:
  default:
    return -1;
  }

  self->local_time++;
  return 0;
}

int receive_any(void *self_, Message *msg) {
  struct Self *self = self_;
  (void)self;
  /* Unimplemented */
  return -1;
}
