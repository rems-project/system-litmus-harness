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
  u8 picked_region;
  u8 picked_offset;
  region_idx_t partial_va;
  u64 va;
  pin_st_t pins[NUM_PIN_LEVELS];
} var_st_t;

typedef struct {
  u64 _;
  var_st_t var_sts[];
} concretization_st_t;

u8 overlaps_owned_regions(concretization_st_t* st, var_info_t* var, var_info_t* with) {
  if (!var_owns_region(var) || !var_owns_region(with))
    return 0;

  own_level_t with_region = var_owned_region_size(with);

  u64 va = st->var_sts[var->varidx].va;
  u64 otherva = st->var_sts[with->varidx].va;

  u64 lo = otherva & ~BITMASK(LEVEL_SHIFTS[with_region]);
  u64 hi = lo + LEVEL_SIZES[with_region];

  if (lo <= va && va < hi) {
    return 1;
  }

  return 0;
}

u8 has_same_va(concretization_st_t* st, var_info_t* var, var_info_t* with) {
  u64 va = st->var_sts[var->varidx].va;
  u64 otherva = st->var_sts[with->varidx].va;

  return (va == otherva);
}

u8 __can_virtually_alias_with(concretization_st_t* st, var_info_t* var, var_info_t* with) {
  return (
       (var->ty == VAR_PINNED)
    && (var->pin.pin_region_var == with->varidx)
    && (var->pin.pin_region_level == REGION_SAME_VAR)
  );
}

u8 can_virtually_alias(concretization_st_t* st, var_info_t* var, var_info_t* with) {
  return __can_virtually_alias_with(st, var, with) || __can_virtually_alias_with(st, with, var);
}

u8 validate_selection(test_ctx_t* ctx, concretization_st_t* st, var_info_t* thisvar) {
  var_info_t* var;
  FOREACH_HEAP_VAR(ctx, var) {
    if (var->varidx == thisvar->varidx)
      continue;

    if (  (has_same_va(st, thisvar, var) && !can_virtually_alias(st, thisvar, var))
       || overlaps_owned_regions(st, thisvar, var)
    ) {
      debug("OVERLAPS between %s and %s\n", thisvar->name, var->name);
      return 0;
    }
  }

  return 1;
}

region_idx_t rand_idx(region_idx_t start, region_idx_t end) {
  u64 reg = randrange(start.reg_ix, end.reg_ix);
  u64 offs = randrange(start.reg_offs, MIN(end.reg_offs, NR_DIRS_PER_REGION*DIR_SIZE));
  offs = ALIGN_TO(offs, 4);  /* always allocated 64-bit aligned values */
  return (region_idx_t){.reg_ix=reg, .reg_offs=offs};
}

/** select the VA for a var that OWNs a region
 */
static bool try_pick_one_offset(test_ctx_t* ctx, concretization_st_t* st, var_info_t* var) {
  region_idx_t va_idx = st->var_sts[var->varidx].partial_va;

  if (var->init_attrs.has_region_offset) {
    u64 othervaridx = var->init_attrs.region_offset.offset_var;
    if (st->var_sts[othervaridx].picked_offset) {
      u64 othershift = LEVEL_SHIFTS[var->init_attrs.region_offset.offset_level];
      u64 otherva = st->var_sts[othervaridx].va;

      va_idx.reg_offs &= ~BITMASK(othershift);
      va_idx.reg_offs |= otherva & BITMASK(othershift);
    } else {
      return false;
    }
  }

  st->var_sts[var->varidx].partial_va = va_idx;
  st->var_sts[var->varidx].va = va_from_region_idx(ctx, var, va_idx);
  st->var_sts[var->varidx].picked_offset = 1;

  DEBUG(DEBUG_CONCRETIZATION, "picked (offset)! %s => %p\n", var->name, st->var_sts[var->varidx].va);
  return true;
}

static void pick_pin_region(test_ctx_t* ctx, concretization_st_t* st, var_info_t* rootvar, region_idx_t rootidx, var_info_t* pinnedvar) {
  pin_level_t lvl = pinnedvar->pin.pin_region_level;
  region_idx_t va_begin = align_down_region_idx(rootidx, lvl);
  region_idx_t va_end = align_up_region_idx(rootidx, lvl);
  region_idx_t va_idx = rand_idx(va_begin, va_end);

  st->var_sts[pinnedvar->varidx].partial_va = va_idx;
  st->var_sts[pinnedvar->varidx].picked_region = 1;

  DEBUG(DEBUG_CONCRETIZATION, "picked pin (region)! %s => %p\n", pinnedvar->name, st->var_sts[pinnedvar->varidx].partial_va.reg_ix);
}

static void pick_one_region(test_ctx_t* ctx, concretization_st_t* st, var_info_t* var, own_level_t lvl) {
  region_idx_t va_begin = region_idx_bottom();
  region_idx_t va_top = region_idx_top();
  region_idx_t va_idx = rand_idx(va_begin, va_top);

  DEBUG(DEBUG_CONCRETIZATION, "for var=%s,  between [%o, %o) = %o\n", var->name, TOSTR(region_idx_t, &va_begin), TOSTR(region_idx_t, &va_top), TOSTR(region_idx_t, &va_idx));

  st->var_sts[var->varidx].partial_va = va_idx;
  st->var_sts[var->varidx].picked_region = 1;

  DEBUG(DEBUG_CONCRETIZATION, "picked (region)! %s => %p\n", var->name, st->var_sts[var->varidx].partial_va.reg_ix);


  /* for each pinned var, pick a VA for it too */
  var_st_t* varst = &st->var_sts[var->varidx];
  for (pin_level_t pin_lvl = REGION_SAME_VAR; pin_lvl < REGION_SAME_PGD; pin_lvl++) {
    pin_st_t* varpin = &varst->pins[pin_lvl];
    var_info_t** pinned_vars = varpin->vars;
    for (int pidx = 0; pidx < varpin->no_pins; pidx++) {
      var_info_t* pinnedvar = pinned_vars[pidx];
      pick_pin_region(ctx, st, var, va_idx, pinnedvar);
    }
  }
}

void* concretize_random_init(test_ctx_t* ctx, const litmus_test_t* cfg) {
  concretization_st_t* st = alloc(sizeof(concretization_st_t) + sizeof(var_st_t)*cfg->no_heap_vars);

  for (int varidx = 0; varidx < cfg->no_heap_vars; varidx++) {
    for (int i = 0; i < NUM_PIN_LEVELS; i++) {
      var_st_t* var_st = &st->var_sts[varidx];
      pin_st_t* pin_st = &var_st->pins[i];
      pin_st->no_pins = 0;
      pin_st->vars = ALLOC_MANY(var_info_t*, cfg->no_heap_vars);
    }
  }

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
    vst->picked_region = 0;
    vst->picked_offset = 0;
  }
}

void concretize_random_one(test_ctx_t* ctx, const litmus_test_t* cfg, concretization_st_t* st, int run) {
  var_info_t* var;

  int count = 1;
  int tryagain = 1;
  while (tryagain) {
    tryagain = 0;

    reset_st(ctx, cfg, st);

    /* first, pick all the unconstrained heap variables */
    FOREACH_HEAP_VAR(ctx, var) {
      if (var_owns_region(var) && !var->init_attrs.has_region_offset) {
        pick_one_region(ctx, st, var, var_owned_region_size(var));
      }
    }

    /* pick the heap vars that are constrained
     * and any that are pinned to those
     * this should pick any HEAP,UNMAPPED or FIXED regions
     * and any PINNED vars within those regions
     */
    FOREACH_HEAP_VAR(ctx, var) {
      if (var_owns_region(var) && var->init_attrs.has_region_offset) {
        pick_one_region(ctx, st, var, var_owned_region_size(var));
      }
    }

    /* should be none left over */
    FOREACH_HEAP_VAR(ctx, var) {
      fail_on (!st->var_sts[var->varidx].picked_region, "failed to pick a VA (region) for \"%s\"\n", var->name);
    }

    /* now try pick the offsets */
    bool picked_offset = true;
    while (picked_offset) {
      picked_offset = false;
      FOREACH_HEAP_VAR(ctx, var) {
        if (! st->var_sts[var->varidx].picked_offset) {
          picked_offset = try_pick_one_offset(ctx, st, var);

          if (picked_offset)
            break;
        }
      }
    }

    /* should be none left over */
    FOREACH_HEAP_VAR(ctx, var) {
      fail_on (!st->var_sts[var->varidx].picked_offset, "failed to pick a VA (offset) for \"%s\"\n", var->name);
    }

    /* because we pick the owned regions at random, they may overlap
     * so check for overlaps and explicitly discard if so.
     */
    FOREACH_HEAP_VAR(ctx, var) {
      if (! validate_selection(ctx, st, var)) {
        debug("for run #%ld failed to validate the picks, trying again...\n", run);
        count++;
        tryagain = 1;
        break;
      }
    }
  }

  FOREACH_HEAP_VAR(ctx, var) {
    var->values[run] = (u64*)st->var_sts[var->varidx].va;
  }

  if (DEBUG_CONCRETIZATION) {
    debug("# allocate_random_one: tried %d times to allocate VAs\n", count);

    FOREACH_HEAP_VAR(ctx, var) {
      debug("%s.va = %p\n", var->name, var->values[run]);
    }
  }
}

void concretize_random_all(test_ctx_t* ctx, const litmus_test_t* cfg, concretization_st_t* st, int no_runs) {
  for (u64 i = 0; i < ctx->no_runs; i++) {
    concretize_random_one(ctx, cfg, st, i);
  }
}

void concretize_random_finalize(test_ctx_t* ctx, const litmus_test_t* cfg, concretization_st_t* st) {
  for (int varidx = 0; varidx < ctx->cfg->no_heap_vars; varidx++) {
    for (int i = 0; i < NUM_PIN_LEVELS; i++) {
      var_st_t* var_st = &st->var_sts[varidx];
      pin_st_t* pin_st = &var_st->pins[i];
      FREE(pin_st->vars);
    }
  }
  FREE(st);
}