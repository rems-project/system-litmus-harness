#include "lib.h"

#define FOREACH_HEAP_VAR(ctx, el) \
    el = &ctx->heap_vars[0]; \
    for (int __v##__COUNTER__ = 0; __v##__COUNTER__ < ctx->cfg->no_heap_vars; __v##__COUNTER__++, el++)

/* asks if var_info_t* v owns a own_level_t r */
#define OWNS_REGION(v, r) ((v)->init_owns_region && (v)->init_owned_region_size == (r))

/* is v1 pinned to v2 ? */
#define PINNED_TO(ctx, v1, v2) (idx_from_varname((ctx), (v1)->pin_region_var) == (v2)->varidx)

int __count_fit(uint64_t x, uint64_t bound) {
  int count = 0;

  while (bound >= x) {
    bound -= x;
  }

  return count;
}

/* how many x fit into bound ? */
#define COUNT_FIT(x, bound) __count_fit((uint64_t)x, (uint64_t)bound)

typedef struct {
  int no_pins;
  var_info_t** vars;
} pin_st_t;

typedef struct {
  pin_st_t pins[NUM_PIN_LEVELS];
} var_st_t;

typedef struct {
  var_st_t* var_sts;
  uint64_t* offsets;
} concretization_st_t ;

concretization_st_t* init_st(test_ctx_t* ctx) {
  var_st_t* var_sts = ALLOC_MANY(var_st_t, NUM_PIN_LEVELS);
  for (int varidx = 0; varidx < ctx->cfg->no_heap_vars; varidx++) {
    for (int i = 0; i < NUM_PIN_LEVELS; i++) {
      var_st_t* var_st = &var_sts[varidx];
      pin_st_t* pin_st = &var_st->pins[i];
      pin_st->no_pins = 0;
      pin_st->vars = ALLOC_MANY(var_info_t*, ctx->cfg->no_heap_vars);
    }
  }

  concretization_st_t* st = ALLOC_ONE(concretization_st_t);
  st->offsets = ALLOC_MANY(uint64_t, ctx->cfg->no_heap_vars);
  st->var_sts = var_sts;
  return st;
}

void free_st(test_ctx_t* ctx, concretization_st_t* st) {
  for (int varidx = 0; varidx < ctx->cfg->no_heap_vars; varidx++) {
    for (int i = 0; i < NUM_PIN_LEVELS; i++) {
      var_st_t* var_st = &st->var_sts[varidx];
      pin_st_t* pin_st = &var_st->pins[i];
      free(pin_st->vars);
    }
  }
  free(st->var_sts);
  free(st->offsets);
  free(st);
}

static int count_pinned_to(var_info_t** out_vinfos, test_ctx_t* ctx, var_info_t* var, pin_level_t lvl) {
  var_info_t* othervar;
  int count = 0;

  FOREACH_HEAP_VAR(ctx, othervar) {
    if ( (var->varidx != othervar->varidx)
         && othervar->init_region_pinned
         && idx_from_varname(ctx, othervar->pin_region_var) == var->varidx
         && othervar->pin_region_level == lvl
    ) {
      out_vinfos[count] = othervar;
      count += 1;
    }
  }

  return count;
}

/** place each VA in its own region
 * in-order,  allocating all the memory that var needs before continuing
 */

static uint64_t calculate_offsets(test_ctx_t* ctx, const litmus_test_t* cfg, concretization_st_t* st, var_info_t* rootvar, uint64_t start) {
  /* get offsets from root */
  uint64_t level_sizes[] = {
    8,
    CACHE_LINE_SIZE,
    PAGE_SIZE,
    PMD_SIZE,
    PUD_SIZE,
    0
  };

  uint64_t root_size = rootvar->init_owns_region ? level_sizes[rootvar->init_owned_region_size] : PAGE_SIZE;

  uint64_t no_levels[] = {
    0, /* same loc */
    0, /* same cache line */
    0, /* same page */
    0, /* same dir */
    0, /* same pud */
    0, /* same pgd */
  };

  st->offsets[rootvar->varidx] = start;

  for (pin_level_t lvl = REGION_SAME_CACHE_LINE; lvl <= REGION_SAME_PGD; lvl++) {
    debug("lvl = %d\n", lvl);
    pin_st_t* pin_st = &st->var_sts[rootvar->varidx].pins[lvl];
    for (int pinidx = 0; pinidx < pin_st->no_pins; pinidx++) {
      debug("pinidx %d\n", pinidx);
      var_info_t* pin_var = pin_st->vars[pinidx];
      debug(" = %s\n", pin_var->name);
      st->offsets[pin_var->varidx] = start + (++no_levels[lvl - 1])*level_sizes[lvl - 1];
    }
  }

  return start + root_size;
}

static uint64_t __region_shift(pin_level_t lvl) {
  switch (lvl) {
    case REGION_SAME_CACHE_LINE:
      return CACHE_LINE_SHIFT;
    case REGION_SAME_PAGE:
      return PAGE_SHIFT;
    case REGION_SAME_PMD:
      return PMD_SHIFT;
    case REGION_SAME_PUD:
      return PUD_SHIFT;
    default:
      return 0;
  }
}

static uint64_t __region(uint64_t* va, pin_level_t lvl) {
  uint64_t n = (uint64_t )va;
  return n >> __region_shift(lvl);
}

static uint8_t __same_region(uint64_t* va1, uint64_t* va2, pin_level_t lvl) {
  return __region(va1, lvl) == __region(va2, lvl);
}

static uint64_t __aligned(test_ctx_t* ctx,  const litmus_test_t* cfg, concretization_st_t* st, var_info_t* rootvar, int run) {
  /* check all the things pinned to var are in the same region */
  uint64_t diff = 0;
  for (pin_level_t lvl = REGION_SAME_CACHE_LINE; lvl <= REGION_SAME_PGD; lvl++) {
    pin_st_t* pin_st = &st->var_sts[rootvar->varidx].pins[lvl];
    for (int pinidx = 0; pinidx < pin_st->no_pins; pinidx++) {
      var_info_t* pin_var = pin_st->vars[pinidx];
      uint64_t* rootva = rootvar->values[run];
      uint64_t* pinva = pin_var->values[run];

      if (! __same_region(rootva, pinva, lvl)) {
        diff = MAX(
          diff,
          MIN((1 << __region_shift(lvl)) - ((uint64_t)rootva & BITMASK(__region_shift(lvl))),
              (1 << __region_shift(lvl)) - ((uint64_t)pinva  & BITMASK(__region_shift(lvl))))
        );
      }
    }
  }

  if (diff != 0) {
    return diff;
  }

  return 0;
}

void concretize_linear_all(test_ctx_t* ctx, const litmus_test_t* cfg, int no_runs) {
  concretization_st_t* st = init_st(ctx);
  valloc_memset(ctx->heap_memory, 0, sizeof(regions_t));

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

  /* calculate offsets */
  uint64_t offs = 0;
  FOREACH_HEAP_VAR(ctx, var) {
    if (var->init_owns_region) {
      offs = calculate_offsets(ctx, cfg, st, var, offs);
    } else if (! var->init_region_pinned) {
      offs = calculate_offsets(ctx, cfg, st, var, PAGE_SIZE+offs);
    }
  }

  debug("calcd offsets\n");
  FOREACH_HEAP_VAR(ctx, var) {
    debug("offset[%s] = %d\n", var->name, st->offsets[var->varidx]);
  }

  /* walk over memory */
  uint64_t start = (uint64_t )ctx->heap_memory;
  for (int i = 0; i < ctx->no_runs; i++) {
    start += 8;

    while (1) {
      FOREACH_HEAP_VAR(ctx, var) {
        uint64_t* va = (uint64_t* )(start + st->offsets[var->varidx]);

        if (*va == 1) {
          goto try_again;
        }

        var->values[i] = va;
      }

      /* check alignments */
      FOREACH_HEAP_VAR(ctx, var) {
        uint64_t diff = __aligned(ctx, cfg, st, var, i);
        if (diff > 0) {
          start += diff - 8;
          debug("var (%s) unaligned \n", var->name);
          goto try_again;
        }
      }

      /* mark */
      FOREACH_HEAP_VAR(ctx, var) {
        *var->values[i] = 1;
      }

      break;
try_again:
      start += 8;
    }
  }


  free_st(ctx, st);
}
