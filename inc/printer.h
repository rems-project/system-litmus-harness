#ifndef PRINTER_H
#define PRINTER_H

#include <stdarg.h>
#include "types.h"

/* printf support */
void putc(char c);
void puts(const char *s);
void puthex(u64 n);
void putdec(u64 n);

void vprintf(int mode, const char* fmt, va_list ap);
char* sprintf(char* out, const char* fmt, ...);
char* vsprintf(char* out, int mode, const char* fmt, va_list ap);
void printf(const char* fmt, ...);
void printf_with_fileloc(const char* level_name, int mode, const char* filename, const int line, const char* func, const char* fmt, ...);
void trace(const char* fmt, ...);
void verbose(const char* fmt, ...);
/* debug() declared in debug.h */


#define SPRINT_TIME_HHMMSS 0
#define SPRINT_TIME_HHMMSSCLK 1

char* sprint_time(char* out, u64 clk, int mode);

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