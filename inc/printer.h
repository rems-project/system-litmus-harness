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
void printf_with_fileloc(const char* level_name, int mode, const char* filename, const int line, const char* func, const char* fmt, ...);
void trace(const char* fmt, ...);
void verbose(const char* fmt, ...);
/* debug() declared in debug.h */


#define SPRINT_TIME_HHMMSS 0
#define SPRINT_TIME_HHMMSSCLK 1

char* sprint_time(char* out, uint64_t clk, int mode);

/* print macros */
#define PRuint64_t "%lx"
#define PRTyuint64_t val

#define PRuint32_t "%x"
#define PRTyuint32_t val

#define PRuint8_t "%d"
#define PRTyuint8_t val

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