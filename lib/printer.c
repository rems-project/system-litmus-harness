#include <stdarg.h>

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

char* sputc(char* out, char c) {
  *out = c;
  return out + 1;
}

char* sputs(char* out, char* s) {
  if (!s) {
    return sputs(out, "NULL");
  }

  while (*s) {
    out = sputc(out, *s++);
  }

  return out;
}

char __get_hexchar(u64 n) {
  if (n < 10) {
    return n + 48;
  } else if (n < 16) {
    return n - 10 + 65;
  } else {
    puts("! fail: __get_hexchar out of bounds\n");
  }

  return 0;
}

char* sputdec(char* out, u64 n) {
  char digits[64];
  int i = 0;
  if (n == 0) {
    digits[0] = '0';
    i++;
  }

  while (n > 0) {
    u64 mod = n % 10;
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

char* sputhex(char* out, u64 n) {
  char _hex[64];
  int i = 0;
  if (n == 0) {
    _hex[0] = '0';
    i++;
  }
  while (n > 0) {
    u64 mod = n % 16;
    n -= mod;
    n /= 16;

    _hex[i] = __get_hexchar(mod);
    i++;
  }

  for (i--; i >= 0; i--) {
    out = sputc(out, _hex[i]);

    /* group 2-byte sequences */
    if (((i % 4) == 0) && (i > 0))
      out = sputc(out, '_');
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

void puthex(u64 n) {
  char s[100];
  char* q = s;
  q = sputhex(q, n);
  q = sputc(q, '\0');
  puts(s);
}

void putdec(u64 n) {
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
        /* Note that char/u8/etc gets promoted to 'int'
         * on passing through varargs
         * so we want to take a whole int here
         */
        int arg = va_arg(ap, int);
        out = sputc(out, (char)arg);
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
        char arr_item_fmt[10];
        sprintf(arr_item_fmt, "%%%c", *(p + 1));
        out = sputarray(out, arr_item_fmt, va_arg(ap, void*), va_arg(ap, int));
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

/** print buffer used for the stack output
 * protected by __PR_VERB_LOCK
 */
static char __debug_frame_buf[1024];
static char __debug_stack_buf[1024];
static char __debug_time_buf[1024];

void __print_frame_unwind(char* out, int skip) {
  stack_t* stack = (stack_t*)__debug_stack_buf;
  walk_stack(stack);

  for (int i = skip; i < stack->no_frames; i++) {
    stack_frame_t* frame = &stack->frames[i];
    out = sprintf(out, "%p", frame->ret);
    if (i < stack->no_frames - 1) {
      out = sprintf(out, ":");
    }
  }

  out = sprintf(out, ""); /* put a NUL at the end always, even if no frames */
}

void printf_with_fileloc(const char* level, int mode, const char* filename, const int line, const char* func, const char* fmt, ...) {
  int cpu = get_cpu();
  lock(&__PR_VERB_LOCK);
  __print_frame_unwind(__debug_frame_buf, 2);

  if (strlen(__debug_frame_buf) > 1024) {
    /* can't use fail() here
     * so call abort() manually */
    printf("! printf_with_fileloc: overflow, unwound frame string too large.\n");
    raise_to_el1();
    abort();
  }

  sprint_time(__debug_time_buf, read_clk(), SPRINT_TIME_HHMMSSCLK);
  sprintf(__verbose_print_buf, "(%s) CPU%d:%s:[%s:%s:%d (%s)] %s", __debug_time_buf, cpu, level, __debug_frame_buf, filename, line, func, fmt);

  if (strlen(__verbose_print_buf) > 1024) {
    /* can't use fail() here
     * so call abort() manually */
    printf("! printf_with_fileloc: overflow, format string too large.\n");
    raise_to_el1();
    abort();
  }

  va_list ap;
  va_start(ap, fmt);
  vprintf(mode, __verbose_print_buf, ap);
  va_end(ap);
  unlock(&__PR_VERB_LOCK);
}

void _fail(const char* filename, const int line, const char* func, const char* fmt, ...) {
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

/** printing times */
char* sprint_time(char* out, u64 clk, int mode) {
  /* clk is absolute */
  /* theoretically INIT_CLOCK is ~0, but we cannot be sure */
  clk = clk - INIT_CLOCK;

  u64 tot_secs = clk / TICKS_PER_SEC;
  u64 rem_ticks = clk % TICKS_PER_SEC;
  u64 rem_secs = tot_secs;
  u64 rem_hours = rem_secs / (60 * 60);
  rem_secs = rem_secs % (60 * 60);
  u64 rem_mins = rem_secs / 60;
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