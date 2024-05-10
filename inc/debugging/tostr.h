#ifndef TOSTR_H
#define TOSTR_H

#include "lib.h"

#define __REPR_SPRINTF_FMT_1(ty, name, ...) "." STR_LITERAL(name) "=" PR##ty
#define __REPR_SPRINTF_FMT_2(ty, name, ...) __REPR_SPRINTF_FMT_1(ty, name) ", " __REPR_SPRINTF_FMT_1(__VA_ARGS__)
#define __REPR_SPRINTF_FMT_3(ty, name, ...) __REPR_SPRINTF_FMT_1(ty, name) ", " __REPR_SPRINTF_FMT_2(__VA_ARGS__)
#define __REPR_SPRINTF_FMT_4(ty, name, ...) __REPR_SPRINTF_FMT_1(ty, name) ", " __REPR_SPRINTF_FMT_3(__VA_ARGS__)
#define __REPR_SPRINTF_FMT_5(ty, name, ...) __REPR_SPRINTF_FMT_1(ty, name) ", " __REPR_SPRINTF_FMT_4(__VA_ARGS__)
#define __REPR_SPRINTF_FMT_6(ty, name, ...) __REPR_SPRINTF_FMT_1(ty, name) ", " __REPR_SPRINTF_FMT_5(__VA_ARGS__)
#define __REPR_SPRINTF_FMT_7(ty, name, ...) __REPR_SPRINTF_FMT_1(ty, name) ", " __REPR_SPRINTF_FMT_6(__VA_ARGS__)
#define __REPR_SPRINTF_FMT_8(ty, name, ...) __REPR_SPRINTF_FMT_1(ty, name) ", " __REPR_SPRINTF_FMT_7(__VA_ARGS__)
#define __REPR_SPRINTF_FMT_9(ty, name, ...) __REPR_SPRINTF_FMT_1(ty, name) ", " __REPR_SPRINTF_FMT_8(__VA_ARGS__)
#define __REPR_SPRINTF_FMT_10(ty, name, ...) __REPR_SPRINTF_FMT_1(ty, name) ", " __REPR_SPRINTF_FMT_9(__VA_ARGS__)

#define __REPR_SPRINTF_ARG_ONE_obj(ty, name) (TOSTR(ty, x->name))
#define __REPR_SPRINTF_ARG_ONE_val(ty, name) x->name
#define __PRTY(kind, ty, name) __REPR_SPRINTF_ARG_ONE_##kind(ty, name)
#define _PRTY(kind, ty, name) __PRTY(kind, ty, name)
#define __REPR_SPRINTF_ARGS_1(ty, name, ...) _PRTY(PRTy##ty, ty, name)
#define __REPR_SPRINTF_ARGS_2(ty, name, ...) __REPR_SPRINTF_ARGS_1(ty, name), __REPR_SPRINTF_ARGS_1(__VA_ARGS__)
#define __REPR_SPRINTF_ARGS_3(ty, name, ...) __REPR_SPRINTF_ARGS_1(ty, name), __REPR_SPRINTF_ARGS_2(__VA_ARGS__)
#define __REPR_SPRINTF_ARGS_4(ty, name, ...) __REPR_SPRINTF_ARGS_1(ty, name), __REPR_SPRINTF_ARGS_3(__VA_ARGS__)
#define __REPR_SPRINTF_ARGS_5(ty, name, ...) __REPR_SPRINTF_ARGS_1(ty, name), __REPR_SPRINTF_ARGS_4(__VA_ARGS__)
#define __REPR_SPRINTF_ARGS_6(ty, name, ...) __REPR_SPRINTF_ARGS_1(ty, name), __REPR_SPRINTF_ARGS_5(__VA_ARGS__)
#define __REPR_SPRINTF_ARGS_7(ty, name, ...) __REPR_SPRINTF_ARGS_1(ty, name), __REPR_SPRINTF_ARGS_6(__VA_ARGS__)
#define __REPR_SPRINTF_ARGS_8(ty, name, ...) __REPR_SPRINTF_ARGS_1(ty, name), __REPR_SPRINTF_ARGS_7(__VA_ARGS__)
#define __REPR_SPRINTF_ARGS_9(ty, name, ...) __REPR_SPRINTF_ARGS_1(ty, name), __REPR_SPRINTF_ARGS_8(__VA_ARGS__)
#define __REPR_SPRINTF_ARGS_10(ty, name, ...) __REPR_SPRINTF_ARGS_1(ty, name), __REPR_SPRINTF_ARGS_9(__VA_ARGS__)

/* define up to 20 fields */
#define __REPR_SPRINTF_FMT(                                                                           \
  a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, len, ... \
)                                                                                                     \
  __REPR_SPRINTF_FMT_##len(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20)
#define __REPR_SPRINTF_ARGS(                                                                          \
  a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, len, ... \
)                                                                                                     \
  __REPR_SPRINTF_ARGS_##len(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20)

#define __MAKE_REPR_SPRINTF(ty, ...) \
  ("(" STR_LITERAL(ty                \
  ) "){" __REPR_SPRINTF_FMT(__VA_ARGS__, 10, 10, 9, 9, 8, 8, 7, 7, 6, 6, 5, 5, 4, 4, 3, 3, 2, 2, 1, 1) "}")
#define __MAKE_REPR_SPRINTF_ARGS(...) \
  __REPR_SPRINTF_ARGS(__VA_ARGS__, 10, 10, 9, 9, 8, 8, 7, 7, 6, 6, 5, 5, 4, 4, 3, 3, 2, 2, 1, 1)

#define _MAKE_REPR_SPRINTF(ty, a) __MAKE_REPR_SPRINTF(ty, a)
#define _MAKE_REPR_SPRINTF_ARGS(a) __MAKE_REPR_SPRINTF_ARGS(a)

/** define the string representation of a struct
 * usage:
 *
 * typedef struct {
 *   int x;
 *   char* y;
 *   ...
 * } tyname_t;
 * #define PRtyname_t "%o"
 * #define PRTytyname_t obj
 * #define REPR_ARGS_tyname_t \
 *    int, x, \
 *    str, y
 *
 *
 * then in the source file:
 * printf("t = %o\n", TOSTR(tyname_t, p));
 */
#define TOSTR(ty, v)                                                                               \
  ({                                                                                               \
    ty* x = (v);                                                                                   \
    char* s = ALLOC_SIZED(1024);                                                                   \
    STREAM* out = NEW_BUFFER(s, 1024);                                                             \
    sprintf(out, _MAKE_REPR_SPRINTF(ty, REPR_ARGS_##ty), _MAKE_REPR_SPRINTF_ARGS(REPR_ARGS_##ty)); \
    s;                                                                                             \
  })

#endif /* TOSTR_H */