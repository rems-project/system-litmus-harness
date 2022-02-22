
#include "lib.h"

void writeb(u8 byte, u64 addr) {
    asm volatile (
        "strb %w[byte], [%[addr]]\n\t"
    :
    : [byte] "r" (byte), [addr] "r" (addr)
    : "memory"
    );
}

void writew(u32 word, u64 addr) {
    asm volatile (
        "str %w[word], [%[addr]]\n\t"
    :
    : [word] "r" (word), [addr] "r" (addr)
    : "memory"
    );
}

u8 readb(u64 addr) {
  u8 out;
  asm volatile (
    "ldrb %w[out], [%[addr]]\n"
  : [out] "=&r" (out)
  : [addr] "r" (addr)
  : "memory"
  );
  return out;
}

u32 readw(u64 addr) {
  u32 out;
  asm volatile (
    "ldr %w[out], [%[addr]]\n"
  : [out] "=&r" (out)
  : [addr] "r" (addr)
  : "memory"
  );
  return out;
}

u64 get_cpu(void) {
    return read_sysreg(tpidr_el0) & 0xff;
}

u64 get_vcpu(void) {
    return current_thread_info()->vcpu_no;
}

void set_vcpu(int vcpu) {
    current_thread_info()->vcpu_no = vcpu;
}