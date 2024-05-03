
#include "lib.h"

void tlbi_va(u64 va) {
  u64 page = va >> 12;
  asm volatile("tlbi vaae1is, %[va]\n" : : [va] "r"(page) : "memory");
}

void tlbi_asid(u64 asid) {
  u64 reg = (asid & 0xff) << 48;
  asm volatile("tlbi aside1is, %[asid]\n" : : [asid] "r"(reg) : "memory");
}

void tlbi_all(void) {
  asm volatile("tlbi vmalle1is\n" ::: "memory");
}

void vmm_flush_tlb_vaddr(u64 va) {
  dsb();
  tlbi_va(va);
  dsb();
  isb();
}

void vmm_flush_tlb_asid(u64 asid) {
  dsb();
  tlbi_asid(asid);
  dsb();
  isb();
}

void vmm_flush_tlb(void) {
  dsb(); // wait for memory actions to complete
  tlbi_all();
  dsb(); // wait for TLB invalidate to complete
  isb(); // synchronize this hardware thread's translations.
}