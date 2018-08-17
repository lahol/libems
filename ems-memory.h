/* Handle allocations/frees. */
#pragma once

#include <stdlib.h>

/* These are wrappers to allow other alloc/free functions. */
void *ems_alloc(size_t size);
void ems_free(void *ptr);
void *ems_realloc(void *ptr, size_t size);

/* The memory handler. These have the same signature as the standard
 * glibc malloc/realloc/free functions, which are used by default.
 * However, we provide this mechanism to allow other types of memory management.
 */
typedef struct {
    void *(*alloc)(size_t);
    void *(*realloc)(void *, size_t);
    void (*free)(void *);
} EMSMemoryHandler;

/* Set an alternative memory handler. */
void ems_set_memory_handler(EMSMemoryHandler *handler);
