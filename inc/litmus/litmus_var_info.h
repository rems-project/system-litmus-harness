#ifndef LITMUS_VAR_INFO_H
#define LITMUS_VAR_INFO_H

#include "litmus_idxs.h"
#include "config.h"
#include "abort.h"

typedef struct {
  u8 has_attridx;
  u64 attridx;

  u8 has_ap;
  u64 ap;

  u8 has_region_offset;
  struct {
    var_idx_t offset_var;
    rel_offset_t offset_level;
  } region_offset;
} var_info_attrs_t;

typedef struct {
  u64 val;
  u8 is_identity_mapped;
} var_info_backing_t;


/** heap var info
 *
 * Each Var corresponds to an address in the virtual address space.
 *
 * Those addresses live in the `TESTDATA' MMAP region,
 * and usually linearly mapped into a reserved block of physical TESTDATA space.
 *
 * Each Var has some generic data about the virtual address:
 *  - Its human-readable name and opaque harness identifier (varindex)
 *  - (Optionally) some access permissions
 *  - (Optionally) a manual MAIR AttrIndx
 *  - (Optionally) a fixed offset (i.e. at same index in cacheline/page/pmd etc)
 *                 as a VAR_HEAP (see below)
 *
 * A Var can be one of:
 * - VAR_HEAP
 *    A (mostly) unconstrained variable, which owns a region of testdata memory.
 *    The owned region size may be anything from a single cacheline to a whole GiB of memory.
 *    Can be placed at the same offset as another heap variable.
 * - VAR_ALIAS
 *    Like a heap variable, but instead is mapped to the same PA as a heap variable.
 * - VAR_PINNED
 *    A constrained variable, that exists inside the owned region of a heap variable.
 *    e.g. in the same cacheline or page or whatever.
 *    Can be placed at the same offset as another heap variable.
 * - VAR_FIXED
 *    The physical address is always mapped to some fixed value.
 *    NOTE that this just means that the whole page is mapped to some other whole page,
 *    the AArch64 architecture does not permit finer grained mapping.
 *
 * The variables that are backed by some allocated physical memory (VAR_HEAP and VAR_PIN),
 * can be set to be identity mapped.
 */
typedef enum {
  VAR_NOTSET,  /* unused */
  VAR_HEAP,
  VAR_ALIAS,
  VAR_PINNED,
  VAR_FIXED,
  VAR_UNMAPPED,
} var_info_type_t;
const char *var_info_type_to_str(var_info_type_t ty);

typedef struct {
  var_idx_t varidx;
  const char* name;

  /**
   * attributes of the virtual address,
   * these are generic over all types
   */
  var_info_attrs_t init_attrs;

  var_info_type_t ty;
  union {
    struct {
      var_info_backing_t back;
      own_level_t owned_region_size;
    } heap;

    struct {
      var_idx_t aliased_with;
    } alias;

    struct {
      var_info_backing_t back;
      var_idx_t pin_region_var;
      pin_level_t pin_region_level;
    } pin;

    struct {
      u64 phys;
    } fixed;
  };

  /** array of pointers into memory region for each run
   *
   * this is what actually defines the concrete tests
   */
  u64** values;
} var_info_t;

/**
 * debug print a single varinfo
 * given the array of all the vars to lookup indexes in
 */
void debug_print_var_info(var_info_t *vinfo, var_info_t *infos);

static inline u8 is_backed_var(var_info_t *vinfo) {
  return (vinfo->ty == VAR_HEAP) || (vinfo->ty == VAR_PINNED);
}

/**
 * Whether this var_info_t owns a chunk of the (virtual) address space,
 * in which other vars should not be allocated in
 * (except when explicitly pinned)
 */
static inline u8 var_owns_region(var_info_t *vinfo) {
  return (
       (vinfo->ty == VAR_HEAP)
    || (vinfo->ty == VAR_UNMAPPED)
    || (vinfo->ty == VAR_FIXED)
    || (vinfo->ty == VAR_ALIAS)
  );
}

/**
 * Whether this var_info_t owns a chunk of the (physical) address space,
 * in which other vars should not be allocated in
 * (except when explicitly pinned)
 */
static inline u8 var_owns_phys_region(var_info_t *vinfo) {
  return (
       (vinfo->ty == VAR_HEAP)
    || (vinfo->ty == VAR_FIXED)
  );
}
static inline own_level_t var_owned_region_size(var_info_t *vinfo) {
  fail_on(!var_owns_region(vinfo), "cannot get owned region size of \"%s\"", vinfo->name);
  return vinfo->ty == VAR_HEAP ? vinfo->heap.owned_region_size : REGION_OWN_PAGE;
}

static inline var_info_backing_t *var_backing(var_info_t *vinfo) {
  fail_on(!is_backed_var(vinfo), "can't get backing of unbacked var");
  return vinfo->ty == VAR_HEAP ? &vinfo->heap.back : &vinfo->pin.back;
}

#endif /* LITMUS_VAR_INFO_H */