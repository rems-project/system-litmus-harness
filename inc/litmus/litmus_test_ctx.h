#ifndef LITMUS_CTX_H
#define LITMUS_CTX_H

#include "config.h"

#define NR_ENTRIES_PER_PAGE 512
typedef struct {
  uint64_t values[NR_ENTRIES_PER_PAGE];
} page_t;

#define NR_PAGES_PER_DIR 512
typedef struct {
  page_t pages[NR_PAGES_PER_DIR];
} dir_t;

/** a 8M region
 * is split into 4 middle-level dirs
 * each of which contains 512 pages
 * which each contain 4k bytes of data.
 */
#define NR_DIRS_PER_REGION 4
typedef struct {
  dir_t dirs[NR_DIRS_PER_REGION];
} region_t;

/** heap variable data is split over regions
 *
 * each region covers 1 8M section of memory
 */
#define NR_REGIONS 8
typedef struct {
  region_t regions[NR_REGIONS];
} regions_t;

/** this region ticker keeps track
 * of the current directory and page in that directory
 * for each region
 *
 * this is useful during initial allocation of the test data
 * to regions and pages
 *
 * we assume each cache line = 16 words aka 64 cache lines per page
 */
#define NR_u64_PER_CACHE_LINE 8
#define NR_CACHE_LINES_PER_PAGE 64

typedef struct {
  const char* exclusive_owner;  /* if some var owns this cache line, its name, otherwise NULL */
  uint64_t curr_val_ix;
} cache_line_tracker_t;

typedef struct {
  const char* exclusive_owner;  /* if some var owns this page, its name, otherwise NULL */
  uint64_t curr_scl_ix;
  cache_line_tracker_t scl_ix [NR_CACHE_LINES_PER_PAGE];
} page_tracker_t;

typedef struct {
  const char* exclusive_owner;  /* if some var owns this dir, its name, otherwise NULL */
  uint64_t curr_page_ix;
  page_tracker_t page_ixs [NR_PAGES_PER_DIR];
} dir_tracker_t;

typedef struct {
  uint64_t curr_dir_ix;
  dir_tracker_t dir_ixs [NR_DIRS_PER_REGION];
} region_tracker_t;

typedef struct {
  region_tracker_t regions [NR_REGIONS];
} region_trackers_t;

typedef struct {
  uint64_t reg_ix;
  uint64_t dir_ix;
  uint64_t page_ix;
  uint64_t scl_ix;
  uint64_t val_ix;
} tracker_loc_t;

/** heap var info
 *
 */
typedef struct {
  int varidx;
  const char* name;
  uint64_t init_value;
  uint64_t init_ap;
  uint64_t init_unmapped;


  uint8_t init_region_pinned;
  union {
    /** if the region is pinned then this var is pinned to another var with some region offset */
    struct {
      const char* pin_region_var;
      pin_level_t pin_region_level;
    };

    /** if not pinned, then this var can move about freely */
    struct {
      uint64_t curr_region;
    };
  };

  uint8_t init_owns_region;
  own_level_t init_owned_region_size;

  /** if aliased, the name of the variable this one aliases
   */
  const char* alias;

  /** array of pointers into memory region for each run
   *
   * this is what actually defines the concrete tests
   */
  uint64_t** values;
} var_info_t;

/**
 * the test_ctx_t type is the dynamic configuration generated at runtime
 * it holds live pointers to the actual blocks of memory for variables and registers and the
 * collected output histogram that the test is actually using
 * as well as a reference to the static test configuration (the litmus_test_t)
 */
struct test_ctx {
  uint64_t no_runs;
  regions_t* heap_memory;       /* pointer to set of regions */
  var_info_t* heap_vars;        /* set of heap variables: x, y, z etc */
  uint64_t** out_regs;          /* set of output register values: P1:x1,  P2:x3 etc */
  bar_t* initial_sync_barrier;
  bar_t* start_of_run_barriers;
  bar_t* concretize_barriers;
  bar_t* start_barriers;
  bar_t* end_barriers;
  bar_t* cleanup_barriers;
  bar_t* final_barrier;
  int* shuffled_ixs;
  volatile int* affinity;
  test_hist_t* hist;
  uint64_t* ptable;
  uint64_t current_run;
  uint64_t current_EL;
  uint64_t privileged_harness;  /* require harness to run at EL1 between runs ? */
  uint64_t last_tick; /* clock ticks since last verbose print */
  uint64_t asid;  /* current ASID */
  void* concretization_st; /* current state of the concretizer */
  const litmus_test_t* cfg;
};

void init_test_ctx(test_ctx_t* ctx, const litmus_test_t* cfg, int no_runs);
void free_test_ctx(test_ctx_t* ctx);


/* helper functions */
uint64_t ctx_pa(test_ctx_t* ctx, uint64_t va);
uint64_t* ctx_pte(test_ctx_t* ctx, uint64_t va);

uint64_t idx_from_varname_infos(const litmus_test_t* cfg, var_info_t* infos, const char* varname);
uint64_t idx_from_varname(test_ctx_t* ctx, const char* varname);
uint64_t idx_from_regname(test_ctx_t* ctx, const char* regname);
const char* varname_from_idx(test_ctx_t* ctx, uint64_t idx);
const char* regname_from_idx(test_ctx_t* ctx, uint64_t idx);

/* for loading var_info_t */
void read_var_infos(test_ctx_t* ctx, const litmus_test_t* cfg, var_info_t* infos, int no_runs);

void set_init_var(test_ctx_t* ctx, var_info_t* infos, uint64_t varidx, uint64_t idx);
void concretization_precheck(test_ctx_t* ctx, const litmus_test_t* cfg, var_info_t* infos);
void concretization_postcheck(test_ctx_t*, const litmus_test_t* cfg, var_info_t* infos, int run);
void concretize_one(concretize_type_t type, test_ctx_t* ctx, const litmus_test_t* cfg, void* st, int run);
void concretize(concretize_type_t type, test_ctx_t* ctx, const litmus_test_t* cfg, var_info_t* infos, int no_runs);

void* concretize_allocate_st(concretize_type_t type, test_ctx_t* ctx, const litmus_test_t* cfg, int no_runs);
void  concretize_free_st(concretize_type_t type, test_ctx_t* ctx, const litmus_test_t* cfg, int no_runs, void* st);

#endif /* LITMUS_CTX_H */