#ifndef STACK_H
#define STACK_H

/** header for a simple stack walker
 *
 * requires -fno-omit-frame-pointer
 */

#include "lib.h"

typedef struct
{
  u64 next;
  u64 ret;
} stack_frame_t;

typedef struct
{
  int no_frames;
  stack_frame_t frames[];
} stack_t;

typedef void(stack_cb_fn)(void* arg, u64* fp, u64 ret_addr);

void collect_stack_from(u64* fp, stack_t* buf);
void collect_stack(stack_t* buf);

void walk_stack(stack_cb_fn cb, void* arg);
void walk_stack_from(u64* fp, stack_cb_fn cb, void* arg);

void clear_stack(stack_t* buf);

#endif /* STACK_H */