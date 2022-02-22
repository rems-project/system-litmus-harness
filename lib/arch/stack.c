#include "lib.h"

void walk_stack(stack_t* buf) {
  /* with -fno-omit-frame-pointer the frame pointer gets put
   * in register x29 */
  u64 init_fp = read_reg(x29);
  u64* fp = (u64*)init_fp;

  walk_stack_from(fp, buf);
}

static u64 _stack_range_top(u64 fp) {
  u64 init_fp = STACK_SAFE_PA(fp);

  u64 stack_end;
  for (int i = 0; i < NO_CPUS; i++) {
    for (int el = 0; el < 2; el++) {
      u64 _stack_start, _stack_end;
      _stack_start = STACK_PYS_THREAD_BOT_ELx(i, el);
      _stack_end = STACK_PYS_THREAD_TOP_ELx(i, el);

      if (_stack_start <= init_fp && init_fp < _stack_end) {
        stack_end = _stack_end;
        break;
      }
    }
  }

  return stack_end;
}

u8 stack_in_range(u64 fp, u64 stack_top) {
  /* get PA */
  fp = STACK_SAFE_PA(fp);
  return (stack_top - STACK_SIZE) <= fp && fp < stack_top;
}

void walk_stack_from(u64* fp, stack_t* buf) {
  int i = 0;

  u64 init_fp = (u64)fp;
  u64 stack_top = _stack_range_top(init_fp);

  while (1) {
    /* each stack frame is
    * ------ fp
    * u64: fp
    * u64: ret
    */

    if (! stack_in_range((u64)fp, stack_top)) {
      break;
    }

    u64* prev_fp  = (u64*)*fp;
    u64 ret       = *(fp+1);
    fp = prev_fp;
    buf->frames[i  ].next = (u64)fp;
    buf->frames[i++].ret  = ret;

    /* the frame pointer is initialised to 0
     * for all entry points */
    if (prev_fp == 0) {
      break;
    }
  }

  buf->no_frames = i;
}