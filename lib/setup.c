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
  debug("%p: TOP_OF_MEM\n", TOP_OF_MEM);
  PR_BARS(TOP_OF_MEM, TOP_OF_STACK);
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
    vmm_pgtable = vmm_alloc_new_idmap_4k();
    /* re-map vector_base_addr to some non-executable mapping of the vector table
    */
    vector_base_addr_rw = (uint64_t)alloc(4096*4);
    for (int i = 0; i < 4; i++) {
      vmm_update_mapping(vmm_pgtable, vector_base_addr_rw+i*4096, vector_base_pa+i*4096, PROT_PGTABLE);
    }
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
  current_thread_info()->mmu_enabled = 0;

  if (ENABLE_PGTABLE) {
    vmm_set_id_translation(vmm_pgtable);
  }

  if (ENABLE_PERF_COUNTS) {
    /* enable virtual/physical timers */
    write_sysreg((1 << 0) | (1 << 1), cntkctl_el1);

    /* enable PMU cycle counter */
    write_sysreg((1UL << 27), pmccfiltr_el0);
    write_sysreg((1UL << 6) | (1UL << 0), pmcr_el0);  /* reset to 0 */
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