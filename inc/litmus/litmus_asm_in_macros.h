#ifndef LITMUS_ASM_IN_MACROS_H
#define LITMUS_ASM_IN_MACROS_H

#include "lib.h"
#include "litmus_asm_reg_macros.h"

/* for ASM_VARS and ASM_REGS */
#define VA_COUNT_(a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,n,...) n
#define VA_COUNT(...) VA_COUNT_(__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)

#define VAR_FNs_1(fn, data, suffix, a, ...) [a##suffix] "r" (fn(data,#a))
#define VAR_FNs_2(fn, data, suffix, a, ...) VAR_FNs_1(fn, data, suffix, a), VAR_FNs_1(fn, data, suffix, __VA_ARGS__)
#define VAR_FNs_3(fn, data, suffix, a, ...) VAR_FNs_1(fn, data, suffix, a), VAR_FNs_2(fn, data, suffix, __VA_ARGS__)
#define VAR_FNs_4(fn, data, suffix, a, ...) VAR_FNs_1(fn, data, suffix, a), VAR_FNs_3(fn, data, suffix, __VA_ARGS__)
#define VAR_FNs_5(fn, data, suffix, a, ...) VAR_FNs_1(fn, data, suffix, a), VAR_FNs_4(fn, data, suffix, __VA_ARGS__)
#define VAR_FNs_6(fn, data, suffix, a, ...) VAR_FNs_1(fn, data, suffix, a), VAR_FNs_5(fn, data, suffix, __VA_ARGS__)
#define VAR_FNs_(fn, data, suffix, a, b, c, d, e, f, n, ...) VAR_FNs_##n(fn, data, suffix, a, b, c, d, e, f)
#define VAR_FNs_UNKNOWN(fn, data, suffix, ...) VAR_FNs_(fn, data, suffix, __VA_ARGS__, 6, 5, 4, 3, 2, 1)

#define VAR_VAs(data, ...) VAR_FNs_UNKNOWN(var_va, data,, __VA_ARGS__)
#define VAR_PTEs(data, ...) VAR_FNs_UNKNOWN(var_pte, data,pte, __VA_ARGS__)
#define VAR_DESCs(data, ...) VAR_FNs_UNKNOWN(var_desc, data,desc, __VA_ARGS__)
#define VAR_PAGEs(data, ...) VAR_FNs_UNKNOWN(var_page, data,page, __VA_ARGS__)

#define REG_FNs_1(data, a, ...) [IDENT(a)] "r" (out_reg(data,HUMAN(a)))
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
#define STRINGIFY_N(a, b, c, d, e, f, n, ...) STRINGIFY_##n(a, b, c, d, e, f)
#define STRINGIFY(...) STRINGIFY_N(__VA_ARGS__, 6, 5, 4, 3, 2, 1)

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
#define BUILD_THREADS_1 (th_f*)P0
#define BUILD_THREADS_2 (th_f*)P1, BUILD_THREADS_1
#define BUILD_THREADS_3 (th_f*)P2, BUILD_THREADS_2
#define BUILD_THREADS_4 (th_f*)P3, BUILD_THREADS_3
#define BUILD_THREADS_5 (th_f*)P4, BUILD_THREADS_4
#define BUILD_THREADS_6 (th_f*)P5, BUILD_THREADS_5

#endif /* LITMUS_ASM_IN_MACROS_H */