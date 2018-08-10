#pragma once

#define ems_likely(x)   __builtin_expect((x), 1)
#define ems_unlikely(x) __builtin_expect((x), 0)

#include "ems-util-list.h"
