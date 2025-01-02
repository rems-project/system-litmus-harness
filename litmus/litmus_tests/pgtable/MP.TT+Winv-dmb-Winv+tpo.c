
#include "lib.h"

#define VARS a, b, c, d
#define REGS p1x0, p1x2

static void P0(litmus_test_run* data) {
  asm volatile(
    /* move from C vars into machine regs */
    "mov x0, %[cdesc]\n\t"
    "mov x1, %[apte]\n\t"
    "mov x2, %[ddesc]\n\t"
    "mov x3, %[bpte]\n\t"

    /* test */
    "str x0, [x1]\n\t"
    "dmb sy\n\t"
    "str x2, [x3]\n\t"
    :
    : ASM_VARS(data, VARS), ASM_REGS(data, REGS)
    : "cc", "memory", "x0", "x1", "x2", "x3"
  );
}

static void P1(litmus_test_run* data) {
  asm volatile(
    /* move from C vars into machine regs */
    "mov x1, %[b]\n\t"
    "mov x3, %[a]\n\t"
    "mov x8, #0\n\t"

    /* test payload */
    "ldr x7, [x1]\n\t"
    "mov x0, x8\n\t"
    "mov x8, #0\n\t"
    "ldr x7, [x3]\n\t"
    "mov x2, x8\n\t"

    /* save results */
    "str x0, [%[outp1r0]]\n\t"
    "str x2, [%[outp1r2]]\n\t"
    :
    : ASM_VARS(data, VARS), ASM_REGS(data, REGS)
    : "cc", "memory", "x0", "x1", "x2", "x3", "x7", "x8", "x10"
  );
}

static void sync_handler(void) {
  asm volatile("mov x8, #1\n\t"

               ERET_TO_NEXT(x10));
}

litmus_test_t MPTT_WinvdmbWinv_tpo = {
  "MP.TT+Winv-dmb-Winv+tpo",
  MAKE_THREADS(2),
  MAKE_VARS(VARS),
  MAKE_REGS(REGS),
  INIT_STATE(4, INIT_VAR(a, 0), INIT_VAR(b, 0), INIT_UNMAPPED(c), INIT_UNMAPPED(d), ),
  .thread_sync_handlers =
    (u32 * *[]){
      (u32*[]){ NULL, NULL },
      (u32*[]){ (u32*)sync_handler, NULL },
    },
  .interesting_result = (u64[]){
    /* p0:x0 =*/1,
    /* p0:x2 =*/0,
  },
  .requires=REQUIRES_PGTABLE,
  .no_sc_results = 3,
};