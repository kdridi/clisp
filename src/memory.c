#include "memory.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

struct s_memory {
  bool alive;
  size_t counter;
  void (*free)(void *ptr);
  char data[1];
};

void *memory_create(size_t size, void (*free)(void *)) {
  struct s_memory *self = NULL;

  size += sizeof(*self) - 1;
  self = malloc(size);
  assert(self != NULL);
  memset(self, 0, size);

  self->alive = true;
  self->free = free;

  return (self->data);
}

#define get(data)                                                              \
  ((struct s_memory *)(((char *)data) - offsetof(struct s_memory, data)))

#include <stdio.h>

void *memory_retain(void *data) {
  struct s_memory *ptr = get(data);

  assert(ptr->alive == true);
  ptr->counter += 1;
  return (data);
}

#include "object.h"

void *memory_release(void *data) {
  struct s_memory *ptr = get(data);
  assert(ptr->alive == true);

  if (ptr->counter > 100) {
    printf("ptr->counter: %ld\n", ptr->counter);
    assert(false);
  }
  if (ptr->counter > 0) {
    ptr->counter -= 1;
    return (data);
  }

  ptr->alive = false;
  if (ptr->free != NULL)
    ptr->free(data);

  free(ptr);

  return (NULL);
}
