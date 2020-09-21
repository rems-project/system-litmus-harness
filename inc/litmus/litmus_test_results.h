#ifndef LITMUS_TEST_RESULTS_H
#define LITMUS_TEST_RESULTS_H

#include "lib.h"

/* test data */
typedef struct {
    uint64_t is_relaxed;
    uint64_t counter;
    uint64_t values[];
} test_result_t;

typedef struct {
    uint64_t allocated;
    uint64_t limit;
    test_result_t** lut;
    test_result_t* results[];
} test_hist_t;


/* print the collected results out */
void print_results(test_hist_t* results, test_ctx_t* ctx);

/* save the result from a run */
void handle_new_result(test_ctx_t* ctx, run_idx_t idx, run_count_t run);

#endif /* LITMUS_TEST_RESULTS_H */