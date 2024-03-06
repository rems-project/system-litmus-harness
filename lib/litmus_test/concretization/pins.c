#include "lib.h"

var_idx_t count_pinned_to(var_info_t** out_vinfos, test_ctx_t* ctx, var_info_t* var, pin_level_t lvl) {
  var_info_t* othervar;
  var_idx_t count = 0;

  FOREACH_HEAP_VAR(ctx, othervar) {
    if ( (var->varidx != othervar->varidx)
         && othervar->ty == VAR_PINNED
         && othervar->pin.pin_region_var == var->varidx
         && othervar->pin.pin_region_level == lvl
    ) {
      out_vinfos[count] = othervar;
      count += 1;
    }
  }

  return count;
}