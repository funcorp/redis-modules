#ifndef REDIS_MODULES_ZREVMERGE_H
#define REDIS_MODULES_ZREVMERGE_H

#include <stddef.h>

void *ZRevMerge_Realloc(void *ptr, size_t bytes);
void *ZRevMerge_Alloc(size_t bytes);
void ZRevMerge_FreeString(void *ctx, void *str);
void ZRevMerge_Free(void *ptr);

#endif //REDIS_MODULES_ZREVMERGE_H
