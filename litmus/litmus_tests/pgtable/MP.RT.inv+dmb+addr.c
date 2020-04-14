#include <stdint.h>

#include "lib.h"

static void P0(test_ctx_t* ctx, int i, uint64_t** heap_vars, uint64_t** ptes, uint64_t* pas, uint64_t** out_regs) {
  uint64_t* x = heap_vars[0];
  uint64_t* y = heap_vars[1];
  uint64_t* z = heap_vars[2];
  
  uint64_t* xpte = ptes[0];
  uint64_t* zpte = ptes[2];
  
  uint64_t zdesc = *zpte;

  asm volatile (
    "mov x0, %[zdesc]\n\t"
    "mov x1, %[xpte]\n\t"
    "mov x2, #1\n\t"
    "mov x3, %[y]\n\t"

    /* test */
    "str x0, [x1]\n\t"
    "dmb sy\n\t"
    "str x2, [x3]\n\t"
  :
  : [zdesc] "r" (zdesc), [xpte] "r" (xpte), [y] "r" (y)
  : "cc", "memory", "x0", "x1", "x2", "x3"
  );
}

static void P1(test_ctx_t* ctx, int i, uint64_t** heap_vars, uint64_t** ptes, uint64_t* pas, uint64_t** out_regs) {
  uint64_t* x = heap_vars[0];
  uint64_t* y = heap_vars[1];
  uint64_t* z = heap_vars[2];

  uint64_t* outp1r0 = out_regs[0];
  uint64_t* outp1r2 = out_regs[1];

  asm volatile (
    "mov x1, %[y]\n\t"
    "mov x3, %[x]\n\t"
    "mov x10, #1\n\t"
    "mov x11, %[outp1r2]\n\t"

    /* test */
    "ldr x0, [x1]\n\t"
    "eor x4, x0, x0\n\t"
    "add x4, x4, x3\n\t"
    "ldr x2, [x4]\n\t"

    /* collect */
    "str x0, [%[outp1r0]]\n\t"
  :
  : [x] "r" (x), [y] "r" (y), [outp1r0] "r" (outp1r0), [outp1r2] "r" (outp1r2)
  : "cc", "memory", "x0", "x1", "x2", "x3", "x4", "x10", "x11"
  );
}

static void sync_handler(void) {
  asm volatile (
    "str x10, [x11]\n\t"

    "mrs x10, elr_el1\n\t"
    "add x10,x10,#4\n\t"
    "msr elr_el1, x10\n\t"
    "eret\n\t"
  );
}

litmus_test_t MPRTinv_dmb_addr = {
  "MP.RT.inv+dmb+addr",
  2, (th_f**[]){ 
    (th_f* []){NULL, P0, NULL},
    (th_f* []){NULL, P1, NULL},
  },
  3, (const char*[]){"x", "y", "z"}, 
  2, (const char*[]){"p1:x0", "p1:x2"}, 
  .no_init_states=2,
  .init_states=(init_varstate_t*[]){
      &(init_varstate_t){"x", TYPE_PTE, 0},
      &(init_varstate_t){"z", TYPE_HEAP, 1},
    },
  .thread_sync_handlers =
    (uint32_t**[]){
     (uint32_t*[]){NULL, NULL},
     (uint32_t*[]){(uint32_t*)sync_handler, NULL},
    },
  .interesting_result =
    (uint64_t[]){
      /* p2:x0 =*/ 1,
      /* p2:x2 =*/ 1,
    },
  .requires_pgtable=1,
};
