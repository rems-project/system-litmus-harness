#include <stdint.h>

#include "lib.h"

void vmm_flush_tlb_vaddr(uint64_t va) {
  asm volatile("tlbi vae1is, %[va]\n" : : [va] "r"(va));
  dsb();
  isb();
}

void vmm_flush_tlb(void) {
  dsb();  // wait for memory actions to complete
  asm volatile("tlbi vmalle1is\n" ::: "memory");  // clear TLB
  dsb();  // wait for TLB invalidate to complete
  isb();  // synchronize this hardware thread's translations.
}