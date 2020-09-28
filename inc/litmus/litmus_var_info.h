#ifndef LITMUS_VAR_INFO_H
#define LITMUS_VAR_INFO_H

#include "litmus_idxs.h"
#include "config.h"

/** heap var info
 *
 */
typedef struct {
  var_idx_t varidx;
  const char* name;
  uint64_t init_value;
  uint64_t init_ap;
  uint64_t init_unmapped;

  uint8_t init_pinned_region;
  uint8_t init_owns_region;
  union {
    /** if not pinned, then this var owns a region */
    own_level_t init_owned_region_size;

    /** if the region is pinned then this var is pinned to another var within some offset */
    struct {
      var_idx_t pin_region_var;
      pin_level_t pin_region_level;
    };
  };

  /* optionally each var may have a fixed offset in relation to 1 other var */
  uint8_t init_region_offset;
  struct {
    var_idx_t offset_var;
    rel_offset_t offset_level;
  };

  /** if aliased, the name of the variable this one aliases
   */
  uint8_t is_alias;
  var_idx_t alias;

  /** array of pointers into memory region for each run
   *
   * this is what actually defines the concrete tests
   */
  uint64_t** values;
} var_info_t;

#endif /* LITMUS_VAR_INFO_H */