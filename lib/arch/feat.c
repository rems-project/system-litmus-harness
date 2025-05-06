#include "lib.h"

u8 arch_feature_version(enum arm_feature id) {
  switch (id) {
  case FEAT_SVE:
    return BIT_SLICE(PFR0, PFR0_FIELD_SVE);
  case FEAT_SPE:
    return BIT_SLICE(DFR0, DFR0_FIELD_PMSVer);
  case FEAT_RAS:
    return BIT_SLICE(PFR0, PFR0_FIELD_RAS);
  case FEAT_PMUv3:
    return BIT_SLICE(DFR0, DFR0_FIELD_PMUVer);
  case FEAT_TRBE:
    return BIT_SLICE(DFR0, DFR0_FIELD_TraceVer);
  case FEAT_LSE:
    return BIT_SLICE(ISAR0, ISAR0_FIELD_ATOMIC);
  default:
    unreachable();
  }
}

void arch_read_feature_matrix(struct arch_feature_matrix* m_out) {
  m_out->features[FEAT_SVE] = arch_feature_version(FEAT_SVE);
  m_out->features[FEAT_SPE] = arch_feature_version(FEAT_SPE);
  m_out->features[FEAT_RAS] = arch_feature_version(FEAT_RAS);
  m_out->features[FEAT_PMUv3] = arch_feature_version(FEAT_PMUv3);
  m_out->features[FEAT_TRBE] = arch_feature_version(FEAT_TRBE);
  m_out->features[FEAT_LSE] = arch_feature_version(FEAT_LSE);
}

bool arch_has_feature(enum arm_feature id) {
  return arch_feature_version(id) > 0;
}

void arch_read_implementation(struct arm_implementation* impl_out) {
  impl_out->implementor = BIT_SLICE(MIDR, MIDR_FIELD_Implementor);
  impl_out->part = BIT_SLICE(MIDR, MIDR_FIELD_PartNum);
  impl_out->revision = BIT_SLICE(MIDR, MIDR_FIELD_Revision);
  impl_out->variant = BIT_SLICE(MIDR, MIDR_FIELD_Variant);
}

bool arch_implementation_eq(struct arm_implementation* lhs, struct arm_implementation* rhs) {
  return (
    lhs->implementor == rhs->implementor && lhs->part == rhs->part && lhs->revision == rhs->revision &&
    lhs->variant == rhs->variant
  );
}