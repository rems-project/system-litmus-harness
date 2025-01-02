/*
 * this test does not run on the raspberry pi
 * since it requires atomics which are >=ARMv8.1
 * but the RPi4 (and all previous) are ARMv8.0
 */

#include "lib.h"

#define VARS x, y
#define REGS p1x2, p1x4

static void P0(litmus_test_run* data) {
  asm volatile(
    "mov x0, %[x]\n\t"
    "mov x1, %[y]\n\t"
    "mov x2, #1\n\t"
    "mov x3, #0\n\t"
    "mov x4, #0\n\t"

    "str x2,[x0]\n\t"
    "casl x3,x4,[x1]\n\t"
    "str x2,[x1]\n\t"
    :
    : ASM_VARS(data, VARS), ASM_REGS(data, REGS)
    : "cc", "memory", "x0", "x1", "x2", "x3", "x4"
  );
}

static void P1(litmus_test_run* data) {
  asm volatile(
    "mov x0, %[x]\n\t"
    "mov x1, %[y]\n\t"

    "ldr x2,[x1]\n\t"
    "eor x3,x2,x2\n\t"
    "ldr x4,[x0,x3]\n\t"

    "str x0, [%[outp1r2]]\n\t"
    "str x2, [%[outp1r4]]\n\t"
    :
    : ASM_VARS(data, VARS), ASM_REGS(data, REGS)
    : "cc", "memory", "x0", "x1", "x2", "x3", "x4"
  );
}

litmus_test_t MP_pop_amoswap_pl_coi_addr = {
  "MP+po_p_amoswap_pl-coi+addr",
  MAKE_THREADS(2),
  MAKE_VARS(VARS),
  MAKE_REGS(REGS),
  INIT_STATE(2, INIT_VAR(x, 0), INIT_VAR(y, 0), ),
  .interesting_result = (u64[]){
    /* p1:x2 =*/1,
    /* p1:x4 =*/0,
  },
  .no_sc_results = 3,
  .requires = REQUIRES_ARM_AARCH64_FEAT_LSE,
};