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
      fail(
        "overallocated results\n"
         "this probably means the test had too many outcomes\n"
      );
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

static void print_test_header(const litmus_test_t* cfg) {
  static bool first_result = true;

  if (! first_result)
    printf("\n");
  else
    first_result = false;

  switch (OUTPUT_FORMAT) {
  case STYLE_HERDTOOLS:
    printf("Test %s Allowed\n", cfg->name);
    break;
  case STYLE_ORIGINAL:
    printf("Test %s:\n", cfg->name);
    break;
  default:
    unreachable();
  }
}

static void print_hash(const litmus_test_t* test) {
  digest d = litmus_test_hash(test);
  char computed_hash[41];
  hexdigest(&d, computed_hash);

  if (test->hash) {
    printf("Hash=%s\n", test->hash);

    if (!strcmp(test->hash, computed_hash)) {
      warning(WARN_HASH_MISMATCH, "computed hash %s did not match hash in test file\n", computed_hash);
    }
  } else {
    printf("Hash=%s\n", computed_hash);
  }
}

/** at the end of the test print out the results histogram
 */
static void print_results_original(test_hist_t* res, test_ctx_t* ctx) {
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
  print_hash(ctx->cfg);
  printf("Observation %s: %d (of %d)\n", ctx->cfg->name, marked, ctx->no_runs);
  if (ctx->cfg->no_sc_results > 0 && no_sc_results_seen != ctx->cfg->no_sc_results && ENABLE_RESULTS_MISSING_SC_WARNING) {
    warning(
      WARN_MISSING_SC_RESULTS,
      "on %s: saw %d SC results but expected %d\n", ctx->cfg->name, no_sc_results_seen, ctx->cfg->no_sc_results
    );
  }
}

static void print_results_herd(test_hist_t* res, test_ctx_t* ctx) {
  u64 marked = 0;
  u64 no_sc_results_seen = 0;
  u64 total_count = 0;

  print_test_header(ctx->cfg);
  printf("Histogram (%d states)\n", res->allocated);

  for (int r = 0; r < res->allocated; r++) {
    int was_interesting = res->results[r]->is_relaxed;
    total_count += res->results[r]->counter;

    char line[1024];
    STREAM* buf = NEW_BUFFER(line, 1024);

    if (ENABLE_RESULTS_OUTREG_PRINT) {
      char marker = was_interesting ? '*' : ':';
      sprintf(buf, "%ld%c>", res->results[r]->counter, marker);
      for (reg_idx_t reg = 0; reg < ctx->cfg->no_regs; reg++) {
        sprint_reg(buf, ctx->cfg->reg_names[reg], STYLE_HERDTOOLS);
        sprintf(buf, "=%d;", res->results[r]->values[reg]);
      }
      printf("%s\n", line);
    }

    if (was_interesting) {
      marked += res->results[r]->counter;
    } else {
      no_sc_results_seen++;
    }
  }

  if (marked)
    printf("Ok\n");
  else
    printf("No\n");

  printf("Witnesses\n");
  printf("Positive: %ld Negative: %ld\n", marked, total_count - marked);
  print_hash(ctx->cfg);
  printf("Observation %s ", ctx->cfg->name);
  if (marked == 0) {
    printf("Never");
  } else if (marked == total_count) {
    printf("Always");
  } else {
    printf("Sometimes");
  }
  printf(" %ld %ld\n", marked, total_count - marked);

  char time_str[100];
  sprint_time(NEW_BUFFER(time_str, 100), ctx->end_clock - ctx->start_clock, SPRINT_TIME_SSDOTMS);
  printf("Time %s %s\n", ctx->cfg->name, time_str);

  if (ctx->cfg->no_sc_results > 0 && no_sc_results_seen != ctx->cfg->no_sc_results && ENABLE_RESULTS_MISSING_SC_WARNING) {
    warning(
      WARN_MISSING_SC_RESULTS,
      "on %s: saw %d SC results but expected %d\n", ctx->cfg->name, no_sc_results_seen, ctx->cfg->no_sc_results
    );
  }
}

void print_results(test_hist_t* res, test_ctx_t* ctx) {
  switch (OUTPUT_FORMAT) {
  case STYLE_HERDTOOLS:
    print_results_herd(res, ctx);
    break;
  case STYLE_ORIGINAL:
    print_results_original(res, ctx);
    break;
  default:
    unreachable();
  }
}