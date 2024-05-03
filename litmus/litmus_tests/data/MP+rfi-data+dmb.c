
#include "lib.h"

#define VARS x, y
#define REGS p0x2, p1x0, p1x2

static void P0(litmus_test_run* data) {
  asm volatile(
    /* load registers */
    "mov x0, #1\n\t"
    "mov x1, %[x]\n\t"
    "mov x3, %[x]\n\t"
    "mov x5, %[y]\n\t"

    /* run test */
    "str x0, [x1]\n\t"
    "ldr x2, [x3]\n\t"
    "mov x4, x2\n\t"
    "str x4, [x5]\n\t"

    "str x2, [%[outp0r2]]\n\t"
    :
    : ASM_VARS(data, VARS), ASM_REGS(data, REGS)
    : "cc", "memory", "x0", "x1", "x2", "x3", "x4", "x5"
  );
}

static void P1(litmus_test_run* data) {
  asm volatile(
    /* load registers */
    "mov x1, %[y]\n\t"
    "mov x3, %[x]\n\t"

    /* run test */
    "ldr x0, [x1]\n\t"
    "dmb sy\n\t"
    "ldr x2, [x3]\n\t"

    /* collect results */
    "str x0, [%[outp1r0]]\n\t"
    "str x2, [%[outp1r2]]\n\t"
    :
    : ASM_VARS(data, VARS), ASM_REGS(data, REGS)
    : "cc", "memory", "x0", "x1", "x2", "x3"
  );
}

litmus_test_t MP_rfidata_dmb = {
  "MP+rfi-data+dmb",
  MAKE_THREADS(2),
  MAKE_VARS(VARS),
  MAKE_REGS(REGS),
  INIT_STATE(2, INIT_VAR(x, 0), INIT_VAR(y, 0), ),
  .interesting_result = (u64[]){
    /* p0:x2 =*/1,
    /* p1:x0 =*/1,
    /* p1:x2 =*/0,
  },

  /* we do not expect p0:x2 to be 0 */
  .no_sc_results = 3,
};