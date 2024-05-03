
#include "lib.h"

#define VARS x1, x2, y1, y2
#define REGS p0x2, p0x4

static void P0(litmus_test_run* data) {
  asm volatile(
    /* move from C vars into machine regs */
    "mov x0, %[y1desc]\n\t"
    "mov x1, %[x1pte]\n\t"
    "mov x3, %[x1]\n\t"
    "mov x5, %[x2]\n\t"
    /* test */
    "str x0, [x1]\n\t"
    "dsb sy\n\t"
    "ldr x2, [x3]\n\t"
    "ldr x4, [x5]\n\t"
    /* output */
    "str x2, [%[outp0r2]]\n\t"
    "str x4, [%[outp0r4]]\n\t"
    :
    : ASM_VARS(data, VARS), ASM_REGS(data, REGS)
    : "cc", "memory", "x0", "x1", "x2", "x3", "x4", "x5"
  );
}

litmus_test_t CoTT_dsbpopage = {
  "CoTT+dsb-popage",
  MAKE_THREADS(1),
  MAKE_VARS(VARS),
  MAKE_REGS(REGS),
  INIT_STATE(
    7, INIT_VAR(x1, 0), INIT_VAR(x2, 0), INIT_VAR(y1, 1), INIT_VAR(y2, 1), INIT_REGION_PIN(x2, x1, REGION_SAME_PAGE),
    INIT_REGION_PIN(y2, y1, REGION_SAME_PAGE), INIT_REGION_OFFSET(y2, x2, REGION_SAME_PAGE_OFFSET),
  ),
  .interesting_result = (u64[]){
    /* p0:x2 =*/1,
    /* p0:x4 =*/0,
  },
  .requires_pgtable = 1,
  .no_sc_results = 3,
};