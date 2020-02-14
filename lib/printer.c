#include <stdint.h>
#include <stdarg.h>

#include "lib.h"

void putc(char c) {
	writeb(c, UART0_BASE);
}

void puts(const char *s) {
	while (*s)
		putc(*s++);
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

void putdec(uint64_t n) {
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
		putc(digits[i]);
	}
}

void puthex(uint64_t n) {
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
		putc(_hex[i]);
	}
}

static volatile lock_t __PR_LOCK;
static void vprintf(const char* fmt, va_list ap) {
	lock(&__PR_LOCK);
        for (int i = 0; i < get_cpu(); i++)
          puts("\t\t\t");
	char* p = (char*)fmt;
	while (*p) {
		char c = *p;
		if (c != '%') {
			putc(c);
		} else {
			c = *++p;
			if (c == 'd') {
				if (*(p+1) == 'x') {
					puthex(va_arg(ap, int));
					p++;
				} else {
					putdec(va_arg(ap, int));
				}
			} else if (c == 'l') {
				if (*(p+1) == 'x') {
					puthex(va_arg(ap, long));
					p++;
				} else {
					putdec(va_arg(ap, long));
				}
			} else if (c == 's') {
				puts(va_arg(ap, const char*));
			} else if (c == 'p') {
				puts("0x");
				puthex(va_arg(ap, long));
			} else {
				puts("!! printf: unknown symbol: ");
				putc(c);
				puts("\n");
				abort();
			}
		}
		
		p++;
	}
	unlock(&__PR_LOCK);
}

void printf(const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
}

void trace(const char* fmt, ...) {
#ifdef TRACE
	va_list ap;
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
#endif
}
