#ifndef STACK_H
#define STACK_H

/** header for a simple stack walker
 *
 * requires -fno-omit-frame-pointer
 */

#include "lib.h"

typedef struct {
  u64 next;
  u64 ret;
} stack_frame_t;

typedef struct {
  int no_frames;
  stack_frame_t frames[];
} stack_t;

void walk_stack_from(u64* fp, stack_t* buf);
void walk_stack(stack_t* buf);

#endif /* STACK_H */