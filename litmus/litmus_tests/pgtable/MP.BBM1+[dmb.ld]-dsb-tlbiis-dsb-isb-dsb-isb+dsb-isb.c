
#include "lib.h"

#define VARS x, y, z, a
#define REGS p0x7, p1x0, p1x2, p2x0

static void P0(litmus_test_run* data) {
  asm volatile(
    "mov x0, #0\n\t"
    "mov x1, %[xpte]\n\t"
    "mov x2, %[xpage]\n\t"
    "mov x3, %[zdesc]\n\t"
    "mov x4, %[xpte]\n\t"
    "mov x5, #1\n\t"
    "mov x6, %[y]\n\t"
    "mov x8, %[a]\n\t"
    /* test payload */
    "str x0,[x1]\n\t"
    "ldr x7,[x8]\n\t"
    "dsb sy\n\t"
    "tlbi vae1is,x2\n\t"
    "dsb sy\n\t"
    "isb\n\t"
    "str x3,[x4]\n\t"
    "dsb sy\n\t"
    "isb\n\t"
    "str x5,[x6]\n\t"
    /* collect */
    "str x7, [%[outp0r7]]\n\t"
    :
    : ASM_VARS(data, VARS), ASM_REGS(data, REGS)
    : "memory", "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7", "x8"
  );
}

static void P2(litmus_test_run* data) {
  asm volatile(
    "mov x1, %[xpte]\n\t"
    "mov x2, #1\n\t"
    "mov x3, %[a]\n\t"

    "ldr x0,[x1]\n\t"
    "dmb ld\n\t"
    "str x2,[x3]\n\t"

    "str x0, [%[outp2r0]]\n\t"
    :
    : ASM_VARS(data, VARS), ASM_REGS(data, REGS)
    : "memory", "x0", "x1", "x2", "x3"
  );

  u64* reg_va = out_reg(data, "p2:x0");
  if (*reg_va == 0) {
    *reg_va = 1;
  } else {
    *reg_va = 0;
  }
}

static void sync_handler(void) {
  asm volatile("mov x2, #2\n\t"

               ERET_TO_NEXT(x10));
}

static void P1(litmus_test_run* data) {
  asm volatile(
    /* move from C vars into machine regs */
    "mov x1, %[y]\n\t"
    "mov x3, %[x]\n\t"
    /* test */
    "ldr x0,[x1]\n\t"
    "dsb sy\n\t"
    "isb\n\t"
    "ldr x2,[x3]\n\t"
    /* output */
    "str x0, [%[outp1r0]]\n\t"
    "str x2, [%[outp1r2]]\n\t"
    :
    : ASM_VARS(data, VARS), ASM_REGS(data, REGS)
    : "cc", "memory", "x0", "x1", "x2", "x3", "x10"
  );
}

litmus_test_t BBM1_dmblddsbtlbiisdsbisbdsbisb_dsbisb = {
  "MP.BBM1+[dmb.ld]-dsb-tlbiis-dsb-isb-dsb-isb+dsb-isb",
  MAKE_THREADS(3),
  MAKE_VARS(VARS),
  MAKE_REGS(REGS),
  INIT_STATE(4, INIT_VAR(x, 0), INIT_VAR(y, 0), INIT_VAR(a, 0), INIT_VAR(z, 1)),
  .no_interesting_results = 2,
  .interesting_results =
    (u64*[]){
      (u64[]){
        /* p0:x7 =*/1,
        /* p1:x0 =*/1,
        /* p1:x2 =*/0, /* stale translation */
        /* p2:x0 =*/1,
      },
      (u64[]){
        /* p0:x7 =*/1,
        /* p1:x0 =*/1,
        /* p1:x2 =*/2, /* spurious abort */
        /* p2:x0 =*/1,
      },
    },
  .start_els = (int[]){ 1, 0, 0 },
  .thread_sync_handlers =
    (u32 * *[]){
      (u32*[]){ NULL, NULL },
      (u32*[]){ (u32*)sync_handler, NULL },
      (u32*[]){ NULL, NULL },
    },
  .requires_pgtable = 1,
  .no_sc_results = 16,
};