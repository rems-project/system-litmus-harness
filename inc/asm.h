#ifndef ASM_H
#define ASM_H
#define SCTLR_NOTRAP_CACHE_MAINTENANCE (1 << 26)
#define SCTLR_LITTLE_ENDIAN (0 << 26)  /* bits [25:24] are 0 for little endian */
#define SCTLR_PAN (1 << 23)  /* Privileged Access Never */
#define SCTLR_EXCEPTION_ISB (1 << 22)  /* Exceptions are context synchronizing */
#define SCTLR_NOIESB (0 << 21)  /* No Explicit Error Synchronization */
#define SCTLR_NOWXR (0 << 19)  /* No Write-Execute-Never */
#define SCTLR_NOTRAP_WFE (1 << 18) /* No Trap WFE */
#define SCTLR_NOTRAP_WFI (1 << 16) /* No Trap WFI */
#define SCTLR_NOTRAP_CTR (1 << 15) /* No Trap access CTR_EL0 */
#define SCTLR_I (1 << 12) /* Instructon accesses are not non-cacheable */
#define SCTLR_ERET_ISB (1 << 11) /* Exception exit are context synchronizing */
#define SCTLR_SP_ALIGN (3 << 3)  /* SP Alignment Check */
#define SCTLR_C (1 << 2)  /* Data Accesess not non-cacheable */
#define SCTLR_NOALIGN_CHECK (0 << 1)  /* No alignment check */
#define SCTLR_MMU_ON (1)
#define SCTLR_MMU_OFF (0)

#define SCTLR_HI ((SCTLR_NOTRAP_CACHE_MAINTENANCE | SCTLR_LITTLE_ENDIAN | SCTLR_PAN | SCTLR_EXCEPTION_ISB | SCTLR_NOIESB | SCTLR_NOWXR | SCTLR_NOTRAP_WFE | SCTLR_NOTRAP_WFI | SCTLR_NOTRAP_CTR | SCTLR_I) >> 16)
#define SCTLR_LO (SCTLR_ERET_ISB | SCTLR_SP_ALIGN | SCTLR_C | SCTLR_NOALIGN_CHECK | SCTLR_MMU_OFF)

#define SCTLR ((SCTLR_HI << 16) | SCTLR_LO)P

#ifndef __ASSEMBLY__
#include <stdint.h>


/* barriers and wrappers */
#define wfe() do { asm volatile ("wfe"); } while (0)
#define sev() do { asm volatile ("sev"); } while (0)
#define dsb() do { asm volatile ("dsb sy"); } while (0)
#define dmb() do { asm volatile ("dmb sy"); } while (0)
#define isb() do { asm volatile ("isb"); } while (0)

/* stores and loads */
void writeb(uint8_t byte, uint64_t addr);

/* register read/write */
#define read_sysreg(r) ({  \
    uint64_t reg;  \
    asm volatile("mrs %[reg], " #r : [reg] "=r" (reg));  \
    reg;  \
})

#define write_sysreg(v, r) do {  \
    asm volatile("msr " #r ", %[reg]" : : [reg] "rZ" (v));  \
} while (0)

#endif
#endif /* ASM_H */
