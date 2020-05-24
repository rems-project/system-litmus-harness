#include <stdint.h>

#include "lib.h"

#define VARS x, y
#define REGS p0x2, p1x2

static void P0(litmus_test_run* data) {
  asm volatile (
    "mov x1, %[x]\n\t"
    "mov x2, #1\n\t"
    "mov x3, %[y]\n\t"
    /* test */
    "ldr x0, [x1]\n\t"
    "str x2, [x3]\n\t"
    "str x0, [%[outp0r2]]\n\t"
  : 
  : ASM_VARS(data, VARS),
    ASM_REGS(data, REGS)
  : "cc", "memory", "x0", "x1", "x2", "x3"
  );
}

static void P1(litmus_test_run* data) {
  asm volatile (
    "mov x1, %[y]\n\t"
    "mov x2, #1\n\t"
    "mov x3, %[x]\n\t"
    /* test */
    "ldr x0, [x1]\n\t"
    "str x2, [x3]\n\t"
    "str x0, [%[outp1r2]]\n\t"
  : 
  : ASM_VARS(data, VARS),
    ASM_REGS(data, REGS)
  : "cc", "memory", "x0", "x1", "x2", "x3"
  );
}



litmus_test_t LB_pos = {
  "LB+pos",
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
  .no_sc_results = 3,
};