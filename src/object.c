#include "object.h"
#include "memory.h"

#include <assert.h>
#include <stdbool.h>
#include <string.h>

static void unmake(void *ptr) {
  object_t object = ptr;
  switch (object->type) {
  case kOT_list:
    memory_release(object->list.tail);
    memory_release(object->list.head);
    break;
  case kOT_env:
    memory_release(object->env.vars);
    memory_release(object->env.parent);
    break;
  case kOT_constant:
  case kOT_integer:
  case kOT_symbol:
  case kOT_string:
  case kOT_primitive:
  case kOT_function:
    break;
  default:
    assert(false);
    break;
  }
}

static object_t make(object_type_t type, size_t size) {
  size += offsetof(struct s_object, _);
  object_t self = memory_create(size, unmake);
  self->type = type;
  return (self);
}

static object_t make_constant(constant_type_t constant) {
  object_t self = make(kOT_constant, sizeof(self->constant));
  self->constant = constant;
  return (self);
}

static object_t make_integer(int integer) {
  object_t self = make(kOT_integer, sizeof(self->integer));
  self->integer = integer;
  return (self);
}

static object_t make_list(object_t car, object_t cdr) {
  assert(car != NULL);
  assert(cdr != NULL);
  object_t self = make(kOT_list, sizeof(self->list));
  self->list.head = memory_retain(car);
  self->list.tail = memory_retain(cdr);
  self->list.last = NULL;
  return (self);
}

static object_t make_symbol(const char *symbol) {
  assert(symbol != NULL);
  object_t self = make(kOT_symbol, strlen(symbol) + sizeof(self->symbol));
  strcpy(self->symbol, symbol);
  return (self);
}

static object_t make_string(const char *string) {
  assert(string != NULL);
  object_t self = make(kOT_string, strlen(string) + sizeof(self->string));
  strcpy(self->string, string);
  return (self);
}

static object_t make_env(object_t vars, object_t parent) {
  assert(vars != NULL);
  assert(parent != NULL);
  object_t self = make(kOT_env, sizeof(self->env));
  self->env.vars = memory_retain(vars);
  self->env.parent = memory_retain(parent);
  return (self);
}

static object_t make_primitive(primitive_t primitive) {
  object_t self = make(kOT_primitive, sizeof(self->primitive));
  self->primitive = primitive;
  return (self);
}

object_t object_create_symbol(const char *name) { //
  return make_symbol(name);
}

object_t object_create_string(const char *value) { //
  return make_string(value);
}

object_t object_create_integer(int value) { //
  return make_integer(value);
}

object_t object_list_create(void) { //
  return make_constant(kCT_nil);
}

size_t object_list_length(object_t object) {
  size_t result = 0;
  object_t list = object;
  while (object_list_is_empty(list) == false) {
    assert(list->type == kOT_list);
    list = list->list.tail;
    result += 1;
  }
  assert(object_list_is_empty(object) == true || result > 0);
  return result;
}

void object_list_push(object_t *list_ptr, object_t object) {
  object_t list = *list_ptr;
  if (object_list_is_empty(list)) {
    list = make_list(object, list);
    list->list.last = list;
    memory_release(*list_ptr);
  } else {
    assert(list->type == kOT_list);
    assert(object_list_is_empty(list->list.last->list.tail));
    memory_release(list->list.last->list.tail);

    object_t node = NULL;
    object_new(last, object_list_create(), { //
      node = make_list(object, last);
    });

    list->list.last = list->list.last->list.tail = node;
  }

  assert(list->type == kOT_list);
  *list_ptr = list;
}

bool object_list_is_empty(object_t object) {
  assert(object != NULL);
  return ((object->type == kOT_constant) && (object->constant == kCT_nil));
}

object_t env_find(object_t env, object_t object) {
  assert(object != NULL);
  assert(object->type == kOT_symbol);

  while (env != NULL) {
    assert(env != NULL);
    assert(env->type == kOT_env);

    object_t vars = env->env.vars;
    assert(vars != NULL);
    while (object_list_is_empty(vars) == false) {
      assert(vars->type == kOT_list);
      object_t pair = vars->list.head;
      assert(pair != NULL);
      assert(pair->type == kOT_list);
      assert(pair->list.head != NULL);
      assert(pair->list.head->type == kOT_symbol);
      if (strcmp(pair->list.head->symbol, object->symbol) == 0)
        return pair->list.tail;
      vars = vars->list.tail;
    }

    env = env->env.parent;
  }

  return (NULL);
}

static void env_add(object_t env, object_t k, object_t v) {
  assert(env != NULL);
  assert(env->type == kOT_env);
  object_new(pair, make_list(k, v), {
    object_t vars = env->env.vars;
    env->env.vars = make_list(pair, env->env.vars);
    memory_release(vars);
  });
}

static void env_add_constant(object_t env, const char *name,
                             constant_type_t type) {
  object_new(key, object_create_symbol(name), {
    object_new(value, make_constant(type), { //
      env_add(env, key, value);
    });
  });
}

static void env_add_primitive(object_t env, const char *name,
                              primitive_t primitive) {
  object_new(key, object_create_symbol(name), {
    object_new(value, make_primitive(primitive), { //
      env_add(env, key, value);
    });
  });
}

static object_t primitive_do(object_t env, object_t args) {
  object_t result = NULL;
  while (object_list_is_empty(args) == false) {
    result = object_eval(env, args->list.head);
    args = args->list.tail;
  }
  return result;
}

static object_t primitive_if(object_t env, object_t args) {
  assert(object_list_length(args) >= 2);
  object_t condition = object_eval(env, args->list.head);

  if (object_list_is_empty(condition) == false) {
    return object_eval(env, args->list.tail->list.head);
  }

  if (object_list_is_empty(args->list.tail->list.tail))
    return object_eval(env, args->list.tail->list.tail);

  return primitive_do(env, args->list.tail->list.tail);
}

static object_t primitive_define(object_t env, object_t args) {
  assert(object_list_length(args) == 2);

  object_t key = args->list.head;
  assert(key->type == kOT_symbol);
  object_t value = object_eval(env, args->list.tail->list.head);
  env_add(env, key, value);
  return value;
}

#define primitive_arithmetic(_sign)                                            \
  assert(object_list_length(args) > 1);                                        \
  bool first = true;                                                           \
  int result = 0;                                                              \
  do {                                                                         \
    assert(args->type == kOT_list);                                            \
    object_t value = object_eval(env, args->list.head);                        \
    assert(value->type == kOT_integer);                                        \
    if (first) {                                                               \
      first = false;                                                           \
      result = value->integer;                                                 \
    } else {                                                                   \
      result = result _sign value->integer;                                    \
    }                                                                          \
    args = args->list.tail;                                                    \
  } while (object_list_is_empty(args) == false);                               \
  return make_integer(result)

static object_t primitive_add(object_t env, object_t args) {
  primitive_arithmetic(+);
}
static object_t primitive_sub(object_t env, object_t args) {
  primitive_arithmetic(-);
}
static object_t primitive_mul(object_t env, object_t args) {
  primitive_arithmetic(*);
}
static object_t primitive_div(object_t env, object_t args) {
  primitive_arithmetic(/);
}

object_t object_create_env(void) {
  object_t env = NULL;
  object_new(vars, object_list_create(), {
    object_new(parent, object_list_create(), {
      env = make_env(vars, parent);

      env_add_constant(env, "nil", kCT_nil);
      env_add_constant(env, "true", kCT_true);
      env_add_constant(env, "false", kCT_nil);

      env_add_primitive(env, "if", primitive_if);
      env_add_primitive(env, "do", primitive_do);
      env_add_primitive(env, "define", primitive_define);

      env_add_primitive(env, "+", primitive_add);
      env_add_primitive(env, "-", primitive_sub);
      env_add_primitive(env, "*", primitive_mul);
      env_add_primitive(env, "/", primitive_div);
    });
  });
  return env;
}

#include <stdio.h>

void object_print(object_t self) {
  assert(self != NULL);
  switch (self->type) {
  case kOT_string:
    printf("STRING[%s]", self->string);
    break;
  case kOT_symbol:
    printf("SYMBOL[%s]", self->symbol);
    break;
  case kOT_integer:
    printf("INTEGER[%d]", self->integer);
    break;
  case kOT_list:
    printf("LIST[");
    for (object_t p = self; !object_list_is_empty(p); p = p->list.tail) {
      printf("%s", p == self ? "" : " ");
      object_print(p->list.head);
    }
    printf("]");
    break;
  case kOT_constant:
    switch (self->constant) {
    case kCT_nil:
      printf("LIST[]");
      break;
    case kCT_true:
      printf("TRUE");
      break;
    default:
      assert(false);
      break;
    }
    break;
  default:
    assert(false);
    break;
  }
}

object_t object_apply(object_t env, object_t func, object_t args) {
  assert(env != NULL);
  assert(env->type == kOT_env);
  assert(func != NULL);
  assert(func->type == kOT_primitive || func->type == kOT_function);
  assert(args != NULL);

  switch (func->type) {
  case kOT_primitive:
    return func->primitive(env, args);
    break;
  default:
    assert(false);
    break;
  }

  return (NULL);
}

object_t object_eval(object_t env, object_t object) {
  assert(env != NULL);
  assert(object != NULL);
  switch (object->type) {
  case kOT_symbol:
    object = env_find(env, object);
    assert(object != NULL);
  case kOT_integer:
  case kOT_string:
  case kOT_constant:
    break;
  case kOT_list: {
    object_t func = object_eval(env, object->list.head);
    object_t args = object->list.tail;
    assert(func->type == kOT_primitive || func->type == kOT_function);
    object = object_apply(env, func, args);
    break;
  }
  default:
    assert(false);
    break;
  }
  return object;
}

void object_delete(void *ptr) {
  object_t *object_ptr = (object_t *)ptr;
  object_t object = *((object_t *)object_ptr);
  *object_ptr = memory_release(object);
}