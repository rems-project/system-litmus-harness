#include "lib.h"

/** read the clock */
uint64_t read_clk(void) {
    return read_sysreg(cntvct_el0);
}
