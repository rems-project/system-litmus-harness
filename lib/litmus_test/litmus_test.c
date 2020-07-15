#include <stdint.h>

#include "lib.h"

/** lock for critical sections inside harness
 * during litmus test execution
 */
static lock_t __harness_lock;

static void go_cpus(int cpu, void* a);
static void run_thread(test_ctx_t* ctx, int cpu);

/* entry point */
void run_test(const litmus_test_t* cfg) {
  /* create test context obj */
  test_ctx_t ctx;

  /* use same seed for each test
   * this means we can re-run just 1 test from whole batch
   * and still get determinism (up to relaxation) */
  reset_seed();

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
      case (TYPE_ALIAS):
        set_init_alias(&ctx, name, var->aliasname);
        break;
      case (TYPE_AP):
        set_init_ap(&ctx, name, var->value);
        break;
    }
  }

  /* run it */
  printf("\n");
  printf("Test %s:\n", ctx.cfg->name);
  trace("%s\n", "Running Tests ...");
  if (TRACE) {
    for(int i = 0; i < 4; i++) {
      printf("P%d\t\t\t", i);
    }
    printf("\n");
  }
  run_on_cpus((async_fn_t*)go_cpus, (void*)&ctx);

  /* clean up and display results */
  end_of_test(&ctx);
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
  run_thread(ctx, cpu);
  end_of_thread(ctx, cpu);
}

/** reset the pagtable back to the state it was before the test
 */
static void _check_ptes(test_ctx_t* ctx, uint64_t n, uint64_t** vas,
                        uint64_t** ptes, uint64_t* original) {
  for (int i = 0; i < n; i++) {
    *ptes[i] = original[i];
  }

  vmm_flush_tlb();
}

/** run the tests in a loop
 */
static void run_thread(test_ctx_t* ctx, int cpu) {
  for (int j = 0; j < ctx->no_runs; j++) {
    int i = ctx->shuffled_ixs[j];
    int vcpu = ctx->affinity[cpu];
    set_vcpu(vcpu);

    uint64_t* heaps[ctx->cfg->no_heap_vars];
    uint64_t* ptes[ctx->cfg->no_heap_vars];
    uint64_t pas[ctx->cfg->no_heap_vars];
    uint64_t* regs[ctx->cfg->no_regs];
    uint64_t descs[ctx->cfg->no_heap_vars];
    uint64_t saved_ptes[ctx->cfg->no_heap_vars];

    /* since some vCPUs will skip over the tests
     * it's possible for the test to finish before they get their affinity assignment
     * but thinks it's for the *old* run
     *
     * this bwait ensures that does not happen and that all affinity assignments are per-run
     */
    bwait(cpu, 0, &ctx->start_of_run_barriers[i], NO_CPUS);

    if (vcpu >= ctx->cfg->no_threads) {
      goto run_thread_after_execution;
    }

    for (int v = 0; v < ctx->cfg->no_heap_vars; v++) {
      uint64_t* p = &ctx->heap_vars[v][i];
      heaps[v] = p;
      if (ENABLE_PGTABLE) {
        ptes[v] = ctx_pte(ctx, (uint64_t)p);
        descs[v] = *ptes[v];
        saved_ptes[v] = descs[v];
        pas[v] = ctx_pa(ctx, (uint64_t)p);
      }
    }

    for (int r = 0; r < ctx->cfg->no_regs; r++) {
      regs[r] = &ctx->out_regs[r][i];
    }

    uint32_t* old_sync_handler_el0 = NULL;
    uint32_t* old_sync_handler_el1 = NULL;
    uint32_t* old_sync_handler_el1_spx = NULL;

    litmus_test_run run = {
      .ctx = ctx,
      .i = (uint64_t)i,
      .va = heaps,
      .pte = ptes,
      .pa = pas,
      .out_reg = regs,
      .desc = descs,
    };

    th_f* pre = ctx->cfg->setup_fns == NULL ? NULL : ctx->cfg->setup_fns[vcpu];
    th_f* func = ctx->cfg->threads[vcpu];
    th_f* post = ctx->cfg->teardown_fns == NULL ? NULL : ctx->cfg->teardown_fns[vcpu];

    start_of_run(ctx, cpu, vcpu, i, j);
    if (pre != NULL)
      pre(&run);
    if (ctx->cfg->thread_sync_handlers) {
      if (ctx->cfg->thread_sync_handlers[vcpu][0] != NULL) {
        old_sync_handler_el0     = hotswap_exception(0x400, (uint32_t*)ctx->cfg->thread_sync_handlers[vcpu][0]);
      }
      if (ctx->cfg->thread_sync_handlers[vcpu][1] != NULL) {
        old_sync_handler_el1     = hotswap_exception(0x000, (uint32_t*)ctx->cfg->thread_sync_handlers[vcpu][1]);
        old_sync_handler_el1_spx = hotswap_exception(0x200, (uint32_t*)ctx->cfg->thread_sync_handlers[vcpu][1]);
      }
    }

    /* this barrier must be last thing before running function */
    bwait(vcpu, i % ctx->cfg->no_threads, &ctx->start_barriers[i], ctx->cfg->no_threads);
    func(&run);

    if (ctx->cfg->thread_sync_handlers) {
      if (old_sync_handler_el0 != NULL) {
        restore_hotswapped_exception(0x400, old_sync_handler_el0);
      }
      if (old_sync_handler_el1 != NULL) {
        restore_hotswapped_exception(0x000, old_sync_handler_el1);
        restore_hotswapped_exception(0x200, old_sync_handler_el1_spx);
      }
    }

    if (post != NULL)
      post(&run);

    end_of_run(ctx, cpu, vcpu, i);

    if (ENABLE_PGTABLE)
      _check_ptes(ctx, ctx->cfg->no_heap_vars, heaps, ptes, saved_ptes);

run_thread_after_execution:
    bwait(cpu, i % NO_CPUS, &ctx->cleanup_barriers[i], NO_CPUS);
  }
}

void prefetch(test_ctx_t* ctx, int i, int r) {
  for (int v = 0; v < ctx->cfg->no_heap_vars; v++) {
    /* TODO: read initial state */
    lock(&__harness_lock);
    uint64_t is_valid = vmm_pte_valid(ctx->ptable, &ctx->heap_vars[v][i]);
    unlock(&__harness_lock);
    if (randn() % 2 && is_valid && ctx->heap_vars[v][i] != ctx_initial_heap_value(ctx, v)) {
      fail(
          "! fatal: initial state for heap var \"%s\" on run %d was %ld not %ld\n",
          varname_from_idx(ctx, v), r, ctx->heap_vars[v][i], ctx_initial_heap_value(ctx, v));
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

void start_of_run(test_ctx_t* ctx, int cpu, int vcpu, int i, int r) {
  /* do not prefetch anymore .. not safe! */
  prefetch(ctx, i, r);
  if (! ctx->cfg->start_els || ctx->cfg->start_els[vcpu] == 0) {
    drop_to_el0();
  }
}

/** every N/10 runs we shuffle the CPUs about
 */
static void reshuffle(test_ctx_t* ctx) {
  shuffle((int*)ctx->affinity, sizeof(int), NO_CPUS);
  debug("set affinity = %Ad\n", ctx->affinity, NO_CPUS);
}

void end_of_run(test_ctx_t* ctx, int cpu, int vcpu, int i) {
  if (! ctx->cfg->start_els || ctx->cfg->start_els[vcpu] == 0)
    raise_to_el1();
  bwait(vcpu, i % ctx->cfg->no_threads, &ctx->end_barriers[i], ctx->cfg->no_threads);

  /* only 1 thread should collect the results, else they will be duplicated */
  if (vcpu == 0) {
    uint64_t r = ctx->current_run++;
    handle_new_result(ctx, i, r);

    /* progress indicator */
    uint64_t step = (ctx->no_runs / 10);
    if (r % step == 0) {
      trace("[%d/%d]\n", r, ctx->no_runs);
      reshuffle(ctx);
    } else if (r == ctx->no_runs - 1) {
      trace("[%d/%d]\n", r + 1, ctx->no_runs);
    }
  }
}

void start_of_thread(test_ctx_t* ctx, int cpu) {
  /* ensure initial state gets propagated to all cores before continuing ...
  */
  bwait(cpu, 0, ctx->initial_sync_barrier, NO_CPUS);

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

  bwait(cpu, 0, ctx->final_barrier, NO_CPUS);
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
      vmm_update_mapping(ctx->ptable, vector_base_addr_rw+i*4096, vector_base_pa+i*4096, PROT_PGTABLE);
    }
  }
}

void end_of_test(test_ctx_t* ctx) {
  if (! DISABLE_RESULTS_HIST) {
    trace("%s\n", "Printing Results...");
    print_results(ctx->hist, ctx);
  }
  trace("Finished test %s\n", ctx->cfg->name);
  free_test_ctx(ctx);

  if (ENABLE_PGTABLE)
    vmm_free_pgtable(ctx->ptable);
}