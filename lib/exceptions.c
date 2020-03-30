#include <stdint.h>

#include "lib.h"

static lock_t _EXC_PRINT_LOCK;

static const char* vec_names[16] = {
  "VEC_EL1T_SYNC",
  "VEC_EL1T_IRQ",
  "VEC_EL1T_FIQ",
  "VEC_EL1T_ERROR",
  "VEC_EL1H_SYNC",
  "VEC_EL1H_IRQ",
  "VEC_EL1H_FIQ",
  "VEC_EL1H_ERROR",
  "VEC_EL0_SYNC_64",
  "VEC_EL0_IRQ_64",
  "VEC_EL0_FIQ_64",
  "VEC_EL0_ERROR_64",
  "VEC_EL0_SYNC_32",
  "VEC_EL0_IRQ_32",
  "VEC_EL0_FIQ_32",
  "VEC_EL0_ERROR_32",
};


static const char* ec_names[0x26] = {
  [0x15] = "EC_SVC64",
  [0x18] = "EC_MRS_TRAP",
  [0x20] = "EC_IABT_EL0",
  [0x21] = "EC_IABT_EL1",
  [0x22] = "EC_PC_ALIGN",
  [0x24] = "EC_DABT_EL0",
  [0x25] = "EC_DABT_EL1",
};


void* default_handler(uint64_t vec, uint64_t esr) {
  uint64_t ec = esr >> 26;
  uint64_t cpu = get_cpu();
  if (current_thread_info()->locking_enabled)
    lock(&_EXC_PRINT_LOCK);

  printf("Unhandled Exception (CPU%d): \n", cpu);
  printf("  [VBAR: 0x%lx]\n", read_sysreg(VBAR_EL1));
  printf("  [Vector: 0x%lx (%s)]\n", vec, vec_names[vec]);
  printf("  [EC: 0x%lx (%s)]\n", ec, ec_names[ec]);
  printf("  [ESR_EL1: 0x%lx]\n", esr);
  printf("  [FAR_EL1: 0x%lx]\n", read_sysreg(far_el1));
  printf("  [ELR_EL1: 0x%lx]\n", read_sysreg(elr_el1));
  printf("  \n");

  if (current_thread_info()->locking_enabled)
    unlock(&_EXC_PRINT_LOCK);
  abort();

  /* unreachable */
  return NULL;
}

void set_handler(uint64_t vec, uint64_t ec, exception_vector_fn* fn) {
  int cpu = get_cpu();
  table[cpu][vec][ec] = fn;
}

void reset_handler(uint64_t vec, uint64_t ec) {
  int cpu = get_cpu();
  table[cpu][vec][ec] = NULL;
}

void drop_to_el0(void) {
  asm volatile("svc #10\n\t"
               :
               :
               : "x0", "x1", "x2", "x3", "x4", "x5", "x6",
                 "x7" /* dont touch parameter registers */
  );
}

void raise_to_el1(void) {
  asm volatile("svc #11\n\t"
               :
               :
               : "x0", "x1", "x2", "x3", "x4", "x5", "x6",
                 "x7" /* dont touch parameter registers */
  );
}

static void* default_svc_drop_el0(uint64_t vec, uint64_t esr, regvals_t* regs) {
  asm volatile(
    "mrs x18, spsr_el1\n"

    /* zero SPSR_EL1[0:3] */
    "lsr x18, x18, #4\n"
    "lsl x18, x18, #4\n"

    /* write back to SPSR */
    "msr spsr_el1, x18\n"
    "dsb nsh\n"
    "isb\n"
    :
    :
    : "memory", "x18");

  return NULL;
}

static void* default_svc_raise_el1(uint64_t vec, uint64_t esr,
                                   regvals_t* regs) {
  /* raise to EL1h */
  asm volatile(
      "mrs x18, spsr_el1\n"

      /* zero SPSR_EL1[0:3] */
      "lsr x18, x18, #4\n"
      "lsl x18, x18, #4\n"

      /* add targetEL and writeback */
      "add x18, x18, #4\n" /* EL1 */
      "add x18, x18, #0\n" /* h */
      "msr spsr_el1, x18\n"
      "dsb nsh\n"
      "isb\n"
      :
      :
      : "memory", "x18");
  return NULL;
}

static void* default_svc_read_currentel(uint64_t vec, uint64_t esr,
                                        regvals_t* regs) {
  /* read CurrentEL via SPSPR */
  uint64_t cel;
  asm volatile(
      "mrs %[cel], SPSR_EL1\n"
      "and %[cel], %[cel], #12\n"
      : [cel] "=r"(cel));
  return (void*)cel;
}

static void* default_svc_handler(uint64_t vec, uint64_t esr, regvals_t* regs) {
  uint64_t imm = esr & 0xffffff;
  int cpu = get_cpu();
  if (table_svc[cpu][imm] == NULL)
    if (imm == 10)
      return default_svc_drop_el0(vec, esr, regs);
    else if (imm == 11)
      return default_svc_raise_el1(vec, esr, regs);
    else if (imm == 12)
      return default_svc_read_currentel(vec, esr, regs);
    else
      return default_handler(vec, esr);
  else
    return (void*)table_svc[cpu][imm](esr, regs);
}

static void* default_pgfault_handler(uint64_t vec, uint64_t esr,
                                     regvals_t* regs) {
  uint64_t far = read_sysreg(far_el1);
  int cpu = get_cpu();
  uint64_t imm = far % 127;
  if (table_pgfault[cpu][imm] == NULL)
    return default_handler(vec, esr);
  else
    return (void*)table_pgfault[cpu][imm](esr, regs);
}

void* handle_exception(uint64_t vec, uint64_t esr, regvals_t* regs) {
  uint64_t ec = esr >> 26;
  int cpu = get_cpu();
  exception_vector_fn* fn = table[cpu][vec][ec];
  if (fn) {
    return fn(esr, regs);
  } else if (ec == 0x15) {
    return default_svc_handler(vec, esr, regs);
  } else if ((ec | 1) == 0x25) {
    return default_pgfault_handler(vec, esr, regs);
  } else {
    return default_handler(vec, esr);
  }
}

void set_svc_handler(uint64_t svc_no, exception_vector_fn* fn) {
  int cpu = get_cpu();
  table_svc[cpu][svc_no] = fn;
}

void reset_svc_handler(uint64_t svc_no) {
  int cpu = get_cpu();
  table_svc[cpu][svc_no] = NULL;
}

void set_pgfault_handler(uint64_t va, exception_vector_fn* fn) {
  int cpu = get_cpu();
  int idx = va % 127;
  if (table_pgfault[cpu][idx] != NULL) {
    puts("! err: cannot set pagefault handler for same page twice.\n");
    abort();
  }

  table_pgfault[cpu][idx] = fn;
  dsb();
}

void reset_pgfault_handler(uint64_t va) {
  int cpu = get_cpu();
  table_pgfault[cpu][va % 127] = NULL;
}

uint32_t* hotswap_exception(uint64_t vector_slot, uint32_t data[32]) {
  uint32_t* p = alloc(sizeof(uint32_t) * 32);
  uint32_t* vbar = (uint32_t*)(vector_base_addr_rw + (4096*get_cpu()) + vector_slot);
  for (int i = 0; i < 32; i++) {
    p[i] = *(vbar + i);
    *(vbar + i) = data[i];
  }

  uint64_t vbar_start = vector_base_addr_rw + (4096*get_cpu());
  uint64_t iline = 1 << BIT_SLICE(read_sysreg(ctr_el0), 3, 0);
  uint64_t dline = 1 << BIT_SLICE(read_sysreg(ctr_el0), 19, 16);
  uint64_t line = MIN(iline, dline);
  for (uint64_t vbar_va = vbar_start; vbar_va < vbar_start+4096; vbar_va += line) {
    asm volatile(
        "dc cvau, %[vbar]\n\t"
        "dsb ish\n\t"
        "ic ivau, %[vbar]\n\t"
        "dsb ish\n\t"
        "isb\n\t"  /* not required if SCTLR_EL1.EIS is set (or if in v8.4 or earlier) */
        /** TODO: BS: ^ is the above True?  Does EIS require ISB-like sync before vector itself is fetched ?*/
        :
        : [vbar] "r"(vbar_va)
        : "memory");
  }

  return p;
}

void restore_hotswapped_exception(uint64_t vector_slot, uint32_t* ptr) {
  uint32_t* vbar = (uint32_t*)(vector_base_addr_rw + (4096*read_sysreg(tpidr_el0)) + vector_slot);

  for (int i = 0; i < 32; i++) {
    *(vbar + i) = ptr[i];
  }

  uint64_t vbar_start = vector_base_addr_rw + (4096*read_sysreg(tpidr_el0));
  uint64_t iline = 1 << BIT_SLICE(read_sysreg(ctr_el0), 3, 0);
  uint64_t dline = 1 << BIT_SLICE(read_sysreg(ctr_el0), 19, 16);
  uint64_t line = MIN(iline, dline);
  for (uint64_t vbar_va = vbar_start; vbar_va < vbar_start+4096; vbar_va += line) {
    asm volatile(
        "dc cvau, %[vbar]\n\t"
        "dsb ish\n\t"
        "ic ivau, %[vbar]\n\t"
        "dsb ish\n\t"
        "isb\n\t"  /* not required if SCTLR_EL1.EIS is set (or if in v8.4 or earlier) */
        /** TODO: BS: ^ is the above True?  Does EIS require ISB-like sync before vector itself is fetched ?*/
        :
        : [vbar] "r"(vbar_va)
        : "memory");
  }

  free(ptr);
}
