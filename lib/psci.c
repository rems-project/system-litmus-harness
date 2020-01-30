#include <stdint.h>

#include "lib.h"

int psci_invoke(uint64_t func, uint64_t x1, uint64_t x2, uint64_t x3) {
	asm volatile(
		"hvc #0"
	: "+r" (func)
	: "r" (x1), "r" (x2), "r" (x3));
	return func;  /* return x0 */
}

void psci_cpu_on(uint64_t cpuid, uint64_t va_entry)
{
	int _ = psci_invoke(PSCI_CPU_ON, cpuid, va_entry, 0);
}

void psci_system_off(void)
{
	int _ = psci_invoke(PSCI_SYSTEM_OFF, 0, 0, 0);
}