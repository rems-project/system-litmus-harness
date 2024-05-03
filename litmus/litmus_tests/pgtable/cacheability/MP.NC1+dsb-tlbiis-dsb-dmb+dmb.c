
#include "lib.h"

#define VARS x, y
#define REGS p1x0, p1x2

static void P0(litmus_test_run* data) {
  asm volatile(
    /* initial setup */
    "mov x0, %[xdesc]\n\t"
    /* write AttrIdx to be PROT_MEMTYPE_NONCACHING */
    "bfm x0, xzr, #62, #1\n\t"
    "mov x6, #" STR_LITERAL(PROT_ATTR_NORMAL_NC
    ) "\n\t"
      "lsl x6, x6, #2\n\t"
      "add x0, x0, x6\n\t"
      "mov x1, %[xpte]\n\t"
      "mov x7, %[xpage]\n\t"
      "mov x2, #1\n\t"
      "mov x3, %[x]\n\t"
      "mov x4, #1\n\t"
      "mov x5, %[y]\n\t"

      /* test */
      "str x0, [x1]\n\t"
      "dsb sy\n\t"
      "tlbi vae1is, x7\n\t"
      "dsb sy\n\t"
      "str x2, [x3]\n\t"
      "dmb sy\n\t"
      "str x4, [x5]\n\t"
    :
    : ASM_VARS(data, VARS), ASM_REGS(data, REGS)
    : "cc", "memory", "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7"
  );
}

static void P1(litmus_test_run* data) {
  asm volatile(
    "mov x1, %[y]\n\t"
    "mov x3, %[x]\n\t"

    /* test */
    "ldr x0, [x1]\n\t"
    "dmb sy\n\t"
    "ldr x2, [x3]\n\t"

    /* collect */
    "str x0, [%[outp1r0]]\n\t"
    "str x2, [%[outp1r2]]\n\t"
    :
    : ASM_VARS(data, VARS), ASM_REGS(data, REGS)
    : "cc", "memory", "x0", "x1", "x2", "x3"
  );
}

litmus_test_t MPNC1_dsbtlbiisdsbdmb_dmb = {
  "MP.NC1+dsb-tlbiis-dsb-dmb+dmb",
  MAKE_THREADS(2),
  MAKE_VARS(VARS),
  MAKE_REGS(REGS),
  INIT_STATE(2, INIT_VAR(x, 0), INIT_VAR(y, 0)),
  .start_els = (int[]){ 1, 0 },
  .interesting_result = (u64[]){
    /* p2:x0 =*/1,
    /* p2:x2 =*/0,
  },
  .requires_pgtable = 1,
  .no_sc_results = 3,
};