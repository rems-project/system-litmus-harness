#include "lib.h"

/** the value of the clock at the beginning of time
 */
uint64_t INIT_CLOCK;

/** fixed frequency
 */
uint64_t TICKS_PER_SEC;

/** read the clock */
uint64_t read_clk(void) {
    return read_sysreg(cntvct_el0);
}


/** read the clock frequency in Hz */
uint64_t read_clk_freq(void) {
    return read_sysreg(cntfrq_el0);
}