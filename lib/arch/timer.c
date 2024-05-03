#include "lib.h"

/** the value of the clock at the beginning of time
 */
u64 INIT_CLOCK;

/** fixed frequency
 */
u64 TICKS_PER_SEC;

/** read the clock */
u64 read_clk(void) {
  return read_sysreg(cntvct_el0);
}

/** read the clock frequency in Hz */
u64 read_clk_freq(void) {
  return read_sysreg(cntfrq_el0);
}