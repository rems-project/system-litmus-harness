
#include "lib.h"

#define VARS x, y
#define REGS p1x0, p1x2, time_str0, time_str1, time_ldr0, time_ldr1

/* add hooks for time regs */
#define USER_time_str0 "time_str0"
#define IDENT_time_str0 tstr0
#define USER_time_str1 "time_str1"
#define IDENT_time_str1 tstr1
#define USER_time_ldr0 "time_ldr0"
#define IDENT_time_ldr0 tldr0
#define USER_time_ldr1 "time_ldr1"
#define IDENT_time_ldr1 tldr1


static void P0(litmus_test_run* data) {
  printf("pmccntr = %p\n", read_sysreg(pmccntr_el0));
  asm volatile (
    "mov x0, #1\n\t"
    "mrs x5, pmccntr_el0\n\t"
    "str x0, [%[x]]\n\t"
    "mrs x6, pmccntr_el0\n\t"
    "mov x2, #1\n\t"
    "str x2, [%[y]]\n\t"
    "mrs x7, pmccntr_el0\n\t"
    "sub x7, x6, x7\n\t"
    "sub x6, x5, x6\n\t"
    "str x6, [%[tstr0]]\n\t"
    "str x7, [%[tstr1]]\n\t"
  :
  : ASM_VARS(data, VARS),
    ASM_REGS(data, REGS)
  : "cc", "memory", "x0", "x2", "x5", "x6", "x7"
  );
}

static void P1(litmus_test_run* data) {
  u64* x0 = data->out_reg[0];
  u64* x2 = data->out_reg[1];
  asm volatile (
    "mrs x5, pmccntr_el0\n\t"
    "ldr %[x0], [%[y]]\n\t"
    "mrs x6, pmccntr_el0\n\t"
    "ldr %[x2], [%[x]]\n\t"
    "mrs x7, pmccntr_el0\n\t"
    "sub x7, x6, x7\n\t"
    "sub x6, x5, x6\n\t"
    "str x6, [%[tldr0]]\n\t"
    "str x7, [%[tldr1]]\n\t"
  : [x0] "=&r" (*x0), [x2] "=&r" (*x2)
  : ASM_VARS(data, VARS),
    ASM_REGS(data, REGS)
  : "cc", "memory", "x5", "x6", "x7"
  );
}


litmus_test_t MPtimes_pos = {
  "MP.times+pos",
  MAKE_THREADS(2),
  MAKE_VARS(VARS),
  MAKE_REGS(REGS),
  INIT_STATE(
    2,
    INIT_VAR(x, 0),
    INIT_VAR(y, 0)
  ),
  .interesting_result = NULL,
  .requires_perf = 1,
};
