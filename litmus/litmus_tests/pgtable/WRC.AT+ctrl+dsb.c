
#include "lib.h"

#define VARS x, y, z
#define REGS p1x2, p2x0, p2x2

static void P0(litmus_test_run* data) {
  asm volatile (
    "mov x0, %[zdesc]\n\t"
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
    "mov x1, %[xpte]\n\t"
    "mov x2, 1\n\t"
    "mov x3, %[y]\n\t"
    "mov x4, %[zdesc]\n\t"
    /* test */
    "ldr x0, [x1]\n\t"
    /* because the post state cannot read pte(z) (yet)
     * have this hack to work out if they were equal here */
    "eor x0, x4, x0\n\t"
    "cbz x0, .after\n\t"
    "mov x0, #1\n\t"
    ".after:\n\t"
    "str x2, [x3]\n\t"
    /* output */
    "str x0, [%[outp1r2]]\n\t"
  :
  : ASM_VARS(data, VARS),
    ASM_REGS(data, REGS)
  : "cc", "memory", "x0", "x1", "x2", "x3", "x4"
  );
}

static void P2(litmus_test_run* data) {
  asm volatile (
    "mov x1, %[y]\n\t"
    "mov x3, %[x]\n\t"

    /* test */
    "ldr x0, [x1]\n\t"
    "dsb sy\n\t"
    "ldr x2, [x3]\n\t"

    /* output */
    "str x0, [%[outp2r0]]\n\t"
    "str x2, [%[outp2r2]]\n\t"
  :
  : ASM_VARS(data, VARS),
    ASM_REGS(data, REGS)
  : "cc", "memory", "x0", "x1", "x2", "x3"
  );
}




litmus_test_t WRCat_ctrl_dsb = {
  "WRC.AT+ctrl+dsb",
  MAKE_THREADS(3),
  MAKE_VARS(VARS),
  MAKE_REGS(REGS),
  INIT_STATE(
    3,
    INIT_VAR(x, 0),
    INIT_VAR(y, 0),
    INIT_VAR(z, 1)
  ),
   .interesting_result =
    (u64[]){
      /* p1:x2 =*/ 0,
      /* p2:x0 =*/ 1,
      /* p2:x2 =*/ 0,
    },
  .requires_pgtable=1,
  .no_sc_results = 7,
};
