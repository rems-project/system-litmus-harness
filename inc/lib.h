#include <stdint.h>

/* QEMU memory mapped addresses */
#define UART0_BASE 0x09000000


/* PSCI Mandatory Functions */
#define PSCI_SYSTEM_OFF 0x84000008
#define PSCI_CPU_ON 0xC4000003  /* SMC64 starts at 0xC4... */
#define PSCI_CPU_OFF 0x84000002

void psci_cpu_on(uint64_t cpuid, uint64_t va_entry);
void psci_system_off(void);

void abort(void);

/* printf support */
void putc(char c);
void puts(const char *s);
void puthex(uint64_t n);