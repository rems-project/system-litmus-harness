#ifndef RE_H
#define RE_H

#include "re_types.h"

re_t* re_compile(const char* expr);
int re_matches(re_t* re, const char* input);
void re_free(re_t* re);

#endif /* RE_H */