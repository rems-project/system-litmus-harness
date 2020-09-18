#include "lib.h"

void init_test_ctx(test_ctx_t* ctx, const litmus_test_t* cfg, int no_runs) {
  var_info_t* var_infos = ALLOC_MANY(var_info_t, cfg->no_heap_vars);
  uint64_t** out_regs = ALLOC_MANY(uint64_t*, cfg->no_regs);
  bar_t* init_sync_bar = ALLOC_ONE(bar_t);
  /* we don't need millions of barriers, just a handful */
  bar_t* start_run_bars = ALLOC_MANY(bar_t, 512);
  bar_t* bars = ALLOC_MANY(bar_t, 512);
  bar_t* end_bars = ALLOC_MANY(bar_t, 512);
  bar_t* conc_bars = ALLOC_MANY(bar_t, 512);
  bar_t* clean_bars = ALLOC_MANY(bar_t, no_runs);
  bar_t* final_barrier = ALLOC_ONE(bar_t);
  int* shuffled = ALLOC_MANY(int, no_runs);
  int* affinity = ALLOC_MANY(int, NO_CPUS);

  for (int v = 0; v < cfg->no_heap_vars; v++) {
    valloc_memset(&var_infos[v], 0, sizeof(var_info_t));
    var_infos[v].values = ALLOC_MANY(uint64_t*, no_runs);
  }

  read_var_infos(ctx, cfg, var_infos, no_runs);

  for (int r = 0; r < cfg->no_regs; r++) {
    uint64_t* out_reg = ALLOC_MANY(uint64_t, no_runs);
    out_regs[r] = out_reg;
  }

  for (int i = 0; i < no_runs; i++) {
    for (int r = 0; r < cfg->no_regs; r++) {
      out_regs[r][i] = 0;
    }

    clean_bars[i] = (bar_t){0};
    shuffled[i] = i;
  }

  shuffle(shuffled, sizeof(int), no_runs);

  for (int i = 0; i < 512; i++) {
    start_run_bars[i] = (bar_t){0};
    bars[i] = (bar_t){0};
    end_bars[i] = (bar_t){0};
    conc_bars[i] = (bar_t){0};
  }
  *final_barrier = (bar_t){0};
  *init_sync_bar = (bar_t){0};

  for (int i = 0; i < NO_CPUS; i++) {
    affinity[i] = i;
  }

  test_hist_t* hist = alloc(sizeof(test_hist_t) + sizeof(test_result_t) * 200);
  hist->allocated = 0;
  hist->limit = 200;
  test_result_t** lut = ALLOC_MANY(test_result_t*, hist->limit);
  hist->lut = lut;

  for (int t = 0; t < hist->limit; t++) {
    test_result_t* new_res =
        alloc(sizeof(test_result_t) + sizeof(uint64_t) * cfg->no_regs);
    hist->results[t] = new_res;
    lut[t] = NULL;
  }

  ctx->no_runs = no_runs;
  ctx->heap_vars = var_infos;
  ctx->out_regs = out_regs;
  ctx->initial_sync_barrier = init_sync_bar;
  ctx->start_of_run_barriers = start_run_bars;
  ctx->concretize_barriers = conc_bars;
  ctx->start_barriers = bars;
  ctx->end_barriers = end_bars;
  ctx->cleanup_barriers = clean_bars;
  ctx->final_barrier = final_barrier;
  ctx->shuffled_ixs = shuffled;
  ctx->affinity = affinity;
  ctx->last_tick = 0;
  ctx->hist = hist;
  ctx->ptable = NULL;
  ctx->current_run = 0;
  ctx->privileged_harness = 0;
  ctx->cfg = cfg;
  ctx->concretization_st = NULL;
}

uint64_t ctx_pa(test_ctx_t* ctx, uint64_t va) {
  /* return the PA associated with the given va in a particular iteration */
  return (uint64_t)vmm_pa(ctx->ptable, va);
}

uint64_t* ctx_pte(test_ctx_t* ctx, uint64_t va) {
  /* return the VA at which the pte lives for the given va in a particular
   * iteration */
  return vmm_pte(ctx->ptable, va);
}


const char* varname_from_idx(test_ctx_t* ctx, uint64_t idx) {
  for (int i = 0; i < ctx->cfg->no_heap_vars; i++) {
    if (idx == i)
      return ctx->cfg->heap_var_names[i];
  }

  fail("! err: no such variable with index %ld.\n", idx);
  return 0;
}

uint64_t idx_from_varname_infos(const litmus_test_t* cfg, var_info_t* infos, const char* varname) {
  for (int i = 0; i < cfg->no_heap_vars; i++) {
    if (strcmp(infos[i].name, varname)) {
      return i;
    }
  }

  return cfg->no_heap_vars;
}

uint64_t idx_from_varname(test_ctx_t* ctx, const char* varname) {
  for (int i = 0; i < ctx->cfg->no_heap_vars; i++) {
    if (strcmp(ctx->heap_vars[i].name, varname)) {
      return i;
    }
  }

  fail("! err: no such variable \"%s\".\n", varname);
  return 0;
}

const char* regname_from_idx(test_ctx_t* ctx, uint64_t idx) {
  for (int i = 0; i < ctx->cfg->no_regs; i++) {
    if (idx == i)
      return ctx->cfg->reg_names[i];
  }

  fail("! err: no such register with index %ld.\n", idx);
  return 0;
}

uint64_t idx_from_regname(test_ctx_t* ctx, const char* varname) {
  for (int i = 0; i < ctx->cfg->no_regs; i++) {
    if (strcmp(ctx->cfg->reg_names[i], varname)) {
      return i;
    }
  }

  fail("! err: no such register \"%s\".\n", varname);
  return 0;
}

void free_test_ctx(test_ctx_t* ctx) {
  for (int t = 0; t < ctx->hist->limit; t++) {
    free(ctx->hist->results[t]);
  }

  free(ctx->hist->lut);
  free(ctx->hist);

  for (int r = 0; r < ctx->cfg->no_regs; r++) {
    free(ctx->out_regs[r]);
  }

  for (int v = 0; v < ctx->cfg->no_heap_vars; v++) {
    free(ctx->heap_vars[v].values);
  }

  free((int*)ctx->affinity);
  free(ctx->shuffled_ixs);
  free((bar_t*)ctx->final_barrier);
  free((bar_t*)ctx->cleanup_barriers);
  free((bar_t*)ctx->concretize_barriers);
  free((bar_t*)ctx->end_barriers);
  free((bar_t*)ctx->start_barriers);
  free((bar_t*)ctx->start_of_run_barriers);
  free((bar_t*)ctx->initial_sync_barrier);
  free(ctx->out_regs);
  free(ctx->heap_vars);
}