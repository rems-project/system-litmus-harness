/** this file contains the code that takes the symbolic litmus test information
 * the desired number of concrete tests and the allocated regions
 */

#include "lib.h"

#define FOREACH_HEAP_VAR(ctx, el) \
    el = &ctx->heap_vars[0]; \
    for (int __v##__COUNTER__ = 0; __v##__COUNTER__ < ctx->cfg->no_heap_vars; __v##__COUNTER__++, el++)

/* asks if var_info_t* v owns a own_level_t r */
#define OWNS_REGION(v, r) ((v)->init_owns_region && (v)->init_owned_region_size == (r))

typedef struct {
  tracker_loc_t curr_loc;
  int free_space_in_reg;
} var_st_t;

typedef struct {
  region_trackers_t* trackers;
  var_st_t* var_sts;
} concretization_st_t;


void pick_2m_region(test_ctx_t* ctx, concretization_st_t* st, var_info_t* var, int run) {
  /* find a dir in st->trackers that is empty and set it to exclusively this var */
}

void concretize_standard(test_ctx_t* ctx, const litmus_test_t* cfg, var_info_t* infos, int no_runs) {
  /* TODO: just use region itself ? */
  region_trackers_t* tickers = alloc(sizeof(region_trackers_t));
  valloc_memset(tickers, 0, sizeof(region_trackers_t));

  concretization_st_t st;
  st.trackers = tickers;
  st.var_sts = ALLOC_MANY(var_st_t, cfg->no_heap_vars);

  for (int i = 0; i < no_runs; i++) {
    uint8_t failed = 1;

    debug("picking VAs for run %d/%d\n", i, no_runs);

    /* pick VAs */
    while (failed) {
      failed = 0;
      tracker_loc_t locs [cfg->no_heap_vars];

      var_info_t* var;

      /* greedy part: pick all the 1G regions first */
      FOREACH_HEAP_VAR(ctx, var) {
        if (OWNS_REGION(var, REGION_OWN_PGD)) {
          fail("! concretize: REGION_OWN_PGD not supported.\n"); /* not enough memory */
        }
      }
      /* then the 2M regions */
      FOREACH_HEAP_VAR(ctx, var) {
        if (OWNS_REGION(var, REGION_OWN_PMD)) {
          pick_2m_region(ctx, var, i);
        }
      }
      /* then the 4k regions */
      FOREACH_HEAP_VAR(ctx, var) {
        if (OWNS_REGION(var, REGION_OWN_PAGE)) {
          pick_4k_region(ctx, var, i);
        }
      }

      /* then all the remaining unpinned regions */
      FOREACH_HEAP_VAR(ctx, var) {
        if (!var->init_region_pinned && !var->init_owns_region) {
          pick_unpinned_region(ctx, var);
        }
      }


      for (int v = 0; v < ctx->cfg->no_heap_vars; v++) {
        var_info_t* vinfo = &infos[v];

        if (! vinfo->init_region_pinned) {
          increment_region(vinfo);
          uint64_t* va = choose_va(ctx, cfg, tickers, locs, v, vinfo);

          if (va == NULL) {
            failed = 1;
            break;
          }

          vinfo->values[i] = va;
        }
      }

      for (int v = 0; v < cfg->no_heap_vars; v++) {
        var_info_t* vinfo = &infos[v];

        if (vinfo->init_region_pinned) {
          uint64_t* va = choose_va(ctx, cfg, tickers, locs, v, vinfo);

          if (va == NULL) {
            failed = 1;
            break;
          }

          vinfo->values[i] = va;
        }
      }
    }
  }

  free(tickers);
}