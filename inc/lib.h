#ifndef __LIB__
#define __LIB__

#include <stdint.h>

#include "valloc.h"
#include "sync.h" 
#include "asm.h" 
#include "vmm.h"
#include "exceptions.h"

#define NULL (void*)0


/* QEMU memory mapped addresses */
#define UART0_BASE 0x09000000


/* PSCI Mandatory Functions */
#define PSCI_SYSTEM_OFF 0x84000008UL
#define PSCI_CPU_ON 0xC4000003UL  /* SMC64 starts at 0xC4... */
#define PSCI_CPU_OFF 0x84000002UL

void psci_cpu_on(uint64_t cpu);
void psci_system_off(void);

void abort(void);

/* secondary entry data */
typedef void async_fn_t(int cpu, void* arg);

typedef struct {
    async_fn_t* to_execute;
    void* arg;
    uint64_t started;
    volatile int lock;
} cpu_data_t;

cpu_data_t cpu_data[4];

void cpu_data_init(void);
void cpu_boot(uint64_t cpu);
void run_on_cpu(uint64_t cpu, async_fn_t* fn, void* arg);

/* printf support */
void putc(char c);
void puts(const char *s);
void puthex(uint64_t n);
void putdec(uint64_t n);

/* bitwise operations */
#define BIT(x, i) ((x >> i) & 0x1)
#define BIT_SLICE(x, i, j) ((x >> i) & ((1 << (1 + j - i)) - 1))
#define IS_ALIGNED(v, bits) ((v & ((1UL << bits) - 1)) == 0)
#define ALIGN_POW2(v, to) (v & ~(to - 1))
#define ALIGN_TO(v, bits) (v & ~((1UL << bits) - 1))
#define ALIGN_UP(v, bits) ((v + ((1UL << bits) - 1)) & ~((1UL << bits) - 1))

#endif