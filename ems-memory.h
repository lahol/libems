#pragma once

#include <stdlib.h>

/* These are wrappers to allow other alloc/free functions. */
void *ems_alloc(size_t size);
void ems_free(void *ptr);
void *ems_realloc(void *ptr, size_t size);

typedef struct {
    void *(*alloc)(size_t);
    void *(*realloc)(void *, size_t);
    void (*free)(void *);
} EMSMemoryHandler;

void ems_set_memory_handler(EMSMemoryHandler *handler);
