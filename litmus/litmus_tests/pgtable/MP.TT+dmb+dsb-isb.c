#include <stdint.h>

#include "lib.h"

#define VARS a, b, c, d
#define REGS p1x0, p1x2

static void P0(litmus_test_run* data) {
  asm volatile (
    /* move from C vars into machine regs */
    "mov x0, %[cdesc]\n\t"
    "mov x1, %[apte]\n\t"
    "mov x2, %[ddesc]\n\t"
    "mov x3, %[bpte]\n\t"

    /* test */
    "str x0, [x1]\n\t"
    "dmb sy\n\t"
    "str x2, [x3]\n\t"
  :
  : ASM_VARS(data, VARS),
    ASM_REGS(data, REGS)
  : "cc", "memory", "x0", "x1", "x2", "x3"
  );
}


static void P1(litmus_test_run* data) {
  asm volatile (
    /* move from C vars into machine regs */
    "mov x1, %[b]\n\t"
    "mov x3, %[a]\n\t"

    /* test payload */
    "ldr x0, [x1]\n\t"
    "dsb sy\n\t"
    "isb\n\t"
    "ldr x2, [x3]\n\t"

    /* save results */
    "str x0, [%[outp1r0]]\n\t"
    "str x2, [%[outp1r2]]\n\t"
  :
  : ASM_VARS(data, VARS),
    ASM_REGS(data, REGS)
  : "cc", "memory", "x0", "x1", "x2", "x3"
  );
}



litmus_test_t MPTT_dmb_dsbisb = {
  "MP.TT+dmb+dsb-isb",
  MAKE_THREADS(2),
  MAKE_VARS(VARS),
  MAKE_REGS(REGS),
  INIT_STATE(
    4,
    INIT_VAR(a, 0),
    INIT_VAR(b, 0),
    INIT_VAR(c, 1),
    INIT_VAR(d, 1),
  ),
  .interesting_result = (uint64_t[]){
      /* p0:x0 =*/1,
      /* p0:x2 =*/0,
  },
  .requires_pgtable = 1,
  .no_sc_results = 3,
};