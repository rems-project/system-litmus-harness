#include "lib.h"

void vmm_mmu_off(void) {
  switch_to_pys_stack();

  asm volatile(
      "mrs x18, SCTLR_EL1\n"
      "mov x19, #0\n"
      "bfi x18, x19, #0, #1\n"
      "msr SCTLR_EL1, x18\n"
      "dsb ish\n"
      "isb\n"
      :
      :
      : "x18", "x19", "memory");

  thread_infos[get_cpu()].mmu_enabled = 0;
  thread_infos[get_cpu()].locking_enabled = 0;
}

void vmm_mmu_on(void) {
  asm volatile(
      "mrs x18, SCTLR_EL1\n"
      "orr x18, x18, #1\n"
      "msr SCTLR_EL1, x18\n"
      "dsb ish\n"
      "isb\n"
      :
      :
      : "x18", "x19", "memory");

  thread_infos[get_cpu()].mmu_enabled = 1;
  thread_infos[get_cpu()].locking_enabled = 1;

  switch_to_vm_stack();
}