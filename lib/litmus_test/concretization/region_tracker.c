#include "lib.h"

/** calculate the offset this region is at in the regions_t
 */
static uint64_t __region_offset(regions_t* regions, region_trackers_t* trackers, uint64_t* va) {
  return BIT_SLICE((uint64_t)va, 38, 30) - BIT_SLICE((uint64_t)regions, 38, 30);
}

/** calculate the offset this dir is into the region
 */
static uint64_t __dir_offset(regions_t* regions, region_trackers_t* trackers, uint64_t* va) {
  /* since the region isn't aligned on a 1G boundary
   * but is aligned on a 2M boundary
   * we work out the offset from the start of the regions.
   */
  return BIT_SLICE((uint64_t)va, 29, 21) - BIT_SLICE((uint64_t)regions, 29, 21);
}

static uint64_t __page_offset(regions_t* regions, region_trackers_t* trackers, uint64_t* va) {
  return BIT_SLICE((uint64_t)va, 20, 12);
}

static uint64_t __cache_line_offset(regions_t* regions, region_trackers_t* trackers, uint64_t* va) {
  /* the cache-line-of-minimum-size is not fixed in the architecture
   * but it must be a power of 2
   */
  return BIT_SLICE((uint64_t)va, 11, CACHE_LINE_SHIFT);
}

// static uint64_t __value_offset(regions_t* regions, region_trackers_t* trackers, uint64_t* va) {
//   uint64_t offset =
//     (uint64_t)va & BITMASK(CACHE_LINE_SHIFT);
//   return offset;
// }

region_tracker_t* tracker_region(regions_t* regions, region_trackers_t* trackers, uint64_t* va) {
  return &trackers->regions[__region_offset(regions, trackers, va)];
}

dir_tracker_t* tracker_dir(regions_t* regions, region_trackers_t* trackers, uint64_t* va) {
  region_tracker_t* reg = tracker_region(regions, trackers, va);
  return &reg->dir_ixs[__dir_offset(regions, trackers, va)];
}

page_tracker_t* tracker_page(regions_t* regions, region_trackers_t* trackers, uint64_t* va) {
  dir_tracker_t* dir = tracker_dir(regions, trackers, va);
  return &dir->page_ixs[__page_offset(regions, trackers, va)];
}

cache_line_tracker_t* tracker_cache_line(regions_t* regions, region_trackers_t* trackers, uint64_t* va) {
  page_tracker_t* pg = tracker_page(regions, trackers, va);
  return &pg->scl_ix[__cache_line_offset(regions, trackers, va)];
}