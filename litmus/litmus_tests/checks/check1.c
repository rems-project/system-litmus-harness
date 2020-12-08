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
    "mov x1, %[x]\n\t"
    /* test */
    "str x0, [x1]\n\t"
  :
  : ASM_VARS(data, VARS),
    ASM_REGS(data, REGS)
  : "cc", "memory", "x0", "x1", "x10", "x11", "x12"
  );
}

static void P1(litmus_test_run* data) {
  asm volatile (
    "mov x10, %[exc]\n\t"
    "mov x1, %[x]\n\t"
    "mov x2, #1\n\t"
    "mov x3, %[y]\n\t"
    /* test */
    "ldr x0, [x1]\n\t"
    "eor x4, x0, x0\n\t"
    "add x4, x4, x3\n\t"
    "str x2, [x4]\n\t"
  :
  : ASM_VARS(data, VARS),
    ASM_REGS(data, REGS)
  : "cc", "memory", "x0", "x1", "x2", "x3", "x4", "x10", "x11", "x12"
  );
}

static void P2(litmus_test_run* data) {
  uint64_t _x2;
  asm volatile (
    "mov x10, %[exc]\n\t"
    "mov x1, %[y]\n\t"
    "mov x3, %[x]\n\t"
    /* test */
    "ldr x0, [x1]\n\t"
    "eor x4, x0, x0\n\t"
    "add x4, x4, x3\n\t"
    "ldr %[x2], [x4]\n\t"
  : [x2] "=&r" (_x2)
  : ASM_VARS(data, VARS),
    ASM_REGS(data, REGS)
  : "cc", "memory", "x0", "x1", "x2", "x3", "x4", "x10", "x11", "x12"
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
  bwait(get_vcpu(), bar, 3);
  if (get_vcpu() == 0) {
    *data->tt_entries[0][3] = 0;
  }
}


litmus_test_t check1 = {
  "check1",
  MAKE_THREADS(3),
  MAKE_VARS(VARS),
  MAKE_REGS(REGS),
  INIT_STATE(
    2,
    INIT_VAR(x, 0),
    INIT_VAR(y, 0)
  ),
  .interesting_result =
    (uint64_t[]){
      /* exc=*/ 1,
  },
  .start_els=(int[]){1,1,1},
  .thread_sync_handlers =
    (uint32_t**[]){
     (uint32_t*[]){NULL, (uint32_t*)handler},
     (uint32_t*[]){NULL, (uint32_t*)handler},
     (uint32_t*[]){NULL, (uint32_t*)handler},
    },
  .setup_fns = (th_f*[]){
    (th_f*)p0_setup,
    NULL,
    NULL,
  },
  .teardown_fns = (th_f*[]){
    (th_f*)teardown,
    (th_f*)teardown,
    (th_f*)teardown,
  },
  .requires_pgtable = 1,
};