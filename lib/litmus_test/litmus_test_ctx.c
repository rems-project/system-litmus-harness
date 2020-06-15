#include "lib.h"

void init_test_ctx(test_ctx_t* ctx, const litmus_test_t* cfg, int no_runs) {
  uint64_t** heap_vars = alloc(sizeof(uint64_t*) * cfg->no_heap_vars);
  uint64_t** out_regs = alloc(sizeof(uint64_t*) * cfg->no_regs);
  bar_t* init_sync_bar = alloc(sizeof(bar_t));
  bar_t* start_run_bars = alloc(sizeof(bar_t) * no_runs);
  bar_t* bars = alloc(sizeof(bar_t) * no_runs);
  bar_t* end_bars = alloc(sizeof(bar_t) * no_runs);
  bar_t* clean_bars = alloc(sizeof(bar_t) * no_runs);
  bar_t* final_barrier = alloc(sizeof(bar_t));
  uint64_t* shuffled = alloc(sizeof(uint64_t) * no_runs);
  int* affinity = alloc(sizeof(int)*NO_CPUS);

  for (int v = 0; v < cfg->no_heap_vars; v++) {
    // ensure each heap var alloc'd into its own page...
    uint64_t* heap_var =
        alloc_with_alignment(ALIGN_UP(sizeof(uint64_t) * no_runs, 12), 4096UL);
    heap_vars[v] = heap_var;
  }

  for (int r = 0; r < cfg->no_regs; r++) {
    uint64_t* out_reg = alloc(sizeof(uint64_t) * no_runs);
    out_regs[r] = out_reg;
  }

  for (int i = 0; i < no_runs; i++) {
    /* one time init so column major doesnt matter */
    for (int v = 0; v < cfg->no_heap_vars; v++) heap_vars[v][i] = 0;

    for (int r = 0; r < cfg->no_regs; r++) out_regs[r][i] = 0;

    start_run_bars[i] = (bar_t){0};
    bars[i] = (bar_t){0};
    end_bars[i] = (bar_t){0};
    clean_bars[i] = (bar_t){0};
    shuffled[i] = i;
  }
  *final_barrier = (bar_t){0};
  *init_sync_bar = (bar_t){0};
  shuffle(shuffled, sizeof(uint64_t), no_runs);

  for (int i = 0; i < NO_CPUS; i++) {
    affinity[i] = i;
  }

  test_hist_t* hist = alloc(sizeof(test_hist_t) + sizeof(test_result_t) * 200);
  hist->allocated = 0;
  hist->limit = 200;
  test_result_t** lut = alloc(sizeof(test_result_t*) * hist->limit);
  hist->lut = lut;

  for (int t = 0; t < hist->limit; t++) {
    test_result_t* new_res =
        alloc(sizeof(test_result_t) + sizeof(uint64_t) * cfg->no_regs);
    hist->results[t] = new_res;
    lut[t] = NULL;
  }

  ctx->no_runs = no_runs;
  ctx->heap_vars = heap_vars;
  ctx->out_regs = out_regs;
  ctx->initial_sync_barrier = init_sync_bar;
  ctx->start_of_run_barriers = start_run_bars;
  ctx->start_barriers = bars;
  ctx->end_barriers = end_bars;
  ctx->cleanup_barriers = clean_bars;
  ctx->final_barrier = final_barrier;
  ctx->shuffled_ixs = shuffled;
  ctx->affinity = affinity;
  ctx->hist = hist;
  ctx->ptable = NULL;
  ctx->current_run = 0;
  ctx->privileged_harness = 0;
  ctx->cfg = cfg;
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


uint64_t idx_from_varname(test_ctx_t* ctx, const char* varname) {
  for (int i = 0; i < ctx->cfg->no_heap_vars; i++) {
    if (strcmp(ctx->cfg->heap_var_names[i], varname)) {
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


void set_init_pte(test_ctx_t* ctx, const char* varname, uint64_t desc) {
  uint64_t idx = idx_from_varname(ctx, varname);

  for (uint64_t i = 0; i < ctx->no_runs; i += 4096 / sizeof(uint64_t)) {
    uint64_t va = (uint64_t)&ctx->heap_vars[idx][i];
    uint64_t* pte = ctx_pte(ctx, va);
    *pte = desc; /* assuming 1 page per heap var */
  }

  /* no need to flush, assume this call happens before setting new pgtable */
}

void set_init_heap(test_ctx_t* ctx, const char* varname, uint64_t value) {
  uint64_t idx = idx_from_varname(ctx, varname);
  for (int i = 0; i < ctx->no_runs; i++) {
    ctx->heap_vars[idx][i] = value;
  }
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
    free(ctx->heap_vars[v]);
  }

  free(ctx->heap_vars);
  free(ctx->out_regs);
  free((bar_t*)ctx->initial_sync_barrier);
  free((bar_t*)ctx->start_of_run_barriers);
  free((bar_t*)ctx->start_barriers);
  free((bar_t*)ctx->end_barriers);
  free((bar_t*)ctx->cleanup_barriers);
  free((bar_t*)ctx->final_barrier);
  free(ctx->shuffled_ixs);
  free((int*)ctx->affinity);
}