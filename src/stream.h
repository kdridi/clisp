#ifndef __STREAM_H__
#define __STREAM_H__

#include "memory.h"

#include <stdio.h>

typedef struct s_stream *stream_t;

struct s_stream
{
  int (*next)(stream_t);
  int (*peek)(stream_t);
};

#ifdef __cplusplus
extern "C"
{
#endif

  stream_t stream_create_from_file(FILE *fp);
  stream_t stream_create_from_string(const char *str);

  stream_t stream_skip(stream_t stream, const char *string);

  void stream_delete(void *ptr);

#define stream_new(_name, _value, _block)                                \
  do                                                                     \
  {                                                                      \
    __attribute__((__cleanup__(stream_delete))) stream_t _name = _value; \
    assert(_name != NULL);                                               \
    _block;                                                              \
  } while (0)

#ifdef __cplusplus
}
#endif

#endif /* __STREAM_H__ */
