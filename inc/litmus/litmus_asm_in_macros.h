#ifndef LITMUS_ASM_IN_MACROS_H
#define LITMUS_ASM_IN_MACROS_H

#include "lib.h"
#include "litmus_asm_reg_macros.h"

/* for ASM_VARS and ASM_REGS */
#define VA_COUNT_(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, n, ...) n
#define VA_COUNT(...) VA_COUNT_(__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)

#define VAR_FNs_1(fn, data, suffix, a, ...) [a##suffix] "r"(fn(data, #a))
#define VAR_FNs_2(fn, data, suffix, a, ...) VAR_FNs_1(fn, data, suffix, a), VAR_FNs_1(fn, data, suffix, __VA_ARGS__)
#define VAR_FNs_3(fn, data, suffix, a, ...) VAR_FNs_1(fn, data, suffix, a), VAR_FNs_2(fn, data, suffix, __VA_ARGS__)
#define VAR_FNs_4(fn, data, suffix, a, ...) VAR_FNs_1(fn, data, suffix, a), VAR_FNs_3(fn, data, suffix, __VA_ARGS__)
#define VAR_FNs_5(fn, data, suffix, a, ...) VAR_FNs_1(fn, data, suffix, a), VAR_FNs_4(fn, data, suffix, __VA_ARGS__)
#define VAR_FNs_6(fn, data, suffix, a, ...) VAR_FNs_1(fn, data, suffix, a), VAR_FNs_5(fn, data, suffix, __VA_ARGS__)
#define VAR_FNs_(fn, data, suffix, a, b, c, d, e, f, n, ...) VAR_FNs_##n(fn, data, suffix, a, b, c, d, e, f)
#define VAR_FNs_UNKNOWN(fn, data, suffix, ...) VAR_FNs_(fn, data, suffix, __VA_ARGS__, 6, 5, 4, 3, 2, 1)

#define VAR_VAs(data, ...) VAR_FNs_UNKNOWN(var_va, data, , __VA_ARGS__)
#define VAR_PAs(data, ...) VAR_FNs_UNKNOWN(var_pa, data, , __VA_ARGS__)
#define VAR_PTEs(data, ...) VAR_FNs_UNKNOWN(var_pte, data, pte, __VA_ARGS__)
#define VAR_PTE_LEVEL(data, level, ...) VAR_FNs_UNKNOWN(var_pte_level##level, data, pte##level, __VA_ARGS__)
#define VAR_DESCs(data, ...) VAR_FNs_UNKNOWN(var_desc, data, desc, __VA_ARGS__)
#define VAR_PAGEs(data, ...) VAR_FNs_UNKNOWN(var_page, data, page, __VA_ARGS__)

#define VAR_PMDs(data, ...) VAR_FNs_UNKNOWN(var_pmd, data, pmd, __VA_ARGS__)
#define VAR_PUDs(data, ...) VAR_FNs_UNKNOWN(var_pud, data, pud, __VA_ARGS__)
#define VAR_PMDDESCs(data, ...) VAR_FNs_UNKNOWN(var_pmddesc, data, pmddesc, __VA_ARGS__)
#define VAR_PUDDESCs(data, ...) VAR_FNs_UNKNOWN(var_puddesc, data, puddesc, __VA_ARGS__)

#define REG_FNs_1(data, a, ...) [IDENT(a)] "r"(out_reg(data, HUMAN(a)))
#define REG_FNs_2(data, a, ...) REG_FNs_1(data, a), REG_FNs_1(data, __VA_ARGS__)
#define REG_FNs_3(data, a, ...) REG_FNs_1(data, a), REG_FNs_2(data, __VA_ARGS__)
#define REG_FNs_4(data, a, ...) REG_FNs_1(data, a), REG_FNs_3(data, __VA_ARGS__)
#define REG_FNs_5(data, a, ...) REG_FNs_1(data, a), REG_FNs_4(data, __VA_ARGS__)
#define REG_FNs_6(data, a, ...) REG_FNs_1(data, a), REG_FNs_5(data, __VA_ARGS__)
#define REG_FNs_(data, a, b, c, d, e, f, n, ...) REG_FNs_##n(data, a, b, c, d, e, f)
#define REG_FNs_UNKNOWN(data, ...) REG_FNs_(data, __VA_ARGS__, 6, 5, 4, 3, 2, 1)

/* for defining MAKE_VARS / MAKE_REGS */
#define STRINGIFY_1(a, ...) #a
#define STRINGIFY_2(a, ...) STRINGIFY_1(a), STRINGIFY_1(__VA_ARGS__)
#define STRINGIFY_3(a, ...) STRINGIFY_1(a), STRINGIFY_2(__VA_ARGS__)
#define STRINGIFY_4(a, ...) STRINGIFY_1(a), STRINGIFY_3(__VA_ARGS__)
#define STRINGIFY_5(a, ...) STRINGIFY_1(a), STRINGIFY_4(__VA_ARGS__)
#define STRINGIFY_6(a, ...) STRINGIFY_1(a), STRINGIFY_5(__VA_ARGS__)
#define STRINGIFY_7(a, ...) STRINGIFY_1(a), STRINGIFY_6(__VA_ARGS__)
#define STRINGIFY_8(a, ...) STRINGIFY_1(a), STRINGIFY_7(__VA_ARGS__)
#define STRINGIFY_9(a, ...) STRINGIFY_1(a), STRINGIFY_8(__VA_ARGS__)
#define STRINGIFY_10(a, ...) STRINGIFY_1(a), STRINGIFY_9(__VA_ARGS__)
#define STRINGIFY_11(a, ...) STRINGIFY_1(a), STRINGIFY_10(__VA_ARGS__)
#define STRINGIFY_12(a, ...) STRINGIFY_1(a), STRINGIFY_11(__VA_ARGS__)
#define STRINGIFY_N(a, b, c, d, e, f, g, h, i, j, k, l, n, ...) STRINGIFY_##n(a, b, c, d, e, f, g, h, i, j, k, l)
#define STRINGIFY(...) STRINGIFY_N(__VA_ARGS__, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)

#define HUMANIZE_1(a, ...) HUMAN(a)
#define HUMANIZE_2(a, ...) HUMANIZE_1(a), HUMANIZE_1(__VA_ARGS__)
#define HUMANIZE_3(a, ...) HUMANIZE_1(a), HUMANIZE_2(__VA_ARGS__)
#define HUMANIZE_4(a, ...) HUMANIZE_1(a), HUMANIZE_3(__VA_ARGS__)
#define HUMANIZE_5(a, ...) HUMANIZE_1(a), HUMANIZE_4(__VA_ARGS__)
#define HUMANIZE_6(a, ...) HUMANIZE_1(a), HUMANIZE_5(__VA_ARGS__)
#define HUMANIZE_N(a, b, c, d, e, f, n, ...) HUMANIZE_##n(a, b, c, d, e, f)
#define HUMANIZE(...) HUMANIZE_N(__VA_ARGS__, 6, 5, 4, 3, 2, 1)

#define IDENT(r) IDENT_##r
#define HUMAN(r) USER_##r

/* for defining MAKE_THREADS(n) */
#define BUILD_THREADS_0 NULL
#define BUILD_THREADS_1 (th_f*)P0
#define BUILD_THREADS_2 BUILD_THREADS_1, (th_f*)P1
#define BUILD_THREADS_3 BUILD_THREADS_2, (th_f*)P2
#define BUILD_THREADS_4 BUILD_THREADS_3, (th_f*)P3
#define BUILD_THREADS_5 BUILD_THREADS_4, (th_f*)P4
#define BUILD_THREADS_6 BUILD_THREADS_5, (th_f*)P5

#endif /* LITMUS_ASM_IN_MACROS_H */