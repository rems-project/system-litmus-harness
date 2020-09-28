#ifndef CACHES_H
#define CACHES_H

#include "lib.h"

/** perform a DC CIVAC of the given addr
 */
void dc_civac(uint64_t addr);

/** flush caches for an entire range
 */
void flush_data_cache(char* start_addr, char* end_addr);

#endif /* CACHES_H */