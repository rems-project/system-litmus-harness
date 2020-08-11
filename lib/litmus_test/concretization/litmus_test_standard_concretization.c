/** this file contains the code that takes the symbolic litmus test information
 * the desired number of concrete tests and the allocated regions
 */

#include "lib.h"

/** given all the variables try find some initial region allocation
 * that satisfies all their requirements
 */
static void init_allocate_regions(test_ctx_t* ctx, var_info_t* infos);

/* ensure each unpinned var goes into its own region
 */
static void init_allocate_next_free_region(test_ctx_t* ctx, var_info_t* vinfo, uint8_t* allocated) {
  for (int i = 0; i < NR_REGIONS; i++) {
    if (! allocated[i]) {
      allocated[i] = 1;
      vinfo->curr_region = i;
      return;
    }
  }

  fail("! concretization: cannot pick initial region for var '%s': no free spaces.\n",
    vinfo->name
  );
}

static void init_allocate_regions(test_ctx_t* ctx, var_info_t* infos) {
  uint8_t allocated [NR_REGIONS];
  for (int i = 0; i < ctx->cfg->no_heap_vars; i++) {
    var_info_t* vinfo = &infos[i];
    if (! vinfo->init_region_pinned) {
      init_allocate_next_free_region(ctx, vinfo, &allocated[0]);
    }
  }
}

/** given a loc update it in-place to the next loc position that is free
 * subject to some conditions:
 *  max_level_change: do not increment past this boundary.
 *    e.g. if max_level_change=4 do not increment past the end of the cache line
 *
 *  stop_level: do not go past this level during propagation
 *
 * returns:
 *  0 on success
 *  1 if reached stop_level
 *  2 if ran past max_level_change
 */
static int increment_loc(region_trackers_t* trackers, tracker_loc_t* loc, int max_level_change, int stop_level) {
  int region = loc->reg_ix;

  if (region >= NR_REGIONS) {
    fail("! concretize no regions left\n");
  } else if (stop_level == 0) {
    return 1;
  }

  region_tracker_t* reg = &trackers->tickers[region];
  uint64_t d = loc->dir_ix;
  if (d < reg->curr_dir_ix)
    d = reg->curr_dir_ix;

  if (d >= NR_DIRS_PER_REGION) {
    fail("! concretize: cannot spill into a neighbouring region\n");
  } else if (stop_level == 1) {
    return 1;
  }

  dir_tracker_t* dir = &reg->dir_ixs[d];
  uint64_t p = loc->page_ix;
  if (p < dir->curr_page_ix)
    p = dir->curr_page_ix;

  if (p >= NR_PAGES_PER_DIR) {
    if (max_level_change > 2) {
      return 2;
    }

    loc->dir_ix ++;
    increment_loc(trackers, loc, max_level_change, 2);

    d = reg->curr_dir_ix;
    dir = &reg->dir_ixs[d];
    p = loc->page_ix = dir->curr_page_ix;
  } else if (stop_level == 2) {
    return 1;
  }

  page_tracker_t* page = &dir->page_ixs[p];
  uint64_t c = loc->scl_ix;
  if (c < page->curr_scl_ix)
    c = page->curr_scl_ix;

  if (c >= NR_CACHE_LINES_PER_PAGE) {
    if (max_level_change > 3) {
      return 2;
    }

    loc->page_ix ++;
    increment_loc(trackers, loc, max_level_change, 3);

    d = reg->curr_dir_ix;
    dir = &reg->dir_ixs[d];
    p = loc->page_ix = dir->curr_page_ix;
    page = &dir->page_ixs[p];
    c = loc->scl_ix = page->curr_scl_ix;
  } else if (stop_level == 3) {
    return 1;
  }

  cache_line_tracker_t* cl = &page->scl_ix[c];

  if (cl->curr_val_ix >= NR_u64_PER_CACHE_LINE) {
    if (max_level_change > 4) {
      return 2;
    }

    loc->scl_ix ++;
    increment_loc(trackers, loc, max_level_change, 4);

    d = reg->curr_dir_ix;
    dir = &reg->dir_ixs[d];
    p = loc->page_ix = dir->curr_page_ix;
    page = &dir->page_ixs[p];
    c = loc->scl_ix = page->curr_scl_ix;
    cl = &page->scl_ix[c];
  } else if (stop_level == 4) {
    return 1;
  }

  loc->val_ix = cl->curr_val_ix;
  loc->scl_ix = page->curr_scl_ix;
  loc->page_ix = dir->curr_page_ix;
  loc->dir_ix = reg->curr_dir_ix;
  loc->reg_ix = region;
  return 0;
}

static uint64_t* va_from_loc(test_ctx_t* ctx, region_trackers_t* regions, uint64_t varidx, tracker_loc_t* loc) {
  region_t* reg = &ctx->heap_memory->regions[loc->reg_ix];
  dir_t* dir = &reg->dirs[loc->dir_ix];
  page_t* pg = &dir->pages[loc->page_ix];
  uint64_t* va = &pg->values[loc->val_ix + loc->scl_ix*NR_u64_PER_CACHE_LINE];

  region_tracker_t* reg_track = &regions->tickers[loc->reg_ix];
  dir_tracker_t* dir_track = &reg_track->dir_ixs[loc->dir_ix];
  page_tracker_t* pg_track = &dir_track->page_ixs[loc->page_ix];
  cache_line_tracker_t* cl_track = &pg_track->scl_ix[loc->scl_ix];

  /* mark as used */
  pg_track->curr_scl_ix ++;

  /* spill over */
  if (cl_track->curr_val_ix >= NR_u64_PER_CACHE_LINE) {
    pg_track->curr_scl_ix ++;
  }

  if (pg_track->curr_scl_ix >= NR_CACHE_LINES_PER_PAGE) {
    dir_track->curr_page_ix ++;
  }

  if (dir_track->curr_page_ix >= NR_PAGES_PER_DIR) {
    reg_track->curr_dir_ix ++;
  }

  if (reg_track->curr_dir_ix >= NR_DIRS_PER_REGION) {
    /* do not fail yet */
  }

  return va;
}


/** to choose unpinned VAs we find the next free slot
 *
 * we do not have to worry about PTE entries as unpinned VAs may not have PTE attributes set
 */
static uint64_t* choose_va_unpinned(test_ctx_t* ctx, const litmus_test_t* cfg, region_trackers_t* tickers, tracker_loc_t* var_locs, uint64_t varidx, var_info_t* vinfo) {
  /* load the current pointer */
  var_locs[varidx].reg_ix = vinfo->curr_region;
  region_tracker_t* reg_tracker = &tickers->tickers[var_locs[varidx].reg_ix];

  var_locs[varidx].dir_ix = reg_tracker->curr_dir_ix;
  dir_tracker_t* dir_tracker = &reg_tracker->dir_ixs[var_locs[varidx].dir_ix];

  var_locs[varidx].page_ix = dir_tracker->curr_page_ix;
  page_tracker_t* pg_tracker = &dir_tracker->page_ixs[var_locs[varidx].page_ix];

  var_locs[varidx].scl_ix = pg_tracker->curr_scl_ix;
  cache_line_tracker_t* cl_tracker = &pg_tracker->scl_ix[var_locs[varidx].scl_ix];

  var_locs[varidx].val_ix = cl_tracker->curr_val_ix;

  /* if val_ix out of bounds, then find next free slot */
  increment_loc(tickers, &var_locs[varidx], 2, 5);

  /* *only now* mark that location as taken in the trackers */
  return va_from_loc(ctx, tickers, varidx, &var_locs[varidx]);
}

/** chooing VAs for variables that are pinned to some other variable, with varying PTE attributes is more difficult
 *
 * firstly we must ensure that the constraints are maintained (if X is SAME_PAGE as Y e.g.)
 * second we must ensure that if two variables supply PTE attributes then they do not get placed
 * in the same page.
 */
static uint64_t* choose_va_pinned(test_ctx_t* ctx, const litmus_test_t* cfg, region_trackers_t* tickers, tracker_loc_t* var_locs, uint64_t varidx, var_info_t* vinfo) {
  uint64_t pinned_idx = idx_from_varname_infos(ctx->cfg, ctx->heap_vars, vinfo->pin_region_var);
  tracker_loc_t* pinned_loc = &var_locs[pinned_idx];
  tracker_loc_t* loc = &var_locs[varidx];

  int err;

  switch (vinfo->pin_region_level) {
    case REGION_SAME_CACHE_LINE:
      loc->reg_ix = pinned_loc->reg_ix;
      loc->dir_ix = pinned_loc->dir_ix;
      loc->page_ix = pinned_loc->page_ix;
      loc->scl_ix = pinned_loc->scl_ix;
      loc->val_ix = pinned_loc->val_ix + 1;
      err = increment_loc(tickers, loc, 5, 5);
      if (err == 2) {
        return NULL;
      }
      break;
    case REGION_SAME_PAGE:
      loc->reg_ix = pinned_loc->reg_ix;
      loc->dir_ix = pinned_loc->dir_ix;
      loc->page_ix = pinned_loc->page_ix;
      loc->scl_ix = pinned_loc->scl_ix + 1;
      loc->val_ix = 0;
      err = increment_loc(tickers, loc, 4, 5);
      if (err == 2) {
        return NULL;
      }
      break;
    case REGION_SAME_PMD:
      loc->reg_ix = pinned_loc->reg_ix;
      loc->dir_ix = pinned_loc->dir_ix;
      loc->page_ix = pinned_loc->page_ix; /* +N */
      loc->scl_ix = 0;
      loc->val_ix = 0;
      int tryagain = 1;
      while (tryagain) {
        tryagain = 0;

        loc->page_ix ++;
        err = increment_loc(tickers, loc, 3, 5);
        if (err == 2) {
          return NULL;
        }

        for (int v = 0; v < ctx->cfg->no_heap_vars; v++) {
          if (v != varidx) {
            if (
                 (var_locs[v].reg_ix == var_locs[varidx].reg_ix)
              && (var_locs[v].dir_ix == var_locs[varidx].dir_ix)
              && (var_locs[v].page_ix == var_locs[varidx].page_ix)
            ) {
              tryagain = 1;
              break;
            }
          }
        }
      }
      break;
    case REGION_SAME_PGD:
      fail("! concatneize: REGION_SAME_PGD not implemented\n");
      break;
  }

  return va_from_loc(ctx, tickers, varidx, loc);
}

static uint64_t* choose_va(test_ctx_t* ctx, const litmus_test_t* cfg, region_trackers_t* tickers, tracker_loc_t* var_locs, uint64_t varidx,var_info_t* vinfo) {
  if (vinfo->init_region_pinned) {
    return choose_va_pinned(ctx, cfg, tickers, var_locs, varidx, vinfo);
  } else {
    return choose_va_unpinned(ctx, cfg, tickers, var_locs, varidx, vinfo);
  }
}

static void increment_region(var_info_t* vinfo) {
  int new_region = (vinfo->curr_region + 1) % NR_REGIONS;

  if (new_region >= NR_REGIONS) {
    new_region = 0;
  }

  vinfo->curr_region = new_region;
}

void concretize_standard(test_ctx_t* ctx, const litmus_test_t* cfg, var_info_t* infos, int no_runs) {
  region_trackers_t* tickers = alloc(sizeof(region_trackers_t));

  /* check that the test _can_ be concretized correctly
   * or fail if there's some error */
  concretization_precheck(ctx, cfg, infos);

  init_allocate_regions(ctx, infos);

  for (int i = 0; i < no_runs; i++) {
    uint8_t failed = 1;

    debug("picking VAs for run %d/%d\n", i, no_runs);

    /* pick VAs */
    while (failed) {
      failed = 0;
      tracker_loc_t locs [cfg->no_heap_vars];

      for (int v = 0; v < cfg->no_heap_vars; v++) {
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

    /* initialise all those VAs */
    for (int v = 0; v < cfg->no_heap_vars; v++) {
      set_init_var(ctx, infos, v, i);
    }

    /* check that the concretization was successful before continuing */
    concretization_postcheck(ctx, cfg, infos, i);
  }

  free(tickers);
}