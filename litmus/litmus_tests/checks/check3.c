#include <stdint.h>

#include "lib.h"

#define VARS x, y
#define REGS p1x0, p1x2

/* add hooks for exc reg */
#define USER_exc "exc"
#define IDENT_exc exc

static void P0(litmus_test_run* data) {
  asm volatile (
    "ldr %[x0], [%[x]]\n\t"
    "dmb sy\n\t"
    "mov x2, #1\n\t"
    "str x2, [%[y]]\n\t"
    "dmb sy\n\t"
    "str x2, [%[x]]\n\t"
  : [x0] "=&r" (*data->out_reg[0])
  : ASM_VARS(data, VARS),
    ASM_REGS(data, REGS)
  : "cc", "memory", "x0", "x2"
  );
}

static void P1(litmus_test_run* data) {
  asm volatile (
    "ldr %[x0], [%[y]]\n\t"
    "dmb sy\n\t"
    "mov x2, #1\n\t"
    "str x2, [%[x]]\n\t"
    "dmb sy\n\t"
    "str x2, [%[y]]\n\t"
  : [x0] "=&r" (*data->out_reg[1])
  : ASM_VARS(data, VARS),
    ASM_REGS(data, REGS)
  : "cc", "memory", "x0", "x2"
  );
}


litmus_test_t check3 = {
  "check3",
  MAKE_THREADS(2),
  MAKE_VARS(VARS),
  MAKE_REGS(REGS),
  INIT_STATE(
    2,
    INIT_VAR(x, 0),
    INIT_VAR(y, 0)
  ),
  .interesting_result =
    (uint64_t[]){
      /* p1:x0 =*/ 1,
      /* p1:x2 =*/ 1,
  },
};