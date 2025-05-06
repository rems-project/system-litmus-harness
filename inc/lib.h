#ifndef LIB_H
#define LIB_H

#include <stddef.h>
#include <stdarg.h>
#include <str.h>

#include "ints.h"
#include "macros.h"
#include "abort.h"
#include "thread_info.h"
#include "argc.h"
#include "bitwise.h"
#include "device.h"
#include "arm_features.h"
#include "hashlib.h"
#include "printer.h"
#include "valloc/valloc_ptable.h"
#include "valloc/valloc_generic.h"
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
#include "stack.h"
#include "boot.h"

#include "drivers/driver.h"

const char* version_string(void);
const char* build_string(void);

#define unreachable()                                                                         \
  do {                                                                                        \
    warning(WARN_UNREACHABLE, "unreachable: [%s] %s %d\n", __FILE__, __FUNCTION__, __LINE__); \
    raise_to_el1();                                                                           \
    abort();                                                                                  \
  } while (1);

/* some str functions not in <string.h> */
extern int strstartswith(char* s1, char* prefix);
extern int strpartition(char* outL, char* outR, char* s, char sep);

/** for configuration in main.c */
typedef struct grp
{
  const char* name;
  const litmus_test_t** tests;
  const struct grp** groups;
} litmus_test_group;

/** one-time setup */
extern void setup(char* fdt);
extern void ensure_cpus_on(void);
extern void per_cpu_setup(int cpu);

/* secondary entry data */
typedef void async_fn_t(int cpu, void* arg);

typedef struct
{
  async_fn_t* to_execute;
  void* arg;
  u64 started;
  volatile int finished;
} cpu_data_t;

extern cpu_data_t cpu_data[4];

extern void cpu_data_init(void);
extern void cpu_boot(u64 cpu);
extern void run_on_cpu(u64 cpu, async_fn_t* fn, void* arg);
extern void run_on_cpus(async_fn_t* fn, void* arg);

/* assorted numeric functions */
extern int log2(u64 n);

extern u32 read_be(char* p);
extern void write_be(char* p, u32 v);

extern u64 read_be64(char* p);
extern void write_be64(char* p, u64 v);

/* base64 helpers */
u64 b64decode(const char* data, u64 len, char* out);

#endif /* LIB_H */
