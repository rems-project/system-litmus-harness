#include <stdint.h>

#include "lib.h"

static void P0(litmus_test_run* data) {
  asm volatile (
    "mov x10, %[exc_out]\n\t"
    "mov x0, #1\n\t"
    "str x0, [%[x]]\n\t"
    "mov x2, #1\n\t"
    "str x2, [%[y]]\n\t"
  :
  : [x] "r" (data->var[0]), [y] "r" (data->var[1]), [exc_out] "r" (data->out_reg[0])
  : "cc", "memory", "x0", "x2", "x10", "x11"
  );
}

static void P1(litmus_test_run* data) {
  uint64_t _x0, _x2;

  asm volatile (
    "mov x10, %[exc_out]\n\t"
    "ldr %[x0], [%[x1]]\n\t"
    "ldr %[x2], [%[x3]]\n\t"
  : [x0] "=&r" (_x0), [x2] "=&r" (_x2)
  : [x1] "r" (data->var[1]), [x3] "r" (data->var[0]), [exc_out] "r" (data->out_reg[0])
  : "cc", "memory", "x0", "x2", "x10", "x11"
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
  bwait(get_cpu(), data->i % 2, bar, 2);
  if (get_cpu() == 0) {
    *data->pte[0] = 0;
  }
}

litmus_test_t check2 = {
  "check2",
  2,(th_f*[]){
    (th_f*)P0,
    (th_f*)P1
  },
  2,(const char*[]){"x", "y"},
  1,(const char*[]){"exc"},
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