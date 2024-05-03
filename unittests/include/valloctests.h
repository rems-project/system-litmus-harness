#ifndef VALLOCTESTS_H
#define VALLOCTESTS_H

#include "lib.h"

int valloc_freelist_hash(valloc_free_chunk* freelist);

#define UNCHANGED(m1, m2) (((m1).top == (m2).top) && ((m1).freelist == (m2).freelist))

#endif /* VALLOCTESTS_H */