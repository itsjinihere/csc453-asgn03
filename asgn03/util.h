#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>

void *xmalloc(size_t nbytes);
void dawdle_ms(int max_ms);
int  parse_nonneg(const char *s, int *out);

#endif
