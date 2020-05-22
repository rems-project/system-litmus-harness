#include <stdint.h>
#include <stdarg.h>

#include "lib.h"

char* sputc(char* out, const char c) {
	*out = c;
	return out+1;
}

char* sputs(char* out, const char* s) {
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

		digits[i] = mod+48;
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

void puts(const char *s) {
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
			int* x = arr+i*sizeof(int);
			out = sprintf(out, fmt, *x);
		}

		if (i < count-1)
			out = sputc(out, ' ');
	}
	out = sputc(out, ']');
	return out;
}

static volatile lock_t __PR_LOCK;
static char* vsprintf(char* out, const char* fmt, va_list ap) {
	if (! DEBUG) {
		for (int i = 0; i < get_cpu(); i++) {
			out = sputs(out, "\t\t\t");
		}
	}
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
				if (*(p+1) == 'x') {
					out = sputhex(out, va_arg(ap, int));
					p++;
				} else {
					out = sputdec(out, va_arg(ap, int));
				}
			} else if (c == 'l') {
				if (*(p+1) == 'x') {
					out = sputhex(out, va_arg(ap, long));
					p++;
				} else if (*(p+1) == 'd') {
					out = sputdec(out, va_arg(ap, long));
					p++;
				}
			} else if (c == 'c') {
				out = sputc(out, va_arg(ap, int));
			} else if (c == 's') {
				out = sputs(out, va_arg(ap, char*));
			} else if (c == 'p') {
				out = sputs(out, "0x");
				out = sputhex(out, va_arg(ap, long));
			} else if (c == 'A') {
				char fmt[10];
				sprintf(fmt, "%%%c", *(p+1));
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
	return out;  /* return reference to the NUL, not to after it
				  * to make chaining vsprintf() calls easier */
}

void vprintf(const char* fmt, va_list ap) {
	char s[512];
	vsprintf(s, fmt, ap);
	lock(&__PR_LOCK);
	puts(s);
	unlock(&__PR_LOCK);
}

char* sprintf(char* out, const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	out = vsprintf(out, fmt, ap);
	va_end(ap);
	return out;
}

void printf(const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
}

void trace(const char* fmt, ...) {
	if (TRACE) {
		va_list ap;
		va_start(ap, fmt);
		vprintf(fmt, ap);
		va_end(ap);
	}
}

void _debug(const char* filename, const int line, const char* func, const char* fmt, ...) {
	if (DEBUG) {
		char new_fmt[100];
		int cpu = get_cpu();
		sprintf(new_fmt, "[%s:%d %s (CPU%d)] %s", filename, line, func, cpu, fmt);

		va_list ap;
		va_start(ap, fmt);
		vprintf(new_fmt, ap);
		va_end(ap);
	}
}
