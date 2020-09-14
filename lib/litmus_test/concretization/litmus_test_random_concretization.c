/** an ephemeral compatible concretization algorithm
 *
 * do not use for arrayized runners
 */

#include "lib.h"

typedef struct {
  int no_pins;
  var_info_t** vars;
} pin_st_t;

typedef struct {
  uint8_t picked;
  uint64_t va;
  pin_st_t pins[NUM_PIN_LEVELS];
} var_st_t;

typedef struct {
  region_trackers_t* trackers;
  uint64_t mem_hi;
  uint64_t mem_lo;
  var_st_t var_sts[];
} concretization_st_t;

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

uint8_t overlaps_owned_region(concretization_st_t* st, var_info_t* var, var_info_t* with) {
  if (! with->init_owns_region) {
    return 0;
  }

  uint64_t va = st->var_sts[var->varidx].va;
  uint64_t otherva = st->var_sts[with->varidx].va;
  uint64_t level = with->init_owned_region_size;

  uint64_t lo = otherva & ~BITMASK(LEVEL_SHIFTS[level]);
  uint64_t hi = lo + LEVEL_SIZES[level];

  if (lo <= va && va < hi) {
    return 1;
  }

  return 0;
}

uint8_t has_same_va(concretization_st_t* st, var_info_t* var, var_info_t* with) {
  uint64_t va = st->var_sts[var->varidx].va;
  uint64_t otherva = st->var_sts[with->varidx].va;

  return (va == otherva);
}

uint8_t validate_selection(test_ctx_t* ctx, concretization_st_t* st, var_info_t* thisvar) {
  var_info_t* var;
  FOREACH_HEAP_VAR(ctx, var) {
    if (var->varidx == thisvar->varidx)
      continue;

    if (  has_same_va(st, thisvar, var)
       || overlaps_owned_region(st, thisvar, var)
    ) {
      return 0;
    }
  }

  return 1;
}

void pick_pin(test_ctx_t* ctx, concretization_st_t* st, var_info_t* rootvar, uint64_t rootva, var_info_t* pinnedvar) {
  uint64_t lvl = pinnedvar->pin_region_level;
  uint64_t va_begin = rootva & ~BITMASK(LEVEL_SHIFTS[lvl]);
  uint64_t va_end = va_begin + LEVEL_SIZES[lvl];

  uint64_t va = randrange(va_begin, va_end);
  va = ALIGN_TO(va, 4);  /* always allocated 64-bit aligned values */

  if (pinnedvar->init_region_offset) {
    uint64_t othervaridx = pinnedvar->offset_var;
    if (st->var_sts[othervaridx].picked) {
      uint64_t othershift = LEVEL_SHIFTS[pinnedvar->offset_level];
      uint64_t otherva = st->var_sts[othervaridx].va;
      va &= ~BITMASK(othershift);
      va |= otherva & BITMASK(othershift);
    }
  }

  st->var_sts[pinnedvar->varidx].va = va;
  st->var_sts[pinnedvar->varidx].picked = 1;
}

/** select the VA for a var that OWNs a region
 */
void pick_one(test_ctx_t* ctx, concretization_st_t* st, var_info_t* var, own_level_t lvl) {
  uint64_t va = randrange(st->mem_lo, st->mem_hi);
  va = ALIGN_TO(va, 4);  /* always allocated 64-bit aligned values */

  if (var->init_region_offset) {
    uint64_t othervaridx = var->offset_var;
    if (st->var_sts[othervaridx].picked) {
      uint64_t othershift = LEVEL_SHIFTS[var->offset_level];
      uint64_t otherva = st->var_sts[othervaridx].va;
      va &= ~BITMASK(othershift);
      va |= otherva & BITMASK(othershift);
    }
  }

  /* for each pinned var, pick a VA for it too */
  var_st_t* varst = &st->var_sts[var->varidx];
  for (pin_level_t lvl = REGION_SAME_VAR; lvl < REGION_SAME_PGD; lvl++) {
    pin_st_t* varpin = &varst->pins[lvl];
    var_info_t** pinned_vars = varpin->vars;
    for (int pidx = 0; pidx < varpin->no_pins; pidx++) {
      var_info_t* pinnedvar = pinned_vars[pidx];
      pick_pin(ctx, st, var, va, pinnedvar);
    }
  }

  st->var_sts[var->varidx].va = va;
  st->var_sts[var->varidx].picked = 1;
}

void* concretize_random_init(test_ctx_t* ctx, const litmus_test_t* cfg) {
  concretization_st_t* st = alloc(sizeof(concretization_st_t) + sizeof(var_st_t)*cfg->no_heap_vars);
  valloc_memset(st, 0, sizeof(concretization_st_t) + sizeof(var_st_t)*cfg->no_heap_vars);

  st->mem_lo = (uint64_t)ctx->heap_memory;
  st->mem_hi = st->mem_lo + sizeof(regions_t);

  /* collect pins */
  var_info_t* var;
  FOREACH_HEAP_VAR(ctx, var) {
    var_st_t* varst = &st->var_sts[var->varidx];
    for (pin_level_t lvl = REGION_SAME_VAR; lvl < REGION_SAME_PGD; lvl++) {
      pin_st_t* varpin = &varst->pins[lvl];
      var_info_t** pinned_vars = varpin->vars;
      varpin->no_pins = count_pinned_to(pinned_vars, ctx, var, lvl);
    }
  }

  return (void*)st;
}

static void reset_st(test_ctx_t* ctx, const litmus_test_t* cfg, concretization_st_t* st) {
  for (int i = 0; i < cfg->no_heap_vars; i++) {
    var_st_t* vst = &st->var_sts[i];
    vst->picked = 0;
  }
}

void concretize_random_one(test_ctx_t* ctx, const litmus_test_t* cfg, concretization_st_t* st, int run) {
  var_info_t* var;

  int count = 1;
  int tryagain = 1;
  while (tryagain) {
    tryagain = 0;

    reset_st(ctx, cfg, st);

    for (own_level_t lvl = REGION_OWN_PGD; lvl > REGION_OWN_VAR; lvl--) {
      FOREACH_HEAP_VAR(ctx, var) {
        if (OWNS_REGION(var, lvl)) {
          pick_one(ctx, st, var, lvl);
        }
      }
    }

    FOREACH_HEAP_VAR(ctx, var) {
      if (! validate_selection(ctx, st, var)) {
        count++;
        tryagain = 1;
        break;
      }
    }
  }

  debug("# allocate_random_one: tried %d times to allocate VAs\n", count);

  FOREACH_HEAP_VAR(ctx, var) {
    var->values[run] = (uint64_t*)st->var_sts[var->varidx].va;
    debug("%s.va = %p\n", var->name, var->values[run]);
  }
}

void concretize_random_all(test_ctx_t* ctx, const litmus_test_t* cfg, concretization_st_t* st, int no_runs) {
  for (uint64_t i = 0; i < ctx->no_runs; i++) {
    concretize_random_one(ctx, cfg, st, i);
  }
}

void concretize_random_finalize(test_ctx_t* ctx, const litmus_test_t* cfg, void* st) {
  free(st);
}