#ifndef VMM_PROT_H
#define VMM_PROT_H

#define PROT_NG_NOT_GLOBAL 1
#define PROT_AF_ACCESS_FLAG_DISABLE  1

#define PROT_SH_NSH 0
#define PROT_SH_OSH 2
#define PROT_SH_ISH 3

#define PROT_NS_NON_SECURE 0
/** these define access permissions of the AP[2:1] bits
 * in the form PROT_el1_el0
 * where el1 are the permissions for EL1
 *   and el0 are the permissions for EL0
 *
 * e.g. PROT_AP_RW_RWX means:
 *    EL1 => Read/Write
 *    EL0 => Read/Write/eXecute
 */
#define PROT_AP_RWX_X 0
#define PROT_AP_RW_RWX 1
#define PROT_AP_RX_X 2
#define PROT_AP_RX_RX 3

/**
 * AttrIndex[2:0] defines a section of MAIR_EL1 to read the memory attributes from
 * system-litmus-harness sets it up so that MAIR_EL1 has some default configurations
 */
#define PROT_ATTR_DEVICE_nGnRnE 0
#define PROT_ATTR_DEVICE_GRE    1
#define PROT_ATTR_NORMAL_NC     2
#define PROT_ATTR_NORMAL_RA_WA  3
#define PROT_ATTR_UNUSED_1      4
#define PROT_ATTR_UNUSED_2      5
#define PROT_ATTR_UNUSED_3      6
/* AttrIdx == #7 reserved for test usage */
#define PROT_ATTR_TEST_ATTR     7

#define MAIR_DEVICE_nGnRnE 0x00
#define MAIR_DEVICE_GRE    0x0c
#define MAIR_NORMAL_NC     0x44
#define MAIR_NORMAL_RA_WA  0xff

/** Shorthand attributes
 */
#define PROT_RX_X (write_attrs((attrs_t){.AP=PROT_AP_RX_X}))
#define PROT_RX_RX (write_attrs((attrs_t){.AP=PROT_AP_RX_RX}))
#define PROT_RW_RWX (write_attrs((attrs_t){.AP=PROT_AP_RW_RWX}))
#define PROT_RW_RW (write_attrs((attrs_t){.XN=1, .AP=PROT_AP_RW_RWX}))
#define PROT_RW_U (write_attrs((attrs_t){.XN=1, .PXN=1, .AP=PROT_AP_RWX_X}))
#define PROT_RWX_X (write_attrs((attrs_t){.AP=PROT_AP_RWX_X}))
#define PROT_MEMTYPE_DEVICE (write_attrs((attrs_t){.attr=PROT_ATTR_DEVICE_nGnRnE}))
#define PROT_MEMTYPE_NORMAL (write_attrs((attrs_t){.attr=PROT_ATTR_NORMAL_RA_WA}))
#define PROT_MEMTYPE_NONCACHING (write_attrs((attrs_t){.attr=PROT_ATTR_NORMAL_NC}))
#define PROT_MEMTYPE_TESTATTRS (write_attrs((attrs_t){.attr=PROT_ATTR_TEST_ATTR}))
#define PROT_PGTABLE ( PROT_MEMTYPE_NORMAL | PROT_RW_RWX )

/* masks for combining */
#define PROT_WITH(attrs, prot, choice) ((attrs & BITMASK_PROT(prot)) | (choice << BITSTART_PROT_ ## prot))

/* AttrIdx[0:2] == Desc[4:2] */
#define BITSTART_PROT_ATTR 2
#define BITWIDTH_PROT_ATTR 3

/* AP[2:1] == Desc[9:8] */
#define BITSTART_PROT_AP 6
#define BITWIDTH_PROT_AP 2

#define BITMASK_PROT(prot) ~(BITMASK(BITWIDTH_PROT_ ## prot) << BITSTART_PROT_ ## prot)

/* default for all heap attrs */
#define PROT_DEFAULT_HEAP (PROT_MEMTYPE_NORMAL | PROT_RW_RWX)

#endif /* VMM_PROT_H */