#ifndef MACROS_H
#define MACROS_H

/* miscellaneous useful C macros
 */

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) < (b) ? (b) : (a))

#define __STR(x) #x

/** expand a macro into a C string literal
 * e.g.
 * with #define FOO 42
 *
 * then,
 * char* s = STR_LITERAL(FOO);
 *
 * will expand to:
 * char* s = "42";
 */
#define STR_LITERAL(x) __STR(x)

/* for expansion and catenation */

/* this will not expand x or y on substitution since
 * there is a ## between x and y.
 *
 * to ensure expansion, use CONCAT_MACROS
 */
#define CONCAT_NOEXPAND(x, y) x##y

#define CONCAT_MACROS(x, y) CONCAT_NOEXPAND(x, y)

#define FRESH_VAR CONCAT_MACROS(__fresh_var_v, __COUNTER__)

#endif /* MACROS_H */