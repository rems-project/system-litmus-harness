#include "lib.h"

#define VARS x, y
#define REGS p1x0, p1x2

static void svc_handler0(void) {
  asm volatile(
    /* write data */
    "mov x0, #1\n\t"
    "str x0, [x1]\n\t"

    /* try to synchronise with eret */
    "eret\n\t"
  );
}

static void P0(litmus_test_run* data) {
  asm volatile(
    "mov x1, %[x]\n\t"
    "mov x3, %[y]\n\t"

    /* go start at EL1 */
    "svc #0\n\t"

    /* write flag */
    "mov x2, #1\n\t"
    "str x2, [x3]\n\t"
    :
    : ASM_VARS(data, VARS), ASM_REGS(data, REGS)
    : "cc", "memory", "x0", "x1", "x2", "x3"
  );
}

static void svc_handler1(void) {
  asm volatile(
    /* read data */
    "ldr x2, [x3]\n\t"

    /* go back collect results */
    "eret\n\t"
  );
}

static void P1(litmus_test_run* data) {
  asm volatile(
    /* initial registers */
    "mov x1, %[y]\n\t"
    "mov x3, %[x]\n\t"

    /* read flag */
    "ldr x0, [x1]\n\t"
    /* try to synchronise with svc */
    "svc #0\n\t"

    /* extract values */
    "str x0, [%[outp1r0]]\n\t"
    "str x2, [%[outp1r2]]\n\t"
    :
    : ASM_VARS(data, VARS), ASM_REGS(data, REGS)
    : "cc", "memory", "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7" /* dont touch parameter registers */
  );
}

litmus_test_t MP_eret_svc = {
  "MP+eret+svc",
  MAKE_THREADS(2),
  MAKE_VARS(VARS),
  MAKE_REGS(REGS),
  INIT_STATE(2, INIT_VAR(x, 0), INIT_VAR(y, 0)),
  .thread_sync_handlers =
    (u32 * *[]){
      (u32*[]){ (u32*)svc_handler0, NULL },
      (u32*[]){ (u32*)svc_handler1, NULL },
    },
  .interesting_result = (u64[]){
    /* p1:x0 =*/1,
    /* p1:x2 =*/0,
  },
};
