
#include "lib.h"

#define VARS x, y
#define REGS p1x0, p1x2

static void P0(litmus_test_run* data) {
  asm volatile(
    LITMUS_START_ASM
    "mov x0, #1\n\t"
    "str x0, [%[x]]\n\t"
    "mov x2, #1\n\t"
    "str x2, [%[y]]\n\t"
    LITMUS_END_ASM
    :
    : ASM_VARS(data, VARS), ASM_REGS(data, REGS)
    : "cc", "memory", "x0", "x2"
  );
}

static void P1(litmus_test_run* data) {
  asm volatile(
    LITMUS_START_ASM
    "ldr x0, [%[y]]\n\t"
    "ldr x2, [%[x]]\n\t"
    LITMUS_END_ASM

    "str x0, [%[outp1r0]]\n\t"
    "str x2, [%[outp1r2]]\n\t"
    :
    : ASM_VARS(data, VARS), ASM_REGS(data, REGS)
    : "cc", "memory", "x0", "x2"
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
  .hash="123d1aa81c7a71c5c955b7d855015e48be52ec94",
};