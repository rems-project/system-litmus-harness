#include "lib.h"

extern void cpu_entry(void);

void spin_cpu_on(uint64_t cpu) {
  *(uint64_t*)boot_data.spin_base[cpu] = (uint64_t)cpu_entry;
}