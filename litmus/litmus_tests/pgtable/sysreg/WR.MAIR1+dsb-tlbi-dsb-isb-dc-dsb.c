
#include "lib.h"

#define VARS x, y
#define REGS p0x4

static void P0(litmus_test_run* data) {
  asm volatile(
    /* change  */
    "mrs x0, mair_el1\n\t"
    "bfm x0, xzr, #8, #7\n\t"
    "mov x6, #" STR_LITERAL(MAIR_NORMAL_RA_WA
    ) "\n\t"
      "lsl x6, x6, #56\n\t"
      "add x0, x0, x6\n\t"
      "mov x2, #1\n\t"
      "mov x3, %[y]\n\t"
      "mov x5, %[x]\n\t"
      "mov x7, %[xpage]\n\t"

      /* test */
      "msr mair_el1, x0\n\t"
      "dsb sy\n\t"
      "tlbi vae1, x7\n\t"
      "dsb sy\n\t"
      "isb\n\t"
      "str x2, [x3]\n\t"
      "dc civac, x3\n\t"
      "dsb sy\n\t"
      "ldr x4, [x5]\n\t"

      /* output */
      "str x4, [%[outp0r4]]\n\t"
    :
    : ASM_VARS(data, VARS), ASM_REGS(data, REGS)
    : "cc", "memory", "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7"
  );
}

litmus_test_t WRMAIR1_dsbtlbidsbisbdcdsb = {
  "WR.MAIR1+dsb-tlbi-dsb-isb-dc-dsb",
  MAKE_THREADS(1),
  MAKE_VARS(VARS),
  MAKE_REGS(REGS),
  INIT_STATE(
    5, INIT_VAR(x, 0),
    /* setup MAIR_EL1.Attr7 to be non-cachable normal memory
     * and make x use it */
    INIT_MAIR(MAIR_NORMAL_NC), INIT_PERMISSIONS(x, PROT_ATTRIDX, PROT_ATTR_TEST_ATTR),
    /* and setup y to be a cacheable alias to x */
    INIT_ALIAS(y, x), INIT_PERMISSIONS(y, PROT_ATTRIDX, PROT_ATTR_NORMAL_RA_WA),
  ),
  .start_els =
    (int[]){
      1,
    }, /* must start from EL1 to modify MAIR */
  .interesting_result = (u64[]){
    /* p0:x4 =*/0,
  },
  .requires=REQUIRES_PGTABLE,
  .no_sc_results = 1,
};