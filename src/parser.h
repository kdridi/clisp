#ifndef __PARSER_H__
#define __PARSER_H__

#include "object.h"
#include "stream.h"

#ifdef __cplusplus
extern "C"
{
#endif

    object_t parse(stream_t s);

#ifdef __cplusplus
}
#endif

#endif /* __PARSER_H__ */
