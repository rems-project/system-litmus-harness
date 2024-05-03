
#include "lib.h"

#define VARS x, y
#define REGS p0x5

static void sync_handler(void) {
  asm volatile("mov x5, #2\n\t"

               ERET_TO_NEXT(x10));
}

static void P0(litmus_test_run* data) {
  asm volatile(
    /* move from C vars into machine regs */
    "mov x0, #0\n\t"
    "mov x1, %[xpte]\n\t"
    "mov x2, %[xpage]\n\t"
    "mov x3, %[ydesc]\n\t"
    "mov x4, %[xpte]\n\t"
    "mov x6, %[x]\n\t"

    /* test */
    "str x0, [x1]\n\t"
    "dsb sy\n\t"
    "tlbi vae1,x2\n\t"
    "dsb sy\n\t"
    "str x3, [x4]\n\t"
    "dsb sy\n\t"
    "isb\n\t"
    "ldr x5, [x6]\n\t"

    /* output */
    "str x5, [%[outp0r5]]\n\t"
    :
    : ASM_VARS(data, VARS), ASM_REGS(data, REGS)
    : "cc", "memory", "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x10"
  );
}

litmus_test_t CoWinvWT1_dsbtlbidsbdsbisb = {
  "CoWinvWT1+dsb-tlbi-dsb-dsb-isb",
  MAKE_THREADS(1),
  MAKE_VARS(VARS),
  MAKE_REGS(REGS),
  INIT_STATE(2, INIT_VAR(x, 0), INIT_VAR(y, 1)),
  .no_interesting_results = 2,
  .interesting_results =
    (u64*[]){
      (u64[]){
        /* p0:x5 =*/0, /* stale translation */
      },
      (u64[]){
        /* p0:x5 =*/2, /* spurious abort */
      },
    },
  .start_els =
    (int[]){
      1,
    },
  .thread_sync_handlers =
    (u32 * *[]){
      (u32*[]){ NULL, (u32*)sync_handler },
    },
  .requires_pgtable = 1,
  .no_sc_results = 1,
};