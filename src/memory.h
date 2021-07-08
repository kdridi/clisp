#ifndef __MEMORY_H_
#define __MEMORY_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    void *memory_create(size_t size, void (*free)(void *ptr));

    void *memory_retain(void *ptr);
    void *memory_release(void *ptr);

#ifdef __cplusplus
}
#endif

#endif /* __MEMORY_H_ */
