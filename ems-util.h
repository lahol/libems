#pragma once

#include <stdint.h>

#define ems_likely(x)   __builtin_expect((x), 1)
#define ems_unlikely(x) __builtin_expect((x), 0)

typedef void *(*PThreadCallback)(void *);

#define EMS_UTIL_POINTER_TO_INT(p) ((int)(long)(p))

typedef enum {
    EMS_SOCKET_TYPE_UNKNOWN = 0,
    EMS_SOCKET_TYPE_CONTROL = 1,
    EMS_SOCKET_TYPE_MASTER,
    EMS_SOCKET_TYPE_DATA
} EMSSocketType;

typedef struct {
    int fd;
    EMSSocketType type;
    uint32_t id;
} EMSSocketInfo;

#include "ems-util-list.h"
#include "ems-util-fd.h"
