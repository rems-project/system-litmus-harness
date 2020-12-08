#include "lib.h"

/* these functions switch to using the
 * memory-mapped stack regions to provide
 * protections over other threads stacks' during execution.
 *
 * note that although these functions are called from EL1
 * we use SP_EL0 during execution of the harness, so these must
 * point to the EL0 mapping not the EL1 one.
 */

void switch_to_vm_stack(void) {
  uint64_t cpu = get_cpu();
  uint64_t sp = read_reg(sp);
  uint64_t new_sp = sp - STACK_PYS_THREAD_BOT_EL0(cpu) + STACK_MMAP_THREAD_BOT_EL0(cpu);
  write_reg(new_sp, sp);
}

void switch_to_pys_stack(void) {
  uint64_t cpu = get_cpu();
  uint64_t sp = read_reg(sp);
  uint64_t new_sp = sp - STACK_MMAP_THREAD_BOT_EL0(cpu) + STACK_PYS_THREAD_BOT_EL0(cpu);
  write_reg(new_sp, sp);
}