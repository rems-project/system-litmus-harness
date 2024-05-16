#include "lib.h"

#define DFR0 read_sysreg(ID_AA64DFR0_EL1)
#define PFR0 read_sysreg(ID_AA64PFR0_EL1)
#define MMFR0 read_sysreg(ID_AA64MMFR0_EL1);
#define MMFR1 read_sysreg(ID_AA64MMFR1_EL1);

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
}

bool arch_has_feature(enum arm_feature id) {
  return arch_feature_version(id) > 0;
}