#ifndef LITMUS_VAR_INFO_H
#define LITMUS_VAR_INFO_H

#include "config.h"

/** heap var info
 *
 */
typedef struct {
  int varidx;
  const char* name;
  uint64_t init_value;
  uint64_t init_ap;
  uint64_t init_unmapped;


  uint8_t init_region_pinned;
  union {
    /** if the region is pinned then this var is pinned to another var with some region offset */
    struct {
      const char* pin_region_var;
      pin_level_t pin_region_level;
    };

    /** if not pinned, then this var can move about freely */
    struct {
      uint64_t curr_region;
    };
  };

  uint8_t init_owns_region;
  own_level_t init_owned_region_size;

  /** if aliased, the name of the variable this one aliases
   */
  const char* alias;

  /** array of pointers into memory region for each run
   *
   * this is what actually defines the concrete tests
   */
  uint64_t** values;
} var_info_t;

#endif /* LITMUS_VAR_INFO_H */