#ifndef LITMUS_TEST_H
#define LITMUS_TEST_H

#include "litmus_idxs.h"
#include "litmus_test_def.h"
#include "litmus_test_results.h"
#include "litmus_test_ctx.h"


/* entry point for tests */
void run_test(const litmus_test_t* cfg);

#endif /* LITMUS_TEST_H */
