#include <stdint.h>

#include "lib.h"

static void P0(litmus_test_run* data) {
  asm volatile (
    "ldr %[x0], [%[x]]\n\t"
    "str %[x1], [%[x]]\n\t"
  : [x0] "=&r" (*data->out_reg[0])
  : [x1] "r" (4), [x] "r" (var_va(data, "x"))
  : "cc", "memory"
  );
}

litmus_test_t check4 = {
  "check4",
  1, (th_f*[]){
    (th_f*)P0,
  },
  1,(const char*[]){"x"},
  1,(const char*[]){"p0:x0"},
  .interesting_result =
    (uint64_t[]){
      /* p0:x0 =*/ 4,
    },
};