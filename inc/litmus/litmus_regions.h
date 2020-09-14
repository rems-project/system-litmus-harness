#ifndef LITMUS_REGIONS_H
#define LITMUS_REGIONS_H

typedef enum {
  REGION_VAR,
  REGION_CACHE_LINE,
  REGION_PAGE,
  REGION_PMD,
  REGION_PUD,
  REGION_PGD,
} region_names_t;

extern uint64_t LEVEL_SIZES[6];
extern uint64_t LEVEL_SHIFTS[6];

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


uint8_t in_same_region(uint64_t va1, uint64_t va2, own_level_t size);

#endif /* LITMUS_REGIONS_H */