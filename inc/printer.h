#ifndef PRINTER_H
#define PRINTER_H

#include <stdarg.h>
#include "types.h"

/* some printing are config-dependent */
#include "config.h"

/* printf support */
void putc(char c);
void puts(const char* s);
void puthex(u64 n);
void putdec(u64 n);

void vprintf(int mode, const char* fmt, va_list ap);
char* sprintf(char* out, const char* fmt, ...);
char* vsprintf(char* out, int mode, const char* fmt, va_list ap);
void printf(const char* fmt, ...);
void printf_with_fileloc(
  const char* level_name, int mode, const char* filename, const int line, const char* func, const char* fmt, ...
);
void trace(const char* fmt, ...);
void verbose(const char* fmt, ...);
/* debug() declared in debug.h */

/* atoi variant for single char
 * nowhere to put this really */
int ctoi(char c);

typedef enum {
  SPRINT_TIME_SSDOTMS,
  SPRINT_TIME_SS,
  SPRINT_TIME_HHMMSS,
  SPRINT_TIME_HHMMSSCLK,
} time_format_t;

char* sprint_time(char* out, u64 clk, time_format_t mode);

char* sprint_reg(char* out, const char* reg_name, output_style_t style);

/* print macros */
#define PRu64 "%lx"
#define PRTyu64 val

#define PRu32 "%x"
#define PRTyu32 val

#define PRu8 "%d"
#define PRTyu8 val

#define PRint "%d"
#define PRTyint val

#define PRstr "%s"
#define PRTystr val

#define PRptr "%p"
#define PRTyptr val

/** "obj" just means an ALLOC'd str that needs to
 * be free'd. Usually from a call to REPR(t) */
#define PRobj "%o"
#define PRTyobj obj

#endif /* PRINTER_H */