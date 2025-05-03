#ifndef LITMUS_CONCRETIZATION_H
#define LITMUS_CONCRETIZATION_H

#include "litmus_idxs.h"
#include "litmus_regions.h"
#include "litmus_var_info.h"
#include "litmus_regions_idxs.h"
#include "config.h"

/* useful macros */
#define __FOREACH_HEAP_VAR(var, ctx, el) \
  el = &ctx->heap_vars[0];               \
  for (int var = 0; var < ctx->cfg->no_heap_vars; var++, el++)

#define FOREACH_HEAP_VAR(ctx, el) __FOREACH_HEAP_VAR(FRESH_VAR, ctx, el)

/* asks if var_info_t* v owns a own_level_t r */
// #define OWNS_REGION(v, r) ((v)->init_owns_region && (v)->init_owned_region_size == (r))
static inline u8 owns_region(var_info_t* vinfo, own_level_t r) {
  return (vinfo->ty == VAR_HEAP && vinfo->heap.owned_region_size == r);
}

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

typedef struct
{
  const char* exclusive_owner; /* if some var owns this cache line, its name, otherwise NULL */
  u64 curr_val_ix;
} cache_line_tracker_t;

typedef struct
{
  const char* exclusive_owner; /* if some var owns this page, its name, otherwise NULL */
  u64 curr_scl_ix;
  cache_line_tracker_t scl_ix[NR_CACHE_LINES_PER_PAGE];
} page_tracker_t;

typedef struct
{
  const char* exclusive_owner; /* if some var owns this dir, its name, otherwise NULL */
  u64 curr_page_ix;
  page_tracker_t page_ixs[NR_PAGES_PER_DIR];
} dir_tracker_t;

typedef struct
{
  u64 curr_dir_ix;
  dir_tracker_t dir_ixs[NR_DIRS_PER_REGION];
} region_tracker_t;

typedef struct
{
  region_tracker_t regions[NR_REGIONS];
} region_trackers_t;

region_tracker_t* tracker_region(regions_t* regions, region_trackers_t* trackers, u64* va);
dir_tracker_t* tracker_dir(regions_t* regions, region_trackers_t* trackers, u64* va);
page_tracker_t* tracker_page(regions_t* regions, region_trackers_t* trackers, u64* va);
cache_line_tracker_t* tracker_cache_line(regions_t* regions, region_trackers_t* trackers, u64* va);

/* generic concretization functions */

var_idx_t count_pinned_to(var_info_t** out_vinfos, test_ctx_t* ctx, var_info_t* var, pin_level_t lvl);

void concretization_precheck(test_ctx_t* ctx, const litmus_test_t* cfg, var_info_t* infos);
void concretization_postcheck(test_ctx_t*, const litmus_test_t* cfg, var_info_t* infos, run_idx_t run);
void concretization_postcheck_batch(test_ctx_t*, const litmus_test_t* cfg, var_info_t* infos, run_count_t batch_start_idx, run_count_t batch_end_idx);
void concretize_one(concretize_type_t type, test_ctx_t* ctx, const litmus_test_t* cfg, void* st, run_idx_t run);
void concretize(concretize_type_t type, test_ctx_t* ctx, const litmus_test_t* cfg, void* st, run_idx_t no_runs);

/** concretize a whole batch at once
 * assuming the pagetables on each batch does not interfere
 * but that the PAs cannot overlap
 */
void concretize_batch(
  concretize_type_t type, test_ctx_t* ctx, const litmus_test_t* cfg, run_count_t batch_start_idx,
  run_count_t batch_end_idx
);

void write_init_state(test_ctx_t* ctx, const litmus_test_t* cfg, run_idx_t run);
void write_init_states(test_ctx_t* ctx, const litmus_test_t* cfg, run_idx_t no_runs);

void* concretize_init(concretize_type_t type, test_ctx_t* ctx, const litmus_test_t* cfg, run_idx_t no_runs);
void concretize_finalize(concretize_type_t type, test_ctx_t* ctx, const litmus_test_t* cfg, run_idx_t no_runs, void* st);

#endif /* LITMUS_CONCRETIZATION_H */