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

#endif /* MACROS_H */