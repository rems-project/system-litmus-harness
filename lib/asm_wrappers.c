#include <lib.h>

#include <stdint.h>

void dsb(void) {
    asm volatile ("dsb sy\n\t");
}

void wfe(void) {
    asm volatile ("wfe\n\t");
}

void sev(void) {
    asm volatile ("sev\n\t");
}

void dmb(void) {
    asm volatile ("dmb sy\n\t");
}

void isb(void) {
    asm volatile("isb\n\t");
}

void writeb(uint8_t byte, uint64_t addr) {
    asm volatile (
        "strb %w[byte], [%[addr]]\n\t"
    :
    : [byte] "r" (byte), [addr] "r" (addr)
    );
}

uint64_t get_cpu(void) {
    return read_sysreg(tpidr_el0) & 0xff;
}