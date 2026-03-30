#include <assert.h>
#include <stdio.h>

#include "test.h"

#include "common/test/tests.h"

struct test_entry_t tests[] = {
    TEST_ENTRY(grid_ui_encoder_relative),
};

int main() {

  int total = sizeof(tests) / sizeof(tests[0]);
  int passed = 0;

  for (int i = 0; i < total; ++i) {

    struct test_entry_t* entry = &tests[i];
    struct test_return_t ret = entry->fun();
    passed += ret.status == TEST_PASS;

    switch (ret.status) {
    case TEST_PASS: {
      fprintf(stderr, "pass \"%s\"\n", entry->name);
    } break;
    case TEST_FAIL: {
      fprintf(stderr, "FAIL \"%s\" at %s:%d\n", entry->name, ret.file, ret.line);
    } break;
    default: {
      assert(0);
    } break;
    }
  }

  fprintf(stderr, "passed/total: %d/%d\n", passed, total);

  return passed == total ? 0 : 1;
}
