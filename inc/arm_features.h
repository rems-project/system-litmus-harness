#ifndef ARM_FEAT_H
#define ARM_FEAT_H

#include "types.h"

/* a collection of Arm features we check for, or use,
 * either in the harness itself or in tests.
 */

enum arm_feature {
  /* Scalable-Vector-Extensions */
  FEAT_SVE,

  /* Statistical profiling */
  FEAT_SPE,

  /* Reliability-Assurance-Servicability
   * aka errors and abort recovery */
  FEAT_RAS,

  /* Performance monitoring */
  FEAT_PMUv3,

  /* Tracing */
  FEAT_TRBE,
};

/** struct arch_feature_matrix - Arch-specific implementation state for each feature.
 *
 * The Arm architecture defines a 4-bit bitvector for each feature,
 * typically containing the implemented version.
 */
struct arch_feature_matrix {
  u8 features[1+FEAT_TRBE];
};

/* Arm ID registers */
#define DFR0 read_sysreg(ID_AA64DFR0_EL1)
#define PFR0 read_sysreg(ID_AA64PFR0_EL1)
#define MMFR0 read_sysreg(ID_AA64MMFR0_EL1)
#define MMFR1 read_sysreg(ID_AA64MMFR1_EL1)
#define MIDR read_sysreg(MIDR_EL1)

/* instruction set attribute register(s) */
#define ISAR0_FIELD_BF16 BITMASK(47, 44)
#define ISAR0_FIELD_BF16_LSB 44

/* memory model feature register(s) */
#define MMFR0_FIELD_ASIDBits 7, 4
#define MMFR0_FIELD_ExS 47, 44

#define MMFR1_FIELD_VMIDBits 7, 4

#define MMFR2_FIELD_TTL 51, 48

/* debug feature register */
#define DFR0_FIELD_TraceVer 7, 4
#define DFR0_FIELD_PMUVer 11, 8
#define DFR0_FIELD_PMSVer 35, 32

/* processor feature register */
#define PFR0_FIELD_FP 19, 16
#define PFR0_FIELD_GIC 27, 24
#define PFR0_FIELD_RAS 31, 28
#define PFR0_FIELD_SVE 35, 32

/* main id register */
#define MIDR_FIELD_Implementor 31, 24
#define MIDR_FIELD_Variant 23, 20
#define MIDR_FIELD_Architecture 19, 16
#define MIDR_FIELD_PartNum 15, 4
#define MIDR_FIELD_Revision 3, 0

void arch_read_feature_matrix(struct arch_feature_matrix *m_out);
u8 arch_feature_version(enum arm_feature id);
bool arch_has_feature(enum arm_feature id);

struct arm_implementation {
  char implementor;
  u64 part;
  u64 variant;
  u64 revision;
};

void arch_read_implementation(struct arm_implementation* impl_out);
bool arch_implementation_eq(struct arm_implementation* lhs, struct arm_implementation* rhs);

#endif /* ARM_FEAT_H */
