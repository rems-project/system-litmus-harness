#ifndef LITMUS_TEST_DEF_H
#define LITMUS_TEST_DEF_H

#include "litmus_prot.h"
#include "litmus_idxs.h"

/* litmus_macros to help the user define a litmus test */
#include "litmus_asm.h"
#include "litmus_macros.h"

/* test configuration */
typedef enum {
  TYPE_HEAP,
  TYPE_PTE,
  TYPE_FIX,
  TYPE_UNMAPPED,
  TYPE_IDENTITY_MAP,
  TYPE_ALIAS,
  TYPE_ATTRS,
  TYPE_REGION_OWN,
  TYPE_REGION_PIN,
  TYPE_REGION_OFFSET,
  TYPE_MAIR, /* can set the test MAIR value */
} init_type_t;

char* init_type_to_str(init_type_t type);

typedef enum {
  REGION_SAME_VAR_OFFSET,        /* unused, but for completeness of type */
  REGION_SAME_CACHE_LINE_OFFSET, /* both have same lower CACHE_LINE_SHIFT bits */
  REGION_SAME_PAGE_OFFSET,       /* both have same offset into the page, aka bits 12-0 */
  REGION_SAME_PMD_OFFSET,        /* both have same offset into the 2M pmd region aka bits 20-12 */
  REGION_SAME_PUD_OFFSET,        /* both have same offset into the 1G pud region aka bits 29-20 */
  REGION_SAME_PGD_OFFSET,        /* unused; too large */
} rel_offset_t;

typedef enum {
  REGION_SAME_VAR,        /* unused */
  REGION_SAME_CACHE_LINE, /* both in the same cache line */
  REGION_SAME_PAGE,       /* both in same 4k page */
  REGION_SAME_PMD,        /* both same 2M region */
  REGION_SAME_PUD,        /* both same 1G region */
  REGION_SAME_PGD,        /* unused; too large */
} pin_level_t;

char* pin_level_to_str(pin_level_t lvl);
char* rel_offset_to_str(rel_offset_t lvl);

#define NUM_PIN_LEVELS 6

typedef enum {
  REGION_OWN_VAR,        /* unused, but for completeness of type */
  REGION_OWN_CACHE_LINE, /* no other vars should be allocated in this cache line */
  REGION_OWN_PAGE,       /* no other vars should be allocated in this 4k region */
  REGION_OWN_PMD,        /* no other vars should be allocated in this 2M region */
  REGION_OWN_PUD,        /* no other vars should be allocated in this 1G region */
  REGION_OWN_PGD,        /* unused, too large */
} own_level_t;

char* own_level_to_str(own_level_t lvl);

typedef struct
{
  const char* varname;
  init_type_t type;
  union {
    u64 value;             /* TYPE_HEAP, TYPE_PTE, TYPE_MAIR, TYPE_FIX */
    const char* aliasname; /* TYPE_ALIAS */
    struct
    { /* TYPE_ATTRS */
      prot_type_t prot_type;
      u64 attr_value;
    };
    struct
    { /* TYPE_REGION_PIN */
      const char* pinned_var_name;
      pin_level_t pinned_level;
    };
    struct
    { /* TYPE_REGION_OFFSET */
      const char* offset_var_name;
      rel_offset_t offset_level;
    };
    own_level_t ownership_level; /* TYPE_REGION_OWN */

    /* TYPE_UNMAPPED does not contain data, so this field is unused */
  };
} init_varstate_t;

/* Each thread is a functon that takes pointers to a slice of heap variables and output registers */
typedef struct test_ctx test_ctx_t;

typedef struct
{
  test_ctx_t* ctx;
  u64 i;
  u64** va;
  u64** out_reg;
  u64* pa;

  u64*** tt_entries;
  u64** tt_descs;
} litmus_test_run;

typedef void th_f(litmus_test_run* data);

u64* var_va(litmus_test_run* data, const char* name);
u64 var_pa(litmus_test_run* data, const char* name);
u64* var_pte(litmus_test_run* data, const char* name);
u64* var_pte_level(litmus_test_run* data, const char* name, int level);
u64* var_pmd(litmus_test_run* data, const char* name);
u64* var_pud(litmus_test_run* data, const char* name);
u64* var_pgd(litmus_test_run* data, const char* name);

u64 var_desc(litmus_test_run* data, const char* name);
u64 var_desc_level(litmus_test_run* data, const char* name, int leve);
u64 var_ptedesc(litmus_test_run* data, const char* name);
u64 var_pmddesc(litmus_test_run* data, const char* name);
u64 var_puddesc(litmus_test_run* data, const char* name);
u64 var_pgddesc(litmus_test_run* data, const char* name);
u64 var_page(litmus_test_run* data, const char* name);
u64* out_reg(litmus_test_run* data, const char* name);

/* for annotating the outcome of a test
 * whether it's allowed under certain models or not
 */
typedef enum {
  OUTCOME_FORBIDDEN, /* this test outcome is forbidden by this model */
  OUTCOME_ALLOWED,   /* this test outcome is allowed by this model */
  OUTCOME_UNKNOWN,   /* it is not known whether this outcome is allowed in this model */
  OUTCOME_UNDEFINED, /* the model does not define an outcome for this test */
} arch_model_status_t;

typedef struct
{
  const char* model_name;
  arch_model_status_t allowed;
} arch_allow_st;

/**
 * definition of a litmus test
 *
 * aka the static configuration that defines a litmus test
 */
typedef struct
{
  const char* name;

  int no_threads;
  th_f** threads;
  var_idx_t no_heap_vars;
  const char** heap_var_names;
  reg_idx_t no_regs;
  const char** reg_names;

  u64* interesting_result; /* interesting (relaxed) result to highlight */
  u64 no_interesting_results;
  u64** interesting_results; /* same as above, but plural */

  u64 no_sc_results; /* a count of SC results, used in sanity-checking output */
  th_f** setup_fns;
  th_f** teardown_fns;

  int no_init_states;
  init_varstate_t** init_states; /* initial state array */

  int* start_els;
  u32*** thread_sync_handlers;

  /** whether the test requires any special options to be enabled */
  u8 requires_pgtable; /* requires --pgtable */
  u8 requires_perf;    /* requires --perf */
  u8 requires_debug;   /* requires -d */

  /* annotate the test with whether it's allowed for certain models or not
   * e.g.
   * {
   *    ...
   *    .expected_allowed = (arch_allow_st[]) {
   *      {"armv8", OUTCOME_FORBIDDEN},
   *      {"armv8-cseh", OUTCOME_ALLOWED},
   *    }
   * }
   */
  arch_allow_st* expected_allowed;

  /**
   * predefined hash (NULL if none)
   */
  const char* hash;
} litmus_test_t;


/**
 * litmus_test_hash() - Generates a hash from a litmus test.
 *
 * SEE: litmus_test_hash.c for details
 */
digest litmus_test_hash(const litmus_test_t *test);

#endif /* LITMUS_TEST_DEF_H */