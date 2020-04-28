#include <stdint.h>

#include "lib.h"

static void P0(litmus_test_run* data) {
  asm volatile (
    "mov x0, #1\n\t"
    "str x0, [%[x1]]\n\t"
    "mov x2, #1\n\t"
    "str x2, [%[x3]]\n\t"
  :
  : [x1] "r" (data->var[0]), [x3] "r" (data->var[1])
  : "cc", "memory", "x0", "x2"
  );
}

static void P1(litmus_test_run* data) {
  uint64_t* x0 = data->out_reg[0];
  uint64_t* x2 = data->out_reg[1];
  asm volatile (
    "ldr %[x0], [%[x1]]\n\t"
    "ldr %[x2], [%[x3]]\n\t"
  : [x0] "=&r" (*x0), [x2] "=&r" (*x2)
  : [x1] "r" (data->var[1]), [x3] "r" (data->var[0])
  : "cc", "memory"
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
    *data->PTE[0] = 0;
  }
}


litmus_test_t check2 = {
  "check2",
  2,(th_f*[]){
    (th_f*)P0,
    (th_f*)P1
  },
  2,(const char*[]){"x", "y"},
  2,(const char*[]){"p1:x0", "p1:x2"},
  .start_els=(int[]){1,1},
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