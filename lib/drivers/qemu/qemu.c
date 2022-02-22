#include "lib.h"

#include "drivers/qemu/qemu.h"

void init_driver(void) {
  /* nop */
}

void write_stdout(u8 c) {
    writeb(c, UART0_BASE);
}