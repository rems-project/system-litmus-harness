#include "lib.h"

bool requires_errata_workaround_map[NR_ERRATA];

struct arm_implementation_range
{
  char implementor;
  u64 part;
  u64 min_variant;
  u64 min_revision;
  u64 max_variant;
  u64 max_revision;
};

bool impl_in_range(struct arm_implementation* impl, struct arm_implementation_range range) {
  if (impl->implementor != range.implementor)
    return false;

  if (impl->part != range.part)
    return false;

  return (
    (range.min_revision <= impl->revision && impl->revision <= range.max_revision) &&
    (range.min_variant <= impl->variant && impl->variant <= range.max_variant)
  );
}

typedef bool (*matcher_fn)(struct arm_implementation*);

struct errata_workaround
{
  char* description;
  enum errata_workaround_label label;
  matcher_fn matcher;
};

static bool matches_a76_r0p0_3(struct arm_implementation* impl) {
  struct arm_implementation_range a76_r0p0_3 = { 'A', 0xD0B, 0, 0, 0, 3 };
  return impl_in_range(impl, a76_r0p0_3);
}

static bool matches_a72_r0p0_3_or_r1p0(struct arm_implementation* impl) {
  struct arm_implementation_range matches_a72_r0p0_3 = { 'A', 0xD08, 0, 0, 0, 3 };
  struct arm_implementation_range matches_a72_r1p0 = { 'A', 0xD08, 1, 0, 1, 0 };
  return impl_in_range(impl, matches_a72_r0p0_3) || impl_in_range(impl, matches_a72_r1p0);
}

/* clang-format off */
struct errata_workaround workarounds[] = {
{
      .description = "Arm Errata 1286807",
      .label = ERRATA_WORKAROUND_REPEAT_TLBI,
      .matcher = matches_a76_r0p0_3,
    },
    {
      .description = "Arm Errata 1387635",
      .label = ERRATA_WORKAROUND_ISB_AFTER_PTE_ST,
      .matcher = matches_a72_r0p0_3_or_r1p0,
    }
};
/* clang-format on */

void initialise_errata_workarounds(void) {
  struct arm_implementation impl;
  arch_read_implementation(&impl);

  for (int i = 0; i < sizeof(workarounds) / sizeof(workarounds[0]); i++) {
    struct errata_workaround* workaround = &workarounds[i];
    if (workaround->matcher(&impl))
      requires_errata_workaround_map[workaround->label] = true;
  }
}