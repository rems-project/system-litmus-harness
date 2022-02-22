
#include "lib.h"

extern void cpu_entry(void);

__attribute__((noinline))  /* ensure preserve ABI by not inlining */
void __psci_invoke(u64 func, u64 x1, u64 x2, u64 x3) {
	asm volatile(
		"hvc #0"
	: "+r" (func)
	: "r" ((void*)x1), "r" ((void*)x2), "r" ((void*)x3));
	/* could return something, but ignoring it */
}

void psci_cpu_on(u64 cpu) {
	__psci_invoke(PSCI_CPU_ON, cpu, (u64)cpu_entry, 0);
}

void psci_system_off(void) {
	__psci_invoke(PSCI_SYSTEM_OFF, 0, 0, 0);
}