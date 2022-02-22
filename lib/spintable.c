#include "lib.h"

extern void cpu_entry(void);

void spin_cpu_on(u64 cpu) {
  *(u64*)boot_data.spin_base[cpu] = (u64)cpu_entry;
}