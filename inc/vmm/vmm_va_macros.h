#ifndef VMM_VA_MACROS_H
#define VMM_VA_MACROS_H

#include "lib.h"

/** build a value suitable for writing to TTBR
 * from the pagetable VA and the desired ASID.
 */
#define TTBR0(pgtable, asid) (((u64)(pgtable) & (BITMASK(48) ^ 1)) | ((u64)(asid) << 48))

#endif /* VMM_VA_MACROS_H */
