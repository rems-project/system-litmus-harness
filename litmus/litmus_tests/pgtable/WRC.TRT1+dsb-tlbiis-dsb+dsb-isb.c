
#include "lib.h"

#define VARS x, y, z
#define REGS p1x0, p2x0, p2x2

static void P0(litmus_test_run* data) {
  asm volatile(
    "mov x0, %[zdesc]\n\t"
    "mov x1, %[xpte]\n\t"
    /* test */
    "str x0, [x1]\n\t"
    :
    : ASM_VARS(data, VARS), ASM_REGS(data, REGS)
    : "cc", "memory", "x0", "x1"
  );
}

static void P1(litmus_test_run* data) {
  asm volatile(
    "mov x1, %[x]\n\t"
    "mov x2, 1\n\t"
    "mov x3, %[y]\n\t"
    "mov x4, %[xpage]\n\t"
    /* test */
    "ldr x0, [x1]\n\t"
    "dsb sy\n\t"
    "tlbi vae1is,x4\n\t"
    "dsb sy\n\t"
    "str x2, [x3]\n\t"
    /* output */
    "str x0, [%[outp1r0]]\n\t"
    :
    : ASM_VARS(data, VARS), ASM_REGS(data, REGS)
    : "cc", "memory", "x0", "x1", "x2", "x3", "x4"
  );
}

static void P2(litmus_test_run* data) {
  asm volatile(
    /* initial regsiters */
    "mov x1, %[y]\n\t"
    "mov x3, %[x]\n\t"

    /* test */
    "ldr x0, [x1]\n\t"
    "dsb sy\n\t"
    "isb\n\t"
    "ldr x2, [x3]\n\t"

    /* output */
    "str x0, [%[outp2r0]]\n\t"
    "str x2, [%[outp2r2]]\n\t"
    :
    : ASM_VARS(data, VARS), ASM_REGS(data, REGS)
    : "cc", "memory", "x0", "x1", "x2", "x3"
  );
}

litmus_test_t WRCtrt1_dsbtlbiisdsb_dsbisb = {
  "WRC.TRT1+dsb-tlbiis-dsb+dsb-isb",
  MAKE_THREADS(3),
  MAKE_VARS(VARS),
  MAKE_REGS(REGS),
  INIT_STATE(3, INIT_VAR(x, 0), INIT_VAR(y, 0), INIT_VAR(z, 1)),
  .start_els = (int[]){ 0, 1, 0 },
  .interesting_result = (u64[]){
    /* p1:x2 =*/1,
    /* p2:x0 =*/1,
    /* p2:x2 =*/0,
  },
  .requires=REQUIRES_PGTABLE,
  .no_sc_results = 7,
};