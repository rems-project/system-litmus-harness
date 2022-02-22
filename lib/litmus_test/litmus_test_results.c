#include "lib.h"

static int matches(test_result_t* result, test_ctx_t* ctx, run_idx_t run) {
  for (reg_idx_t reg = 0; reg < ctx->cfg->no_regs; reg++) {
    if (result->values[reg] != ctx->out_regs[reg][run]) {
      return 0;
    }
  }
  return 1;
}


static int matches_interesting(test_hist_t* res, test_ctx_t* ctx, test_result_t* r) {
  u64 no_interesting = ctx->cfg->no_interesting_results;

  u64** relaxed = ctx->cfg->interesting_results;
  u64* _spare[1] = { ctx->cfg->interesting_result };
  if (relaxed == NULL) {
    relaxed = &_spare[0];
    if (ctx->cfg->interesting_result != NULL) {
      no_interesting = 1;
    }
  }

  for (int i = 0; i < no_interesting; i++) {
    u64* regs = relaxed[i];
    int was_interesting = 1;

     for (int reg = 0; reg < ctx->cfg->no_regs; reg++) {
      if (relaxed != NULL)
        if (r->values[reg] != regs[reg])
          was_interesting = 0;
    }

    if (was_interesting)
      return 1;
  }

  return 0;
}

static int ix_from_values(test_ctx_t* ctx, run_idx_t run) {
  int val = 0;
  for (reg_idx_t reg = 0; reg < ctx->cfg->no_regs; reg++) {
    u64 v = ctx->out_regs[reg][run];
    if (v < 4) {
      val *= 4;
      val += (int)(v % 4); /* must be less than 4 so fine ... */
    } else {
      return -1;
    }
  }
  return val;
}


static void add_results(test_hist_t* res, test_ctx_t* ctx, run_idx_t run) {
  /* fast case: check lut */
  test_result_t** lut = res->lut;
  int ix = ix_from_values(ctx, run);

  if (ix != -1 && lut[ix] != NULL) {
    lut[ix]->counter++;
    return;
  }

  /* otherwise, slow case: walk table for entry */
  /* if already allocated */
  int missing = 1;
  for (int i = 0; i < res->allocated; i++) {
    /* found a matching entry */
    if (matches(res->results[i], ctx, run)) {
      /* just increment its count */
      missing = 0;
      res->results[i]->counter++;
      break;
    }
  }

  /* if not found, insert it */
  if (missing) {
    if (res->allocated >= res->limit) {
      raise_to_el1(); /* can only abort at EL1 */
      printf("! fatal:  overallocated results\n");
      printf("this probably means the test had too many outcomes\n");
      abort();
    }
    test_result_t* new_res = res->results[res->allocated];

    for (reg_idx_t reg = 0; reg < ctx->cfg->no_regs; reg++) {
      new_res->values[reg] = ctx->out_regs[reg][run];
    }
    new_res->counter = 1;
    new_res->is_relaxed = matches_interesting(res, ctx, new_res);
    res->allocated++;

    /* update LUT to point if future accesses should be fast */
    if (ix != -1) {
      lut[ix] = new_res;
    }
  }
}


static void print_single_result(test_ctx_t* ctx, run_count_t i) {
  printf("* ");
  for (reg_idx_t r = 0; r < ctx->cfg->no_regs; r++) {
    printf(" %s=%d", ctx->cfg->reg_names[r], ctx->out_regs[r][i]);
  }

  printf(" : 1\n");
}


/** store or print the result from the previous run
 */
void handle_new_result(test_ctx_t* ctx, run_idx_t idx, run_count_t r) {
    if (ENABLE_RESULTS_HIST) {
      test_hist_t* res = ctx->hist;
      add_results(res, ctx, idx);
    } else {
      /* TODO: why does this use a run_count_t rather than the run_idx_t ? */
      print_single_result(ctx, r);
    }
}


/** at the end of the test print out the results histogram
 */
void print_results(test_hist_t* res, test_ctx_t* ctx) {
  int marked = 0;
  int no_sc_results_seen = 0;
  for (int r = 0; r < res->allocated; r++) {
    int was_interesting = res->results[r]->is_relaxed;

    if (ENABLE_RESULTS_OUTREG_PRINT) {
      for (reg_idx_t reg = 0; reg < ctx->cfg->no_regs; reg++) {
        printf(" %s=%d ", ctx->cfg->reg_names[reg], res->results[r]->values[reg]);
      }
    }

    if (was_interesting) {
      marked += res->results[r]->counter;
      if (ENABLE_RESULTS_OUTREG_PRINT)
        printf(" : %d *\n", res->results[r]->counter);
    } else {
      no_sc_results_seen++;
      if (ENABLE_RESULTS_OUTREG_PRINT)
        printf(" : %d\n", res->results[r]->counter);
    }
  }
  printf("Observation %s: %d (of %d)\n", ctx->cfg->name, marked, ctx->no_runs);
  if (ctx->cfg->no_sc_results > 0 && no_sc_results_seen != ctx->cfg->no_sc_results && ENABLE_RESULTS_MISSING_SC_WARNING) {
    printf("Warning on %s: saw %d SC results but expected %d\n", ctx->cfg->name, no_sc_results_seen, ctx->cfg->no_sc_results);
  }
}