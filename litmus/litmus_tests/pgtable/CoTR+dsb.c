#include <stdint.h>

#include "lib.h"

#define VARS x, y
#define REGS p1x0, p1x2

static void P0(litmus_test_run* data) {
  asm volatile (
    /* move from C vars into machine regs */
    "mov x0, %[ydesc]\n\t"
    "mov x1, %[xpte]\n\t"
    /* test */
    "str x0, [x1]\n\t"
  : 
  : ASM_VARS(data, VARS),
    ASM_REGS(data, REGS)
  : "cc", "memory", "x0", "x1"
  );
}


static void P1(litmus_test_run* data) {
  asm volatile (
    /* move from C vars into machine regs */
    "mov x1, %[x]\n\t"
    "mov x3, %[xpte]\n\t"
    /* test payload */
    "ldr x0, [x1]\n\t"
    "dsb sy\n\t"
    "ldr x2, [x3]\n\t"
    /* save results */
    "str x0, [%[outp1r0]]\n\t"
    "eor x2, x2, %[ydesc]\n\t"
    "cbz x2, .after\n\t"
    "mov x2, #1\n\t"
    ".after:\n\t"
    "eor x2, x2, #1\n\t"
    "str x2, [%[outp1r2]]\n\t"
  : 
  : ASM_VARS(data, VARS),
    ASM_REGS(data, REGS)
  : "cc", "memory", "x0", "x1", "x2", "x3"
  );
}



litmus_test_t CoTR_dsb = {
  "CoTR+dsb",
  MAKE_THREADS(2),
  MAKE_VARS(VARS),
  MAKE_REGS(REGS),
  INIT_STATE(
    2,
    INIT_VAR(x, 0),
    INIT_VAR(y, 1)
  ),
  .interesting_result = (uint64_t[]){
      /* p0:x0 =*/1,
      /* p0:x2 =*/0,
  },
  .requires_pgtable = 1,
  .no_sc_results = 3,
};