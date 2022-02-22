#include "lib.h"

/** perform a DC CIVAC of the given addr
 *
 * cleaning and invalidating the line that holds addr
 * to the point of coherence.
 */
void dc_civac(u64 addr) {
  asm volatile (
    "dc civac, %[va]\n\t"
  :
  : [va] "r" (addr)
  :
  );
}

/** perform an IC ALLUIS
 *
 * invalidating all entries in all instruction caches
 */
void ic_ialluis(void) {
  asm volatile (
    "ic ialluis\n"
  :
  :
  : "memory"
  );
}


/** flush data caches for an entire range
 */
void flush_data_cache(char* start_addr, char* end_addr) {
  u64 dcacheline = CACHE_LINE_SIZE;

  for (u64 va = (u64)start_addr; va < (u64)end_addr + dcacheline; va += dcacheline) {
    dc_civac(va);
  }

  /* ensure po-later memory events wait for cache clean+invalidates to finish
   */
  dsb();
}

void flush_all_UNKNOWN_reset_caches(void) {
  /* XXX: there is no data cache invalidate+clean all
   * so just assume data cache is clean on reset
   */
  ic_ialluis();
  tlbi_all();
}