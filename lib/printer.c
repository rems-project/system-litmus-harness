#include <stdarg.h>

#include "lib.h"

/* print buffers
 * are protected by __PR_LOCK
 */
static volatile lock_t __PR_LOCK;

/** print buffer used by verbose/debug/etc
 * to store computed format strs
 * protected by __PR_VERB_LOCK
 */
static char __verbose_print_buf[1024];
static volatile lock_t __PR_VERB_LOCK;

STREAM __UART = {
  .kind = STREAM_UART,
};

void sputc(STREAM* out, char c) {
  switch (out->kind) {
  case STREAM_UART:
    putc(c);
    break;
  case STREAM_BUFFER:
    if (out->rem == 0)
      fail("empty stream");

    *out->__buffer++ = c;
    out->rem--;
    break;
  }
}

void sputs(STREAM* out, char* s) {
  if (!s) {
    sputs(out, "NULL");
    return;
  }

  while (*s) {
    sputc(out, *s++);
  }
}

void sputchar(STREAM* out, char c) {
  /* dont write NUL */
  if (c == 0)
    return;

  sputc(out, c);
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

void sputdec(STREAM* out, u64 n) {
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
    sputc(out, digits[i]);
  }
}

void sputhex(STREAM* out, u64 n) {
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
    sputc(out, _hex[i]);

    /* group 2-byte sequences */
    if (((i % 4) == 0) && (i > 0))
      sputc(out, '_');
  }
}

void putc(char c) {
  write_stdout(c);
}

void puts(const char* s) {
  while (*s)
    putc(*s++);
}

void puthex(u64 n) {
  sputhex(UART, n);
}

void putdec(u64 n) {
  sputdec(UART, n);
}

void sputarray(STREAM* out, char* fmt, void* p, int count) {
  sputc(out, '[');
  char* arr = p;
  for (int i = 0; i < count; i++) {
    if (strcmp(fmt, "%d")) {
      int* x = (int*)(arr + i * sizeof(int));
      sprintf(out, fmt, *x);
    }

    if (i < count - 1)
      sputc(out, ' ');
  }
  sputc(out, ']');
}

void vsprintf(STREAM* out, int mode, const char* fmt, va_list ap) {
  char* p = (char*)fmt;
  while (*p) {
    char c = *p;
    if (c != '%') {
      sputc(out, c);
    } else if (c == '%') {
      c = *++p;
      if (c == '%') {
        sputc(out, '%');
      } else if (c == 'd') {
        if (*(p + 1) == 'x') {
          sputhex(out, va_arg(ap, int));
          p++;
        } else {
          sputdec(out, va_arg(ap, int));
        }
      } else if (c == 'x') {
        sputhex(out, va_arg(ap, u32));
      } else if (c == 'l') {
        if (*(p + 1) == 'x') {
          sputhex(out, va_arg(ap, long));
          p++;
        } else if (*(p + 1) == 'd') {
          sputdec(out, va_arg(ap, long));
          p++;
        }
      } else if (c == 'c') {
        /* Note that char/u8/etc gets promoted to 'int'
         * on passing through varargs
         * so we want to take a whole int here
         */
        int arg = va_arg(ap, int);
        sputchar(out, (char)arg);
      } else if (c == 's' || c == 'o') {
        char* sp = va_arg(ap, char*);
        sputs(out, sp);
        if (c == 'o') {
          free(sp);
        }
      } else if (c == 'p') {
        sputs(out, "0x");
        sputhex(out, va_arg(ap, long));
      } else if (c == 'A') {
        char arr_item_fmt[10];
        sprintf(NEW_BUFFER(arr_item_fmt, 10), "%%%c", *(p + 1));
        sputarray(out, arr_item_fmt, va_arg(ap, void*), va_arg(ap, int));
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

  /* revert stream back to NUL
   * so we can overwrite it by successive calls to sprintf */
  if (out->kind == STREAM_BUFFER) {
    sputc(out, '\0');
    out->__buffer--;
    out->rem++;
  }
}

void vprintf(int mode, const char* fmt, va_list ap) {
  lock(&__PR_LOCK);
  if (mode == 0) {
    for (int i = 0; i < get_cpu(); i++) {
      sputs(UART, "\t\t\t");
    }
  }
  vsprintf(UART, mode, fmt, ap);
  unlock(&__PR_LOCK);
}

void sprintf(STREAM* out, const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vsprintf(out, 0, fmt, ap);
  va_end(ap);
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

    va_list ap;
    va_start(ap, fmt);
    sputc(UART, '#');
    vprintf(1, fmt, ap);
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

void __print_frame_unwind(STREAM* out, int skip) {
  stack_t* stack = (stack_t*)__debug_stack_buf;
  clear_stack(stack);
  collect_stack(stack);

  for (int i = skip; i < stack->no_frames; i++) {
    stack_frame_t* frame = &stack->frames[i];
    sprintf(out, "%p", frame->ret);
    if (i < stack->no_frames - 1) {
      sprintf(out, ":");
    }
  }

  sprintf(out, ""); /* put a NUL at the end always, even if no frames */
}

void printf_with_fileloc(
  const char* level, int mode, const char* filename, const int line, const char* func, const char* fmt, ...
) {
  int cpu = get_cpu();
  lock(&__PR_VERB_LOCK);

  /* construct format string dynamically
   * by prepending stack trace */
  __print_frame_unwind(NEW_BUFFER(__debug_frame_buf, 1024), 2);
  sprint_time(NEW_BUFFER(__debug_time_buf, 1024), read_clk(), SPRINT_TIME_HHMMSSCLK);
  sprintf(
    NEW_BUFFER(__verbose_print_buf, 1024),
    "(%s) CPU%d:%s:[%s %s:%d (%s)] %s",
    __debug_time_buf,
    cpu,
    level,
    __debug_frame_buf,
    filename,
    line,
    func,
    fmt
  );

  va_list ap;
  va_start(ap, fmt);
  vprintf(mode, __verbose_print_buf, ap);
  va_end(ap);
  unlock(&__PR_VERB_LOCK);
}

void _fail(const char* filename, const int line, const char* func, const char* fmt, ...) {
  int cpu = get_cpu();
  lock(&__PR_VERB_LOCK);

  sprintf(NEW_BUFFER(__verbose_print_buf, 1024), "[%s:%d %s (CPU%d)] %s", filename, line, func, cpu, fmt);

  va_list ap;
  va_start(ap, fmt);
  vprintf(1, __verbose_print_buf, ap);
  va_end(ap);
  unlock(&__PR_VERB_LOCK);
}

/** printing times */
void sprint_time(STREAM* out, u64 clk, time_format_t mode) {
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

  if (mode == SPRINT_TIME_SS) {
    sprintf(out, "%ld", tot_secs);
    return;
  } else if (mode == SPRINT_TIME_SSDOTMS) {
    u64 rem_ms = rem_ticks / (TICKS_PER_SEC / 1000);
    sprintf(out, "%ld.%ld", tot_secs, rem_ms);
    return;
  }

  if (rem_hours < 10) {
    sprintf(out, "0%d", rem_hours);
  } else {
    sprintf(out, "%d", rem_hours);
  }

  sprintf(out, ":");

  if (rem_mins < 10) {
    sprintf(out, "0%d", rem_mins);
  } else {
    sprintf(out, "%d", rem_mins);
  }

  sprintf(out, ":");

  if (rem_secs < 10) {
    sprintf(out, "0%d", rem_secs);
  } else {
    sprintf(out, "%d", rem_secs);
  }

  if (mode == SPRINT_TIME_HHMMSSCLK) {
    sprintf(out, ":%ld", rem_ticks);
  }
}

/** printing registers */
int extract_tid(const char* reg_name) {
  if (reg_name[0] != 'p') {
    fail("unknown register %s\n", reg_name);
  }
  return ctoi(reg_name[1]);
}

int extract_gprid(const char* reg_name) {
  if (reg_name[0] != 'p' || reg_name[2] != ':') {
    fail("unknown register %s\n", reg_name);
  }
  return atoi((char*)reg_name + 4);
}

void sprint_reg(STREAM* out, const char* reg_name, output_style_t style) {
  switch (style) {
  case STYLE_ORIGINAL:
    sprintf(out, "%s", reg_name);
    return;
  case STYLE_HERDTOOLS:
    sprintf(out, "%d:X%d", extract_tid(reg_name), extract_gprid(reg_name));
    return;
  default:
    unreachable();
  }
}