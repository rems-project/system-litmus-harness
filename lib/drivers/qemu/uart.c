#include "lib.h"

#include "drivers/qemu/qemu.h"

void write_stdout(uint8_t c) {
    writeb(c, UART0_BASE);
}