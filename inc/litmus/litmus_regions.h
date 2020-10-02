#ifndef LITMUS_REGIONS_H
#define LITMUS_REGIONS_H

#include <stdint.h>

#include "litmus_test_def.h"

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

/** each region is one pmd (or page-middle-directory)
 * each pmd entry points to 1 2M region, so each pmd is 1G aligned
 *
 * we ask QEMU to allocate 1 G of mem right now
 * but we cannote use all of that for the regions
 * so we cannot allocate a whole 1G of of dirs
 *
 * instead owe allocate 8 M which is more than enough for the current tests
 *
 * For now this region is physically contiguous, in future we may
 * want to make this a virtual array where only some of the points
 * are mapped in, allowing tests over a wider range of VAs.
 */
#define NR_DIRS_PER_REGION 4
#define REGION_SHIFT (2 + PMD_SHIFT)
#define REGION_SIZE (NR_DIRS_PER_REGION * NR_PAGES_PER_DIR * PAGE_SIZE)
typedef struct {
  dir_t dirs[NR_DIRS_PER_REGION];
} region_t;

/** heap variable data is split over regions
 *
 * each region covers a different pud (or page-upper-directory)
 * they may not cover an entire pud, and different regions may not be contiguous
 * so here we store a reference to the start of each
 *
 * right now each region is 8M
 * and we allocate 16 of them for the tests
 * for a total of 64M of test data space to play with
 */
#define NR_REGIONS 16
#define NR_REGIONS_SHIFT 4
typedef struct {
  region_t* regions[NR_REGIONS];
} regions_t;

/**
 * perform a static initialization of a regions_t
 * with the known VA ranges for each region
 */
void initialize_regions(regions_t* r);

uint8_t in_same_region(uint64_t va1, uint64_t va2, own_level_t size);

#endif /* LITMUS_REGIONS_H */