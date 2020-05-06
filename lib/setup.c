#include <lib.h>

#include "vmm.h"

extern uint64_t __argc;
extern char*    __argv[100];

void setup(char* fdtloc) {
  fdt = fdtloc;

  current_thread_info()->mmu_enabled = 0;
  current_thread_info()->locking_enabled = 0;

  init_device();
  init_valloc();

  /* read passed args */
  init_args();
  read_args(__argc, __argv);

  debug("setup\n");

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

  debug("setting up CPU0\n");
  cpu_data_init();
  per_cpu_setup(0);

  debug("booting all CPUs ...\n");
  cpu_boot(1);
  cpu_boot(2);
  cpu_boot(3);

  debug("booted.\n");
}

void per_cpu_setup(int cpu) {
  current_thread_info()->mmu_enabled = 0;

  if (ENABLE_PGTABLE) {
    vmm_set_id_translation(vmm_pgtable);
  }

  if (ENABLE_PERF_COUNTS) {
    uint64_t enable = read_sysreg(pmuserenr_el0);
    enable ^= 1UL << 2;  /* Enable Counter aka PMCCNTR_EL0 */
    write_sysreg(enable, pmuserenr_el0);
  }

  current_thread_info()->locking_enabled = 1;

  cpu_data[cpu].to_execute = 0;
  dmb();
  cpu_data[cpu].started = 1;
}