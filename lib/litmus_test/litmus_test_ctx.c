#include "lib.h"

static void sanity_check_test(const litmus_test_t* cfg, int no_runs, int runs_in_batch) {
  /* we have 1+MAX_ASID ASIDs */
  if ((cfg->requires & REQUIRES_PGTABLE) && runs_in_batch > 1 + MAX_ASID)
    fail("cannot have more than the number of possible ASIDs (%ld) as runs in a batch with --pgtable.\n", 1 + MAX_ASID);
}

void init_test_ctx(test_ctx_t* ctx, const litmus_test_t* cfg, int no_runs, int runs_in_batch) {
  sanity_check_test(cfg, no_runs, runs_in_batch);

  ctx->start_clock = read_clk();

  var_info_t* var_infos = ALLOC_MANY(var_info_t, cfg->no_heap_vars);
  u64** out_regs = ALLOC_MANY(u64*, cfg->no_regs);
  init_system_state_t* sys_st = ALLOC_ONE(init_system_state_t);
  bar_t* generic_cpu_bar = ALLOC_ONE(bar_t);
  bar_t* generic_vcpu_bar = ALLOC_ONE(bar_t);
  /* TODO: instead of asids/runs_in_batch everywhere, have proper batch type
   */
  bar_t* bars = ALLOC_MANY(bar_t, runs_in_batch);
  run_idx_t* shuffled = ALLOC_MANY(run_idx_t, no_runs);
  run_count_t* rev_lookup = ALLOC_MANY(run_count_t, no_runs);
  int* affinity = ALLOC_MANY(int, NO_CPUS);
  u64** ptables = ALLOC_MANY(u64*, 1 + runs_in_batch);

  for (int v = 0; v < cfg->no_heap_vars; v++) {
    var_infos[v].values = ALLOC_MANY(u64*, no_runs);
  }

  sys_st->enable_mair = 0;
  read_var_infos(cfg, sys_st, var_infos, no_runs);

  debug("thread_sync_handlers = {\n");
  if (cfg->thread_sync_handlers != NULL) {
    for (int i = 0; i < cfg->no_threads; i++) {
      debug(" [%d] = {%p, %p}\n", i, cfg->thread_sync_handlers[i][0], cfg->thread_sync_handlers[i][1]);
    }
  }
  debug("}\n");

  for (reg_idx_t r = 0; r < cfg->no_regs; r++) {
    u64* out_reg = ALLOC_MANY(u64, no_runs);
    out_regs[r] = out_reg;
  }

  for (run_idx_t i = 0; i < no_runs; i++) {
    for (reg_idx_t r = 0; r < cfg->no_regs; r++) {
      out_regs[r][i] = 0;
    }

    shuffled[i] = i;
  }

  shuffle(shuffled, sizeof(run_idx_t), no_runs);
  for (run_count_t i = 0; i < no_runs; i++) {
    rev_lookup[shuffled[i]] = i;
  }

  for (int i = 0; i < runs_in_batch; i++) {
    bars[i] = EMPTY_BAR;
  }
  *generic_cpu_bar = EMPTY_BAR;
  *generic_vcpu_bar = EMPTY_BAR;

  for (int i = 0; i < NO_CPUS; i++) {
    affinity[i] = i;
  }

  test_hist_t* hist = ALLOC_SIZED(sizeof(test_hist_t) + sizeof(test_result_t) * 200);
  hist->allocated = 0;
  hist->limit = 200;
  test_result_t** lut = ALLOC_MANY(test_result_t*, hist->limit);
  hist->lut = lut;

  for (int t = 0; t < hist->limit; t++) {
    test_result_t* new_res = ALLOC_SIZED(sizeof(test_result_t) + sizeof(u64) * cfg->no_regs);
    hist->results[t] = new_res;
    lut[t] = NULL;
  }

  ctx->no_runs = no_runs;
  ctx->heap_vars = var_infos;
  ctx->system_state = sys_st;
  ctx->out_regs = out_regs;
  ctx->start_barriers = bars;
  ctx->generic_cpu_barrier = generic_cpu_bar;
  ctx->generic_vcpu_barrier = generic_vcpu_bar;
  ctx->shuffled_ixs = shuffled;
  ctx->shuffled_ixs_inverse = rev_lookup;
  ctx->affinity = affinity;
  ctx->batch_size = runs_in_batch;
  ctx->last_tick = 0;
  ctx->hist = hist;
  ctx->ptables = ptables;
  ctx->current_run = 0;
  ctx->privileged_harness = 0;
  ctx->cfg = cfg;
  ctx->concretization_st = NULL;

  debug("initialized test ctx @ %p\n", ctx);
  DEBUG(DEBUG_ALLOCS, "now using %ld/%ld alloc chunks\n", valloc_alloclist_count_chunks(), NUM_ALLOC_CHUNKS);
}

u64 ctx_pa(test_ctx_t* ctx, run_idx_t run, u64 va) {
  /* return the PA associated with the given va in a particular iteration */
  return (u64)vmm_pa(ptable_from_run(ctx, run), va);
}

u64* ctx_pte(test_ctx_t* ctx, run_idx_t run, u64 va) {
  /* return the VA at which the pte lives for the given va in a particular
   * iteration */
  return vmm_pte(ptable_from_run(ctx, run), va);
}

const char* varname_from_idx(test_ctx_t* ctx, var_idx_t idx) {
  for (var_idx_t i = 0; i < ctx->cfg->no_heap_vars; i++) {
    if (idx == i)
      return ctx->cfg->heap_var_names[i];
  }

  fail("! err: no such variable with index %ld.\n", idx);
  return 0;
}

var_idx_t idx_from_varname_infos(const litmus_test_t* cfg, var_info_t* infos, const char* varname) {
  for (var_idx_t i = 0; i < cfg->no_heap_vars; i++) {
    if (strcmp(infos[i].name, varname)) {
      return i;
    }
  }

  return cfg->no_heap_vars;
}

var_idx_t idx_from_varname(test_ctx_t* ctx, const char* varname) {
  for (var_idx_t i = 0; i < ctx->cfg->no_heap_vars; i++) {
    if (strcmp(ctx->heap_vars[i].name, varname)) {
      return i;
    }
  }

  fail("! err: no such variable \"%s\".\n", varname);
  return 0;
}

const char* regname_from_idx(test_ctx_t* ctx, var_idx_t idx) {
  for (int i = 0; i < ctx->cfg->no_regs; i++) {
    if (idx == i)
      return ctx->cfg->reg_names[i];
  }

  fail("! err: no such register with index %ld.\n", idx);
  return 0;
}

reg_idx_t idx_from_regname(test_ctx_t* ctx, const char* varname) {
  for (reg_idx_t i = 0; i < ctx->cfg->no_regs; i++) {
    if (strcmp(ctx->cfg->reg_names[i], varname)) {
      return i;
    }
  }

  fail("! err: no such register \"%s\".\n", varname);
  return 0;
}

run_count_t run_count_from_idx(test_ctx_t* ctx, run_idx_t idx) {
  switch (LITMUS_SHUFFLE_TYPE) {
    case SHUF_NONE:
      return (run_count_t)idx;
    case SHUF_RAND:
      return ctx->shuffled_ixs_inverse[idx];
    default:
      fail("! unknown LITMUS_SHUFFLE_TYPE: %d/%s\n", LITMUS_SHUFFLE_TYPE, shuff_type_to_str(LITMUS_SHUFFLE_TYPE));
      return 0;
    }
}

u64* ctx_heap_var_va(test_ctx_t* ctx, u64 varidx, run_idx_t i) {
  return ctx->heap_vars[varidx].values[i];
}

u64 ctx_initial_heap_value(test_ctx_t* ctx, run_idx_t idx) {
  var_info_t* var = &ctx->heap_vars[idx];
  fail_on(!is_backed_var(var), "cannot get initial heap value from unbacked var \"s\"\n", var->name);
  return var_backing(var)->val;
}

u64 asid_from_run_count(test_ctx_t* ctx, run_count_t r) {
  /* reserve ASID 0 for harness */
  if (ctx->cfg->requires & REQUIRES_PGTABLE)
    return 1 + (r % ctx->batch_size);
  else
    /* if pgtables are enabled, but the test does not require editing them
     * allocate one test pagetable as ASID #1 */
    return 1;
}

u64* ptable_from_run(test_ctx_t* ctx, run_idx_t i) {
  run_count_t r = run_count_from_idx(ctx, i);
  u64 asid = asid_from_run_count(ctx, r);
  return ctx->ptables[asid];
}

void free_test_ctx(test_ctx_t* ctx) {
  for (int t = 0; t < ctx->hist->limit; t++) {
    FREE(ctx->hist->results[t]);
  }

  FREE(ctx->hist->lut);
  FREE(ctx->hist);

  for (int r = 0; r < ctx->cfg->no_regs; r++) {
    FREE(ctx->out_regs[r]);
  }

  for (int v = 0; v < ctx->cfg->no_heap_vars; v++) {
    FREE(ctx->heap_vars[v].values);
  }

  FREE((int*)ctx->affinity);
  FREE(ctx->shuffled_ixs_inverse);
  FREE(ctx->shuffled_ixs);
  FREE((bar_t*)ctx->start_barriers);
  FREE((bar_t*)ctx->generic_cpu_barrier);
  FREE((bar_t*)ctx->generic_vcpu_barrier);
  FREE(ctx->ptables);
  FREE(ctx->out_regs);
  FREE(ctx->system_state);
  FREE(ctx->heap_vars);
}