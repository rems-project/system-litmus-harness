#include <lib.h>

#include "vmm.h"

extern uint64_t __argc;
extern char*    __argv[100];

void setup(char* fdtloc) {
  fdt = fdtloc;

  current_thread_info()->mmu_enabled = 0;
  current_thread_info()->locking_enabled = 0;

  init_device(fdt);
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

  char c = 'm';
  char seps [] = { c, c, c, c, c, c, c, c, c, c, c, c, c, '\0' };
#define PR_BARS(top, bot) \
  { uint64_t diff##__COUNTER__ = top-bot; int d##__COUNTER__ = log2(diff##__COUNTER__)/5; \
    seps[d##__COUNTER__] = 0; for (int i##__COUNTER__ = 0; i##__COUNTER__ < (diff##__COUNTER__/(1<<(5*d##__COUNTER__))); i##__COUNTER__++) \
    debug("\t%s\n", &seps[0]); seps[d##__COUNTER__] = c;  }
  debug("memory layout:\n");
  debug("--------------------------------\n");
  debug("%p: TOP_OF_MEM\n", TOP_OF_HEAP);
  typedef struct {
    const char* name;
    uint64_t bottom;
    uint64_t top;
  } bar_region_t;

  bar_region_t bars[] = {
    {"TEXT", BOT_OF_TEXT, TOP_OF_TEXT},
    {"STACK", BOT_OF_STACK_PA, TOP_OF_STACK_PA},
    {"DATA", BOT_OF_DATA, TOP_OF_DATA},
    {"HEAP", BOT_OF_HEAP, TOP_OF_HEAP},
    {"MEM", TOP_OF_HEAP, TOP_OF_MEM},
  };

  /* sort */
  int no_bars = sizeof(bars)/sizeof(bar_region_t);
  for (int i = 0; i < no_bars; i++) {
    for (int j = i; j < no_bars; j++) {
      if (bars[i].top < bars[j].top) {
        bar_region_t tmp = bars[i];
        bars[i] = bars[j];
        bars[j] = tmp;
      }
    }
  }

  for (int j = 0; j < no_bars; j++) {
    debug("%p: TOP_OF_%s\n", bars[j].top, bars[j].name);
  }
  debug("%p: TOP_OF_IO\n", GiB);
  debug("0x0: BOTTOM_OF_MEMORY\n");
  debug("--------------------------------\n");

  for (int i = 0; i < NO_CPUS; i++) {
    debug("CPU%d STACK EL1 : [%p -> %p => %p -> %p]\n", i, STACK_MMAP_THREAD_TOP_EL1(i), STACK_MMAP_THREAD_BOT_EL1(i), STACK_PYS_THREAD_TOP_EL1(i), STACK_PYS_THREAD_BOT_EL1(i));
    debug("CPU%d STACK EL0 : [%p -> %p => %p -> %p]\n", i, STACK_MMAP_THREAD_TOP_EL0(i), STACK_MMAP_THREAD_BOT_EL0(i), STACK_PYS_THREAD_TOP_EL0(i), STACK_PYS_THREAD_BOT_EL0(i));
  }

  vector_base_pa = (uint64_t)&el1_exception_vector_table_p0;

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