
#include "lib.h"

#define VARS x, y
#define REGS p1x0, p1x2

static void P0(litmus_test_run* data) {
  asm volatile(
    "mov x1, %[x]\n\t"
    "mov x3, %[y]\n\t"

    LITMUS_START_ASM
    "mov x0, #1\n\t"
    "str x0, [x1]\n\t"
    "mov x2, #1\n\t"
    "str x2, [x3]\n\t"
    LITMUS_END_ASM
    :
    : ASM_VARS(data, VARS), ASM_REGS(data, REGS)
    : "cc", "memory", "x0", "x1", "x2", "x3"
  );
}

static void P1(litmus_test_run* data) {
  asm volatile(
    "mov x1, %[y]\n\t"
    "mov x3, %[x]\n\t"

    LITMUS_START_ASM
    "ldr x0, [x1]\n\t"
    "ldr x2, [x3]\n\t"
    LITMUS_END_ASM

    "str x0, [%[outp1r0]]\n\t"
    "str x2, [%[outp1r2]]\n\t"
    :
    : ASM_VARS(data, VARS), ASM_REGS(data, REGS)
    : "cc", "memory", "x0", "x1", "x2", "x3"
  );
}

litmus_test_t MP_pos = {
  "MP+pos",
  MAKE_THREADS(2),
  MAKE_VARS(VARS),
  MAKE_REGS(REGS),
  INIT_STATE(2, INIT_VAR(x, 0), INIT_VAR(y, 0), ),
  .interesting_result = (u64[]){
    /* p1:x0 =*/1,
    /* p1:x2 =*/0,
  },
  .no_sc_results = 3,
  .hash="7b09ead834cb4ed71899e3f0470d437dd1b3c894",
};