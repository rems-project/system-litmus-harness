#include <lib.h>

void abort(void) {
    psci_system_off();
}

void fail(const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vprintf(1, fmt, ap);
	va_end(ap);
    raise_to_el1();
    abort();
}