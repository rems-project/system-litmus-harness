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

  u8 has_init_value;
  u64 init_value;

  u8 init_set_ap;
  u64 init_ap;

  u8 init_set_attridx;
  u64 init_attridx;

  u8 init_unmapped;

  u8 init_pinned_region;
  u8 init_owns_region;
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
  u8 init_region_offset;
  struct {
    var_idx_t offset_var;
    rel_offset_t offset_level;
  };

  /** if aliased, the name of the variable this one aliases
   */
  u8 is_alias;
  var_idx_t alias;

  /** if fixed, the phyiscal addr
   */
  u8 is_fixed;
  u64 fixed_pa;

  /* whether this variable is mapped by an identity mapping
   * in the pagetable
   */
  u8 id_mapped;

  /** array of pointers into memory region for each run
   *
   * this is what actually defines the concrete tests
   */
  u64** values;
} var_info_t;

#endif /* LITMUS_VAR_INFO_H */