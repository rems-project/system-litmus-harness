
#include "lib.h"

#define VARS x, y
#define REGS p0x2

static void svc_handler(void) {
  asm volatile(
    "tlbi vae1, x4\n\t"
    "dsb sy\n\t"
    "ldr x2, [x3]\n\t"
    "eret\n\t"
  );
}

static void P0(litmus_test_run* data) {
  /* assuming x, y initialised to 1, 2 */
  asm volatile(
    /* move from C vars into machine regs */
    "mov x0, %[ydesc]\n\t"
    "mov x1, %[xpte]\n\t"
    "mov x3, %[x]\n\t"
    "mov x4, x3\n\t"
    "lsr x4, x4, #12\n\t"

    /* test */
    "str x0, [x1]\n\t"
    "dsb sy\n\t"
    "svc #0\n\t"

    /* output back to C vars */
    "str x2, [%[outp0r2]]\n\t"
    :
    : ASM_VARS(data, VARS), ASM_REGS(data, REGS)
    : "cc", "memory", "x0", "x1", "x2", "x3", "x4"
  );
}

litmus_test_t CoWT_dsbsvctlbidsb = {
  "CoWT+dsb-svc-tlbi-dsb",
  MAKE_THREADS(1),
  MAKE_VARS(VARS),
  MAKE_REGS(REGS),
  INIT_STATE(2, INIT_VAR(x, 1), INIT_VAR(y, 2)),
  .interesting_result = (u64[]){
    /* p0:x2 =*/1,
  },
  .thread_sync_handlers =
    (u32 * *[]){
      (u32*[]){ (u32*)svc_handler, NULL },
    },
  .requires_pgtable = 1,
  .no_sc_results = 1,
};