
#include "lib.h"

#define VARS x
#define REGS p0x2

static void sync_handler(void) {
  asm volatile("mov x2, #1\n\t" ERET_TO_NEXT(x10));
}

static void P0(litmus_test_run* data) {
  asm volatile(
    /* move from C vars into machine regs */
    "mov x0, #0\n\t"
    "mov x1, %[xpte]\n\t"
    "mov x3, %[x]\n\t"
    "mov x4, %[xpage]\n\t"

    /* test */
    "str x0, [x1]\n\t"
    "tlbi vae1,x4\n\t"
    "dsb sy\n\t"
    "isb\n\t"
    "ldr x2, [x3]\n\t"

    /* output */
    "str x2, [%[outp0r2]]\n\t"
    :
    : ASM_VARS(data, VARS), ASM_REGS(data, REGS)
    : "cc", "memory", "x0", "x1", "x2", "x3", "x4", "x10"
  );
}

litmus_test_t CoWinvT1_tlbidsbisb = {
  "CoWinvT1+tlbi-dsb-isb",
  MAKE_THREADS(1),
  MAKE_VARS(VARS),
  MAKE_REGS(REGS),
  INIT_STATE(1, INIT_VAR(x, 0)),
  .start_els =
    (int[]){
      1,
    },
  .interesting_result = (u64[]){
    /* p0:x2 =*/0,
  },
  .thread_sync_handlers =
    (u32 * *[]){
      (u32*[]){ NULL, (u32*)sync_handler },
    },
  .requires_pgtable = 1,
  .no_sc_results = 1,
};