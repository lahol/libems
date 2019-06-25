#pragma once

#include <stdint.h>

typedef enum {
    EMS_TYPE_UNKNOWN = 0,
    EMS_TYPE_UINT,
    EMS_TYPE_INT,
    EMS_TYPE_UINT64,
    EMS_TYPE_INT64,
    EMS_TYPE_DOUBLE,
    EMS_TYPE_POINTER,
    EMS_TYPE_FIXED_STRING,
    EMS_TYPE_STRING,
} EMSType;
