#ifndef VMM_VA_MACROS_H
#define VMM_VA_MACROS_H

#include "lib.h"

/**
 * Given a VA return a value usable as a register input to a TLBI that affects that address.
 */
#define PAGE(va) (((uint64_t)(va) >> 12) & BITMASK(48))

/** build a value suitable for writing to TTBR
 * from the pagetable VA and the desired ASID.
 */
#define TTBR0(pgtable, asid) (((uint64_t)(pgtable) & (BITMASK(48) ^ 1)) | ((uint64_t)asid << 48))

#endif /* VMM_VA_MACROS_H */
