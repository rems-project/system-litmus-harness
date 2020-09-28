#include <stdint.h>

#include "lib.h"

#define VARS x, y
#define REGS p0x2

static void P0(litmus_test_run* data) {
  asm volatile (
    /* change  */
    "mov x0, #1\n\t"
    "mov x1, %[y]\n\t"
    "mov x3, %[x]\n\t"

    /* test */
    "str x0, [x1]\n\t"
    "dsb sy\n\t"
    "ldr x2, [x3]\n\t"

    /* output */
    "str x2, [%[outp0r2]]\n\t"
  :
  : ASM_VARS(data, VARS),
    ASM_REGS(data, REGS)
  : "cc", "memory", "x0", "x1", "x2", "x3"
  );
}

litmus_test_t WRWARANC_dsb = {
  "WR.WARA-NC+dsb",
  MAKE_THREADS(1),
  MAKE_VARS(VARS),
  MAKE_REGS(REGS),
  INIT_STATE(
    4,
    INIT_VAR(x, 0),
    /* make x be a cacheable mapping */
    INIT_PERMISSIONS(x, PROT_ATTRIDX, PROT_ATTR_NORMAL_RA_WA),
    /* set y to be a non-cacheable alias to x */
    INIT_ALIAS(y, x),
    INIT_PERMISSIONS(y, PROT_ATTRIDX, PROT_ATTR_NORMAL_NC),
  ),
  .interesting_result = (uint64_t[]){
      /* p0:x4 =*/0,
  },
  .requires_pgtable = 1,
  .no_sc_results = 1,
};