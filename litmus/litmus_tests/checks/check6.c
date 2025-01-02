
#include "lib.h"

#define VARS x, y
#define REGS p0x0

static void P0(litmus_test_run* data) {
  asm volatile(
    "ldr %[x0], [%[x]]\n\t"
    "mov x2, #2\n\t"
    "str x2, [%[x]]\n\t"
    : [x0] "=&r"(*data->out_reg[0])
    : ASM_VARS(data, VARS), ASM_REGS(data, REGS)
    : "cc", "memory", "x2"
  );
}

static void P1(litmus_test_run* data) {
  asm volatile(
    "ldr x0, [%[x]]\n\t"
    "cbz x0, .after\n\t"
    "str %[ydesc], [%[xpte]]\n\t"
    ".after:\n\t"
    :
    : ASM_VARS(data, VARS), ASM_REGS(data, REGS)
    : "cc", "memory", "x0"
  );
}

litmus_test_t check6 = {
  "check6",
  MAKE_THREADS(2),
  MAKE_VARS(VARS),
  MAKE_REGS(REGS),
  INIT_STATE(2, INIT_VAR(x, 0), INIT_VAR(y, 1)),
  .interesting_result = (u64[]){
    /* p0:x0 =*/1,
  },
  .requires=REQUIRES_PGTABLE,
};