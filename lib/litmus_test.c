#include <stdint.h>

#include "lib.h"

/** lock for critical sections inside harness
 * during litmus test execution
 */
static lock_t __harness_lock;

static void go_cpus(int cpu, void* a);
static void run_thread(test_ctx_t* ctx, int cpu);

static uint64_t PA(test_ctx_t* ctx, uint64_t va) {
  /* return the PA associated with the given va in a particular iteration */
  return (uint64_t)vmm_pa(ctx->ptable, va);
}

static uint64_t* PTE(test_ctx_t* ctx, uint64_t va) {
  /* return the VA at which the PTE lives for the given va in a particular
   * iteration */
  return vmm_pte(ctx->ptable, va);
}

/* entry point */
void run_test(const litmus_test_t* cfg) {
  /* create test context obj */
  test_ctx_t ctx;
  start_of_test(&ctx, cfg, NUMBER_OF_RUNS);

  for (int i = 0; i < cfg->no_init_states; i++) {
    init_varstate_t* var = cfg->init_states[i];
    const char* name = var->varname;
    switch (var->type) {
      case (TYPE_HEAP):
        set_init_heap(&ctx, name, var->value);
        break;
      case (TYPE_PTE):
        set_init_pte(&ctx, name, var->value);
        break;
    }
  }

  /* run it */
  trace("%s\n", "Running Tests ...");
  for(int i = 0; i < 4; i++) {
    printf("P%d\t\t\t", i);
  }
  printf("\n");
  run_on_cpus((async_fn_t*)go_cpus, (void*)&ctx);

  /* clean up and display results */
  end_of_test(&ctx, cfg->reg_names, cfg->interesting_result);
}

uint64_t idx_from_varname(test_ctx_t* ctx, const char* varname) {
  for (int i = 0; i < ctx->cfg->no_heap_vars; i++) {
    if (strcmp(ctx->cfg->heap_var_names[i], varname)) {
      return i;
    }
  }

  printf("! err: no such variable \"%s\".\n", varname);
  abort();
  return 0;
}

const char* varname_from_idx(test_ctx_t* ctx, uint64_t idx) {
  for (int i = 0; i < ctx->cfg->no_heap_vars; i++) {
    if (idx == i)
      return ctx->cfg->heap_var_names[i];
  }

  printf("! err: no such variable with index %ld.\n", idx);
  abort();
  return 0;
}

void set_init_pte(test_ctx_t* ctx, const char* varname, uint64_t desc) {
  uint64_t idx = idx_from_varname(ctx, varname);

  for (uint64_t i = 0; i < ctx->no_runs; i += 4096 / sizeof(uint64_t)) {
    uint64_t va = (uint64_t)&ctx->heap_vars[idx][i];
    uint64_t* pte = (uint64_t*)PTE(ctx, va);
    *pte = desc; /* assuming 1 page per heap var */
  }

  /* no need to flush, assume this call happens before setting new pgtable */
}

void set_init_heap(test_ctx_t* ctx, const char* varname, uint64_t value) {
  uint64_t idx = idx_from_varname(ctx, varname);
  uint64_t va = (uint64_t)&ctx->heap_vars[idx][0];
  for (int i = 0; i < ctx->no_runs; i++) {
    ctx->heap_vars[idx][i] = value;
  }
}

uint64_t ctx_initial_heap_value(test_ctx_t* ctx, uint64_t idx) {
  for (int i = 0; i < ctx->cfg->no_init_states; i++) {
    init_varstate_t* var = ctx->cfg->init_states[i];
    const char* varname = var->varname;
    uint64_t var_idx = idx_from_varname(ctx, varname);
    if (var_idx == idx && var->type == TYPE_HEAP) {
      return var->value;
    }
  }

  /* default: assume 0 */
  return 0;
}

static void go_cpus(int cpu, void* a) {
  test_ctx_t* ctx = (test_ctx_t*)a;
  start_of_thread(ctx, cpu);

  if (cpu < ctx->cfg->no_threads) {
    run_thread(ctx, cpu);
  }

  end_of_thread(ctx, cpu);
}

/* detect a change in the pagetable and if so, fix it */
static void _check_ptes(test_ctx_t* ctx, uint64_t n, uint64_t** vas,
                        uint64_t** ptes, uint64_t* original) {
  int reset = 0;


  for (int i = 0; i < n; i++) {
    if (*ptes[i] != original[i]) {
      *ptes[i] = original[i];
    }
  }

  /* have to flush entire TLB pre v8.4
   * ARMv8.4-TLBI adds range instructions
   * that would allow flushing a whole page
   */
  vmm_flush_tlb();
}

static void run_thread(test_ctx_t* ctx, int cpu) {
  th_f* pre = ctx->cfg->setup_fns == NULL ? NULL : ctx->cfg->setup_fns[cpu];
  th_f* func = ctx->cfg->threads[cpu];
  th_f* post = ctx->cfg->teardown_fns == NULL ? NULL : ctx->cfg->teardown_fns[cpu];

  for (int j = 0; j < ctx->no_runs; j++) {
    int i = ctx->shuffled_ixs[j];
    uint64_t* heaps[ctx->cfg->no_heap_vars];
    uint64_t* ptes[ctx->cfg->no_heap_vars];
    uint64_t pas[ctx->cfg->no_heap_vars];
    uint64_t* regs[ctx->cfg->no_regs];
    uint64_t saved_ptes[ctx->cfg->no_heap_vars];

    for (int v = 0; v < ctx->cfg->no_heap_vars; v++) {
      uint64_t* p = &ctx->heap_vars[v][i];
      heaps[v] = p;
      if (ENABLE_PGTABLE) {
        ptes[v] = PTE(ctx, (uint64_t)p);
        saved_ptes[v] = *ptes[v];
        pas[v] = PA(ctx, (uint64_t)p);
      }
    }
    for (int r = 0; r < ctx->cfg->no_regs; r++) {
      regs[r] = &ctx->out_regs[r][i];
    }

    uint32_t* old_sync_handler_el0;
    uint32_t* old_sync_handler_el1;

    start_of_run(ctx, cpu, i);
    if (pre != NULL)
      pre(ctx, i, (uint64_t**)heaps, (uint64_t**)ptes, (uint64_t*)pas, (uint64_t**)regs);

    if (ctx->cfg->thread_sync_handlers) {
      if (ctx->cfg->thread_sync_handlers[cpu][0] != NULL)
        old_sync_handler_el0 = hotswap_exception(0x400, (uint32_t*)ctx->cfg->thread_sync_handlers[cpu][0]);
      if (ctx->cfg->thread_sync_handlers[cpu][1] != NULL)
        old_sync_handler_el1 = hotswap_exception(0x000, (uint32_t*)ctx->cfg->thread_sync_handlers[cpu][1]);
    }

    /* this barrier must be last thing before running function */
    bwait(cpu, i % ctx->cfg->no_threads, &ctx->start_barriers[i], ctx->cfg->no_threads);
    func(ctx, i, (uint64_t**)heaps, (uint64_t**)ptes, (uint64_t*)pas,
         (uint64_t**)regs);
    if (ctx->cfg->thread_sync_handlers) {
      if (ctx->cfg->thread_sync_handlers[cpu][0] != NULL)
        restore_hotswapped_exception(0x400, old_sync_handler_el0);
      if (ctx->cfg->thread_sync_handlers[cpu][1] != NULL)
        restore_hotswapped_exception(0x000, old_sync_handler_el1);
    }

    if (post != NULL)
      post(ctx, i, (uint64_t**)heaps, (uint64_t**)ptes, (uint64_t*)pas, (uint64_t**)regs);

    end_of_run(ctx, cpu, i);

    if (ENABLE_PGTABLE)
      _check_ptes(ctx, ctx->cfg->no_heap_vars, heaps, ptes, saved_ptes);

    bwait(cpu, i % ctx->cfg->no_threads, &ctx->cleanup_barriers[i], ctx->cfg->no_threads);
  }
}

/* Test Data */

uint64_t read_clk(void) {
  uint64_t clk = read_sysreg(pmccntr_el0);
  return clk;
}

void init_test_ctx(test_ctx_t* ctx, const litmus_test_t* cfg, int no_runs) {
  uint64_t** heap_vars = alloc(sizeof(uint64_t*) * cfg->no_heap_vars);
  uint64_t** out_regs = alloc(sizeof(uint64_t*) * cfg->no_regs);
  bar_t* bars = alloc(sizeof(bar_t) * no_runs);
  bar_t* end_bars = alloc(sizeof(bar_t) * no_runs);
  bar_t* clean_bars = alloc(sizeof(bar_t) * no_runs);
  bar_t* final_barrier = alloc(sizeof(bar_t));
  uint64_t* shuffled = alloc(sizeof(uint64_t) * no_runs);

  for (int v = 0; v < cfg->no_heap_vars; v++) {
    // ensure each heap var allloc'd into its own page...
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

    bars[i] = (bar_t){0};
    end_bars[i] = (bar_t){0};
    clean_bars[i] = (bar_t){0};
    shuffled[i] = i;
  }
  *final_barrier = (bar_t){0};

  /* shuffle shuffled */
  rand_seed(read_clk());
  shuffle(shuffled, no_runs);

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
  ctx->start_barriers = bars;
  ctx->end_barriers = end_bars;
  ctx->cleanup_barriers = clean_bars;
  ctx->final_barrier = final_barrier;
  ctx->shuffled_ixs = shuffled;
  ctx->hist = hist;
  ctx->ptable = NULL;
  ctx->current_run = 0;
  ctx->privileged_harness = 0;
  ctx->cfg = cfg;
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
  free((bar_t*)ctx->start_barriers);
  free((bar_t*)ctx->end_barriers);
  free((bar_t*)ctx->cleanup_barriers);
  free((bar_t*)ctx->final_barrier);
  free(ctx->shuffled_ixs);
}

static int matches(test_result_t* result, test_ctx_t* ctx, int run) {
  for (int reg = 0; reg < ctx->cfg->no_regs; reg++) {
    if (result->values[reg] != ctx->out_regs[reg][run]) {
      return 0;
    }
  }
  return 1;
}

static int ix_from_values(test_ctx_t* ctx, int run) {
  int val = 0;
  for (int reg = 0; reg < ctx->cfg->no_regs; reg++) {
    uint64_t v = ctx->out_regs[reg][run];
    if (v < 4) {
      val *= 4;
      val += (int)(v % 4); /* must be less than 4 so fine ... */
    } else {
      return -1;
    }
  }
  return val;
}

static void add_results(test_hist_t* res, test_ctx_t* ctx, int run) {
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
    for (int reg = 0; reg < ctx->cfg->no_regs; reg++) {
      new_res->values[reg] = ctx->out_regs[reg][run];
    }
    new_res->counter = 1;
    res->allocated++;

    /* update LUT to point if future accesses should be fast */
    if (ix != -1) {
      lut[ix] = new_res;
    }
  }
}

void prefetch(test_ctx_t* ctx, int i) {
  for (int v = 0; v < ctx->cfg->no_heap_vars; v++) {
    /* TODO: read initial state */
    lock(&__harness_lock);
    uint64_t is_valid = vmm_pte_valid(ctx->ptable, &ctx->heap_vars[v][i]);
    unlock(&__harness_lock);
    if (randn() % 2 && is_valid && ctx->heap_vars[v][i] != ctx_initial_heap_value(ctx, v)) {
      raise_to_el1(); /* can abort only at EL1 */
      printf(
          "! fatal: initial state for heap var \"%s\" on run %d was %d not %d",
          varname_from_idx(ctx, v), i, ctx->heap_vars[v][i], ctx_initial_heap_value(ctx, v));
      abort();
    }
  }
}

static void resetsp(void) {
  /* check and reset to SP_EL0 */
  uint64_t currentsp;
  asm volatile("mrs %[currentsp], SPSel\n" : [currentsp] "=r"(currentsp));

  if (currentsp == 0b1) { /* if not already using SP_EL0 */
    asm volatile(
        "mov x18, sp\n"
        "msr spsel, x18\n"
        "dsb nsh\n"
        "isb\n"
        "mov sp, x18\n"
        :
        :
        : "x18");
  }
}

void start_of_run(test_ctx_t* ctx, int thread, int i) {
  /* do not prefetch anymore .. not safe! */
  prefetch(ctx, i);
  if (! ctx->cfg->start_els || ctx->cfg->start_els[thread] == 0)
    drop_to_el0();
}

static void print_single_result(test_ctx_t* ctx, int i) {
  printf("* ");
  for (int r = 0; r < ctx->cfg->no_regs; r++) {
    printf(" %s=%d", ctx->cfg->reg_names[r], ctx->out_regs[r][i]);
  }

  printf(" : 1\n");
}

void end_of_run(test_ctx_t* ctx, int thread, int i) {
  if (! ctx->cfg->start_els || ctx->cfg->start_els[thread] == 0)
    raise_to_el1();
  bwait(thread, i % ctx->cfg->no_threads, &ctx->end_barriers[i], ctx->cfg->no_threads);

  /* only 1 thread should collect the results, else they will be duplicated */
  if (thread == 0) {
    uint64_t r = ctx->current_run++;

    if (! DISABLE_RESULTS_HIST) {
      test_hist_t* res = ctx->hist;
      add_results(res, ctx, i);
    } else {
      print_single_result(ctx, r);
    }

    /* progress indicator */
    uint64_t step = (ctx->no_runs / 10);
    if (r % step == 0) {
      trace("[%d/%d]\n", r, ctx->no_runs);
    } else if (r == ctx->no_runs - 1) {
      trace("[%d/%d]\n", r + 1, ctx->no_runs);
    }
  }
}

void start_of_thread(test_ctx_t* ctx, int cpu) {
  /* turn on MMU and switch to new pagetable */
  if (ENABLE_PGTABLE) {
      vmm_switch_ttable(ctx->ptable);
  }

  /* before can drop to EL0, ensure EL0 has a valid mapped stack space
   */
  resetsp();

  trace("CPU%d: starting test\n", cpu);
}

void end_of_thread(test_ctx_t* ctx, int cpu) {
  if (ENABLE_PGTABLE) {
    /* restore global non-test pgtable */
    vmm_switch_ttable(vmm_pgtable);
  }

  bwait(cpu, 0, ctx->final_barrier, 4);
  trace("CPU%d: end of test\n", cpu);
}

void start_of_test(test_ctx_t* ctx, const litmus_test_t* cfg, int no_runs) {
  trace("====== %s ======\n", cfg->name);

  /* create the dynamic configuration (context) from the static information (cfg) */
  init_test_ctx(ctx, cfg, no_runs);

  if (ENABLE_PGTABLE) {
    ctx->ptable = vmm_alloc_new_idmap_4k();

    /* need to add read/write mappings to the exception vector table */
    for (int i = 0; i < 4; i++) {
      vmm_update_mapping(ctx->ptable, vector_base_addr_rw+i*4096, vector_base_pa+i*4096, 0x1 << 6);
    }
  }
}

void end_of_test(test_ctx_t* ctx, const char** out_reg_names,
                 uint64_t* interesting_result) {
  if (! DISABLE_RESULTS_HIST) {
    trace("%s\n", "Printing Results...");
    print_results(ctx->hist, ctx, out_reg_names, interesting_result);
  }
  trace("Finished test %s\n", ctx->cfg->name);
  free_test_ctx(ctx);

  if (ENABLE_PGTABLE)
    vmm_free_pgtable(ctx->ptable);
}

void print_results(test_hist_t* res, test_ctx_t* ctx,
                   const char** out_reg_names, uint64_t* interesting_results) {
  printf("\n");
  printf("Test %s:\n", ctx->cfg->name);
  int marked = 0;
  for (int r = 0; r < res->allocated; r++) {
    int was_interesting = (interesting_results == NULL) ? 0 : 1;
    for (int reg = 0; reg < ctx->cfg->no_regs; reg++) {
      printf(" %s=%d ", out_reg_names[reg], res->results[r]->values[reg]);

      if (interesting_results != NULL)
        if (res->results[r]->values[reg] != interesting_results[reg])
          was_interesting = 0;
    }

    if (was_interesting) {
      marked = res->results[r]->counter;
      printf(" : %d *\n", res->results[r]->counter);
    } else {
      printf(" : %d\n", res->results[r]->counter);
    }
  }
  printf("Observation %s: %d (of %d)\n", ctx->cfg->name, marked, ctx->no_runs);
}
