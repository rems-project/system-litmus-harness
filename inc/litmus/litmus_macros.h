#ifndef LITMUS_MACROS_H
#define LITMUS_MACROS_H

#include "bitwise.h"

/** standardized indexes for heap variables
 *
 * e.g. assume .heap_var_names={"x", "y", "z", ...}
 * then instead of data->var[0]  can write data->var[X]
 */
#define X 0
#define Y 1
#define Z 2

/** Generates the asm sequence to do an exception return to the _next_ instruction
 *
 * uses the given general-purpose register name as a temporary register
 * this register _must_ be in the clobber list of the test
 */
#define ERET_TO_NEXT(reg) "mrs " #reg ", elr_el1\nadd " #reg "," #reg ",#4\nmsr elr_el1," #reg "\neret\n"

/**
 * Given a VA return a value usable as a register input to a TLBI that affects that address.
 */
#define PAGE(va) (((uint64_t)(va) >> 12) & BITMASK(48))

#endif /* LITMUS_MACROS_H */