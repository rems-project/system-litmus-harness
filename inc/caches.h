#ifndef CACHES_H
#define CACHES_H

#include "lib.h"

/** perform a DC CIVAC of the given addr
 */
void dc_civac(u64 addr);

/** flush caches for an entire range
 */
void flush_data_cache(char* start_addr, char* end_addr);

/** flush all caches that may contain stale
 * values on reset
 */
void flush_all_UNKNOWN_reset_caches(void);

#endif /* CACHES_H */