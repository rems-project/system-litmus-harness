#include <lib.h>

#include "device.h"
#include "vmm.h"

extern u64 __argc;
extern char* __argv[100];

/* per-thread boot data */
cpu_data_t cpu_data[MAX_CPUS];

#define PRINT_CFG(...) \
  { \
  if (VERBOSE || FORCE_VERBOSE_SETUP_PRINT) \
    mprintf(PRINT_MODE_NOINDENT | PRINT_MODE_TRACE, __VA_ARGS__); \
  }

/** setup is called very early
 * before the UART is enabled
 */
void setup(char* fdtloc) {
  /* check validity of fdt
   * if the first word is not the magic constant
   * then this is not a valid dtb
   */
  if (read_be(fdtloc) != 0xd00dfeed) {
    printf("INVALID fdt :: fdtloc *%p = 0x%lx\n", fdtloc, *(u32*)fdtloc);
    fdtloc = NULL;
  }

  fdt_load_addr = fdtloc;

  current_thread_info()->mmu_enabled = 0;
  current_thread_info()->locking_enabled = 0;

  /** at the beginning of time the contents of
   * caches (including the TLBs) are UNKNOWN
   *
   * so we force a clean before we do any multiprocessor stuff
   */
  flush_all_UNKNOWN_reset_caches();

  /* read device tree and linker regions */
  init_device(fdt_load_addr);

  /* initialise the allocators */
  init_valloc();
  init_valloc_ptable();

  INIT_CLOCK = read_clk();
  TICKS_PER_SEC = read_clk_freq();

  /* read passed args
   *
   *  it is only after this point that we can use debug() and related macros
   */
  init_args();
  read_args(__argc, __argv);
  init_cfg_state();

  debug("setup\n");
  if (INITIAL_SEED == 0) {
    init_seed();
  }
  reset_seed();

  debug("memory layout:\n");
  debug("--------------------------------\n");
  typedef struct
  {
    const char* name;
    u64 bottom;
    u64 top;
  } bar_region_t;

  typedef struct
  {
    const char* name;
    u8 is_top;
    u64 data;
  } region_border_t;

  bar_region_t regs[] = {
    { "DRAM", BOT_OF_MEM, TOP_OF_MEM },
    { "TESTDATA", BOT_OF_TESTDATA, TOP_OF_TESTDATA },
    { "TEXT", BOT_OF_TEXT, TOP_OF_TEXT },
    { "PTABLES", BOT_OF_PTABLES, TOP_OF_PTABLES },
    { "STACK", BOT_OF_STACK_PA, TOP_OF_STACK_PA },
    { "DATA", BOT_OF_DATA, TOP_OF_DATA },
    { "HEAP", BOT_OF_HEAP, TOP_OF_HEAP },
    { "IO", BOT_OF_IO, TOP_OF_IO },
  };

  const u64 reg_count = sizeof(regs) / sizeof(bar_region_t);
  region_border_t bars[2 * reg_count];
  for (int i = 0; i < reg_count; i++) {
    bar_region_t* r = &regs[i];
    bars[2 * i + 0] = (region_border_t){ r->name, 0, r->bottom };
    bars[2 * i + 1] = (region_border_t){ r->name, 1, r->top };
  }

  /* sort */
  int no_bars = sizeof(bars) / sizeof(region_border_t);
  for (int i = 0; i < no_bars; i++) {
    for (int j = i; j < no_bars; j++) {
      if (bars[i].data < bars[j].data) {
        region_border_t tmp = bars[i];
        bars[i] = bars[j];
        bars[j] = tmp;
      }
    }
  }

  for (int j = 0; j < no_bars; j++) {
    if (bars[j].is_top) {
      debug("%p: TOP_OF_%s\n", bars[j].data, bars[j].name);
    } else {
      debug("%p: BOT_OF_%s\n", bars[j].data, bars[j].name);
    }
  }
  debug("--------------------------------\n");
  for (int i = 0; i < NO_CPUS; i++) {
    debug(
      "CPU%d STACK EL1 : [%p -> %p => %p -> %p]\n",
      i,
      STACK_MMAP_THREAD_TOP_EL1(i),
      STACK_MMAP_THREAD_BOT_EL1(i),
      STACK_PYS_THREAD_TOP_EL1(i),
      STACK_PYS_THREAD_BOT_EL1(i)
    );
    debug(
      "CPU%d STACK EL0 : [%p -> %p => %p -> %p]\n",
      i,
      STACK_MMAP_THREAD_TOP_EL0(i),
      STACK_MMAP_THREAD_BOT_EL0(i),
      STACK_PYS_THREAD_TOP_EL0(i),
      STACK_PYS_THREAD_BOT_EL0(i)
    );
  }

  vector_base_pa = (u64)&el1_exception_vector_table_p0;

  for (int i = 0; i < NO_CPUS; i++) {
    debug("CPU%d VTABLE : %p\n", i, vector_base_pa + 4096 * i);
  }

  /* create pgtable */
  if (ENABLE_PGTABLE) {
    vmm_pgtables = alloc(sizeof(u64*) * NO_CPUS);
  }

  PRINT_CFG("version: %s\n", version_string());
  PRINT_CFG("build: %s\n", build_string());

  char* output = "";
  if (DEBUG) {
    output = "verbose+trace+debug";
  } else if (TRACE) {
    output = "verbose+trace";
  } else if (VERBOSE) {
    output = "verbose";
  } else {
    output = "quiet";
  }
  verbose("output: %s\n", output);

  char pattern[1024];
  STREAM* buf = NEW_BUFFER(pattern, 1024);
  for (int i = 0; i < collected_tests_count; i++) {
    sprintf(buf, "%s ", collected_tests[i]);
  }
  PRINT_CFG("test_patterns: %s\n", pattern);

  PRINT_CFG("seed: %ld\n", INITIAL_SEED);
  PRINT_CFG("pgtable: %ld\n", ENABLE_PGTABLE);
  PRINT_CFG("timing: %ld\n", ENABLE_PERF_COUNTS);
  PRINT_CFG("no_runs: %ld\n", NUMBER_OF_RUNS);
  PRINT_CFG("batch_size: %ld\n", RUNS_IN_BATCH);
  PRINT_CFG("tlbsync: %s\n", sync_type_to_str(LITMUS_SYNC_TYPE));
  PRINT_CFG("aff: %s\n", aff_type_to_str(LITMUS_AFF_TYPE));
  PRINT_CFG("shuffle: %s\n", shuff_type_to_str(LITMUS_SHUFFLE_TYPE));
  PRINT_CFG("concretize: %s\n", concretize_type_to_str(LITMUS_CONCRETIZATION_TYPE));
  PRINT_CFG("runner: %s\n", runner_type_to_str(LITMUS_RUNNER_TYPE));

  /* sanity check */
  if (ENABLE_PERF_COUNTS && !arch_has_feature(FEAT_PMUv3)) {
    fail(
      "[--perf] Cannot do performance measurement without FEAT_PMU, "
      "suggest removing --perf"
    );
  }

  ensure_cpus_on();
}

static void check_homogenous_threads(void) {
  /* Currently we do not support running litmus tests on heterogenous hardware
   * so we validate that the CPUs we run on are all the same implementation.
   *
   * We do this by checking they all agree with CPU0
   */
  for (int i = 0; i < NO_CPUS; i++) {
    if (!arch_implementation_eq(&thread_infos[i].impl, &thread_infos[0].impl)) {
      fail(
        "Detected unsupported heterogenous hardware configuration "
        "(is this a big.LITTLE core?). "
        "Use AFFINITY=... to pin KVM threads to CPUs.\n"
      );
    }
  }
}

void ensure_cpus_on(void) {
  debug("setting up CPU0\n");
  cpu_data_init();
  per_cpu_setup(0);

  debug("booting all CPUs ...\n");
  for (int i = 1; i < NO_CPUS; i++) {
    cpu_boot(i);
  }
  debug("started boot.\n");

  for (int i = 0; i < NO_CPUS; i++) {
    while (!cpu_data[i].started)
      wfe();
  }

  debug("booted all CPUs\n");
  check_homogenous_threads();
}

void per_cpu_setup(int cpu) {
  current_thread_info()->cpu_no = cpu;
  current_thread_info()->mmu_enabled = 0;
  arch_read_implementation(&current_thread_info()->impl);

  u64 el = read_sysreg(currentel) >> 2;

  if (el == 2) {
    u64 spsr = SPSR_FIELD(SPSR_EL, 1) | /* drop to EL1 */
               SPSR_FIELD(SPSR_SP, 0) | /* using SP_EL0 */
               0;
    u64 elr = (u64) && begin_el1; /* and 'return' to begin_el1 */

    write_sysreg(spsr, spsr_el2);
    write_sysreg(elr, elr_el2);
    debug("dropping to EL1 (with SPSR=%p) and ELR=%p\n", spsr, elr);
    eret();
  }

begin_el1:
  debug("Running at EL = %p\n", read_sysreg(currentel) >> 2);
  debug("VBAR = %p\n", read_sysreg(vbar_el1));

  if (ENABLE_PGTABLE) {
    u64* vmm_pgtable = vmm_alloc_new_4k_pgtable();
    vmm_pgtables[cpu] = vmm_pgtable;

    vmm_set_id_translation(vmm_pgtable);
    debug("set new pgtable for CPU%d\n", cpu);
  }

  /* enable virtual/physical timers */
  write_sysreg((1 << 0) | (1 << 1), cntkctl_el1);

  /* enable PMU cycle counter */
  if (arch_has_feature(FEAT_PMUv3)) {
    write_sysreg((1UL << 27), pmccfiltr_el0);
    write_sysreg((1UL << 6) | (1UL << 0), pmcr_el0); /* reset to 0 */
    write_sysreg((1 << 31), pmcntenset_el0);
    write_sysreg((1 << 0), pmuserenr_el0);
  }

  current_thread_info()->locking_enabled = 1;

  cpu_data[cpu].to_execute = 0;
  dmb();
  cpu_data[cpu].started = 1;

  /* wake other threads waiting */
  sev();

  /* ensure setup of sys registers are visible to harness as well as tests */
  isb();
}