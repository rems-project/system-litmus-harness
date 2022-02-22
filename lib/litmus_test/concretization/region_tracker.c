#include "lib.h"

/** calculate the offset this region is at in the regions_t
 */
static u64 __region_offset(regions_t* regions, region_trackers_t* trackers, u64* va) {
  return BIT_SLICE((u64)va, 38, 30) - BIT_SLICE((u64)regions, 38, 30);
}

/** calculate the offset this dir is into the region
 */
static u64 __dir_offset(regions_t* regions, region_trackers_t* trackers, u64* va) {
  /* since the region isn't aligned on a 1G boundary
   * but is aligned on a 2M boundary
   * we work out the offset from the start of the regions.
   */
  return BIT_SLICE((u64)va, 29, 21) - BIT_SLICE((u64)regions, 29, 21);
}

static u64 __page_offset(regions_t* regions, region_trackers_t* trackers, u64* va) {
  return BIT_SLICE((u64)va, 20, 12);
}

static u64 __cache_line_offset(regions_t* regions, region_trackers_t* trackers, u64* va) {
  /* the cache-line-of-minimum-size is not fixed in the architecture
   * but it must be a power of 2
   */
  return BIT_SLICE((u64)va, 11, CACHE_LINE_SHIFT);
}

// static u64 __value_offset(regions_t* regions, region_trackers_t* trackers, u64* va) {
//   u64 offset =
//     (u64)va & BITMASK(CACHE_LINE_SHIFT);
//   return offset;
// }

region_tracker_t* tracker_region(regions_t* regions, region_trackers_t* trackers, u64* va) {
  return &trackers->regions[__region_offset(regions, trackers, va)];
}

dir_tracker_t* tracker_dir(regions_t* regions, region_trackers_t* trackers, u64* va) {
  region_tracker_t* reg = tracker_region(regions, trackers, va);
  return &reg->dir_ixs[__dir_offset(regions, trackers, va)];
}

page_tracker_t* tracker_page(regions_t* regions, region_trackers_t* trackers, u64* va) {
  dir_tracker_t* dir = tracker_dir(regions, trackers, va);
  return &dir->page_ixs[__page_offset(regions, trackers, va)];
}

cache_line_tracker_t* tracker_cache_line(regions_t* regions, region_trackers_t* trackers, u64* va) {
  page_tracker_t* pg = tracker_page(regions, trackers, va);
  return &pg->scl_ix[__cache_line_offset(regions, trackers, va)];
}