
#include "lib.h"

#define VARS x, y, z
#define REGS p0x2, p1x2

static void sync_handler0(void) {
  asm volatile(
    /* x3 = Y */
    "ldr x2, [x3]\n\t"

    ERET_TO_NEXT(x10)
  );
}

static void P0(litmus_test_run* data) {
  asm volatile(
    /* initial registers */
    "mov x0, #1\n\t"
    "mov x1, %[x]\n\t"
    "mov x3, %[y]\n\t"
    "mov x5, %[z]\n\t"

    "str x0, [x1]\n\t"
    "ldr x4, [x5]\n\t"

    /* extract values */
    "str x2, [%[outp0r2]]\n\t"
    "mov x4, #0\n\t" /* IGNORE x4 */
    "dmb st\n\t"
    :
    : ASM_VARS(data, VARS), ASM_REGS(data, REGS)
    : "cc", "memory", "x0", "x1", "x2", "x3", "x4", "x5", "x10"
  );
}

static void sync_handler1(void) {
  asm volatile(
    /* x3 = X */
    "ldr x2, [x3]\n\t"

    ERET_TO_NEXT(x10)
  );
}

static void P1(litmus_test_run* data) {
  asm volatile(
    /* initial registers */
    "mov x0, #1\n\t"
    "mov x1, %[y]\n\t"
    "mov x3, %[x]\n\t"
    "mov x5, %[z]\n\t"

    "str x0, [x1]\n\t"
    "ldr x4, [x5]\n\t"

    "str x2, [%[outp1r2]]\n\t"
    "mov x4, #0\n\t" /* IGNORE x4 */
    "dmb st\n\t"
    :
    : ASM_VARS(data, VARS), ASM_REGS(data, REGS)
    : "cc",
      "memory",
      "x0",
      "x1",
      "x2",
      "x3",
      "x4",
      "x5",
      "x6",
      "x7" /* dont touch parameter registers */
      ,
      "x10"
  );
}

litmus_test_t SB_faults = {
  "SB+faults",
  MAKE_THREADS(2),
  MAKE_VARS(VARS),
  MAKE_REGS(REGS),
  INIT_STATE(3, INIT_VAR(x, 0), INIT_VAR(y, 0), INIT_UNMAPPED(z)),
  .requires=REQUIRES_PGTABLE,
  .thread_sync_handlers =
    (u32 * *[]){
      (u32*[]){ (u32*)sync_handler0, NULL },
      (u32*[]){ (u32*)sync_handler1, NULL },
    },
  .interesting_result = (u64[]){
    /* p0:x2 =*/0,
    /* p1:x2 =*/0,
  },
};
