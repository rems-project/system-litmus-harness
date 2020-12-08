#include <lib.h>

void abort(void) {
    psci_system_off();
}