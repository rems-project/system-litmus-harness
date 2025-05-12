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

enum stream_kind { STREAM_BUFFER, STREAM_UART };

typedef struct
{
  enum stream_kind kind;
  union {
    char* __buffer;
  };
  size_t rem;
} STREAM;

extern STREAM __UART;

#define UART &__UART
#define NEW_BUFFER(B, N) (&(STREAM){ .kind = STREAM_BUFFER, .__buffer = (B), .rem = (N) })

#define PRINT_MODE_NORM 0
#define PRINT_MODE_TRACE 1
#define PRINT_MODE_NOINDENT 2
#define PRINT_MODE_WARNING 4
#define PRINT_MODE_ERROR 8

void vprintf(int mode, const char* fmt, va_list ap);
void sprintf(STREAM* out, const char* fmt, ...);
void vsprintf(STREAM* out, int mode, const char* fmt, va_list ap);
void printf(const char* fmt, ...);
void mprintf(int mode, const char* fmt, ...);
void printf_with_fileloc(
  const char* level_name, int mode, const char* filename, const int line, const char* func, const char* fmt, ...
);
void trace(const char* fmt, ...);
void verbose(const char* fmt, ...);
void warning(const warnings_t category, const char* fmt, ...);
void error(const warnings_t category, const char* fmt, ...);

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

void sprint_time(STREAM* out, u64 clk, time_format_t mode);
void sprint_reg(STREAM* out, const char* reg_name, output_style_t style);

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

/* ANSI */
#define ANSI_ESC "\033"
#define ANSI_CSI "["
#define ANSI_FG_RESET "0"
#define ANSI_FG_RED "31"
#define ANSI_FG_YELLOW "33"
#define ANSI_FG_GRAY "90"
#define ANSI_SEQ ";"

#define ANSI_COLOR(c) ANSI_ESC ANSI_CSI c "m"
#define COLOR_RESET ANSI_COLOR(ANSI_FG_RESET)
#define COLOR_YELLOW ANSI_COLOR(ANSI_FG_RESET ANSI_SEQ ANSI_FG_YELLOW)
#define COLOR_RED ANSI_COLOR(ANSI_FG_RESET ANSI_SEQ ANSI_FG_RED)
#define COLOR_GRAY ANSI_COLOR(ANSI_FG_RESET ANSI_SEQ ANSI_FG_GRAY)

#endif /* PRINTER_H */