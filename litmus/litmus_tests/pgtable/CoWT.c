#include <stdint.h>

#include "lib.h"

static void P0(test_ctx_t* ctx, int i, uint64_t** heap_vars, uint64_t** ptes,
               uint64_t* pas, uint64_t** out_regs) {

  uint64_t* x = heap_vars[0];
  uint64_t* y = heap_vars[1];

  uint64_t* x2 = out_regs[0];

  uint64_t* xpte = ptes[0];
  uint64_t* ypte = ptes[1];

  /* assuming x, y initialised to 1, 2 */

  asm volatile (
      /* move from C vars into machine regs */
      "mov x0, %[ydesc]\n\t"
      "mov x1, %[xpte]\n\t"
      "mov x3, %[x]\n\t"

      /* test */
      "str x0, [x1]\n\t"
      "ldr x2, [x3]\n\t"

      /* output back to C vars */
      "str x2, [%[x2]]\n\t"
      :
      : [ydesc] "r" (*ypte), [xpte] "r" (xpte), [x] "r" (x), [x2] "r" (x2)
      : "cc", "memory", "x0", "x1", "x2", "x3");
}


litmus_test_t CoWT = {
  "CoWT",
  1,(th_f*[]){
    (th_f*)P0
  },
  2,(const char*[]){"x", "y"},
  1,(const char*[]){"p0:x2",},
  .interesting_result = (uint64_t[]){
      /* p0:x2 =*/1,
  },
  .no_init_states=2,
  .init_states=(init_varstate_t*[]){
    &(init_varstate_t){"x", TYPE_HEAP, 1},
    &(init_varstate_t){"y", TYPE_HEAP, 2},
  },
  .requires_pgtable = 1,
};
