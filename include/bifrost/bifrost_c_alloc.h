
#ifndef BIFROST_MALLOC
#define BIFROST_MALLOC(size, align) malloc((size))
#define BIFROST_REALLOC(ptr, new_size, align) realloc((ptr), new_size)
#define BIFROST_FREE(ptr) free((ptr))
#endif
