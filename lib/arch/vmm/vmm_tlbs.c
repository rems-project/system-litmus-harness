
#include "lib.h"

void tlbi_va(u64 va) {
  u64 page = va >> 12;
  asm volatile("tlbi vaae1is, %[va]\n" : : [va] "r"(page) : "memory");

  if (cpu_needs_workaround(ERRATA_WORKAROUND_REPEAT_TLBI)) {
    dsb();
    asm volatile("tlbi vaae1is, %[va]\n" : : [va] "r"(page) : "memory");
  }
}

void tlbi_asid(u64 asid) {
  u64 reg = (asid & 0xff) << 48;
  asm volatile("tlbi aside1is, %[asid]\n" : : [asid] "r"(reg) : "memory");

  if (cpu_needs_workaround(ERRATA_WORKAROUND_REPEAT_TLBI)) {
    dsb();
    asm volatile("tlbi aside1is, %[asid]\n" : : [asid] "r"(reg) : "memory");
  }
}

void tlbi_all(void) {
  asm volatile("tlbi vmalle1is\n" ::: "memory");

  if (cpu_needs_workaround(ERRATA_WORKAROUND_REPEAT_TLBI)) {
    dsb();
    asm volatile("tlbi vmalle1is\n" ::: "memory");
  }
}

void vmm_flush_tlb_vaddr(u64 va) {
  dsb();
  if (cpu_needs_workaround(ERRATA_WORKAROUND_ISB_AFTER_PTE_ST))
    isb();
  tlbi_va(va);
  dsb();
  isb();
}

void vmm_flush_tlb_asid(u64 asid) {
  dsb();
  if (cpu_needs_workaround(ERRATA_WORKAROUND_ISB_AFTER_PTE_ST))
    isb();
  tlbi_asid(asid);
  dsb();
  isb();
}

void vmm_flush_tlb(void) {
  dsb(); // wait for memory actions to complete
  if (cpu_needs_workaround(ERRATA_WORKAROUND_ISB_AFTER_PTE_ST))
    isb();
  tlbi_all();
  dsb(); // wait for TLB invalidate to complete
  isb(); // synchronize this hardware thread's translations.
}