
#include "lib.h"

#define VARS x, y
#define REGS p1x0, p1x2

static void P0(litmus_test_run* data) {
  asm volatile (
      "mov x0, #1\n\t"
      "str x0, [%[x]]\n\t"
      "dmb sy\n\t"
      "mov x2, #1\n\t"
      "str x2, [%[y]]\n\t"
  :
  : ASM_VARS(data, VARS),
    ASM_REGS(data, REGS)
  : "cc", "memory", "x0", "x2"
  );
}

static void svc_handler(void) {
  asm volatile(
      "cmp x4, #0\n\t"
      "b.eq 0f\n\t"
      "cmp x4, #1\n\t"
      "b.eq 1f\n\t"
      "b 2f\n\t"
      "0:\n\t"
      // SCTLR_EL1.EIS = 0
      // TODO: BS: make a INIT_SYS_REG(SCTLR, EIS, 1) INIT_STATE for create/cleanup.
      "mrs x5,sctlr_el1\n\t"
      "mov x6, #0\n\t"
      "bfi x5, x6, 22, 1\n\t"
      "msr sctlr_el1,x5\n\t"
      "b out\n\t"
      "1:\n\t"
      /* x3 = X */
      "ldr x2, [x3]\n\t"
      "b out\n\t"
      "2:\n\t"
      // reset back to SCTLR_EL1.EIS = 1
      "mrs x5,sctlr_el1\n\t"
      "mov x6, #1\n\t"
      "bfi x5, x6, 22, 1\n\t"
      "msr sctlr_el1,x5\n\t"
      "out:\n\t"
      "add x4,x4,#1\n\t"
      "eret\n\t"
  );
}

static void P1(litmus_test_run* data) {
  asm volatile (
    /* load variables into machine registers */
      "mov x1, %[y]\n\t"
      "mov x3, %[x]\n\t"

      /* test */
      "mov x4, #0\n\t"

      // setup SCTLR_EL1.EIS
      "svc #0\n\t"

      // do test...
      "ldr x0, [x1]\n\t"
      "svc #0\n\t"

      // teardown SCTLR_EL1.EIS
      "svc #0\n\t"

      /* extract values */
      "str x0, [%[outp1r0]]\n\t"
      "str x2, [%[outp1r2]]\n\t"
  :
  : ASM_VARS(data, VARS),
    ASM_REGS(data, REGS)
  : "cc", "memory", "x0", "x1", "x2", "x3", "x4", "x5", "x6",
        "x7" /* dont touch parameter registers */
  );
}



litmus_test_t MP_dmb_svcnoeis = {
  "MP+dmb+svcnoeis",
  MAKE_THREADS(2),
  MAKE_VARS(VARS),
  MAKE_REGS(REGS),
  INIT_STATE(
    2,
    INIT_VAR(x, 0),
    INIT_VAR(y, 0)
  ),
  .thread_sync_handlers =
    (u32**[]){
     (u32*[]){NULL, NULL},
     (u32*[]){(u32*)svc_handler, NULL},
    },
  .interesting_result = (u64[]){
      /* p1:x0 =*/1,
      /* p1:x2 =*/0,
  },
};
