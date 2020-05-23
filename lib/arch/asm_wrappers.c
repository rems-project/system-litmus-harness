#include <stdint.h>

#include "lib.h"

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

uint64_t get_vcpu(void) {
    return current_thread_info()->vcpu_no;
}

void set_vcpu(int vcpu) {
    current_thread_info()->vcpu_no = vcpu;
}