
#include "lib.h"

#define VARS x, y
#define REGS p0x2

static void P0(litmus_test_run* data) {
  /* assuming x, y initialised to 1, 2 */
  asm volatile(
    /* move from C vars into machine regs */
    "mov x0, %[ypmddesc]\n\t"
    "mov x1, %[xpmd]\n\t"
    "mov x3, %[x]\n\t"

    /* test */
    "str x0, [x1]\n\t"
    "dsb sy\n\t"
    "isb\n\t"
    "ldr x2, [x3]\n\t"

    /* output back to C vars */
    "str x2, [%[outp0r2]]\n\t"
    :
    : ASM_VARS(data, VARS), ASM_VAR_PMDs(data, VARS), ASM_VAR_PMDDESCs(data, VARS), ASM_REGS(data, REGS)
    : "cc", "memory", "x0", "x1", "x2", "x3"
  );
}

litmus_test_t CoWTL23_dsbisb = {
  "CoWT.L23+dsb-isb",
  MAKE_THREADS(1),
  MAKE_VARS(VARS),
  MAKE_REGS(REGS),
  INIT_STATE(
    4, INIT_VAR(x, 0), INIT_VAR(y, 1), INIT_REGION_OWN(x, REGION_OWN_PMD),
    INIT_REGION_OFFSET(y, x, REGION_SAME_PMD_OFFSET),
  ),
  .interesting_result = (u64[]){
    /* p0:x2 =*/0,
  },
  .requires=REQUIRES_PGTABLE,
  .no_sc_results = 1,
};