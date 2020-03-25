#ifndef LIB_H
#define LIB_H

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>

#include "thread_info.h"
#include "argc.h"
#include "bitwise.h"
#include "device.h"
#include "valloc.h"
#include "sync.h" 
#include "asm.h" 
#include "vmm.h"
#include "exceptions.h"
#include "rand.h"
#include "litmus_test.h"
#include "debug.h"
#include "config.h"

uint64_t vector_base_addr;

/* QEMU memory mapped addresses */
#define UART0_BASE 0x09000000

/* PSCI Mandatory Functions */
#define PSCI_SYSTEM_OFF 0x84000008UL
#define PSCI_CPU_ON 0xC4000003UL  /* SMC64 starts at 0xC4... */
#define PSCI_CPU_OFF 0x84000002UL

void psci_cpu_on(uint64_t cpu);
void psci_system_off(void);

void abort(void);
#define unreachable() do {printf("! unreachable: [%s] %s %d\n", __FILE__, __FUNCTION__, __LINE__); raise_to_el1(); abort();} while (1);


/** one-time setup */
void setup(char* fdt);
void per_cpu_setup(int cpu);

/* secondary entry data */
typedef void async_fn_t(int cpu, void* arg);

typedef void litmus_test_fn_t (void);
typedef struct {
    const char* fn_name;
    litmus_test_fn_t* fn;

    uint8_t requires_pgtable;
    uint8_t requires_perf;
} litmus_test_t;

typedef struct {
    async_fn_t* to_execute;
    void* arg;
    uint64_t started;
    int count;
} cpu_data_t;

cpu_data_t cpu_data[4];

void cpu_data_init(void);
void cpu_boot(uint64_t cpu);
void run_on_cpu(uint64_t cpu, async_fn_t* fn, void* arg);
void run_on_cpus(async_fn_t* fn, void* arg);

/* printf support */
void putc(char c);
void puts(const char *s);
void puthex(uint64_t n);
void putdec(uint64_t n);

void printf(const char* fmt, ...);
void trace(const char* fmt, ...);

#endif /* LIB_H */
