#ifndef VMM_VA_MACROS_H
#define VMM_VA_MACROS_H

#include "lib.h"

#define TTBR0_BADDR_MASK (BITMASK(48) & ~BITMASK(5))
#define TTBR0_ASID_LO 48

/** build a value suitable for writing to TTBR
 * from the pagetable VA and the desired ASID.
 */
#define MK_TTBR(pgtable, asid) (((u64)(pgtable)&TTBR0_BADDR_MASK) | ((u64)(asid) << TTBR0_ASID_LO))

#endif /* VMM_VA_MACROS_H */
