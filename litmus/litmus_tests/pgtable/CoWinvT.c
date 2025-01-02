
#include "lib.h"

#define VARS x
#define REGS p0x2

static void sync_handler(void) {
  asm volatile("mov x2, #1\n\t"

               ERET_TO_NEXT(x10));
}

static void P0(litmus_test_run* data) {
  asm volatile(
    /* move from C vars into machine regs */
    "mov x0, #0\n\t"
    "mov x1, %[xpte]\n\t"
    "mov x3, %[x]\n\t"

    /* test */
    "str x0, [x1]\n\t"
    "ldr x2, [x3]\n\t"

    /* output */
    "str x2, [%[outp0r2]]\n\t"
    :
    : ASM_VARS(data, VARS), ASM_REGS(data, REGS)
    : "cc", "memory", "x0", "x1", "x2", "x3", "x10"
  );
}

litmus_test_t CoWinvT = {
  "CoWinvT",
  MAKE_THREADS(1),
  MAKE_VARS(VARS),
  MAKE_REGS(REGS),
  INIT_STATE(1, INIT_VAR(x, 0)),
  .interesting_result = (u64[]){
    /* p0:x2 =*/0,
  },
  .thread_sync_handlers =
    (u32 * *[]){
      (u32*[]){ (u32*)sync_handler, NULL },
    },
  .requires=REQUIRES_PGTABLE,
  .no_sc_results = 1,
};
