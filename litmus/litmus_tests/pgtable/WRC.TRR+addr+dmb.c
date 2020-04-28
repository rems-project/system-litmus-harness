#include <stdint.h>

#include "lib.h"

static void P0(litmus_test_run* data) {
  uint64_t* x = data->var[0];
  uint64_t* y = data->var[1];
  uint64_t* z = data->var[2];
  uint64_t* zpte = data->PTE[2];
  asm volatile (
    "mov x0, %[zdesc]\n\t"
    "mov x1, %[xpte]\n\t"
    /* test */
    "str x0, [x1]\n\t"
  :
  : [zdesc] "r" (data->DESC[2]), [xpte] "r" (data->PTE[0])
  : "cc", "memory", "x0", "x1"
  );
}

static void P1(litmus_test_run* data) {
  uint64_t* z = data->var[2];
  asm volatile (
    "mov x1, %[x]\n\t"
    "mov x2, 1\n\t"
    "mov x3, %[y]\n\t"
    /* test */
    "ldr x0, [x1]\n\t"
    "eor x4, x0, x0\n\t"
    "add x4, x4, x3\n\t"
    "str x2, [x4]\n\t"
    /* output */
    "str x0, [%[outp1r0]]\n\t"
  :
  : [x] "r" (data->var[0]), [y] "r" (data->var[1]), [outp1r0] "r" (data->out_reg[0])
  : "cc", "memory", "x0", "x1", "x2", "x3", "x4"
  );
}

static void P2(litmus_test_run* data) {
  uint64_t* x = data->var[0];
  uint64_t* zpte = data->PTE[2];
  asm volatile (
    "mov x1, %[y]\n\t"
    "mov x3, %[xpte]\n\t"
    "mov x4, %[zdesc]\n\t"
    "ldr x0, [x1]\n\t"
    "dmb sy\n\t"
    "ldr x2, [x3]\n\t"
    "eor x2, x2, x4\n\t"
    "cbz x2, .after\n\t"
    "mov x2, #0\n\t"
    ".after:\n\t"
    "eor x2, x2, #1\n\t"
    "str x0, [%[p2r0]]\n\t"
    "str x2, [%[p2r2]]\n\t"
  :
  : [y] "r" (data->var[1]), [xpte] "r" (data->PTE[0]), [zdesc] "r" (data->DESC[2]), [p2r0] "r" (data->out_reg[1]), [p2r2] "r" (data->out_reg[2])
  : "cc", "memory", "x0", "x1", "x2", "x3", "x4"
  );
}



litmus_test_t WRCtrr_addr_dmb = {
  "WRC.TRR+addr+dmb",
  3,(th_f*[]){
    (th_f*)P0,
    (th_f*)P1,
    (th_f*)P2
  },
  3,(const char*[]){"x", "y", "z"},
  3,(const char*[]){"p1:x0", "p2:x0", "p2:x2"},
  .no_init_states=1,
  .init_states=(init_varstate_t*[]){
      &(init_varstate_t){"z", TYPE_HEAP, 1},
    },
  .interesting_result =
    (uint64_t[]){
      /* p1:x2 =*/ 1,
      /* p2:x0 =*/ 1,
      /* p2:x2 =*/ 0,
    },
  .requires_pgtable=1,
};
