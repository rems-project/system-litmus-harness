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

void arch_read_feature_matrix(struct arch_feature_matrix *m_out);
u8 arch_feature_version(enum arm_feature id);
bool arch_has_feature(enum arm_feature id);

#endif /* ARM_FEAT_H */
