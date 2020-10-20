#include <lib.h>

#include "vmm.h"

extern uint64_t __argc;
extern char*    __argv[100];

extern char* stackptr;

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
    uint64_t seed = read_clk();
    INITIAL_SEED = seed;
    debug("set initial seed = 0x%lx\n", seed);
  }

  char c = 'm';
  char seps [] = { c, c, c, c, c, c, c, c, c, c, c, c, c, '\0' };
#define PR_BARS(top, bot) \
  { uint64_t diff = top-bot; int d = log2(diff)/5; \
    seps[d] = 0; for (int i = 0; i < (diff/(1<<(5*d))); i++) \
    debug("\t%s\n", &seps[0]); seps[d] = c;  }
  debug("memory layout:\n");
  debug("--------------------------------\n");
  debug("%p: TOP_OF_MEM\n", TOP_OF_HEAP);
  PR_BARS(TOP_OF_MEM, TOP_OF_HEAP);
  debug("%p: TOP_OF_HEAP\n", TOP_OF_HEAP);
  PR_BARS(TOP_OF_HEAP, TOP_OF_STACK);
  debug("%p: TOP_OF_STACK\n", TOP_OF_STACK);
  PR_BARS(TOP_OF_STACK, TOP_OF_TEXT);
  debug("%p: TOP_OF_TEXT\n", TOP_OF_TEXT);
  PR_BARS(TOP_OF_TEXT, 0);
  debug("0x0: BOTTOM_OF_MEMORY\n");
  debug("--------------------------------\n");

  vector_base_pa = (uint64_t)&el1_exception_vector_table_p0;
  vector_base_addr_rw = vector_base_pa;

  /* create pgtable */
  if (ENABLE_PGTABLE) {
    vmm_pgtables = alloc(sizeof(uint64_t*)*NO_CPUS);
    vector_base_addr_rw = (uint64_t)alloc(4096*4);
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

    /* re-map vector_base_addr to some non-executable mapping of the vector table
    */
    vmm_update_mapping(vmm_pgtable, vector_base_addr_rw+cpu*4096, vector_base_pa+cpu*4096, PROT_PGTABLE);

    /* unmap other threads' stack space
     * each thread has a 4k stack
     * so 1 page and we make sure this thread cannot access it
     */
    for (int i = 0; i < NO_CPUS; i++) {
      if (i == cpu)
        continue;

      vmm_unmap_page(vmm_pgtable, (uint64_t)&stackptr + 4096*i);
    }
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