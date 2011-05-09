#include <cstdlib>
#include <err.h>

void *xmalloc(size_t size)
{
    void *result = malloc(size);
    if (!result)
        errx(1, "malloc failed");
    return result;
}

void *xrealloc(void *old, size_t size)
{
    void *result = realloc(old, size);
    if (!result)
        errx(1, "realloc failed");
    return result;
}
