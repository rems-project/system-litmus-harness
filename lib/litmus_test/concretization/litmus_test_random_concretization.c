/** an ephemeral compatible concretization algorithm
 *
 * do not use for arrayized runners
 */

#include "lib.h"

#define FOREACH_HEAP_VAR(ctx, el) \
    el = &ctx->heap_vars[0]; \
    for (int __v##__COUNTER__ = 0; __v##__COUNTER__ < ctx->cfg->no_heap_vars; __v##__COUNTER__++, el++)

/* asks if var_info_t* v owns a own_level_t r */
#define OWNS_REGION(v, r) ((v)->init_owns_region && (v)->init_owned_region_size == (r))


typedef struct {
  tracker_loc_t curr_loc;
} var_st_t;

typedef struct {
  region_trackers_t* trackers;
  var_st_t var_sts[];
} concretization_st_t;

void* concretize_random_alloc_st(test_ctx_t* ctx, const litmus_test_t* cfg, int no_runs) {
  return NULL;
}

uint8_t overlaps(tracker_loc_t* t1, tracker_loc_t* t2, own_level_t lvl) {
  if (   (                               (t1->reg_ix == t2->reg_ix))
      && (lvl > REGION_OWN_PMD        || (t1->dir_ix == t2->dir_ix))
      && (lvl > REGION_OWN_PAGE       || (t1->page_ix == t2->page_ix))
      && (lvl > REGION_OWN_CACHE_LINE || (t1->scl_ix == t2->scl_ix))
      && (lvl > REGION_OWN_VAR        || (t1->val_ix == t2->val_ix))
  ) {
    return 1;
  }

  return 0;
}

uint8_t validate_selection(test_ctx_t* ctx, concretization_st_t* st, var_info_t* thisvar, tracker_loc_t loc, own_level_t lvl) {
  var_info_t* var;
  FOREACH_HEAP_VAR(ctx, var) {
    tracker_loc_t* other_loc = &st->var_sts[var->varidx].curr_loc;

    /* if there are any other vars in the same own_level_t return false */
    if (overlaps(other_loc, &loc, lvl)) {
      return 0;
    }

    /* if this var got allocated in the same region as another own_level_t
     * _and_ this var isn't pinned to that var */
    if (var->init_owns_region) {
      if ( overlaps(&loc, other_loc, var->init_owned_region_size)
        && !(var->init_region_pinned && (idx_from_varname(ctx,var->pin_region_var) != var->varidx))
      ) {
        return 0;
      }
    }

    /* finally ensure this does not overlap with the page of an unpinned unowned var */
    if (!var->init_owns_region && !var->init_region_pinned) {
      if (overlaps(&loc, other_loc, REGION_OWN_PAGE)) {
        return 0;
      }
    }
  }

  return 1;
}

void pick_one(test_ctx_t* ctx, concretization_st_t* st, var_info_t* var, own_level_t lvl) {
  tracker_loc_t loc;
  while (1) {
    loc.reg_ix = randn() % NR_REGIONS;
    loc.dir_ix = randn() % NR_DIRS_PER_REGION;
    loc.page_ix = randn() % NR_PAGES_PER_DIR;
    loc.scl_ix = randn() % NR_CACHE_LINES_PER_PAGE;
    loc.val_ix = randn() % NR_u64_PER_CACHE_LINE;

    if (validate_selection(ctx, st, var, loc, lvl))
      break;
  }

  st->var_sts[var->varidx].curr_loc = loc;
}

void pick_pin(test_ctx_t* ctx, concretization_st_t* st, var_info_t* var) {
  uint64_t othervaridx = idx_from_varname(ctx, var->pin_region_var);
  tracker_loc_t loc;
  tracker_loc_t* other_var_loc = &st->var_sts[othervaridx].curr_loc;

  while (1) {
    if (var->pin_region_level <= REGION_SAME_PMD) {
      loc.reg_ix = other_var_loc->reg_ix;
      loc.dir_ix = other_var_loc->reg_ix;
    } else {
      loc.reg_ix = randn() % NR_REGIONS;
      loc.dir_ix = randn() % NR_DIRS_PER_REGION;
    }

    if (var->pin_region_level <= REGION_SAME_PAGE) {
      loc.page_ix = other_var_loc->page_ix;
    } else {
      loc.page_ix = randn() % NR_PAGES_PER_DIR;
    }

    if (var->pin_region_level <= REGION_SAME_CACHE_LINE) {
      loc.scl_ix = other_var_loc->scl_ix;
    } else {
      loc.scl_ix = randn() % NR_CACHE_LINES_PER_PAGE;
    }

    loc.val_ix = randn() % NR_u64_PER_CACHE_LINE;

    if (validate_selection(ctx, st, var, loc, REGION_OWN_VAR))
      break;
  }

  st->var_sts[var->varidx].curr_loc = loc;
}

uint64_t* va_from_loc(test_ctx_t* ctx, tracker_loc_t loc) {
  return &ctx->heap_memory->
          regions[loc.reg_ix]
          .dirs[loc.dir_ix]
          .pages[loc.page_ix]
          .values[loc.val_ix + loc.scl_ix*NR_CACHE_LINES_PER_PAGE];
}

void concretize_random_one(test_ctx_t* ctx, const litmus_test_t* cfg, void* _, int run) {
  var_info_t* var;

  concretization_st_t* st = alloc(sizeof(concretization_st_t) + sizeof(var_st_t)*cfg->no_heap_vars);
  valloc_memset(st, 0, sizeof(concretization_st_t) + sizeof(var_st_t)*cfg->no_heap_vars);

  for (own_level_t lvl = REGION_OWN_PGD; lvl > REGION_OWN_VAR; lvl--) {
    FOREACH_HEAP_VAR(ctx, var) {
      if (OWNS_REGION(var, lvl)) {
        pick_one(ctx, st, var, lvl);
      }
    }
  }

  FOREACH_HEAP_VAR(ctx, var) {
    if (!var->init_owns_region) {
      pick_one(ctx, st, var, REGION_SAME_PAGE);
    }
  }

  FOREACH_HEAP_VAR(ctx, var) {
    if (var->init_region_pinned) {
      pick_pin(ctx, st, var);
    }
  }

  /* allocate VAs */

  FOREACH_HEAP_VAR(ctx, var) {
    uint64_t* va = va_from_loc(ctx, st->var_sts[var->varidx].curr_loc);
    var->values[run] = va;
  }

  free(st);
}

void concretize_random_all(test_ctx_t* ctx, const litmus_test_t* cfg, void* st, int no_runs) {
  for (uint64_t i = 0; i < ctx->no_runs; i++) {
    concretize_random_one(ctx, cfg, st, i);
  }
}

void concretize_random_free_st(test_ctx_t* ctx, const litmus_test_t* cfg, int no_runs, void* st) {
  /* nop */
}