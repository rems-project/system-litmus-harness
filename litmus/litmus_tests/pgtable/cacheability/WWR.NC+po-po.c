
#include "lib.h"

#define VARS x, y
#define REGS p0x4

static void P0(litmus_test_run* data) {
  asm volatile(
    /* change  */
    "mov x0, #1\n\t"
    "mov x1, %[x]\n\t"
    "mov x2, #2\n\t"
    "mov x3, %[y]\n\t"
    "mov x5, %[y]\n\t"

    /* test */
    "str x0, [x1]\n\t"
    "str x2, [x3]\n\t"
    "ldr x4, [x5]\n\t"

    /* output */
    "str x4, [%[outp0r4]]\n\t"
    :
    : ASM_VARS(data, VARS), ASM_REGS(data, REGS)
    : "cc", "memory", "x0", "x1", "x2", "x3", "x4", "x5"
  );
}

litmus_test_t WWRNC_popo = {
  "WWR.NC+po-po",
  MAKE_THREADS(1),
  MAKE_VARS(VARS),
  MAKE_REGS(REGS),
  INIT_STATE(
    3, INIT_VAR(x, 0),
    /* set y to be a non-cacheable alias to x */
    INIT_ALIAS(y, x), INIT_PERMISSIONS(y, PROT_ATTRIDX, PROT_ATTR_NORMAL_NC),
  ),
  .interesting_results =
    (u64*[]){
      (u64[]){
        /* p1:x4 =*/0,
      },
      (u64[]){
        /* p1:x4 =*/1,
      },
    },
  .requires_pgtable = 1,
  .no_sc_results = 1,
};