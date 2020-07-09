#ifndef LITMUS_MACROS_H
#define LITMUS_MACROS_H

#include "bitwise.h"
#include "litmus_asm_in_macros.h"

/** these macros are for building the litmus_test_t struct easier
 * from a group of regs/vars
 * e.g.
 *  #define VARS x, y, z
 *  #define REGS p0r1, p0r2
 *
 *  litmus_test_t test = {
 *     "name",
 *     MAKE_THREADS(2),
 *     MAKE_VARS(VARS),
 *     MAKE_REGS(REGS),
 *     INIT_STATE(
 *        3,
 *        INIT_VAR(x, 0),
 *        INIT_VAR(y, 1),
 *        INIT_UNMAPPED(z),
 *     )
 *  }
 */
#define MAKE_THREADS(n) n,(th_f*[]){BUILD_THREADS_##n}
#define MAKE_VARS(...) VA_COUNT(__VA_ARGS__), (const char*[]) {STRINGIFY(__VA_ARGS__)}
#define MAKE_REGS(...) VA_COUNT(__VA_ARGS__), (const char*[]) {HUMANIZE(__VA_ARGS__)}

/* for defining the initial state */
#define INIT_STATE(N, ...) .no_init_states=N,.init_states=(init_varstate_t*[]){__VA_ARGS__}
#define INIT_VAR(var, value) &(init_varstate_t){#var, TYPE_HEAP, {value}}
#define INIT_PGT(var, value) &(init_varstate_t){#var, TYPE_PTE, {value}}
#define INIT_ALIAS(var, othervar) &(init_varstate_t){#var, TYPE_ALIAS, {.aliasname=(const char*)#othervar}}
#define INIT_PERMISSIONS(var, ap) &(init_varstate_t){#var, TYPE_AP, {ap}}
#define INIT_UNMAPPED(var) &(init_varstate_t){#var, TYPE_PTE, {0}}

/** these are for building the asm blocks automatically
 * e.g.
 *  #define VARS x, y, z
 *  #define REGS p0r1, p2r3
 *
 *  ...
 *  asm volatile (
 *    ...
 *  :
 *  : ASM_VARS(data, VARS),
 *    ASM_REGS(data, REGS)
 *  : ...
 *  )
 */
#define ASM_VARS(data, ...) VAR_VAs(data, __VA_ARGS__), VAR_PTEs(data, __VA_ARGS__), VAR_DESCs(data, __VA_ARGS__), VAR_PAGEs(data, __VA_ARGS__)
#define ASM_REGS(data, ...) REG_FNs_UNKNOWN(data, __VA_ARGS__)

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