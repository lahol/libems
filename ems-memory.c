#include "ems-memory.h"
#include <stdlib.h>
#include <memory.h>

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

void *ems_alloc0(size_t size)
{
    void *ptr = ems_alloc(size);
    if (ptr && size)
        memset(ptr, 0, size);
    return ptr;
}

void ems_free(void *ptr)
{
    if (ptr)
        memhandler.free(ptr);
}

void *ems_realloc(void *ptr, size_t size)
{
    if (!size) {
        ems_free(ptr);
        return NULL;
    }
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
