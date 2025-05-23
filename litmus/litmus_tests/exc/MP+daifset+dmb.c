
#include "lib.h"

#define VARS x, y
#define REGS p1x0, p1x2

static void P0(litmus_test_run* data) {
  asm volatile (
      "mov x0, #1\n\t"
      "str x0, [%[x]]\n\t"

      "msr daifset, #15\n\t"

      "mov x2, #1\n\t"
      "str x2, [%[y]]\n\t"
  :
  : ASM_VARS(data, VARS),
    ASM_REGS(data, REGS)
  : "cc", "memory", "x0", "x2"
  );
}

static void P1(litmus_test_run* data) {
  asm volatile (
    /* load variables into machine registers */
      "mov x1, %[y]\n\t"
      "mov x3, %[x]\n\t"

      /* test */
      "ldr x0, [x1]\n\t"

      "dmb sy\n\t"

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



litmus_test_t MP_daifset_dmb = {
  "MP+daifset+dmb",
  MAKE_THREADS(2),
  MAKE_VARS(VARS),
  MAKE_REGS(REGS),
  INIT_STATE(
    2,
    INIT_VAR(x, 0),
    INIT_VAR(y, 0)
  ),
  .start_els=(int[]){1,1},
  .thread_sync_handlers =
    (u32**[]){
     (u32*[]){NULL, NULL},
     (u32*[]){NULL, NULL},
    },
  .interesting_result = (u64[]){
      /* p1:x0 =*/1,
      /* p1:x2 =*/0,
  },
};
