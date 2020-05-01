#ifndef LITMUS_TEST_H
#define LITMUS_TEST_H
#include <stdint.h>

/* litmus_macros to help the user define a litmus test */
#include "litmus_macros.h"

/* test configuration */
typedef enum {
  TYPE_HEAP,
  TYPE_PTE,
} init_type_t;

typedef struct {
  const char* varname;
  init_type_t type;
  uint64_t value;
} init_varstate_t;

/* Each thread is a functon that takes pointers to a slice of heap variables and output registers */
typedef struct test_ctx test_ctx_t;

typedef struct {
  test_ctx_t* ctx;
  uint64_t i;
  uint64_t** var;
  uint64_t** out_reg;
  uint64_t* pa;
  uint64_t** pte;
  uint64_t* desc;
} litmus_test_run;

typedef void th_f(litmus_test_run* data);

/**
 * definition of a litmus test
 *
 * aka the static configuration that defines a litmus test
 */
typedef struct {
  const char* name;
  int no_threads;
  th_f** threads;
  int no_heap_vars;
  const char** heap_var_names;
  int no_regs;
  const char** reg_names;

  uint64_t* interesting_result;   /* interesting (relaxed) result to highlight */
  th_f** setup_fns;
  th_f** teardown_fns;

  int no_init_states;
  init_varstate_t** init_states;   /* initial state array */

  int* start_els;
  uint32_t*** thread_sync_handlers;

  /** whether the test requires any special options to be enabled */
  uint8_t requires_pgtable;  /* requires --pgtable */
  uint8_t requires_perf;     /* requires --perf */
  uint8_t requires_debug;    /* requires -d */
} litmus_test_t;

/* test data */
typedef struct {
    uint64_t counter;
    uint64_t values[];
} test_result_t;

typedef struct {
    int allocated;
    int limit;
    test_result_t** lut;
    test_result_t* results[];
} test_hist_t;

/**
 * the test_ctx_t type is the dynamic configuration generated at runtime
 * it holds live pointers to the actual blocks of memory for variables and registers and the
 * collected output histogram that the test is actually using
 * as well as a reference to the static test configuration (the litmus_test_t)
 */
struct test_ctx {
  uint64_t no_runs;
  uint64_t** heap_vars;         /* set of heap variables: x, y, z etc */
  uint64_t** out_regs;          /* set of output register values: P1:x1,  P2:x3 etc */
  bar_t* start_barriers;
  bar_t* end_barriers;
  bar_t* cleanup_barriers;
  bar_t* final_barrier;
  uint64_t* shuffled_ixs;
  test_hist_t* hist;
  uint64_t* ptable;
  uint64_t current_run;
  uint64_t current_EL;
  uint64_t privileged_harness;  /* require harness to run at EL1 between runs ? */
  const litmus_test_t* cfg;
};

/* entry point for tests */
void run_test(const litmus_test_t* cfg);

void init_test_ctx(test_ctx_t* ctx, const litmus_test_t* cfg, int no_runs);
void free_test_ctx(test_ctx_t* ctx);

/* helper functions */
uint64_t idx_from_varname(test_ctx_t* ctx, const char* varname);
void set_init_pte(test_ctx_t* ctx, const char* varname, uint64_t pte);
void set_init_heap(test_ctx_t* ctx, const char* varname, uint64_t value);


/* print the collected results out */
void print_results(test_hist_t* results, test_ctx_t* ctx, const char** out_reg_names, uint64_t* interesting_results);

/* call at the start and end of each run  */
void start_of_run(test_ctx_t* ctx, int thread, int i);
void end_of_run(test_ctx_t* ctx, int thread, int i);

/* call at the start and end of each thread */
void start_of_thread(test_ctx_t* ctx, int cpu);
void end_of_thread(test_ctx_t* ctx, int cpu);

/* call at the beginning and end of each test */
void start_of_test(test_ctx_t* ctx, const litmus_test_t* cfg, int no_runs);
void end_of_test(test_ctx_t* ctx, const char** out_reg_names, uint64_t* interesting_result);

#endif /* LITMUS_TEST_H */
