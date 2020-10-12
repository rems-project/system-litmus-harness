#include <stdarg.h>
#include <stdint.h>

#include "lib.h"

/* print buffer
 * used to store results of non-(s)printf before writing
 * is protected by __PR_LOCK
 */
static char __print_buf[1024];
static volatile lock_t __PR_LOCK;

/** print buffer used by verbose/debug/etc
 * to store computed format strs
 * protected by __PR_VERB_LOCK
 */
static char __verbose_print_buf[1024];
static volatile lock_t __PR_VERB_LOCK;

char* sputc(char* out, const char c) {
  *out = c;
  return out + 1;
}

char* sputs(char* out, const char* s) {
  if (!s) {
    return sputs(out, "NULL");
  }

  while (*s) {
    out = sputc(out, *s++);
  }

  return out;
}

char __get_hexchar(uint64_t n) {
  if (n < 10) {
    return n + 48;
  } else if (n < 16) {
    return n - 10 + 65;
  } else {
    puts("! fail: __get_hexchar out of bounds\n");
  }

  return 0;
}

char* sputdec(char* out, uint64_t n) {
  char digits[64];
  int i = 0;
  if (n == 0) {
    digits[0] = '0';
    i++;
  }

  while (n > 0) {
    uint64_t mod = n % 10;
    n -= mod;
    n /= 10;

    digits[i] = mod + 48;
    i++;
  }

  for (i--; i >= 0; i--) {
    out = sputc(out, digits[i]);
  }

  return out;
}

char* sputhex(char* out, uint64_t n) {
  char _hex[64];
  int i = 0;
  if (n == 0) {
    _hex[0] = '0';
    i++;
  }
  while (n > 0) {
    uint64_t mod = n % 16;
    n -= mod;
    n /= 16;

    _hex[i] = __get_hexchar(mod);
    i++;
  }

  for (i--; i >= 0; i--) {
    out = sputc(out, _hex[i]);
  }

  return out;
}

void putc(char c) {
  write_stdout(c);
}

void puts(const char* s) {
  while (*s)
    putc(*s++);
}

void puthex(uint64_t n) {
  char s[100];
  char* q = s;
  q = sputhex(q, n);
  q = sputc(q, '\0');
  puts(s);
}

void putdec(uint64_t n) {
  char s[100];
  char* q = s;
  q = sputdec(q, n);
  q = sputc(q, '\0');
  puts(s);
}

char* sputarray(char* out, char* fmt, void* p, int count) {
  out = sputc(out, '[');
  char* arr = p;
  for (int i = 0; i < count; i++) {
    if (strcmp(fmt, "%d")) {
      int* x = (int*)(arr + i * sizeof(int));
      out = sprintf(out, fmt, *x);
    }

    if (i < count - 1)
      out = sputc(out, ' ');
  }
  out = sputc(out, ']');
  return out;
}

char* vsprintf(char* out, int mode, const char* fmt, va_list ap) {
  char* p = (char*)fmt;
  while (*p) {
    char c = *p;
    if (c != '%') {
      out = sputc(out, c);
    } else if (c == '%') {
      c = *++p;
      if (c == '%') {
        out = sputc(out, '%');
      } else if (c == 'd') {
        if (*(p + 1) == 'x') {
          out = sputhex(out, va_arg(ap, int));
          p++;
        } else {
          out = sputdec(out, va_arg(ap, int));
        }
      } else if (c == 'l') {
        if (*(p + 1) == 'x') {
          out = sputhex(out, va_arg(ap, long));
          p++;
        } else if (*(p + 1) == 'd') {
          out = sputdec(out, va_arg(ap, long));
          p++;
        }
      } else if (c == 'c') {
        out = sputc(out, va_arg(ap, int));
      } else if (c == 's' || c == 'o') {
        char* sp = va_arg(ap, char*);
        out = sputs(out, sp);
        if (c == 'o') {
          free(sp);
        }
      } else if (c == 'p') {
        out = sputs(out, "0x");
        out = sputhex(out, va_arg(ap, long));
      } else if (c == 'A') {
        char fmt[10];
        sprintf(fmt, "%%%c", *(p + 1));
        out = sputarray(out, fmt, va_arg(ap, void*), va_arg(ap, int));
        p++;
      } else {
        puts("!! printf: unknown symbol: ");
        putc(c);
        puts("\n");
        abort();
      }
    }

    p++;
  }
  sputc(out, '\0');
  return out; /* return reference to the NUL, not to after it
               * to make chaining vsprintf() calls easier */
}

void vprintf(int mode, const char* fmt, va_list ap) {
  lock(&__PR_LOCK);
  char* out = &__print_buf[0];

  if (mode == 0) {
    for (int i = 0; i < get_cpu(); i++) {
      out = sputs(out, "\t\t\t");
    }
  }

  vsprintf(out, mode, fmt, ap);
  if (strlen(__print_buf) > 1024) {
    fail("! vprintf: overflow, format string too large.\n");
  }

  puts(__print_buf);
  unlock(&__PR_LOCK);
}

char* sprintf(char* out, const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  out = vsprintf(out, 0, fmt, ap);
  va_end(ap);
  return out;
}

void printf(const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vprintf(0, fmt, ap);
  va_end(ap);
}

void trace(const char* fmt, ...) {
  if (TRACE) {
    va_list ap;
    va_start(ap, fmt);
    vprintf(0, fmt, ap);
    va_end(ap);
  }
}

void verbose(const char* fmt, ...) {
  if (VERBOSE) {
    lock(&__PR_VERB_LOCK);
    sprintf(__verbose_print_buf, "#%s", fmt);
    if (strlen(__verbose_print_buf) > 100) {
      fail("! verbose: overflow, format string too large.\n");
    }

    va_list ap;
    va_start(ap, fmt);
    vprintf(1, __verbose_print_buf, ap);
    va_end(ap);
    unlock(&__PR_VERB_LOCK);
  }
}

void _debug(const char* filename, const int line, const char* func, const char* fmt, ...) {
  if (DEBUG) {
    int cpu = get_cpu();
    lock(&__PR_VERB_LOCK);
    sprintf(__verbose_print_buf, "[%s:%d %s (CPU%d)] %s", filename, line, func, cpu, fmt);
    if (strlen(__verbose_print_buf) > 1024) {
      fail("! debug: overflow, format string too large.\n");
    }

    va_list ap;
    va_start(ap, fmt);
    vprintf(1, __verbose_print_buf, ap);
    va_end(ap);
    unlock(&__PR_VERB_LOCK);
  }
}

/** printing times */
char* sprint_time(char* out, uint64_t clk, int mode) {
  /* clk is absolute */
  /* theoretically INIT_CLOCK is ~0, but we cannot be sure */
  clk = clk - INIT_CLOCK;

  uint64_t tot_secs = clk / TICKS_PER_SEC;
  uint64_t rem_ticks = clk % TICKS_PER_SEC;
  uint64_t rem_secs = tot_secs;
  uint64_t rem_hours = rem_secs / (60 * 60);
  rem_secs = rem_secs % (60 * 60);
  uint64_t rem_mins = rem_secs / 60;
  rem_secs = rem_secs % 60;

  if (rem_hours < 10) {
    out = sprintf(out, "0%d", rem_hours);
  } else {
    out = sprintf(out, "%d", rem_hours);
  }

  out = sprintf(out, ":");

  if (rem_mins < 10) {
    out = sprintf(out, "0%d", rem_mins);
  } else {
    out = sprintf(out, "%d", rem_mins);
  }

  out = sprintf(out, ":");

  if (rem_secs < 10) {
    out = sprintf(out, "0%d", rem_secs);
  } else {
    out = sprintf(out, "%d", rem_secs);
  }

  if ((mode & 1) == 1) {
    out = sprintf(out, ":%ld", rem_ticks);
  }

  return out;
}