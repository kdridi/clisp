#include "stream.h"

#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
  kST_file = 0xB00B,
  kST_string = 0xCAFE,
} stream_type_t;

typedef struct {
  size_t idx;
  char *str;
} STRING;

struct s_stream_private {
  int (*next)(stream_t);
  int (*peek)(stream_t);

  stream_type_t type;
  union {
    FILE *file;
    STRING string;
  };
};

#define private(ptr) ((struct s_stream_private *)ptr)

static struct s_stream_private *as(stream_t ptr, stream_type_t type) {
  assert(private(ptr)->type == type);
  return (private(ptr));
}

#define as_file(ptr) (as(ptr, kST_file)->file)

static int next_file(stream_t ptr) { //
  return getc(as_file(ptr));
}

static int peek_file(stream_t ptr) {
  int ch = next_file(ptr);
  if (ch != EOF) {
    int err = ungetc(ch, as_file(ptr));
    assert(err != EOF);
  }
  return ch;
}

#define as_string(ptr) (as(ptr, kST_string)->string)

static int peek_string(stream_t ptr) {
  char ch = as_string(ptr).str[as_string(ptr).idx];
  return ch == 0 ? EOF : ch;
}

static int next_string(stream_t ptr) {
  char ch = peek_string(ptr);
  if (ch != EOF)
    as_string(ptr).idx += 1;
  return ch;
}

stream_t stream_skip(stream_t stream, const char *string) {
  while (strchr(string, stream->peek(stream)) != NULL) {
    stream->next(stream);
  }
  return stream;
}

void stream_delete(void *ptr) {
  stream_t self = ptr;
  assert(self != NULL);

  switch (private(self)->type) {
  case kST_file: {
    break;
  }
  case kST_string: {
    free(private(self)->string.str);
    break;
  }
  default: {
    assert(false);
    break;
  }
  }
}

static stream_t stream_create(stream_type_t type, ...) {
  struct s_stream_private *self = memory_create(sizeof(*self), stream_delete);
  va_list ap;

  va_start(ap, type);
  switch (self->type = type) {
  case kST_file: {
    self->file = va_arg(ap, typeof(self->file));
    self->next = next_file;
    self->peek = peek_file;
    break;
  }
  case kST_string: {
    self->string.str = strdup(va_arg(ap, typeof(self->string.str)));
    self->string.idx = 0;
    self->next = next_string;
    self->peek = peek_string;
    break;
  }
  default: {
    assert(false);
    break;
  }
  }
  va_end(ap);

  return ((stream_t)self);
}

stream_t stream_create_from_file(FILE *file) {
  return stream_create(kST_file, file);
}

stream_t stream_create_from_string(const char *str) {
  return stream_create(kST_string, str);
}
