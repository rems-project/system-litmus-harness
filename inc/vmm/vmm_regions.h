#ifndef VMM_REGIONS_H
#define VMM_REGIONS_H

#include "litmus_regions.h"
#include "device.h"

/* at 16G starts the STACK va space */
#define STACK_MMAP_BASE (16 * GiB)
#define STACK_MMAP_SIZE (4 * STACK_SIZE)

/* thread0.EL0 is (16G+  STACK_SIZE -> 16G           )
 * thread0.EL1 is (16G+2*STACK_SIZE -> 16G+STACK_SIZE)
 * etc
 */
#define STACK_MMAP_THREAD_TOP_EL0(cpu) (STACK_MMAP_BASE + (1+(cpu))*2*STACK_SIZE - STACK_SIZE)
#define STACK_MMAP_THREAD_TOP_EL1(cpu) (STACK_MMAP_BASE + (1+(cpu))*2*STACK_SIZE)

#define STACK_MMAP_THREAD_BOT_EL0(cpu) (STACK_MMAP_THREAD_TOP_EL0(cpu) - STACK_SIZE)
#define STACK_MMAP_THREAD_BOT_EL1(cpu) (STACK_MMAP_THREAD_TOP_EL1(cpu) - STACK_SIZE)

#define IN_STACK_MMAP_SPACE(va) ((STACK_MMAP_BASE <= (va)) && ((va) <= STACK_MMAP_BASE+1*GiB))
#define STACK_MMAP_VA_TO_PA(va) ((uint64_t)(va) - STACK_MMAP_BASE + BOT_OF_STACK_PA)

/** convert a stack PA/VA to the safe PA addr
 */
#define STACK_SAFE_PA(va) \
  (! IN_STACK_MMAP_SPACE(va) \
      ? va \
      : STACK_MMAP_VA_TO_PA(va))


/* at 17G starts the VECTOR TABLE va space
 * which is an EL0 R/W mapping to the vtable
 *
 * VTABLE_MMAP_BASE + get_cpu()*PAGE_SIZE ==
 *  R/W Mapping to TTBR for this thread
 */
#define VTABLE_MMAP_BASE (17 * GiB)
#define VTABLE_MMAP_SIZE (16 * KiB)

#define THR_VTABLE_VA(t) ((uint64_t*)(VTABLE_MMAP_BASE + PAGE_SIZE*t))
#define THR_VTABLE_PA(t) ((uint64_t*)(vector_base_pa + PAGE_SIZE*t))

/* size of TESTDATA VA space */
#define TESTDATA_MMAP_SIZE (64 * GiB)
#define TESTDATA_MMAP_SIZE_SHIFT (6 + GiB_SHIFT)

#define HARNESS_MMAP_BASE ((uint64_t)HARNESS_MMAP)
#define TESTDATA_MMAP_BASE ((uint64_t)TESTDATA_MMAP)

/** converts any physical (or identically mapped virtual) address
 * in the 1G -> TOP_OF_MEM region into a R/W mapping in the 64G+ memory map
 */
#define HARNESS_MMAP_PHYS_TO_VIRT(p) \
    (HARNESS_MMAP_BASE + __HARNESS_MMAP_OFFSET(p))

#define __HARNESS_MMAP_OFFSET(p) (p - 1*GiB)

/*
 * converts any physical (or identically mapped virtual) address
 * in the BOT_OF_TESTDATA -> TOP_OF_TESTDATA physical region
 * into a virtual mapping into the 64G region starting at 128G
 *
 * since we are allocating 16 * 8M contiguous regions uniformally over
 * the physical space the calculation is simple:
 *
 * GapBetween8Ms = 128G / NumOfRegions
 * 8MStart(i) = 128G + i*GapBetween8Ms
 * 8MPAOffset(p) = (p - BOT_OF_TESTDATA)
 * 8MIndex(p) = (p % 8M)
 * PAtoVA(p) = 8MStart(8MPAOffset(p) / 8M) + 8MIndex(p)
 *
 * 8MVAIndex(v) = (v - 128G) // GapBetween8Ms
 * 8MVAStart(i) = BOT_OF_TESTDATA + i*8M
 * VAtoPA(v) = 8MVAStart(8MVAIndex(v)) + 8MIndex(v)
 */

#define __MMAP_8M_SHIFT REGION_SHIFT
#define __MMAP_8M_SIZE (1UL << __MMAP_8M_SHIFT)

#define __TESTDATA_MMAP_GAP_SHIFT (TESTDATA_MMAP_SIZE_SHIFT - NR_REGIONS_SHIFT)
#define __TESTDATA_MMAP_GAP (1UL << __TESTDATA_MMAP_GAP_SHIFT)

#define __TESTDATA_MMAP_OFFSET_FROM_BASE(p) ((uint64_t)(p) - BOT_OF_TESTDATA)
#define __TESTDATA_MMAP_8M_RESIDUE(p) ((p) & BITMASK(__MMAP_8M_SHIFT))
#define __TESTDATA_MMAP_8M_INDEX(p) (__TESTDATA_MMAP_OFFSET_FROM_BASE(p) >> __MMAP_8M_SHIFT)
#define __TESTDATA_MMAP_OFFSET_VA_FROM_BASE(v) ((uint64_t)(v) - TESTDATA_MMAP_BASE)
#define __TESTDATA_MMAP_8M_VA_INDEX(v) (__TESTDATA_MMAP_OFFSET_VA_FROM_BASE(v) >> __TESTDATA_MMAP_GAP_SHIFT)

#define TESTDATA_MMAP_8M_VA_FROM_INDEX(idx) (TESTDATA_MMAP_BASE + ((idx) * __TESTDATA_MMAP_GAP))
#define TESTDATA_MMAP_8M_PA_FROM_INDEX(idx) (BOT_OF_TESTDATA + ((idx) * __MMAP_8M_SIZE))

#define TESTDATA_MMAP_PHYS_TO_VIRT(p) \
    ( TESTDATA_MMAP_8M_VA_FROM_INDEX(__TESTDATA_MMAP_8M_INDEX(p)) \
    + __TESTDATA_MMAP_8M_RESIDUE(__TESTDATA_MMAP_OFFSET_FROM_BASE(p)))

#define TESTDATA_MMAP_VIRT_TO_PHYS(v) \
    ( TESTDATA_MMAP_8M_PA_FROM_INDEX(__TESTDATA_MMAP_8M_VA_INDEX(v)) \
    + __TESTDATA_MMAP_8M_RESIDUE(__TESTDATA_MMAP_OFFSET_VA_FROM_BASE(v)))


/** returns true if the given va is in the TESTDATA mapped region starting at 64GiB
 */
#define IN_TESTDATA_RANGE(va) ((TESTDATA_MMAP_BASE <= (va)) && ((va) < (TESTDATA_MMAP_BASE+TESTDATA_MMAP_SIZE)))

/* convert a test VA to a safe VA in the HARNESS MMAP region
 * the VA is either in the TESTDATA range, or in the normal heap range
 * if in the heap range then assume identically mapped and move the VA up into the safe range
 * if in the testdata range, work out the PA then do the same moving up into the safe range
 */
#define SAFE_TESTDATA_VA(va) \
  ( (! IN_TESTDATA_RANGE(va)) ? HARNESS_MMAP_PHYS_TO_VIRT(va) \
                              : HARNESS_MMAP_PHYS_TO_VIRT(TESTDATA_MMAP_VIRT_TO_PHYS(va)) )

#define SAFE_TESTDATA_PA(va) \
  ( (! IN_TESTDATA_RANGE(va)) ? va \
                              : TESTDATA_MMAP_VIRT_TO_PHYS(va) )

#endif /* VMM_REGIONS_H */