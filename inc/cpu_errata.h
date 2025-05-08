#ifndef CPU_ERRATA_H
#define CPU_ERRATA_H

#include "lib.h"

/*
 * We define CPU errata not by their mechanisms or errata numbers,
 * but by the given workarounds.
 *
 * We can then ask e.g. if (cpu_needs_workaround(ERRATA_WORKAROUND_REPEAT_TLBI)) { ... }
 * agnostic to which CPU precisely requires it.
 */

/**
 * enum errata_workaround_label - Names of workarounds for errata.
 */
enum errata_workaround_label {
  /**
   * ERRATA_WORKAROUND_REPEAT_TLBI - Must repeat TLBI instructions once more
   */
  ERRATA_WORKAROUND_REPEAT_TLBI,

  /**
   * ERRATA_WORKAROUND_ISB_AFTER_PTE_ST - Must insert ISB after the DSB synchronising a store with the TTW
   *
   * It seems improbable this affects us, but seems easy enough to avoid.
   */
  ERRATA_WORKAROUND_ISB_AFTER_PTE_ST,

  /* end of list */
  NR_ERRATA,
};

/**
 * requires_errata_workaround_map - internal map
 * look-up table from workaround name to whether the workaround is required for this CPU.
 */
extern bool requires_errata_workaround_map[];

/**
 * cpu_needs_workaround() - Returns whether a workaround is needed
 */
static inline bool cpu_needs_workaround(enum errata_workaround_label name) {
  return requires_errata_workaround_map[name];
}

/**
 * initialise_errata_workarounds() - Populate workaround map
 */
void initialise_errata_workarounds(void);

#endif /* CPU_ERRATA_H */