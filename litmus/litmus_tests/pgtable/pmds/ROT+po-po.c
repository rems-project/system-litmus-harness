
#include "lib.h"

#define VARS x, y, z
#define REGS p0x4

static void P0(litmus_test_run* data) {
  /* assuming x, y initialised to 1, 2 */
  asm volatile (
    /* move from C vars into machine regs */
      "mov x0, %[zdesc]\n\t"
      "mov x1, %[ypte]\n\t"
      "mov x2, %[ypmddesc]\n\t"
      "mov x3, %[xpmd]\n\t"
      "mov x5, %[x]\n\t"

      /* test */
      "str x0, [x1]\n\t"
      "str x2, [x3]\n\t"
      "ldr x4, [x5]\n\t"

      /* output back to C vars */
      "str x4, [%[outp0r4]]\n\t"
  :
  : ASM_VARS(data, VARS),
    ASM_VAR_PMDs(data, VARS),
    ASM_VAR_PMDDESCs(data, VARS),
    ASM_REGS(data, REGS)
  : "cc", "memory", "x0", "x1", "x2", "x3", "x4", "x5"
  );
}

litmus_test_t ROT_popo = {
  "ROT+po-po",
  MAKE_THREADS(1),
  MAKE_VARS(VARS),
  MAKE_REGS(REGS),
  INIT_STATE(
    8,
    INIT_VAR(x, 0),
    INIT_VAR(y, 1),
    INIT_VAR(z, 2),
    INIT_REGION_OWN(x, REGION_OWN_PMD),
    INIT_REGION_PIN(y, x, REGION_SAME_PMD),
    INIT_REGION_OFFSET(y, x, REGION_SAME_PAGE_OFFSET),
    INIT_REGION_OWN(z, REGION_OWN_PMD),
    INIT_REGION_OFFSET(z, x, REGION_SAME_PMD_OFFSET),
  ),
  .interesting_result = (u64[]){
      /* p0:x2 =*/1,
  },
  .requires_pgtable = 1,
  .no_sc_results = 2,
};