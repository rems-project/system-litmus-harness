#include <lib.h>

#include <stdint.h>

void dsb(void) {
    asm volatile ("dsb sy\n\t");
}

void writeb(uint8_t byte, uint64_t addr) {
    asm volatile (
        "strb %w[byte], [%[addr]]\n\t"
    :
    : [byte] "r" (byte), [addr] "r" (addr)
    );
}