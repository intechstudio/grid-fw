#include <string.h>
#include "mjs.h"

void foo(int x) {
  printf("Hello %d!\n", x);
}

void *my_dlsym(void *handle, const char *name) {
  if (strcmp(name, "foo") == 0) return foo;
  return NULL;
}

int main(void) {
  struct mjs *mjs = mjs_create();
  mjs_set_ffi_resolver(mjs, my_dlsym);
  mjs_exec(mjs, "let f = ffi('void foo(int)'); f(1234+63)", NULL);
  return 0;
}
