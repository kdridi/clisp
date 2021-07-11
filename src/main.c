#include "parser.h"

#include <assert.h>
#include <string.h>

int main(int argc, const char *argv[]) {
  const char *prompt = NULL;
  FILE *fp = NULL;
  int offset = 1;

  if (argc > 1) {
    if (strcmp(argv[1], "--") != 0) {
      fp = fopen(argv[1], "r");
      assert(fp != NULL);
    } else {
      fp = stdin;
      prompt = "minilisp> ";
    }
    offset += 1;
  } else {
    fp = stdin;
    prompt = "minilisp> ";
  }

  stream_new(s, stream_create_from_file(fp), {
    object_new(env, object_create_env(argc - offset, argv + offset), {
      while (true) {
        printf("%s", prompt ? prompt : "");
        object_t object = parse(s);
        if (object == NULL)
          break;
        object_t result = object_eval(env, object);
        object_print(result);
        memory_release(result);
        printf("\n");
        memory_release(object);
      }
    });
  });

  if (prompt == NULL) {
    assert(fp != NULL);
    fclose(fp);
    fp = NULL;
  }

  return (0);
}