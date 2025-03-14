#ifndef __OBJECT_H__
#define __OBJECT_H__

#include <stdbool.h>
#include <stddef.h>

typedef enum
{
    kOT_constant = 1,
    kOT_integer = 2,
    kOT_list = 3,
    kOT_symbol = 4,
    kOT_string = 5,
    kOT_env = 6,
    kOT_primitive = 7,
    kOT_function = 8,
} object_type_t;

typedef enum
{
    kCT_nil,
    kCT_true,
} constant_type_t;

typedef struct s_object *object_t;

typedef object_t primitive_t(object_t env, object_t args);

struct s_object
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
        // primitive
        primitive_t *primitive;
        // function
        struct
        {
            struct s_object *params;
            struct s_object *body;
            struct s_object *env;
        } function;
    };
};

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
    size_t object_list_length(object_t list);
    void object_list_push(object_t *list_ptr, object_t object);
    bool object_list_is_empty(object_t object);
    // env
    object_t object_create_env(int argc, const char **argv);
    // eval
    object_t object_eval(object_t env, object_t object);

    void object_print(object_t self);
    void object_dump(object_t self);
    void object_delete(void *ptr);

#ifdef __cplusplus
}
#endif

#define object_new(_name, _value, _block)                                         \
    do                                                                            \
    {                                                                             \
        object_t __value = _value;                                                \
        if (__value != NULL)                                                      \
        {                                                                         \
            __attribute__((__cleanup__(object_delete))) object_t _name = __value; \
            assert(_name != NULL);                                                \
            _block;                                                               \
        }                                                                         \
    } while (0)

#endif /* __OBJECT_H__ */
