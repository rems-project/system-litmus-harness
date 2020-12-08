#include <stdint.h>

#include "lib.h"

#define VARS x, y
#define REGS exc

/* add hooks for exc reg */
#define USER_exc "exc"
#define IDENT_exc exc

static void P0(litmus_test_run* data) {
  asm volatile (
    "mov x10, %[exc]\n\t"
    "mov x0, #1\n\t"
    "str x0, [%[x]]\n\t"
    "mov x2, #1\n\t"
    "str x2, [%[y]]\n\t"
  :
  : ASM_VARS(data, VARS),
    ASM_REGS(data, REGS)
  : "cc", "memory", "x0", "x2", "x10", "x11", "x12"
  );
}

static void P1(litmus_test_run* data) {
  uint64_t _x0, _x2;

  asm volatile (
    "mov x10, %[exc]\n\t"
    "ldr %[x0], [%[y]]\n\t"
    "ldr %[x2], [%[x]]\n\t"
  : [x0] "=&r" (_x0), [x2] "=&r" (_x2)
  : ASM_VARS(data, VARS),
    ASM_REGS(data, REGS)
  : "cc", "memory", "x0", "x2", "x10", "x11", "x12"
  );
}

static void handler(void) {
  asm volatile (
    "mov x11,#1\n\t"
    "str x11,[x10]\n\t"

    ERET_TO_NEXT(x12)
  );
}

static bar_t* bar = NULL;
static void p0_setup(litmus_test_run* data) {
  if (bar != NULL)
    FREE(bar);

  bar = alloc(sizeof(bar_t));
  *bar = EMPTY_BAR;
}

static void teardown(litmus_test_run* data) {
  bwait(get_vcpu(), bar, 2);
  if (get_vcpu() == 0) {
    *data->tt_entries[0][3] = 0;
  }
}


litmus_test_t check2 = {
  "check2",
  MAKE_THREADS(2),
  MAKE_VARS(VARS),
  MAKE_REGS(REGS),
  INIT_STATE(
    2,
    INIT_VAR(x, 0),
    INIT_VAR(y, 0)
  ),
  .interesting_result =
    (uint64_t[]){
      /* exc =*/ 1,
    },
  .thread_sync_handlers =
    (uint32_t**[]){
     (uint32_t*[]){(uint32_t*)handler, NULL},
     (uint32_t*[]){(uint32_t*)handler, NULL},
    },
  .setup_fns = (th_f*[]){
    (th_f*)p0_setup,
    NULL,
  },
  .teardown_fns = (th_f*[]){
    (th_f*)teardown,
    (th_f*)teardown,
  },
  .requires_pgtable = 1,
};