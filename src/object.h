#ifndef __OBJECT_H__
#define __OBJECT_H__

#include <stdbool.h>

typedef enum
{
    kOT_constant,
    kOT_integer,
    kOT_list,
    kOT_symbol,
    kOT_string,
    kOT_env,
} object_type_t;

typedef enum
{
    kCT_nil,
} constant_type_t;

typedef struct s_object
{
    object_type_t type;
    union
    {
        // _
        char _;
        // constant
        constant_type_t constant;
        // integer
        int integer;
        // list
        struct
        {
            struct s_object *head;
            struct s_object *tail;
            struct s_object *last;
        } list;
        // env
        struct
        {
            struct s_object *vars;
            struct s_object *parent;
        } env;
        // symbol
        char symbol[1];
        // string
        char string[1];
    };
} * object_t;

#ifdef __cplusplus
extern "C"
{
#endif

    // string
    object_t object_create_string(const char *name);
    // symbol
    object_t object_create_symbol(const char *name);
    // integer
    object_t object_create_integer(int value);
    // list
    object_t object_list_create();
    void object_list_push(object_t *list_ptr, object_t object);
    bool object_list_is_empty(object_t object);
    // env
    object_t object_create_env(void);

    void object_print(object_t self);

#ifdef __cplusplus
}
#endif

#endif /* __OBJECT_H__ */
