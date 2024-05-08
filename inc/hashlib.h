#ifndef HASH_H
#define HASH_H

#include "ints.h"

typedef struct {
  u32 digest[5];
} digest;

digest hash_sha1(const char* buf, u64 bufsize);

/**
 *
 */
void hexdigest(digest* d, char* out);

#endif /* HASH_H */
