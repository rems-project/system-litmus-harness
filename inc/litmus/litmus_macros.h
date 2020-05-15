#ifndef LITMUS_MACROS_H
#define LITMUS_MACROS_H

#include "bitwise.h"

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