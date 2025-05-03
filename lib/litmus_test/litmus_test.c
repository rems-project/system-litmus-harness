
#include "lib.h"

/** lock for critical sections inside harness
 * during litmus test execution
 */
static lock_t __harness_lock;

static void go_cpus(int cpu, void* a);
static void run_thread(test_ctx_t* ctx, int cpu);
static void end_of_test(test_ctx_t* ctx);
static void start_of_test(test_ctx_t* ctx);
static void end_of_thread(test_ctx_t* ctx, int cpu);
static void start_of_thread(test_ctx_t* ctx, int cpu);
static void end_of_run(test_ctx_t* ctx, int cpu, int vcpu, run_idx_t i, run_count_t r);
static void start_of_run(test_ctx_t* ctx, int cpu, int vcpu, run_idx_t i, run_count_t r);

/* entry point */
void run_test(const litmus_test_t* cfg) {
  /* create test context obj
   * make sure it's on the heap
   * if we're passing to another thread
   */
  test_ctx_t* ctx = ALLOC_ONE(test_ctx_t);

  /* use same seed for each test
   * this means we can re-run just 1 test from whole batch
   * and still get determinism (up to relaxation) */
  reset_seed();

  /* create the dynamic configuration (context) from the static information (cfg) */
  init_test_ctx(ctx, cfg, NUMBER_OF_RUNS, RUNS_IN_BATCH);
  ctx->valloc_ptable_chkpnt = valloc_ptable_checkpoint();
  initialize_regions(&ctx->heap_memory);

  debug("done.  run the tests.\n");
  start_of_test(ctx);

  /* run it */
  if (TRACE) {
    printf("#");
    for (int i = 0; i < NO_CPUS; i++) {
      printf("P%d\t\t\t", i);
    }
    printf("\n");
  }
  run_on_cpus((async_fn_t*)go_cpus, (void*)ctx);

  /* clean up and display results */
  end_of_test(ctx);
}

static void go_cpus(int cpu, void* a) {
  test_ctx_t* ctx = (test_ctx_t*)a;
  start_of_thread(ctx, cpu);
  run_thread(ctx, cpu);
  end_of_thread(ctx, cpu);
}

static void reset_mair_attr7(test_ctx_t* ctx) {
  if (ctx->system_state->enable_mair) {
    u64 mair = read_sysreg(mair_el1);
    u64 bm = ~(BITMASK(8) << 56);
    write_sysreg((mair & bm) | (ctx->system_state->mair_attr7 << 56), mair_el1);
  }
}

/** reset the sysregister state after each run
 */
static void _init_sys_state(test_ctx_t* ctx) {
  reset_mair_attr7(ctx);
}

/** convert the top-level loop counter index into the
 * run offset into the tables
 */
run_idx_t count_to_run_index(test_ctx_t* ctx, run_count_t i) {
  switch (LITMUS_SHUFFLE_TYPE) {
  case SHUF_NONE:
    return (run_idx_t)i;
  case SHUF_RAND:
    return ctx->shuffled_ixs[i];
  default:
    fail("! unknown LITMUS_SHUFFLE_TYPE: %d/%s\n", LITMUS_SHUFFLE_TYPE, shuff_type_to_str(LITMUS_SHUFFLE_TYPE));
    return 0;
  }
}

/* shuffle about the affinities,
 * giving threads new physical CPU assignments
 */
static void allocate_affinities(test_ctx_t* ctx) {
  if (LITMUS_AFF_TYPE != AFF_NONE) {
    shuffle((int*)ctx->affinity, sizeof(int), NO_CPUS);
    debug("set affinity = %Ad\n", ctx->affinity, NO_CPUS);
  }
}

/** run concretization and initialization for the runs of this batch
 */
static void allocate_data_for_batch(test_ctx_t* ctx, u64 vcpu, run_count_t batch_start_idx, run_count_t batch_end_idx) {
  debug("vCPU%d allocating test data for batch starting %ld\n", vcpu, batch_start_idx);

  if (vcpu == 0) {
    /* first we have to allocate new pagetables for the whole batch
    * cleaning up any previous ones as we do
    */
    if (ENABLE_PGTABLE) {
      for (run_count_t r = batch_start_idx; r < batch_end_idx; r++) {
        u64 asid = asid_from_run_count(ctx, r);

        if (ctx->ptables[asid] == NULL) {
          ctx->ptables[asid] = vmm_alloc_new_test_pgtable();
        }

        debug("allocated pgtable for batch_start=%ld, ASID=%ld at %p\n", batch_start_idx, asid, ctx->ptables[asid]);
      }
    }

    if (LITMUS_RUNNER_TYPE != RUNNER_ARRAY) {
      if (LITMUS_RUNNER_TYPE == RUNNER_EPHEMERAL) {
        concretize_batch(LITMUS_CONCRETIZATION_TYPE, ctx, ctx->cfg, batch_start_idx, batch_end_idx);
      }

      /* initialise the memory and pagetables */
      for (run_count_t r = batch_start_idx; r < batch_end_idx; r++) {
        run_idx_t i = count_to_run_index(ctx, r);
        if (LITMUS_RUNNER_TYPE == RUNNER_SEMI_ARRAY || LITMUS_RUNNER_TYPE == RUNNER_EPHEMERAL) {
          write_init_state(ctx, ctx->cfg, i);
        }
      }

      concretization_postcheck_batch(ctx, ctx->cfg, ctx->heap_vars, batch_start_idx, batch_end_idx);
    }
  }
}

static u64 run_data_size(test_ctx_t* ctx) {
  u64 actual_size = (
    /* the VAs of the variables themselves */
    sizeof(u64*) * ctx->cfg->no_heap_vars
    /* ... and the PAs */
    + sizeof(u64) * ctx->cfg->no_heap_vars
    /* each of the output registers */
    + sizeof(u64*) * ctx->cfg->no_regs
    /* and per-variable, each level of the pagetable,
     * the entry pointer and initial descriptor */
    + sizeof(u64) * ctx->cfg->no_heap_vars * 4 +
    sizeof(u64*) * ctx->cfg->no_heap_vars * 4
    /* ... plus spare for pointers to each */
    + sizeof(u64*) * ctx->cfg->no_heap_vars * 2
  );

  /* align up to the next power of 2 */
  u64 bits = log2(actual_size);
  return ALIGN_UP(actual_size, bits + 1);
}

/** initialise the litmus_test_run datas to pass as arguments
 */
static void setup_run_data(
  test_ctx_t* ctx, u64 vcpu, run_count_t batch_start_idx, run_count_t batch_end_idx, valloc_arena* arena,
  litmus_test_run* runs
) {
  int idx;
  run_count_t r;

  for (idx = 0, r = batch_start_idx; r < batch_end_idx; r++, idx++) {
    debug("setting up run %d in batch (%ld overall)\n", idx, r);
    run_idx_t i = count_to_run_index(ctx, r);

    runs[idx].ctx = ctx;
    runs[idx].i = i;
    runs[idx].va = ALLOC_ARENA_MANY(arena, u64*, ctx->cfg->no_heap_vars);
    runs[idx].pa = ALLOC_ARENA_MANY(arena, u64, ctx->cfg->no_heap_vars);
    runs[idx].out_reg = ALLOC_ARENA_MANY(arena, u64*, ctx->cfg->no_regs);

    runs[idx].tt_descs = ALLOC_ARENA_MANY(arena, u64*, ctx->cfg->no_heap_vars);
    runs[idx].tt_entries = ALLOC_ARENA_MANY(arena, u64**, ctx->cfg->no_heap_vars);

    for (var_idx_t v = 0; v < ctx->cfg->no_heap_vars; v++) {
      runs[idx].tt_descs[v] = ALLOC_ARENA_MANY(arena, u64, 4);
      runs[idx].tt_entries[v] = ALLOC_ARENA_MANY(arena, u64*, 4);
    }
  }

  for (idx = 0, r = batch_start_idx; r < batch_end_idx; r++, idx++) {
    run_idx_t i = count_to_run_index(ctx, r);
    litmus_test_run* run = &runs[idx];
    for (var_idx_t v = 0; v < ctx->cfg->no_heap_vars; v++) {
      u64* p = ctx_heap_var_va(ctx, v, i);
      run->va[v] = p;

      if (ENABLE_PGTABLE) {
        for (int lvl = 0; lvl < 4; lvl++) {
          run->tt_entries[v][lvl] = vmm_pte_at_level(ctx->ptables[asid_from_run_count(ctx, r)], (u64)p, lvl);
          run->tt_descs[v][lvl] = *run->tt_entries[v][lvl];
        }

        run->pa[v] = ctx_pa(ctx, i, (u64)p);
      }
    }

    for (reg_idx_t reg = 0; reg < ctx->cfg->no_regs; reg++) {
      run->out_reg[reg] = &ctx->out_regs[reg][i];
    }
  }
}

static void clean_run_data(
  test_ctx_t* ctx, u64 vcpu, run_count_t batch_start_idx, run_count_t batch_end_idx, valloc_arena* arena,
  litmus_test_run* runs
) {
  int idx;
  run_count_t r;

  /* after each run, reset the pagetables back to what they were before
   * we ran the test function
   *
   * this means we do not need to keep allocating new pagetables each time
   *
   * NOTE: this does not mean reset pagetables back to the initial state
   * -- the refreshed tables will still contain level3 entries for the
   * allocated pages.
   */
  if (ENABLE_PGTABLE) {
    for (idx = 0, r = batch_start_idx; r < batch_end_idx; r++, idx++) {
      for (var_idx_t v = 0; v < ctx->cfg->no_heap_vars; v++) {
        for (int l = 0; l < 4; l++) {
          *runs[idx].tt_entries[v][l] = runs[idx].tt_descs[v][l];
        }
      }
    }
  }

  FREE(arena);
}

/** invalidate all the ASIDs being used for the next batch
 */
static void clean_tlb_for_batch(test_ctx_t* ctx, run_count_t batch_start_idx, run_count_t batch_end_idx) {
  dsb();

  for (run_count_t r = batch_start_idx; r < batch_end_idx; r++) {
    u64 asid = asid_from_run_count(ctx, r);
    tlbi_asid(asid);
  }

  dsb();
}

typedef struct
{
  u32* el0;
  u32* el1_sp0;
  u32* el1_spx;
} exception_handlers_refs_t;

static void set_new_sync_exception_handlers(test_ctx_t* ctx, u64 vcpu, exception_handlers_refs_t* handlers) {
  if (ctx->cfg->thread_sync_handlers) {
    if (ctx->cfg->thread_sync_handlers[vcpu][0] != NULL) {
      handlers->el0 = hotswap_exception(0x400, (u32*)ctx->cfg->thread_sync_handlers[vcpu][0]);
    }
    if (ctx->cfg->thread_sync_handlers[vcpu][1] != NULL) {
      handlers->el1_sp0 = hotswap_exception(0x000, (u32*)ctx->cfg->thread_sync_handlers[vcpu][1]);
      handlers->el1_spx = hotswap_exception(0x200, (u32*)ctx->cfg->thread_sync_handlers[vcpu][1]);
    }
  }
}

static void restore_old_sync_exception_handlers(test_ctx_t* ctx, u64 vcpu, exception_handlers_refs_t* handlers) {
  if (ctx->cfg->thread_sync_handlers) {
    if (handlers->el0 != NULL) {
      restore_hotswapped_exception(0x400, handlers->el0);
    }
    if (handlers->el1_sp0 != NULL) {
      restore_hotswapped_exception(0x000, handlers->el1_sp0);
      restore_hotswapped_exception(0x200, handlers->el1_spx);
    }
  }

  handlers->el0 = NULL;
  handlers->el1_sp0 = NULL;
  handlers->el1_spx = NULL;
}

/** prepare the state for the following tests
 * e.g. clean TLBs and install exception handler
 */
static void prepare_test_contexts(
  test_ctx_t* ctx, u64 vcpu, run_count_t batch_start_idx, run_count_t batch_end_idx, exception_handlers_refs_t* handlers
) {
  clean_tlb_for_batch(ctx, batch_start_idx, batch_end_idx);
}

/** switch to a particular run's ASID
 */
static void switch_to_test_context(test_ctx_t* ctx, int vcpu, run_count_t r, exception_handlers_refs_t* handlers) {
  debug("switching to test context for run %ld\n", r);

  /* set sysregs to what the test needs
   * we do this before write_init_state to ensure
   * changes to translation regime are picked up
   */
  _init_sys_state(ctx);

  if (ENABLE_PGTABLE && LITMUS_SYNC_TYPE == SYNC_ASID) {
    u64 asid = asid_from_run_count(ctx, r);
    u64* ptable = ctx->ptables[asid];
    debug("switching to ASID %ld, with ptable = %p\n", asid, ptable);
    vmm_switch_ttable_asid(ptable, asid);
  }

  if (!ctx->cfg->start_els || ctx->cfg->start_els[vcpu] == 0) {
    drop_to_el0();
  }

  set_new_sync_exception_handlers(ctx, vcpu, handlers);
}

static void return_to_harness_context(test_ctx_t* ctx, u64 cpu, u64 vcpu, exception_handlers_refs_t* handlers) {
  debug("return to harness context\n");

  /* have to restore the old handlers
   * before we're able to return to EL1
   * since we potentially overwrote the
   * old SVC handler for the test
   */
  restore_old_sync_exception_handlers(ctx, vcpu, handlers);

  if (!ctx->cfg->start_els || ctx->cfg->start_els[vcpu] == 0) {
    raise_to_el1();
  }

  if (ENABLE_PGTABLE && LITMUS_SYNC_TYPE == SYNC_ASID) {
    vmm_switch_ttable_asid(vmm_pgtables[cpu], 0);
  }
}

/** ensures all CPUs have an allocated affinity
 */
static void ensure_new_affinity(test_ctx_t* ctx, u64 cpu) {
  if (LITMUS_AFF_TYPE != AFF_NONE)
    allocate_affinities(ctx);
}

/** get this physical CPU's allocation to a vCPU (aka test thread)
 */
static u64 get_affinity(test_ctx_t* ctx, u64 cpu) {
  if (LITMUS_AFF_TYPE == AFF_RAND) {
    return ctx->affinity[cpu];
  } else if (LITMUS_AFF_TYPE == AFF_NONE) {
    return cpu;
  }

  fail("! get_affinity: unknown LITMUS_AFF_TYPE\n");
  return 0;
}

/** run the tests in a loop
 */
static void run_thread(test_ctx_t* ctx, int cpu) {
  /**
   * we run the tests in a batched way
   * running $N tests in a row before cleaning up
   *
   * to do this we have to ensure that each test that gets run is given a separate ASID
   * and each test has its own pagetable to manipulate (so they do not interfere with other tests).
   *
   * at the start of a batch we allocate $N pagetables for each test, using ASIDS 0..$N
   * if we have available ASIDs 0..255 (aka 8-bit ASIDs) then we reserve ASID 0 for the test harness itself
   * and allocate ASIDs 1...127 for the tests (aka 127 runs)
   * and reserve ASIDs 128..255 as spare ASIDs for tests.
   *
   * Hence the max batch size is entirely determined by the number of bits available for ASIDs that we have.
   *
   * then at the end of the batch, we clean all the ASIDS, free all the pagetables, flush any caches that may need flushing.
   */
  for (run_count_t j = 0; j < ctx->no_runs;) {
    u64 vcpu; /* the test thread this physical core will execute */

    run_count_t batch_start_idx = j;
    run_count_t batch_end_idx = MIN(batch_start_idx + ctx->batch_size, ctx->no_runs);

    if (cpu == 0)
      /* make sure we only try assign new affinities once per batch
       * CPU0 always gets to this point, so let it do it.
       */
      ensure_new_affinity(ctx, cpu);

    /* wait for affinities to be assigned before continuing */
    BWAIT(cpu, ctx->generic_cpu_barrier, NO_CPUS);

    vcpu = get_affinity(ctx, cpu);
    set_vcpu(vcpu);

    allocate_data_for_batch(ctx, vcpu, batch_start_idx, batch_end_idx);
    /* since only 1 vCPU will allocate pagetables to ensure consistency
     * we wait for them to have finished before continuing and trying to read
     * the PTEs
     */
    BWAIT(cpu, ctx->generic_cpu_barrier, NO_CPUS);

    litmus_test_run runs[ctx->batch_size];
    valloc_arena* arena = ALLOC_SIZED(sizeof(valloc_arena) + run_data_size(ctx) * ctx->batch_size);
    arena_init(arena, run_data_size(ctx) * ctx->batch_size);
    setup_run_data(ctx, vcpu, batch_start_idx, batch_end_idx, arena, runs);

    exception_handlers_refs_t handlers = { NULL, NULL, NULL };

    prepare_test_contexts(ctx, vcpu, batch_start_idx, batch_end_idx, &handlers);
    for (int bi = 0; j < batch_end_idx; bi++, j++) {
      run_idx_t i = count_to_run_index(ctx, j);
      th_f* pre = ctx->cfg->setup_fns == NULL ? NULL : ctx->cfg->setup_fns[vcpu];
      th_f* func = ctx->cfg->threads[vcpu];
      th_f* post = ctx->cfg->teardown_fns == NULL ? NULL : ctx->cfg->teardown_fns[vcpu];

      if (vcpu >= ctx->cfg->no_threads) {
        goto run_thread_after_execution;
      }

      start_of_run(ctx, cpu, vcpu, i, j);
      switch_to_test_context(ctx, vcpu, j, &handlers);

      if (pre != NULL)
        pre(&runs[bi]);

      /* this barrier must be last thing before running function */
      BWAIT(vcpu, &ctx->start_barriers[bi], ctx->cfg->no_threads);
      func(&runs[bi]);

      if (post != NULL) {
        /* TODO: why restore/set vbar here?
         */
        restore_old_sync_exception_handlers(ctx, vcpu, &handlers);
        post(&runs[bi]);
        set_new_sync_exception_handlers(ctx, vcpu, &handlers);
      }

      return_to_harness_context(ctx, cpu, vcpu, &handlers);

      /* wait for all threads to finish and return to the harness' context
       * before attempting to collect results */
      BWAIT(vcpu, ctx->generic_vcpu_barrier, ctx->cfg->no_threads);
      end_of_run(ctx, cpu, vcpu, i, j);
run_thread_after_execution:
      BWAIT(cpu, ctx->generic_cpu_barrier, NO_CPUS);
    }
    clean_run_data(ctx, vcpu, batch_start_idx, batch_end_idx, arena, runs);
  }
}

static void prefetch(test_ctx_t* ctx, run_idx_t i, run_count_t r) {
  debug("prefetching for run %ld\n", r);
  for (var_idx_t v = 0; v < ctx->cfg->no_heap_vars; v++) {
    LOCK(&__harness_lock);
    u64* va = ctx_heap_var_va(ctx, v, i);
    u64 is_valid = vmm_pte_valid(ptable_from_run(ctx, i), va);

    /* careful! with --pgtable the va might not actually be mapped
     * so we access the underlying pa instead via a safe MMAP'd region
     * which hopefully hits the caches in an interesting way too */
    u64* safe_va = ENABLE_PGTABLE ? (u64*)SAFE_TESTDATA_VA((u64)va) : va;

    u8 has_init_value = is_backed_var(&ctx->heap_vars[v]);

    UNLOCK(&__harness_lock);
    if (randn() % 2 && is_valid && has_init_value && *safe_va != ctx_initial_heap_value(ctx, v)) {
      fail(
        "! fatal: initial state for heap var \"%s\" on run %d was %ld not %ld\n",
        varname_from_idx(ctx, v),
        r,
        *safe_va,
        ctx_initial_heap_value(ctx, v)
      );
    }
  }
}

static void resetsp(void) {
  /* check and reset to SP_EL0 */
  u64 currentsp;
  asm volatile("mrs %[currentsp], SPSel\n" : [currentsp] "=r"(currentsp));

  if (currentsp == 0b1) { /* if not already using SP_EL0 */
    asm volatile(
      "mov x18, sp\n"
      "msr spsel, #0\n"
      "dsb nsh\n"
      "isb\n"
      "mov sp, x18\n"
      :
      :
      : "x18"
    );
  }
}

static void check_no_bad_migration(void) {
  struct arm_implementation this_impl;
  arch_read_implementation(&this_impl);

  /* KVM does not pin VM vCPUS to cores
   * that's up to the VMM to call vcpu_run on whichever cores it wants.
   * But our VMM (QEMU) uses userthreads which get migrated around.
   *
   * On a big.LITTLE machine this migration could go from a big to a LITTLE.
   * This would invalidate our results silently.
   *
   * So we see if the Arm implementation has changed out from under us since setup
   * and just fail angrily if that happens.
   */

   if (!arch_implementation_eq(&current_thread_info()->impl, &this_impl)) {
    fail(
      "Caught big.LITTLE migration in the act. "
      "Use AFFINITY=... to pin KVM threads to CPUs.\n"
    );
   }
}

static void start_of_run(test_ctx_t* ctx, int cpu, int vcpu, run_idx_t i, run_count_t r) {
  check_no_bad_migration();
  prefetch(ctx, i, r);
}

static void end_of_run(test_ctx_t* ctx, int cpu, int vcpu, run_idx_t i, run_count_t r) {
  /* only 1 thread should collect the results, else they will be duplicated */
  if (vcpu == 0) {
    u64 time = read_clk();
    if (time - ctx->last_tick > 10 * TICKS_PER_SEC || ctx->last_tick == 0) {
      char time_str[100];
      sprint_time(NEW_BUFFER(&time_str[0], 100), time, SPRINT_TIME_HHMMSS);
      verbose("  [%s] %d/%d\n", time_str, r, ctx->no_runs);
      ctx->last_tick = time;
    }

    handle_new_result(ctx, i, r);

    /* progress indicator */
    u64 step = (ctx->no_runs / 10);
    if (r % step == 0) {
      trace("[%d/%d]\n", r, ctx->no_runs);
    } else if (r == ctx->no_runs - 1) {
      trace("[%d/%d]\n", r + 1, ctx->no_runs);
    }
  }
}

static void start_of_thread(test_ctx_t* ctx, int cpu) {
  /* ensure initial state gets propagated to all cores before continuing ...
  */
  BWAIT(cpu, ctx->generic_cpu_barrier, NO_CPUS);

  /* before can drop to EL0, ensure EL0 has a valid mapped stack space
   */
  resetsp();

  trace("CPU%d: starting test\n", cpu);
}

static void end_of_thread(test_ctx_t* ctx, int cpu) {
  debug("end of thread\n");
  if (ENABLE_PGTABLE) {
    /* restore global non-test pgtable */
    vmm_switch_ttable(vmm_pgtables[cpu]);
  }

  BWAIT(cpu, ctx->generic_cpu_barrier, NO_CPUS);
  trace("CPU%d: end of test\n", cpu);
}

static void start_of_test(test_ctx_t* ctx) {
  ctx->concretization_st = concretize_init(LITMUS_CONCRETIZATION_TYPE, ctx, ctx->cfg, ctx->no_runs);
  if (LITMUS_RUNNER_TYPE == RUNNER_ARRAY) {
    concretize(LITMUS_CONCRETIZATION_TYPE, ctx, ctx->cfg, ctx->concretization_st, ctx->no_runs);
    write_init_states(ctx, ctx->cfg, ctx->no_runs);
  }

  verbose("running test: %s\n", ctx->cfg->name);
  trace("====== %s ======\n", ctx->cfg->name);
}

static void end_of_test(test_ctx_t* ctx) {
  ctx->end_clock = read_clk();

  if (ENABLE_RESULTS_HIST) {
    trace("%s\n", "Printing Results...");
    print_results(ctx->hist, ctx);
  }

  trace("Finished test %s\n", ctx->cfg->name);

  concretize_finalize(LITMUS_CONCRETIZATION_TYPE, ctx, ctx->cfg, ctx->no_runs, ctx->concretization_st);

  valloc_ptable_restore(ctx->valloc_ptable_chkpnt);
  free_test_ctx(ctx);
  FREE(ctx);
}