#include <stdint.h>

#include "lib.h"

void writeb(uint8_t byte, uint64_t addr) {
    asm volatile (
        "strb %w[byte], [%[addr]]\n\t"
    :
    : [byte] "r" (byte), [addr] "r" (addr)
    : "memory"
    );
}

void writew(uint32_t word, uint64_t addr) {
    asm volatile (
        "str %w[word], [%[addr]]\n\t"
    :
    : [word] "r" (word), [addr] "r" (addr)
    : "memory"
    );
}

uint8_t readb(uint64_t addr) {
  uint8_t out;
  asm volatile (
    "ldrb %w[out], [%[addr]]\n"
  : [out] "=&r" (out)
  : [addr] "r" (addr)
  : "memory"
  );
  return out;
}

uint32_t readw(uint64_t addr) {
  uint32_t out;
  asm volatile (
    "ldr %w[out], [%[addr]]\n"
  : [out] "=&r" (out)
  : [addr] "r" (addr)
  : "memory"
  );
  return out;
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