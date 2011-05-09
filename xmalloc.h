#ifndef XMALLOC_H__
#define XMALLOC_H__

#include <cstdlib>

void *xmalloc(size_t size);

void *xrealloc(void *old, size_t size);

#endif
