#ifndef LITMUS_REGIONS_H
#define LITMUS_REGIONS_H


#define NR_ENTRIES_PER_PAGE 512
typedef struct {
  uint64_t values[NR_ENTRIES_PER_PAGE];
} page_t;

#define NR_PAGES_PER_DIR 512
#define DIR_SIZE (NR_PAGES_PER_DIR * PAGE_SIZE)
typedef struct {
  page_t pages[NR_PAGES_PER_DIR];
} dir_t;

/** a 8M region
 * is split into 4 middle-level dirs
 * each of which contains 512 pages
 * which each contain 4k bytes of data.
 */
#define NR_DIRS_PER_REGION 4
#define REGION_SIZE (NR_DIRS_PER_REGION * NR_PAGES_PER_DIR * PAGE_SIZE)
typedef struct {
  dir_t dirs[NR_DIRS_PER_REGION];
} region_t;

/** heap variable data is split over regions
 *
 * each region covers 1 8M section of memory
 */
#define NR_REGIONS 8
typedef struct {
  region_t regions[NR_REGIONS];
} regions_t;

#endif /* LITMUS_REGIONS_H */