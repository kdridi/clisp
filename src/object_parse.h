#ifndef __OBJECT_PARSE_H__
#define __OBJECT_PARSE_H__

#include "object.h"
#include "stream.h"

#ifdef __cplusplus
extern "C"
{
#endif

    object_t object_parse(stream_t s);

#ifdef __cplusplus
}
#endif

#endif /* __OBJECT_PARSE_H__ */
