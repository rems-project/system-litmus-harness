#include <lib.h>

#include <stdint.h>

void putc(char c) {
	writeb(c, UART0_BASE);
}

void puts(const char *s)
{
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

void puthex(uint64_t n) {
	puts("0x");
	char _hex[100];
	int i = 0;
	while (n > 0) {
		uint64_t mod = n % 16;
		n -= mod;
		n /= 16;

		_hex[i]  = __get_hexchar(mod);
		i++;
	}

	for (; i >= 0; i--) {
		putc(_hex[i]);
	}
}