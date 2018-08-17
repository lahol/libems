/* Some utilities used throughout the library. */
#pragma once

#include <stdint.h>

#define ems_likely(x)   __builtin_expect((x), 1)
#define ems_unlikely(x) __builtin_expect((x), 0)

typedef void *(*PThreadCallback)(void *);

#define EMS_UTIL_POINTER_TO_INT(p) ((int)(long)(p))

#include "ems-util-list.h"
#include "ems-util-fd.h"
