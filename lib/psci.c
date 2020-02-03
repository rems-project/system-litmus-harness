#include <stdint.h>

#include "lib.h"

extern void cpu_entry(void);

__attribute__((noinline))  /* ensure preserve ABI by not inlining */
int __psci_invoke(uint64_t func, uint64_t x1, uint64_t x2, uint64_t x3) {
	asm volatile(
		"hvc #0"
	: "+r" (func)
	: "r" ((void*)x1), "r" ((void*)x2), "r" ((void*)x3));
	return func;  /* return x0 */
}

void psci_cpu_on(uint64_t cpu) {
	int _ = __psci_invoke(PSCI_CPU_ON, cpu, (uint64_t)cpu_entry, 0);
}

void psci_system_off(void) {
	int _ = __psci_invoke(PSCI_SYSTEM_OFF, 0, 0, 0);
}