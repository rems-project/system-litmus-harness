
#include "lib.h"

#define VARS x
#define REGS p0x0

static void P0(litmus_test_run* data) {
  asm volatile (
    "ldr %[x0], [%[x]]\n\t"
    "mov x1, #4\n\t"
    "str x1, [%[x]]\n\t"
  : [x0] "=&r" (*data->out_reg[0])
  : ASM_VARS(data, VARS),
    ASM_REGS(data, REGS)
  : "cc", "memory", "x1"
  );
}


litmus_test_t check4 = {
  "check4",
  MAKE_THREADS(1),
  MAKE_VARS(VARS),
  MAKE_REGS(REGS),
  INIT_STATE(
    1,
    INIT_VAR(x, 0)
  ),
  .interesting_result =
    (u64[]){
      /* p0:x0 =*/ 4,
    },
};