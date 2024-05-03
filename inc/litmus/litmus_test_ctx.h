#ifndef LITMUS_CTX_H
#define LITMUS_CTX_H

#include "litmus_idxs.h"
#include "litmus_regions.h"
#include "litmus_concretization.h"
#include "litmus_var_info.h"
#include "litmus_ctx_system_state.h"

#include "config.h"

/**
 * the test_ctx_t type is the dynamic configuration generated at runtime
 * it holds live pointers to the actual blocks of memory for variables and registers and the
 * collected output histogram that the test is actually using
 * as well as a reference to the static test configuration (the litmus_test_t)
 */
struct test_ctx
{
  /** time at the start of the run. */
  u64 start_clock;

  /** time computed at the end,
   * not actually needed on the context object but here for completeness */
  u64 end_clock;

  /* _total_ number of test executions per run
   */
  run_idx_t no_runs;

  /* size of each batch
   * when not using ASIDs this must be 1
   */
  run_count_t batch_size;

  regions_t heap_memory; /* pointers to set of regions */
  var_info_t* heap_vars; /* set of heap variables: x, y, z etc */
  init_system_state_t* system_state;
  u64** out_regs;              /* set of output register values: P1:x1,  P2:x3 etc */
  bar_t* generic_cpu_barrier;  /* generic wait-for-all-cpus */
  bar_t* generic_vcpu_barrier; /* generic wait-for-all-vcpus */
  bar_t* start_barriers;       /* per-run barrier for start */
  run_idx_t* shuffled_ixs;
  run_count_t* shuffled_ixs_inverse; /* the inverse lookup of shuffled_ixs */
  volatile int* affinity;
  test_hist_t* hist;
  run_idx_t current_run;
  u64** ptables;
  u64 current_EL;
  u64 privileged_harness;  /* require harness to run at EL1 between runs ? */
  u64 last_tick;           /* clock ticks since last verbose print */
  void* concretization_st; /* current state of the concretizer */

  /** checkpoint to restore the ptable allocator back to at the end
   */
  valloc_ptable_mem valloc_ptable_chkpnt;

  /* the static configuration of the current test */
  const litmus_test_t* cfg;
};

void init_test_ctx(test_ctx_t* ctx, const litmus_test_t* cfg, int no_runs, int runs_in_batch);
void free_test_ctx(test_ctx_t* ctx);

/* helper functions */
u64 ctx_pa(test_ctx_t* ctx, run_idx_t run, u64 va);
u64* ctx_pte(test_ctx_t* ctx, run_idx_t run, u64 va);

var_idx_t idx_from_varname_infos(const litmus_test_t* cfg, var_info_t* infos, const char* varname);
var_idx_t idx_from_varname(test_ctx_t* ctx, const char* varname);
reg_idx_t idx_from_regname(test_ctx_t* ctx, const char* regname);
const char* varname_from_idx(test_ctx_t* ctx, var_idx_t idx);
const char* regname_from_idx(test_ctx_t* ctx, var_idx_t idx);

run_count_t run_count_from_idx(test_ctx_t* ctx, run_idx_t idx);
u64* ptable_from_run(test_ctx_t* ctx, run_idx_t i);
u64 asid_from_run_count(test_ctx_t* ctx, run_count_t r);

/* for loading var_info_t */
void read_var_infos(const litmus_test_t* cfg, init_system_state_t* sys_st, var_info_t* infos, int no_runs);

/** convert the loop count into an randomized index
 */
run_idx_t count_to_run_index(test_ctx_t* ctx, run_count_t i);

/** given a variable, return the VA for a given run index
 */
u64* ctx_heap_var_va(test_ctx_t* ctx, u64 varidx, run_idx_t i);

/** given a variagble return the initial heap value for a given run index
 */
u64 ctx_initial_heap_value(test_ctx_t* ctx, run_idx_t idx);

#endif /* LITMUS_CTX_H */