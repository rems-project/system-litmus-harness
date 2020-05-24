#include <stdint.h>

#include "lib.h"

#define VARS x, y
#define REGS p0x3

static void P0(litmus_test_run* data) {
  /* assuming x, y initialised to 1, 2 */
  asm volatile (
    /* move from C vars into machine regs */
    "mov x0, %[ydesc]\n\t"
    "mov x1, %[xpte]\n\t"
    "mov x2, %[xpage]\n\t"
    "mov x4, %[x]\n\t"

    /* test */
    "str x0, [x1]\n\t"
    "dsb sy\n\t"
    "tlbi vae1, x2\n\t"
    "dsb sy\n\t"
    "isb\n\t"
    "ldr x3, [x4]\n\t"

    /* output back to C vars */
    "str x3, [%[outp0r3]]\n\t"
  : 
  : ASM_VARS(data, VARS),
    ASM_REGS(data, REGS)
  : "cc", "memory", "x0", "x1", "x2", "x3", "x4"
  );
}


litmus_test_t CoWT1_dsbtlbidsbisb = {
  "CoWT1+dsb-tlbi-dsb-isb",
  MAKE_THREADS(1),
  MAKE_VARS(VARS),
  MAKE_REGS(REGS),
  INIT_STATE(
    2,
    INIT_VAR(x, 0),
    INIT_VAR(y, 1)
  ),
  .interesting_result = (uint64_t[]){
      /* p0:x3 =*/0,
  },
  .start_els=(int[]){1,},
  .requires_pgtable=1,
  .no_sc_results = 1,
};