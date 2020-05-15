#ifndef LITMUS_CTX_H
#define LITMUS_CTX_H

#include "lib.h"

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

void init_test_ctx(test_ctx_t* ctx, const litmus_test_t* cfg, int no_runs);
void free_test_ctx(test_ctx_t* ctx);


/* helper functions */
uint64_t ctx_pa(test_ctx_t* ctx, uint64_t va);
uint64_t* ctx_pte(test_ctx_t* ctx, uint64_t va);

uint64_t idx_from_varname(test_ctx_t* ctx, const char* varname);
uint64_t idx_from_regname(test_ctx_t* ctx, const char* regname);
const char* varname_from_idx(test_ctx_t* ctx, uint64_t idx);
const char* regname_from_idx(test_ctx_t* ctx, uint64_t idx);

void set_init_pte(test_ctx_t* ctx, const char* varname, uint64_t pte);
void set_init_heap(test_ctx_t* ctx, const char* varname, uint64_t value);

#endif /* LITMUS_CTX_H */