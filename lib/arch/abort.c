#include <lib.h>

void abort(void) {
    psci_system_off();
}

void error(const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
    raise_to_el1();
    abort();
}