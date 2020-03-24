#include <lib.h>

#include "vmm.h"

extern uint64_t __argc;
extern char*    __argv[100];

void setup(char* fdtloc) {
  fdt = fdtloc;

  current_thread_info()->mmu_enabled = 0;
  current_thread_info()->printer_lock_enabled = 0;
  
  init_device();
  init_valloc();
  
  /* read passed args */
  init_args(); 
  read_args(__argc, __argv);
  
  vector_base_addr = (uint64_t)&el1_exception_vector_table_p0;
  
  /* create pgtable */
  if (ENABLE_PGTABLE) {
    vmm_pgtable = vmm_alloc_new_idmap_4k();
    vmm_set_id_translation(vmm_pgtable);
  }
  
  cpu_data_init();
  per_cpu_setup(0);
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
  
  current_thread_info()->printer_lock_enabled = 1;
  
  cpu_data[cpu].to_execute = 0;
  dmb();
  cpu_data[cpu].started = 1;
}