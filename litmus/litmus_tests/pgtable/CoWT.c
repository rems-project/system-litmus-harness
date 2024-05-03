
#include "lib.h"

#define VARS x, y
#define REGS p0x2

static void P0(litmus_test_run* data) {
  /* assuming x, y initialised to 1, 2 */
  asm volatile(
    /* move from C vars into machine regs */
    "mov x0, %[ydesc]\n\t"
    "mov x1, %[xpte]\n\t"
    "mov x3, %[x]\n\t"

    /* test */
    "str x0, [x1]\n\t"
    "ldr x2, [x3]\n\t"

    /* output back to C vars */
    "str x2, [%[outp0r2]]\n\t"
    :
    : ASM_VARS(data, VARS), ASM_REGS(data, REGS)
    : "cc", "memory", "x0", "x1", "x2", "x3"
  );
}

litmus_test_t CoWT = {
  "CoWT",
  MAKE_THREADS(1),
  MAKE_VARS(VARS),
  MAKE_REGS(REGS),
  INIT_STATE(2, INIT_VAR(x, 1), INIT_VAR(y, 2)),
  .interesting_result = (u64[]){
    /* p0:x2 =*/1,
  },
  .requires_pgtable = 1,
  .no_sc_results = 1,
};