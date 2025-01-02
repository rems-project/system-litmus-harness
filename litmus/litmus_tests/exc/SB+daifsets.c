#include "lib.h"

#define VARS x, y
#define REGS p0x2, p1x2

static void P0(litmus_test_run* data) {
  asm volatile (
    /* initial registers */
    "mov x0, #1\n\t"
    "mov x1, %[x]\n\t"
    "mov x3, %[y]\n\t"

    /* test */
    "str x0, [x1]\n\t"

    "msr daifset, #15\n\t"

    "ldr x2, [x3]\n\t"

    /* extract values */
    "str x2, [%[outp0r2]]\n\t"
  :
  : ASM_VARS(data, VARS),
    ASM_REGS(data, REGS)
  : "cc", "memory", "x0", "x1", "x2", "x3"
  );
}

static void P1(litmus_test_run* data) {
  asm volatile (
    /* initial registers */
    "mov x0, #1\n\t"
    "mov x1, %[y]\n\t"
    "mov x3, %[x]\n\t"

    /* test */
    "str x0, [x1]\n\t"

    "msr daifset, #5\n\t"

    "ldr x2, [x3]\n\t"

    /* extract values */
    "str x2, [%[outp1r2]]\n\t"
  :
  : ASM_VARS(data, VARS),
    ASM_REGS(data, REGS)
  : "cc", "memory", "x0", "x1", "x2", "x3"
  );
}

litmus_test_t SB_daifsets = {
  "SB+daifsets",
  MAKE_THREADS(2),
  MAKE_VARS(VARS),
  MAKE_REGS(REGS),
  INIT_STATE(
    2,
    INIT_VAR(x, 0),
    INIT_VAR(y, 0)
  ),
  .start_els=(int[]){1,1},
  .thread_sync_handlers = (u32**[]){
     (u32*[]){NULL, NULL},
     (u32*[]){NULL, NULL},
  },
  .interesting_result = (u64[]){
      /* p0:x2 =*/0,
      /* p1:x2 =*/0,
  },
  .no_sc_results=3,
  .expected_allowed = (arch_allow_st[]) {
    {"armv8", OUTCOME_UNKNOWN},
  }
};
