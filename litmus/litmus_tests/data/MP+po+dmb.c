
#include "lib.h"

#define VARS x, y
#define REGS p1x0, p1x2

static void P0(litmus_test_run* data) {
  asm volatile(
    "mov x0, #1\n\t"
    "str x0, [%[x]]\n\t"
    "mov x2, #1\n\t"
    "str x2, [%[y]]\n\t"
    :
    : ASM_VARS(data, VARS), ASM_REGS(data, REGS)
    : "cc", "memory", "x0", "x2"
  );
}

static void P1(litmus_test_run* data) {
  asm volatile(
    "ldr %[x0], [%[y]]\n\t"
    "dmb sy\n\t"
    "ldr %[x2], [%[x]]\n\t"
    : [x0] "=&r"(*data->out_reg[0]), [x2] "=&r"(*data->out_reg[1])
    : ASM_VARS(data, VARS), ASM_REGS(data, REGS)
    : "cc", "memory"
  );
}

litmus_test_t MP_po_dmb = {
  "MP+po+dmb",
  MAKE_THREADS(2),
  MAKE_VARS(VARS),
  MAKE_REGS(REGS),
  INIT_STATE(2, INIT_VAR(x, 0), INIT_VAR(y, 0)),
  .interesting_result = (u64[]){
    /* p1:x0 =*/1,
    /* p1:x2 =*/0,
  },
  .no_sc_results = 3,
};