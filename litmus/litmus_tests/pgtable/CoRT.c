
#include "lib.h"

#define VARS x, y
#define REGS p1x0, p1x2

static void P0(litmus_test_run* data) {
  asm volatile(
    /* move from C vars into machine regs */
    "mov x0, %[ydesc]\n\t"
    "mov x1, %[xpte]\n\t"
    /* test */
    "str x0, [x1]\n\t"
    :
    : ASM_VARS(data, VARS), ASM_REGS(data, REGS)
    : "cc", "memory", "x0", "x1"
  );
}

static void P1(litmus_test_run* data) {
  asm volatile(
    /* move from C vars into machine regs */
    "mov x1, %[xpte]\n\t"
    "mov x3, %[x]\n\t"

    /* test payload */
    "ldr x0, [x1]\n\t"
    "ldr x2, [x3]\n\t"

    /* save results */
    "str x0, [%[outp1r0]]\n\t" /* save desc,  later re-write to be 0, 1 */
    "str x2, [%[outp1r2]]\n\t"
    :
    : ASM_VARS(data, VARS), ASM_REGS(data, REGS)
    : "cc", "memory", "x0", "x1", "x2", "x3", "x4"
  );

  *data->out_reg[0] = (*data->out_reg[0] == var_desc(data, "y")) ? 1 : 0;
}

litmus_test_t CoRT = {
  "CoRT",
  MAKE_THREADS(2),
  MAKE_VARS(VARS),
  MAKE_REGS(REGS),
  INIT_STATE(2, INIT_VAR(x, 0), INIT_VAR(y, 1)),
  .interesting_result = (u64[]){
    /* p0:x0 =*/1,
    /* p0:x2 =*/0,
  },
  .requires=REQUIRES_PGTABLE,
  .no_sc_results = 3,
};