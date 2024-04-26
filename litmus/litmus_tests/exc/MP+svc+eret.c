#include "lib.h"

#define VARS x, y
#define REGS p1x0, p1x2

static void svc_handler0(void) {
  asm volatile(
      /* x3 = Y */
      "mov x2, #1\n\t"
      "str x2, [x3]\n\t"
      "eret\n\t");
}

static void P0(litmus_test_run* data) {
  asm volatile (
    "mov x1, %[x]\n\t"
      "mov x3, %[y]\n\t"

      "mov x0, #1\n\t"
      "str x0, [x1]\n\t"
      "svc #0\n\t"
  : 
  : ASM_VARS(data, VARS),
    ASM_REGS(data, REGS)
  : "cc", "memory", "x0", "x1", "x2", "x3"
  );
}

static void svc_handler1(void) {
  asm volatile(
      /* x1 = Y */
      "ldr x0, [x1]\n\t"
      "eret\n\t");
}

static void P1(litmus_test_run* data) {
  asm volatile (
    /* initial registers */
      "mov x1, %[y]\n\t"
      "mov x3, %[x]\n\t"

      /* test */
      "svc #0\n\t"
      "ldr x2, [x3]\n\t"

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



litmus_test_t MP_svc_eret = {
  "MP+svc+eret",
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
     (u32*[]){(u32*)svc_handler0, NULL},
     (u32*[]){(u32*)svc_handler1, NULL},
    },
  .interesting_result = (u64[]){
      /* p1:x0 =*/1,
      /* p1:x2 =*/0,
  },
};
