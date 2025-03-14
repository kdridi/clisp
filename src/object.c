#include "object.h"
#include "memory.h"
#include "object_parse.h"

#include <assert.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static void unmake(void *ptr) {
  object_t object = ptr;
  switch (object->type) {
  case kOT_list:
    memory_release(object->list.tail);
    memory_release(object->list.head);
    break;
  case kOT_env:
    memory_release(object->env.vars);
    break;
  case kOT_symbol:
  case kOT_constant:
  case kOT_integer:
  case kOT_string:
  case kOT_primitive:
    break;
  case kOT_function:
    // memory_release(object->function.env);
    memory_release(object->function.body);
    memory_release(object->function.params);
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

static object_t make_list(object_t head, object_t tail) {
  assert(head != NULL);
  assert(tail != NULL);
  object_t self = make(kOT_list, sizeof(self->list));
  self->list.head = memory_retain(head);
  self->list.tail = memory_retain(tail);
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
  object_t self = make(kOT_env, sizeof(self->env));
  self->env.vars = memory_retain(vars);
  self->env.parent = parent;
  return (self);
}

static object_t make_primitive(primitive_t primitive) {
  object_t self = make(kOT_primitive, sizeof(self->primitive));
  self->primitive = primitive;
  return (self);
}

static object_t make_function(object_type_t type, object_t params,
                              object_t body, object_t env) {
  assert(type == kOT_function);
  object_t self = make(type, sizeof(self->function));
  self->function.params = memory_retain(params);
  self->function.body = memory_retain(body);
  self->function.env = env;
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

static object_t object_eval_list(object_t env, object_t object) {
  if (object_list_is_empty(object) == true)
    return object_list_create();
  assert(object->type == kOT_list);

  object_t head = object_eval(env, object->list.head);
  object_t tail = object_eval_list(env, object->list.tail);
  object_t result = make_list(head, tail);
  memory_release(head);
  memory_release(tail);

  assert(result->type == kOT_list);
  return result;
}

object_t env_find(object_t env, object_t object) {
  assert(object != NULL);
  assert(object->type == kOT_symbol);

  while (object_list_is_empty(env) == false) {
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
      if (strcmp(pair->list.head->symbol, object->symbol) == 0) {
        return pair->list.tail;
      }
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

static void env_add_integer(object_t env, const char *name, int integer) {
  object_new(key, object_create_symbol(name), {
    object_new(value, make_integer(integer), { //
      env_add(env, key, value);
    });
  });
}

static object_t for_each(object_t env, object_t args, bool print) {
  object_t result = NULL;
  while (object_list_is_empty(args) == false) {
    if (result != NULL)
      memory_release(result);
    result = object_eval(env, args->list.head);
    if (print == true)
      object_print(result);
    args = args->list.tail;
  }
  return result;
}

static object_t primitive_do(object_t env, object_t args) { //
  return for_each(env, args, false);
}

static object_t primitive_print(object_t env, object_t args) { //
  return for_each(env, args, true);
}

static object_t primitive_if(object_t env, object_t args) {
  assert(object_list_length(args) >= 2);
  object_t condition = object_eval(env, args->list.head);
  bool is_true = object_list_is_empty(condition) == false;
  memory_release(condition);

  if (is_true) {
    return object_eval(env, args->list.tail->list.head);
  }

  if (object_list_is_empty(args->list.tail->list.tail))
    return object_eval(env, args->list.tail->list.tail);

  return primitive_do(env, args->list.tail->list.tail);
}

static object_t primitive_let(object_t env, object_t args) {
  assert(object_list_length(args) == 2);

  object_t result = NULL;
  object_new(vars, object_list_create(), {
    object_new(new_env, make_env(vars, env), { //
      object_t params = args->list.head;
      args = args->list.tail;

      while (object_list_is_empty(params) == false) {
        object_t key = params->list.head;
        assert(key->type == kOT_symbol);
        params = params->list.tail;

        assert(object_list_is_empty(params) == false);
        object_new(value, object_eval(new_env, params->list.head), {
          env_add(new_env, key, value);
          params = params->list.tail;
        });
      }

      object_t body = args->list.head;
      args = args->list.tail;
      assert(object_list_is_empty(args) == true);

      result = object_eval(new_env, body);
    });
  });
  return (result);
}

static object_t primitive_define(object_t env, object_t args) {
  assert(object_list_length(args) == 2);

  object_t key = args->list.head;
  assert(key->type == kOT_symbol);
  object_t value = object_eval(env, args->list.tail->list.head);
  env_add(env, key, value);
  return value;
}

static object_t primitive_lambda(object_t env, object_t args) { //
  assert(object_list_length(args) == 2);
  return make_function(kOT_function, args->list.head, args->list.tail, env);
}

static object_t primitive_quote(object_t env, object_t args) { //
  assert(object_list_length(args) == 1);
  assert(env != NULL);
  ((void)env);
  return memory_retain(args->list.head);
}

static object_t primitive_eval(object_t env, object_t args) { //
  assert(object_list_length(args) == 1);
  assert(env != NULL);
  ((void)env);

  object_t result = NULL;
  object_new(value, object_eval(env, args->list.head), { //
    result = object_eval(env, value);
  });
  return (result);
}

static object_t primitive_read(object_t env, object_t args) { //
  assert(object_list_length(args) == 1);
  assert(env != NULL);
  ((void)env);

  object_t result = NULL;
  object_new(value, object_eval(env, args->list.head), {      //
    stream_new(s, stream_create_from_string(value->string), { //
      result = object_parse(s);
    });
  });
  return (result);
}

static object_t primitive_defun(object_t env, object_t args) {
  assert(object_list_length(args) == 3);
  object_t name = args->list.head;
  object_t func = primitive_lambda(env, args->list.tail);
  env_add(env, name, func);
  return func;
}

static object_t primitive_eq(object_t env, object_t args) {
  assert(object_list_length(args) == 2);

  object_t result = object_list_create();
  object_new(argl, object_eval(env, args->list.head), {
    object_new(argr, object_eval(env, args->list.tail->list.head), {
      if (argl->type == argr->type) {
        switch (argl->type) {
        case kOT_integer: {
          if (argl->integer == argr->integer) {
            memory_release(result);
            result = make_constant(kCT_true);
          }
          break;
        }
        default:
          assert(false);
          break;
        }
      }
    });
  });
  return result;
}

static object_t primitive_lt(object_t env, object_t args) {
  assert(object_list_length(args) == 2);

  object_t result = object_list_create();
  object_new(argl, object_eval(env, args->list.head), {
    object_new(argr, object_eval(env, args->list.tail->list.head), {
      if (argl->type == argr->type) {
        switch (argl->type) {
        case kOT_integer: {
          if (argl->integer < argr->integer) {
            memory_release(result);
            result = make_constant(kCT_true);
          }
          break;
        }
        default:
          assert(false);
          break;
        }
      }
    });
  });
  return result;
}

static object_t primitive_gt(object_t env, object_t args) {
  assert(object_list_length(args) == 2);

  object_t result = object_list_create();
  object_new(argl, object_eval(env, args->list.head), {
    object_new(argr, object_eval(env, args->list.tail->list.head), {
      if (argl->type == argr->type) {
        switch (argl->type) {
        case kOT_integer: {
          if (argl->integer > argr->integer) {
            memory_release(result);
            result = make_constant(kCT_true);
          }
          break;
        }
        default:
          assert(false);
          break;
        }
      }
    });
  });
  return result;
}

static object_t primitive_le(object_t env, object_t args) {
  assert(object_list_length(args) == 2);

  object_t result = object_list_create();
  object_new(argl, object_eval(env, args->list.head), {
    object_new(argr, object_eval(env, args->list.tail->list.head), {
      if (argl->type == argr->type) {
        switch (argl->type) {
        case kOT_integer: {
          if (argl->integer <= argr->integer) {
            memory_release(result);
            result = make_constant(kCT_true);
          }
          break;
        }
        default:
          assert(false);
          break;
        }
      }
    });
  });
  return result;
}

static object_t primitive_ge(object_t env, object_t args) {
  assert(object_list_length(args) == 2);

  object_t result = object_list_create();
  object_new(argl, object_eval(env, args->list.head), {
    object_new(argr, object_eval(env, args->list.tail->list.head), {
      if (argl->type == argr->type) {
        switch (argl->type) {
        case kOT_integer: {
          if (argl->integer >= argr->integer) {
            memory_release(result);
            result = make_constant(kCT_true);
          }
          break;
        }
        default:
          assert(false);
          break;
        }
      }
    });
  });
  return result;
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
    memory_release(value);                                                     \
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

static object_t c_open(object_t env, object_t args) {
  assert(object_list_length(args) == 2);
  object_t result = NULL;
  object_new(pathname, object_eval(env, args->list.head), {
    assert(pathname->type == kOT_string);
    object_new(flags, object_eval(env, args->list.tail->list.head), {
      assert(flags->type == kOT_integer);
      result = object_create_integer(open(pathname->string, flags->integer));
    });
  });
  return result;
}

static object_t c_close(object_t env, object_t args) {
  assert(object_list_length(args) == 1);
  object_t result = NULL;
  object_new(fp, object_eval(env, args->list.head), {
    assert(fp->type == kOT_integer);
    result = object_create_integer(close(fp->integer));
  });
  return result;
}

// static object_t c_read(object_t env, object_t args) {
//   assert(object_list_length(args) == 2);

//   object_new(fp, object_eval(env, args->list.head), {
//     assert(fp->type == kOT_integer);
//     object_new(size, object_eval(env, args->list.tail->list.head), {
//       assert(size->type == kOT_integer);

//       char *buffer = memory_create(size->integer);
//       ssize_t result = read(fp->integer, buffer, size->integer);
//       memory_release(buffer);

//       return object_create_integer(result);
//     });
//   });
// }

object_t object_create_env(int argc, const char **argv) {
  object_t env = NULL;
  object_new(vars, object_list_create(), {
    object_new(parent, object_list_create(), {
      env = make_env(vars, parent);

      env_add_constant(env, "nil", kCT_nil);
      env_add_constant(env, "true", kCT_true);
      env_add_constant(env, "false", kCT_nil);

      env_add_primitive(env, "if", primitive_if);
      env_add_primitive(env, "do", primitive_do);
      env_add_primitive(env, "let", primitive_let);
      env_add_primitive(env, "define", primitive_define);
      env_add_primitive(env, "defun", primitive_defun);
      env_add_primitive(env, "lambda", primitive_lambda);
      env_add_primitive(env, "quote", primitive_quote);
      env_add_primitive(env, "print", primitive_print);
      env_add_primitive(env, "eval", primitive_eval);
      env_add_primitive(env, "read", primitive_read);

      env_add_integer(env, "O_RDONLY", O_RDONLY);
      env_add_primitive(env, "c_open", c_open);
      env_add_primitive(env, "c_close", c_close);
      // env_add_primitive(env, "c_read", c_read);

      env_add_primitive(env, "+", primitive_add);
      env_add_primitive(env, "-", primitive_sub);
      env_add_primitive(env, "*", primitive_mul);
      env_add_primitive(env, "/", primitive_div);

      env_add_primitive(env, "=", primitive_eq);
      env_add_primitive(env, "<", primitive_lt);
      env_add_primitive(env, ">", primitive_gt);
      env_add_primitive(env, "<=", primitive_le);
      env_add_primitive(env, ">=", primitive_ge);

      object_new(ARGS, object_list_create(), {
        for (int i = 0; i < argc; i++) {
          object_new(arg, object_create_string(argv[i]), { //
            object_list_push(&ARGS, arg);
          });
        }
        object_new(key, object_create_symbol("ARGS"), { //
          env_add(env, key, ARGS);
        });
      });
    });
  });
  return env;
}

void object_print(object_t self) {
  assert(self != NULL);
  switch (self->type) {
  case kOT_function:
    printf("<function>");
    break;
  case kOT_primitive:
    printf("<primitive>");
    break;
  case kOT_symbol:
    printf("%s", self->symbol);
    break;
  case kOT_string:
    printf("%s", self->string);
    break;
  case kOT_integer:
    printf("%d", self->integer);
    break;
  case kOT_list:
    printf("(");
    for (object_t p = self; !object_list_is_empty(p); p = p->list.tail) {
      printf("%s", p == self ? "" : " ");
      object_print(p->list.head);
    }
    printf(")");
    break;
  case kOT_constant:
    switch (self->constant) {
    case kCT_nil:
      printf("()");
      break;
    case kCT_true:
      printf("true");
      break;
    default:
      assert(false);
      break;
    }
    break;
  default:
    printf("ERROR: type = %d\n", self->type);
    assert(false);
    break;
  }
}

void object_dump(object_t self) {
  assert(self != NULL);
  switch (self->type) {
  case kOT_env:
    printf("ENV[VARS[");
    for (object_t p = self->env.vars; !object_list_is_empty(p);
         p = p->list.tail) {
      printf("(");
      object_dump(p->list.head->list.head);
      printf(",");
      object_dump(p->list.head->list.tail);
      printf(")");
    }
    printf("]PARENT[");
    object_dump(self->env.parent);
    printf("]]");
    break;
  case kOT_function:
    printf("FUNCTION");
    break;
  case kOT_primitive:
    printf("PRIMITIVE");
    break;
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
      object_dump(p->list.head);
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
  if (func->type != kOT_primitive && func->type != kOT_function) {
    object_dump(func);
    printf("\n");
    // object_dump(args);
    // printf("\n");
    assert(false);
  }

  assert(args != NULL);

  switch (func->type) {
  case kOT_primitive: {
    return func->primitive(env, args);
  }
  case kOT_function: {
    object_t result = NULL, params = func->function.params;
    assert(object_list_length(params) == object_list_length(args));
    object_new(empty, object_list_create(), {
      object_new(new_env, make_env(empty, func->function.env), {
        object_new(head, object_eval_list(env, args), {
          object_t values = head;
          while (object_list_is_empty(params) == false) {
            env_add(new_env, params->list.head, values->list.head);
            params = params->list.tail;
            values = values->list.tail;
          }
        });
        result = primitive_do(new_env, func->function.body);
      });
    });
    return result;
  }
  default:
    printf("ERROR: ");
    object_dump(func);
    printf("\n");
    assert(false);
    break;
  }

  return (NULL);
}

object_t object_eval(object_t env, object_t object) {
  assert(env != NULL);
  assert(object != NULL);

  switch (object->type) {
  case kOT_symbol: {
    const char *name = object->symbol;
    object = env_find(env, object);
    if (object == NULL) {
      printf("name: %s\n", name);
      assert(false);
    }
    return memory_retain(object);
  }
  case kOT_integer:
  case kOT_string:
  case kOT_constant:
    return memory_retain(object);
  case kOT_list: {
    object_t result = NULL;
    object_new(func, object_eval(env, object->list.head),
               { result = object_apply(env, func, object->list.tail); });
    return result;
  }
  default:
    assert(false);
    break;
  }

  return (NULL);
}

void object_delete(void *ptr) {
  object_t *object_ptr = (object_t *)ptr;
  object_t object = *((object_t *)object_ptr);
  *object_ptr = memory_release(object);
}