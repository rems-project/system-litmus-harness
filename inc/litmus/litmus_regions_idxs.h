#ifndef LITMUS_REGIONS_IDXS_H
#define LITMUS_REGIONS_IDXS_H

#include "types.h"
#include "litmus_test_ctx.h"

/* keeps track of a VA in the harness testdata VA space
 * as offsets into the region
 */
typedef struct
{
  u64 reg_ix;
  u64 reg_offs;
} region_idx_t;
#define REPR_ARGS_region_idx_t u64, reg_ix, u64, reg_offs
#define PRregion_idx_t "%o"
#define PRTyregion_idx_t obj

region_idx_t align_down_region_idx(region_idx_t idx, pin_level_t alignment);
region_idx_t align_up_region_idx(region_idx_t idx, pin_level_t alignment);

u64 va_from_region_idx(test_ctx_t* ctx, var_info_t* var, region_idx_t idx);

region_idx_t region_idx_top(void);
region_idx_t region_idx_bottom(void);

#endif /* LITMUS_REGIONS_IDXS_H */