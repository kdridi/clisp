#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"

#define next_char(s, skip)                                                     \
  (((skip) ? (stream_skip((s), " \n\r\t")) : (s))->next(s))
#define peek_char(s, skip)                                                     \
  (((skip) ? (stream_skip((s), " \n\r\t")) : (s))->peek(s))

static object_t parse_symbol(stream_t s, char c) {
  size_t capacity = 1;
  size_t length = 0;
  char *name = malloc(capacity + 1);

  name[length++] = c;
  name[length] = 0;
  while (isalnum(peek_char(s, false)) || peek_char(s, false) == '-') {
    if (length == capacity) {
      capacity += capacity;
      name = realloc(name, capacity + 1);
    }
    name[length++] = next_char(s, false);
    name[length] = 0;
  }

  object_t result = object_create_symbol(name);
  free(name);
  return result;
}

static object_t parse_string(stream_t s) {
  size_t capacity = 1;
  size_t length = 0;
  char *name = malloc(capacity + 1);

  int ch = next_char(s, false);
  while (ch != '"') {
    if (ch == '\\')
      ch = next_char(s, false);
    assert(ch != EOF);
    if (length == capacity) {
      capacity += capacity;
      name = realloc(name, capacity + 1);
    }
    name[length++] = ch;
    name[length] = 0;
    ch = next_char(s, false);
  }

  object_t result = object_create_string(name);
  free(name);
  return result;
}

object_t parse_number(stream_t s, int sign, int value) {
  while (isdigit(peek_char(s, false)))
    value = value * 10 + (next_char(s, false) - '0');
  return object_create_integer(sign * value);
}

object_t parse_list(stream_t s) {
  object_t head = object_list_create();
  while (peek_char(s, true) != ')')
    object_new(object, parse(s), { //
      object_list_push(&head, object);
    });
  next_char(s, false);
  return (head);
}

object_t parse_quote(stream_t s) {
  object_t list = object_list_create();

  object_new(object,                        //
             object_create_symbol("quote"), //
             object_list_push(&list, object));

  object_new(object,   //
             parse(s), //
             object_list_push(&list, object));

  return (list);
}

object_t parse(stream_t s) {
  int c = next_char(s, true);

  if (c == EOF)
    return NULL;

  if (c == '(')
    return parse_list(s);

  if (c == '\'')
    return parse_quote(s);

  if (c == '"')
    return parse_string(s);

  if (isdigit(c))
    return parse_number(s, 0 + 1, c - '0');

  if (c == '-' && isdigit(peek_char(s, false)))
    return parse_number(s, 0 - 1, next_char(s, false) - '0');

  if (isalpha(c) || strchr("-+=!@#$%^&*", c))
    return parse_symbol(s, c);

  printf("ERROR: Don't know how to handle '%c'\n", c);
  assert(false);
  return (NULL);
}
