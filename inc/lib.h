#include <stdint.h>

/* QEMU memory mapped addresses */
#define UART0_BASE 0x09000000


/* PSCI Mandatory Functions */
#define PSCI_SYSTEM_OFF 0x84000008
#define PSCI_CPU_ON 0xC4000003  /* SMC64 starts at 0xC4... */
#define PSCI_CPU_OFF 0x84000002

void psci_cpu_on(uint64_t cpu);
void psci_system_off(void);

void abort(void);

/* secondary entry data */
typedef void async_fn_t(int cpu);

typedef struct {
    async_fn_t* to_execute;
    uint64_t started;
} cpu_data_t;

cpu_data_t cpu_data[4];

void cpu_boot(uint64_t cpu);
void run_on_cpu(uint64_t cpu, async_fn_t* fn);

/* printf support */
void putc(char c);
void puts(const char *s);
void puthex(uint64_t n);


/* barriers and wrappers */
void wfe(void);
void dsb(void);
void dmb(void);