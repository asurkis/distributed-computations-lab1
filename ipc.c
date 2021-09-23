#include "main.h"

int send(void *self_, local_id dst_, Message const *msg) {
  struct Self *self = self_;
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
  return 0;
}

int receive_any(void *self_, Message *msg) {
  struct Self *self = self_;
  return 0;
}
