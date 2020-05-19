#include <stdint.h>

#include "lib.h"

#define VARS x, y, z, p
#define REGS p0x2, p1x0, p2x0, p2x2

static void P0(litmus_test_run* data) {
  asm volatile (
    "mov x0, %[zdesc]\n\t"
    "mov x1, %[xpte]\n\t"
    "mov x3, %[p]\n\t"
    "mov x4, %[xpage]\n\t"
    "mov x5, #1\n\t"
    "mov x6, %[y]\n\t"

    /* test payload */
    "str x0,[x1]\n\t"
    "ldr x2,[x3]\n\t"
    "dsb sy\n\t"
    "tlbi vae1is,x4\n\t"
    "dsb sy\n\t"
    "isb\n\t"
    "str x5,[x6]\n\t"

    /* output */
    "str x2, [%[outp0r2]]\n\t"
  :
  : ASM_VARS(data, VARS),
    ASM_REGS(data, REGS)
  : "memory", "x0", "x1", "x2", "x3", "x4", "x5", "x6"
  );
}

static void P1(litmus_test_run* data) {
  asm volatile (
      /* move from C vars into machine regs */
      "mov x1, %[xpte]\n\t"
      "mov x2, #1\n\t"
      "mov x3, %[p]\n\t"
      /* test */
      "ldr x0,[x1]\n\t"
      "dmb ld\n\t"
      "str x2,[x3]\n\t"
      /* output */
      "str x0, [%[outp1r0]]\n\t"
      :
      : ASM_VARS(data, VARS),
        ASM_REGS(data, REGS)
      :  "cc", "memory", "x0", "x1", "x2", "x3", "x4"
  );

  if (*data->out_reg[1] == var_desc(data, "z")) {
    *data->out_reg[1] = 1;
  } else {
    *data->out_reg[1] = 0;
  }
}

static void P2(litmus_test_run* data) {
  asm volatile (
      /* move from C vars into machine regs */
      "mov x1, %[y]\n\t"
      "mov x3, %[x]\n\t"
      /* test */
      "ldr x0,[x1]\n\t"
      "dmb sy\n\t"
      "ldr x2,[x3]\n\t"
      /* output */
      "str x0, [%[outp2r0]]\n\t"
      "str x2, [%[outp2r2]]\n\t"
      :
      : [y] "r" (var_va(data, "y")), [x] "r" (var_va(data, "x")), [outp2r0] "r" (out_reg(data, "p2:x0")), [outp2r2] "r" (out_reg(data, "p2:x2"))
      :  "cc", "memory", "x0", "x1", "x2", "x3", "x4"
  );
}


litmus_test_t MPRT1_dsbdmbldtlbiisdsbisb_dmb = {
  "MP.RT1+[dmb.ld]-dsb-tlbiis-dsb-isb+dmb",
  MAKE_THREADS(3),
  MAKE_VARS(VARS),
  MAKE_REGS(REGS),
  INIT_STATE(
    3,
    INIT_VAR(x, 0),
    INIT_VAR(y, 0),
    INIT_VAR(z, 1),
  ),
  .interesting_result = (uint64_t[]){
      /* p0:x2 =*/1,
      /* p1:x0 =*/1,
      /* p2:x0 =*/1,
      /* p2:x2 =*/0,
  },
  .start_els=(int[]){1,0,0},
  .requires_pgtable=1,
  .no_sc_results = 12,
};