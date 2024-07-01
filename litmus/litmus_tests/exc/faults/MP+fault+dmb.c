
#include "lib.h"

#define VARS x, y, z
#define REGS p1x0, p1x2

static void sync_handler(void) {
  asm volatile(
    /* x3 = Y */
    "mov x2, #1\n\t"
    "str x2, [x3]\n\t"

    ERET_TO_NEXT(x10)
  );
}

static void P0(litmus_test_run* data) {
  asm volatile(
    "mov x1, %[x]\n\t"
    "mov x3, %[y]\n\t"
    "mov x5, %[y]\n\t"

    "mov x0, #1\n\t"
    "str x0, [x1]\n\t"
    "ldr x4, [x5]\n\t"

    "mov x4, #0\n\t" /* IGNORE x4 */
    :
    : ASM_VARS(data, VARS), ASM_REGS(data, REGS)
    : "cc", "memory", "x0", "x1", "x2", "x3", "x4", "x5", "x10"
  );
}

static void P1(litmus_test_run* data) {
  asm volatile(
    /* load variables into machine registers */
    "mov x1, %[y]\n\t"
    "mov x3, %[x]\n\t"

    "ldr x0,[x1]\n\t"
    "dmb sy\n\t"
    "ldr x2,[x3]\n\t"

    /* extract values */
    "str x0, [%[outp1r0]]\n\t"
    "str x2, [%[outp1r2]]\n\t"
    :
    : ASM_VARS(data, VARS), ASM_REGS(data, REGS)
    : "cc", "memory", "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7" /* dont touch parameter registers */
  );
}

litmus_test_t MP_fault_dmb = {
  "MP+fault+dmb",
  MAKE_THREADS(2),
  MAKE_VARS(VARS),
  MAKE_REGS(REGS),
  INIT_STATE(3, INIT_VAR(x, 0), INIT_VAR(y, 0), INIT_UNMAPPED(z)),
  .requires_pgtable = 1,
  .thread_sync_handlers =
    (u32 * *[]){
      (u32*[]){ (u32*)sync_handler, NULL },
      (u32*[]){ NULL, NULL },
    },
  .interesting_result = (u64[]){
    /* p1:x0 =*/1,
    /* p1:x2 =*/0,
  },
};
