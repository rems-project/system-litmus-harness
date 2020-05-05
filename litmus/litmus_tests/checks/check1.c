#include <stdint.h>

#include "lib.h"

static void P0(litmus_test_run* data) {
  asm volatile (
    "mov x10, %[exc]\n\t"
    "mov x0, #1\n\t"
    "mov x1, %[x]\n\t"
    /* test */
    "str x0, [x1]\n\t"
  :
  : [x] "r" (data->var[0]), [exc] "r" (data->out_reg[0])
  : "cc", "memory", "x0", "x1", "x10", "x11"
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
  : [x] "r" (data->var[0]), [y] "r" (data->var[1]), [exc] "r" (data->out_reg[0])
  : "cc", "memory", "x0", "x1", "x2", "x3", "x4", "x10", "x11"
  );
}

static void P2(litmus_test_run* data) {
  asm volatile (
    "mov x10, %[exc]\n\t"
    "mov x1, %[y]\n\t"
    "mov x3, %[x]\n\t"
    /* test */
    "ldr x0, [x1]\n\t"
    "eor x4, x0, x0\n\t"
    "add x4, x4, x3\n\t"
    "ldr x2, [x4]\n\t"
  :
  : [x] "r" (data->var[0]), [y] "r" (data->var[1]), [exc] "r" (data->out_reg[1])
  : "cc", "memory", "x0", "x1", "x2", "x3", "x4", "x10", "x11"
  );
}

static void handler(void) {
  asm volatile (
    "mov x11,#1\n\t"
    "str x11,[x10]\n\t"

    ERET_TO_NEXT(x10)
  );
}

static bar_t* bar = NULL;
static void p0_setup(litmus_test_run* data) {
  if (bar != NULL)
    free(bar);

  bar = alloc(sizeof(bar_t));
  *bar = (bar_t){0};
}

static void teardown(litmus_test_run* data) {
  bwait(get_cpu(), data->i % 3, bar, 3);
  if (get_cpu() == 0) {
    *data->pte[0] = 0;
  }
}

litmus_test_t check1 = {
  "check1",
  3,(th_f*[]){
    (th_f*)P0,
    (th_f*)P1,
    (th_f*)P2
  },
  2,(const char*[]){"x", "y"},
  1,(const char*[]){"exc"},
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