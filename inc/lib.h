#ifndef LIB_H
#define LIB_H

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <str.h>

#include "macros.h"
#include "abort.h"
#include "thread_info.h"
#include "argc.h"
#include "bitwise.h"
#include "device.h"
#include "printer.h"
#include "valloc.h"
#include "sync.h"
#include "asm.h"
#include "caches.h"
#include "vmm.h"
#include "exceptions.h"
#include "rand.h"
#include "litmus_test.h"
#include "debug.h"
#include "config.h"
#include "re.h"
#include "driver.h"
#include "stack.h"

const char* version_string(void);

void psci_cpu_on(uint64_t cpu);
void psci_system_off(void);

#define unreachable() do {printf("! unreachable: [%s] %s %d\n", __FILE__, __FUNCTION__, __LINE__); raise_to_el1(); abort();} while (1);

/* some str functions not in <string.h> */
int strstartswith(char* s1, char* prefix);
int strpartition(char* outL, char* outR, char* s, char sep);

/** for configuration in main.c */
typedef struct grp {
    const char* name;
    const litmus_test_t** tests;
    const struct grp** groups;
} litmus_test_group;

/** one-time setup */
void setup(char* fdt);
void ensure_cpus_on(void);
void per_cpu_setup(int cpu);

/* secondary entry data */
typedef void async_fn_t(int cpu, void* arg);

typedef struct {
    async_fn_t* to_execute;
    void* arg;
    uint64_t started;
    volatile int finished;
} cpu_data_t;

cpu_data_t cpu_data[4];

void cpu_data_init(void);
void cpu_boot(uint64_t cpu);
void run_on_cpu(uint64_t cpu, async_fn_t* fn, void* arg);
void run_on_cpus(async_fn_t* fn, void* arg);

/* assorted numeric functions */
int log2(uint64_t n);

#endif /* LIB_H */
