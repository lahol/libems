#include "ems-memory.h"
#include <stdlib.h>

static EMSMemoryHandler memhandler = {
    .alloc   = malloc,
    .realloc = realloc,
    .free    = free
};

void *ems_alloc(size_t size)
{
    void *ptr = memhandler.alloc(size);
    if (!ptr && size)
        exit(1);
    return ptr;
}

void ems_free(void *ptr)
{
    if (ptr)
        memhandler.free(ptr);
}

void *ems_realloc(void *ptr, size_t size)
{
    ptr = memhandler.realloc(ptr, size);
    if (!ptr && size)
        exit(1);
    return ptr;
}

void ems_set_memory_handler(EMSMemoryHandler *handler)
{
    if (handler) {
        memhandler.alloc = handler->alloc ? handler->alloc : malloc;
        memhandler.realloc = handler->realloc ? handler->realloc : realloc;
        memhandler.free = handler->free ? handler->free : free;
    }
    else {
        memhandler.alloc = malloc;
        memhandler.realloc = realloc;
        memhandler.free = free;
    }
}
