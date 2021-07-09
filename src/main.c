#include "parser.h"

#include <assert.h>

int main(int argc, const char *argv[]) {
  FILE *fp = NULL;
  stream_t s = NULL;
  if (argc == 2) {
    fp = fopen(argv[1], "r");
    assert(fp != NULL);
    s = stream_create_from_file(fp);
  } else {
    const char *str[] = {
        "(do 1 2 3) true nil false \"false\" 12 (if false \"true\" 1 2)",
        "(abc '(def ghi))",
        "'(+ 12 34 56)\n(\"a\\\"b\\\"c\")",
        "(1 2) (1) ()",
        "(1223 () ((())) (add 123 -23 idf () - (+ 12 23)))",
    };
    s = stream_create_from_string(str[0]);
  }

  object_new(env, object_create_env(), {
    while (true) {
      object_t object = parse(s);
      if (object == NULL)
        break;
      object_print(object_eval(env, object));
      printf("\n");
      memory_release(object);
    }
  });
  memory_release(s);

  if (fp != NULL) {
    fclose(fp);
    fp = NULL;
  }

  return (0);
}