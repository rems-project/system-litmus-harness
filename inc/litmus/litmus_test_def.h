#ifndef LITMUS_TEST_DEF_H
#define LITMUS_TEST_DEF_H

#include "lib.h"


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
  uint64_t** va;
  uint64_t** out_reg;
  uint64_t* pa;
  uint64_t** pte;
  uint64_t* desc;
} litmus_test_run;

typedef void th_f(litmus_test_run* data);

uint64_t* var_va(litmus_test_run* data, const char* name);
uint64_t var_pa(litmus_test_run* data, const char* name);
uint64_t* var_pte(litmus_test_run* data, const char* name);
uint64_t var_desc(litmus_test_run* data, const char* name);
uint64_t var_page(litmus_test_run* data, const char* name);
uint64_t* out_reg(litmus_test_run* data, const char* name);

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

  uint64_t* interesting_result;    /* interesting (relaxed) result to highlight */
  uint64_t no_interesting_results;
  uint64_t** interesting_results;  /* same as above, but plural */

  uint64_t no_sc_results;  /* a count of SC results, used in sanity-checking output */
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


#endif /* LITMUS_TEST_DEF_H */