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

static const char* dabt_iss_dfsc[0x40] = {
  [0b000000] = "DABT_DFSC_ADDR_L0",
  [0b000001] = "DABT_DFSC_ADDR_L1",
  [0b000010] = "DABT_DFSC_ADDR_L2",
  [0b000011] = "DABT_DFSC_ADDR_L3",
  [0b000100] = "DABT_DFSC_TRANS_L0",
  [0b000101] = "DABT_DFSC_TRANS_L1",
  [0b000110] = "DABT_DFSC_TRANS_L2",
  [0b000111] = "DABT_DFSC_TRANS_L3",
  [0b001001] = "DABT_DFSC_ACCFLAG_L1",
  [0b001010] = "DABT_DFSC_ACCFLAG_L2",
  [0b001011] = "DABT_DFSC_ACCFLAG_L3",
  [0b001101] = "DABT_DFSC_PERM_L1",
  [0b001110] = "DABT_DFSC_PERM_L2",
  [0b001111] = "DABT_DFSC_PERM_L3",
  [0b010000] = "DABT_DFSC_SYNC_EXTERNAL_UNKNOWN",
  [0b010100] = "DABT_DFSC_SYNC_EXTERNAL_L0",
  [0b010101] = "DABT_DFSC_SYNC_EXTERNAL_L1",
  [0b010110] = "DABT_DFSC_SYNC_EXTERNAL_L2",
  [0b010111] = "DABT_DFSC_SYNC_EXTERNAL_L3",
  [0b100001] = "DABT_DFSC_ALIGN",
  [0b110000] = "DABT_DFSC_TLB",
};

void* default_handler(uint64_t vec, uint64_t esr, regvals_t* regs) {
  uint64_t ec = esr >> 26;
  uint64_t iss = esr & BITMASK(26);
  uint64_t cpu = get_cpu();
  lock(&_EXC_PRINT_LOCK);

  printf("Unhandled Exception (CPU%d): \n", cpu);
  printf("  [VBAR: 0x%lx]\n", read_sysreg(VBAR_EL1));
  printf("  [Vector: 0x%lx (%s)]\n", vec, vec_names[vec]);
  printf("  [EC: 0x%lx (%s)]\n", ec, ec_names[ec]);
  printf("  [ESR_EL1: 0x%lx]\n", esr);
  printf("  [FAR_EL1: 0x%lx]\n", read_sysreg(far_el1));
  printf("  [ELR_EL1: 0x%lx]\n", read_sysreg(elr_el1));
  if (ec == 0x24 || ec == 0x25) {
    printf("  [DATA ABORT ISS]\n");
    printf("  [  FnV] %d\n", (iss >> 10) & 1);
    printf("  [  WnR] %d\n", (iss >> 6) & 1);
    uint64_t dfsc = iss & BITMASK(6);
    printf("  [  DFSC] 0x%lx (%s)\n", dfsc, dabt_iss_dfsc[dfsc]);
  }
  printf("  [REGISTERS]\n");
  for (int i = 0; i < 30; i++) {
    printf("  [  x%d] 0x%lx\n", i, regs->gpr[i]);
  }
  printf("  [   SP] 0x%lx\n", regs->sp);
  printf("  \n");
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
      return default_handler(vec, esr, regs);
  else
    return (void*)table_svc[cpu][imm](esr, regs);
}

static void* default_pgfault_handler(uint64_t vec, uint64_t esr,
                                     regvals_t* regs) {
  uint64_t far = read_sysreg(far_el1);
  int cpu = get_cpu();
  uint64_t imm = far % 127;
  if (table_pgfault[cpu][imm] == NULL)
    return default_handler(vec, esr, regs);
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
    return default_handler(vec, esr, regs);
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

/** flush the icache for this threads' vector table entries
 *
 * note that ARMv8 allows icaches to behave like VIPT caches and therefore
 * we cannot just perform the IC invalidation over the R/W VA we performed the write to
 * but rather have to invalidate the actual VA/PA that is used during execution
 */
static void flush_icache_vector_entries(void) {
  uint64_t vbar_start = vector_base_addr_rw + (4096*get_cpu());
  uint64_t vbar_pa_start = vector_base_pa + (4096*get_cpu());

  uint64_t iline = 1 << BIT_SLICE(read_sysreg(ctr_el0), 3, 0);
  uint64_t dline = 1 << BIT_SLICE(read_sysreg(ctr_el0), 19, 16);

  for (uint64_t vbar_va = vbar_start; vbar_va < vbar_start+4096; vbar_va += 4*dline) {
    asm volatile (
      "dc cvau, %[va]\n\t"
    :
    : [va] "r" (vbar_va)
    : "memory"
    );
  }

  dsb();

  for (uint64_t vbar_pa = vbar_pa_start; vbar_pa < vbar_pa_start+4096; vbar_pa += 4*iline) {
    asm volatile (
      "ic ivau, %[pa]\n\t"
    :
    : [pa] "r" (vbar_pa)
    : "memory"
    );
  }

  dsb();
  isb();
}

uint32_t* hotswap_exception(uint64_t vector_slot, uint32_t data[32]) {
  uint32_t* p = alloc(sizeof(uint32_t) * 32);
  uint32_t* vbar = (uint32_t*)(vector_base_addr_rw + (4096*get_cpu()) + vector_slot);
  for (int i = 0; i < 32; i++) {
    p[i] = *(vbar + i);
    *(vbar + i) = data[i];
  }

  flush_icache_vector_entries();

  return p;
}

void restore_hotswapped_exception(uint64_t vector_slot, uint32_t* ptr) {
  uint32_t* vbar = (uint32_t*)(vector_base_addr_rw + (4096*get_cpu() + vector_slot));

  for (int i = 0; i < 32; i++) {
    *(vbar + i) = ptr[i];
  }

  flush_icache_vector_entries();

  free(ptr);
}
