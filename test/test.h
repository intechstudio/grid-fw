#ifndef TEST_H
#define TEST_H

enum test_status_t {
  TEST_PASS = 0,
  TEST_FAIL,
};

struct test_return_t {
  enum test_status_t status;
  const char* file;
  int line;
};

typedef struct test_return_t (*fn_test_t)(void);

struct test_entry_t {
  fn_test_t fun;
  const char* name;
};

// clang-format off

#define TEST_ENTRY(name) { test_ ## name, #name }

#define TEST_DECL(name) struct test_return_t test_ ## name(void)

#define TEST_SUCCESS (struct test_return_t){ TEST_PASS }
#define TEST_FAILURE (struct test_return_t){ TEST_FAIL, __FILE__, __LINE__ }

#define TEST_ASSERT_MSG(expr, ...) \
do { \
	if (!(expr)) { \
		fprintf(stderr, __VA_ARGS__); \
		return TEST_FAILURE; \
	} \
} while (0)

#define TEST_ASSERT(expr) TEST_ASSERT_MSG(expr, "");

// clang-format on

#endif /* TEST_H */
