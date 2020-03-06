#ifndef TESTLIB_H
#define TESTLIB_H

#include "lib.h"

#ifndef NO_TEST_FILES
#define NO_TEST_FILES 0
#endif

#define UNIT_TEST(foo)

typedef void unit_test_fn(void);

typedef struct {
    const char* name;
    unit_test_fn* fn;
    int result;
    char* msg;
} unit_test_t;

typedef struct {
    const char* name;
    int no_tests;
    unit_test_t* fns;
} test_file_t;

unit_test_t* current_test;

/** if check is false, then raise failure as msg
 */
#define ASSERT2(check, m) do { \
    if (!(check)) { \
        current_test->msg = (m); \
        current_test->result = 0; \
        return; \
    } else { current_test->result = 1; }\
} while (0);

#define ASSERT1(check) ASSERT2(check, NULL)

#define ASSERT_CHOOSE(a, b, c, ...) c
#define ASSERT(...) ASSERT_CHOOSE(__VA_ARGS__, ASSERT2, ASSERT1)(__VA_ARGS__)

#endif /* TESTLIB_H */