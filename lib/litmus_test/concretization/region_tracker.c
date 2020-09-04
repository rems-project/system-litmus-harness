#include "lib.h"

static uint64_t __region_offset(regions_t* regions, region_trackers_t* trackers, uint64_t* va) {
  uint64_t offset = (uint64_t)va - (uint64_t)regions;
  offset = offset >> PMD_SHIFT;
  offset = offset / NR_DIRS_PER_REGION;
  return offset;
}

static uint64_t __dir_offset(regions_t* regions, region_trackers_t* trackers, uint64_t* va) {
  uint64_t reg_off = __region_offset(regions, trackers, va);
  uint64_t offset = (uint64_t)va - (uint64_t)regions - (reg_off*REGION_SIZE*sizeof(uint64_t));
  offset = offset >> PAGE_SHIFT;
  offset = offset / NR_PAGES_PER_DIR;
  return offset;
}

static uint64_t __page_offset(regions_t* regions, region_trackers_t* trackers, uint64_t* va) {
  uint64_t reg_off = __region_offset(regions, trackers, va);
  uint64_t dir_off = __dir_offset(regions, trackers, va);
  uint64_t offset = (uint64_t)va - (uint64_t)regions - (reg_off*REGION_SIZE*sizeof(uint64_t)) - (dir_off*DIR_SIZE*sizeof(uint64_t));
  offset = offset >> CACHE_LINE_SHIFT;
  offset = offset / NR_CACHE_LINES_PER_PAGE;
  return offset;
}

static uint64_t __cache_line_offset(regions_t* regions, region_trackers_t* trackers, uint64_t* va) {
  uint64_t reg_off = __region_offset(regions, trackers, va);
  uint64_t dir_off = __dir_offset(regions, trackers, va);
  uint64_t pg_off = __dir_offset(regions, trackers, va);
  uint64_t offset =
    (uint64_t)va
    - (uint64_t)regions
    - (reg_off*REGION_SIZE)
    - (dir_off*DIR_SIZE)
    - (pg_off*PAGE_SIZE);

  offset = offset >> CACHE_LINE_SHIFT;
  offset = offset / NR_CACHE_LINES_PER_PAGE;
  return offset;
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