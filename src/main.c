#include "object_parse.h"

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
      bool running = true;
      while (running) {
        running = false;
        printf("%s", prompt ? prompt : "");
        object_new(object, object_parse(s), {
          running = true;
          object_new(result, object_eval(env, object), {
#ifdef NDEBUG
            if (prompt != NULL)
#endif
            {
              object_print(result);
              printf("\n");
            }
          });
        });
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