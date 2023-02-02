#ifndef UNITTEST_H
#define UNITTEST_H

/* tag that the runner script checks for
 * to collect tests */
#define UNIT_TEST(foo)

/* a test function is void -> void */
typedef void unit_test_fn(void);

typedef struct {
    const char* name;
    unit_test_fn* fn;
    int result;

    /* we fix the message to 1k here
     * so we do not need dynamic allocations for formatted messages */
    char msg[1024];
} unit_test_t;

typedef struct {
    const char* name;
    int no_tests;
    unit_test_t** fns;
} test_file_t;

extern unit_test_t* current_test;

#endif /* UNITTEST_H */