#include "lib.h"

#define VARS x, y
#define REGS p1x0, p2x0, p2x2

static void svc_handler0(void) {
  asm volatile("eret\n\t");
}

static void P0(litmus_test_run* data) {
  asm volatile(
    /* initialise registers */
    "mov x4, %[x]\n\t"
    "mov x6, %[y]\n\t"

    /* test */
    "mov x0, #2\n\t"
    "str x0, [x4]\n\t"
    "svc #0\n\t"
    "mov x2, #1\n\t"
    "str x2, [x6]\n\t"
    :
    : ASM_VARS(data, VARS), ASM_REGS(data, REGS)
    : "cc", "memory", "x0", "x2", "x4", "x6"
  );
}

static void svc_handler1(void) {
  asm volatile(
    /* x3 = X */
    "eret\n\t"
  );
}

static void P1(litmus_test_run* data) {
  asm volatile(
    /* load variables into machine registers */
    "mov x1, %[y]\n\t"
    "mov x3, %[x]\n\t"

    /* test  */
    "ldr x0, [x1]\n\t"
    "svc #0\n\t"
    "mov x2, #1\n\t"
    "str x2, [x3]\n\t"

    /* extract values */
    "str x0, [%[outp1r0]]\n\t"
    :
    : ASM_VARS(data, VARS), ASM_REGS(data, REGS)
    : "cc", "memory", "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7" /* dont touch parameter registers */
  );
}

/* Observer thread */
static void P2(litmus_test_run* data) {
  asm volatile(
    /* load variables into machine registers */
    "mov x3, %[x]\n\t"

    /* observe  */
    "ldr x0, [x3]\n\t"
    "ldr x2, [x3]\n\t"

    /* extract values */
    "str x0, [%[outp2r0]]\n\t"
    "str x2, [%[outp2r2]]\n\t"
    :
    : ASM_VARS(data, VARS), ASM_REGS(data, REGS)
    : "cc", "memory", "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7" /* dont touch parameter registers */
  );
}

litmus_test_t S_svcerets = {
  "S+svc-erets",
  MAKE_THREADS(3),
  MAKE_VARS(VARS),
  MAKE_REGS(REGS),
  INIT_STATE(2, INIT_VAR(x, 0), INIT_VAR(y, 0)),
  .thread_sync_handlers =
    (u32 * *[]){
      (u32*[]){ (u32*)svc_handler0, NULL },
      (u32*[]){ (u32*)svc_handler1, NULL },
      (u32*[]){ NULL, NULL },
    },
  .interesting_result = (u64[]){
    /* p1:x0 =*/1,
    /* p2:x0 =*/1,
    /* p2:x2 =*/2,
  },
  .no_sc_results = 12,
};
