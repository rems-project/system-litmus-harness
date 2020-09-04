#ifndef LITMUS_CONCRETIZATION_H
#define LITMUS_CONCRETIZATION_H

#include "litmus_regions.h"
#include "litmus_var_info.h"
#include "config.h"

/* useful macros */
#define FOREACH_HEAP_VAR(ctx, el) \
    el = &ctx->heap_vars[0]; \
    for (int __v##__COUNTER__ = 0; __v##__COUNTER__ < ctx->cfg->no_heap_vars; __v##__COUNTER__++, el++)

/* asks if var_info_t* v owns a own_level_t r */
#define OWNS_REGION(v, r) ((v)->init_owns_region && (v)->init_owned_region_size == (r))

/* is v1 pinned to v2 ? */
#define PINNED_TO(ctx, v1, v2) (idx_from_varname((ctx), (v1)->pin_region_var) == (v2)->varidx)


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

region_tracker_t* tracker_region(regions_t* regions, region_trackers_t* trackers, uint64_t* va);
dir_tracker_t* tracker_dir(regions_t* regions, region_trackers_t* trackers, uint64_t* va);
page_tracker_t* tracker_page(regions_t* regions, region_trackers_t* trackers, uint64_t* va);
cache_line_tracker_t* tracker_cache_line(regions_t* regions, region_trackers_t* trackers, uint64_t* va);

/* generic concretization functions */

void set_init_pte(test_ctx_t* ctx, uint64_t varidx, uint64_t idx);
void set_init_var(test_ctx_t* ctx, uint64_t varidx, uint64_t idx);
void init_vars(test_ctx_t* ctx, const litmus_test_t* cfg, int run);
void concretization_precheck(test_ctx_t* ctx, const litmus_test_t* cfg, var_info_t* infos);
void concretization_postcheck(test_ctx_t*, const litmus_test_t* cfg, var_info_t* infos, int run);
void concretize_one(concretize_type_t type, test_ctx_t* ctx, const litmus_test_t* cfg, void* st, int run);
void concretize(concretize_type_t type, test_ctx_t* ctx, const litmus_test_t* cfg, var_info_t* infos, int no_runs);

void* concretize_allocate_st(concretize_type_t type, test_ctx_t* ctx, const litmus_test_t* cfg, int no_runs);
void  concretize_free_st(concretize_type_t type, test_ctx_t* ctx, const litmus_test_t* cfg, int no_runs, void* st);

#endif /* LITMUS_CONCRETIZATION_H */