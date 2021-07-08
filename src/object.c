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
  case kOT_constant:
  case kOT_integer:
  case kOT_symbol:
  case kOT_string:
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

    object_t last = object_list_create();
    object_t node = make_list(object, last);
    memory_release(last);

    list->list.last = list->list.last->list.tail = node;
  }

  assert(list->type == kOT_list);
  *list_ptr = list;
}

bool object_list_is_empty(object_t object) {
  assert(object != NULL);
  return ((object->type == kOT_constant) && (object->constant == kCT_nil));
}

object_t object_create_env(void) { //
  return make_env(object_list_create(), object_list_create());
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