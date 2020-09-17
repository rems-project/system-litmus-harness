#ifndef PRINTER_H
#define PRINTER_H

#include <stdint.h>
#include <stdarg.h>

/* printf support */
void putc(char c);
void puts(const char *s);
void puthex(uint64_t n);
void putdec(uint64_t n);

void vprintf(int mode, const char* fmt, va_list ap);
char* sprintf(char* out, const char* fmt, ...);
char* vsprintf(char* out, int mode, const char* fmt, va_list ap);
void printf(const char* fmt, ...);
void trace(const char* fmt, ...);
void verbose(const char* fmt, ...);
/* debug() declared in debug.h */

#define SPRINT_TIME_HHMMSS 0
#define SPRINT_TIME_HHMMSSCLK 1

char* sprint_time(char* out, uint64_t clk, int mode);

#endif /* PRINTER_H */