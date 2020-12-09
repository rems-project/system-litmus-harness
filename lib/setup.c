#include <lib.h>

#include "vmm.h"

extern uint64_t __argc;
extern char*    __argv[100];

/** setup is called very early
 * before the UART is enabled
 */
void setup(char* fdtloc) {
  fdt_load_addr = fdtloc;

  current_thread_info()->mmu_enabled = 0;
  current_thread_info()->locking_enabled = 0;

  init_device(fdt_load_addr);
  init_valloc();

  INIT_CLOCK = read_clk();
  TICKS_PER_SEC = read_clk_freq();

  /* read passed args */
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
  typedef struct {
    const char* name;
    uint64_t bottom;
    uint64_t top;
  } bar_region_t;

  typedef struct {
    const char* name;
    uint8_t  is_top;
    uint64_t data;
  } region_border_t;

  bar_region_t regs[] = {
    {"DRAM", BOT_OF_MEM, TOP_OF_MEM},
    {"TEXT", BOT_OF_TEXT, TOP_OF_TEXT},
    {"STACK", BOT_OF_STACK_PA, TOP_OF_STACK_PA},
    {"DATA", BOT_OF_DATA, TOP_OF_DATA},
    {"HEAP", BOT_OF_HEAP, TOP_OF_HEAP},
    {"IO", BOT_OF_IO, TOP_OF_IO},
  };

  const uint64_t reg_count = sizeof(regs)/sizeof(bar_region_t);
  region_border_t bars[2*reg_count] ;
  for (int i = 0; i < reg_count; i++) {
    bar_region_t* r = &regs[i];
    bars[2*i + 0] = (region_border_t){r->name, 0, r->bottom};
    bars[2*i + 1] = (region_border_t){r->name, 1, r->top};
  }

  /* sort */
  int no_bars = sizeof(bars)/sizeof(region_border_t);
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
    debug("CPU%d STACK EL1 : [%p -> %p => %p -> %p]\n", i, STACK_MMAP_THREAD_TOP_EL1(i), STACK_MMAP_THREAD_BOT_EL1(i), STACK_PYS_THREAD_TOP_EL1(i), STACK_PYS_THREAD_BOT_EL1(i));
    debug("CPU%d STACK EL0 : [%p -> %p => %p -> %p]\n", i, STACK_MMAP_THREAD_TOP_EL0(i), STACK_MMAP_THREAD_BOT_EL0(i), STACK_PYS_THREAD_TOP_EL0(i), STACK_PYS_THREAD_BOT_EL0(i));
  }

  vector_base_pa = (uint64_t)&el1_exception_vector_table_p0;

  for (int i = 0; i < NO_CPUS; i++) {
    debug("CPU%d VTABLE : %p\n", i, vector_base_pa+4096*i);
  }

  /* create pgtable */
  if (ENABLE_PGTABLE) {
    vmm_pgtables = alloc(sizeof(uint64_t*)*NO_CPUS);
  }

  printf("#build: (%s)\n", version_string());
  printf("#output: ");
  if (DEBUG) {
    printf("verbose+trace+debug");
  } else if (TRACE) {
    printf("verbose+trace");
  } else if (VERBOSE) {
    printf("verbose");
  } else {
    printf("quiet");
  }

  printf("\n");

  printf("#test_patterns: ");
  for (int i = 0; i < collected_tests_count; i++) {
    printf("%s ", collected_tests[i]);
  }
  printf("\n");
  printf("#seed: %ld\n", INITIAL_SEED);
  printf("#pgtable: %ld\n", ENABLE_PGTABLE);
  printf("#timing: %ld\n", ENABLE_PERF_COUNTS);
  printf("#no_runs: %ld\n", NUMBER_OF_RUNS);
  printf("#batch_size: %ld\n", RUNS_IN_BATCH);
  printf("#tlbsync: %s\n", sync_type_to_str(LITMUS_SYNC_TYPE));
  printf("#aff: %s\n", aff_type_to_str(LITMUS_AFF_TYPE));
  printf("#shuffle: %s\n", aff_type_to_str(LITMUS_SHUFFLE_TYPE));
  printf("#concretize: %s\n", concretize_type_to_str(LITMUS_CONCRETIZATION_TYPE));
  printf("#runner: %s\n", runner_type_to_str(LITMUS_RUNNER_TYPE));

  ensure_cpus_on();
}

void ensure_cpus_on(void) {
  debug("setting up CPU0\n");
  cpu_data_init();
  per_cpu_setup(0);

  debug("booting all CPUs ...\n");
  cpu_boot(1);
  cpu_boot(2);
  cpu_boot(3);
  debug("started boot.\n");

  for (int i = 0; i < 4; i++) {
    while (! cpu_data[i].started) wfe();
  }

  debug("booted all CPUs\n");
}

void per_cpu_setup(int cpu) {
  current_thread_info()->cpu_no = cpu;
  current_thread_info()->mmu_enabled = 0;

  debug("VBAR = %p\n", read_sysreg(vbar_el1));

  if (ENABLE_PGTABLE) {
    uint64_t* vmm_pgtable = vmm_alloc_new_4k_pgtable();
    vmm_pgtables[cpu] = vmm_pgtable;

    vmm_set_id_translation(vmm_pgtable);
    debug("set new pgtable for CPU%d\n", cpu);
  }

  /* enable virtual/physical timers */
  write_sysreg((1 << 0) | (1 << 1), cntkctl_el1);

  /* enable PMU cycle counter */
  write_sysreg((1UL << 27), pmccfiltr_el0);
  write_sysreg((1UL << 6) | (1UL << 0), pmcr_el0);  /* reset to 0 */
  write_sysreg((1 << 31), pmcntenset_el0);
  write_sysreg((1 << 0), pmuserenr_el0);

  current_thread_info()->locking_enabled = 1;

  cpu_data[cpu].to_execute = 0;
  dmb();
  cpu_data[cpu].started = 1;

  /* wake other threads waiting */
  sev();

  /* ensure setup of sys registers are visible to harness as well as tests */
  isb();
}