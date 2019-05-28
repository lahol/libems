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
    EMS_TYPE_ARRAY,
} EMSType;

typedef struct {
    EMSType member_type;
    uint32_t length;
    void *data;
} EMSArray;

void ems_array_init(EMSArray *array, EMSType type, uint32_t length);
void ems_array_copy(EMSArray *dst, EMSArray *src);

void ems_array_clear(EMSArray *array);

void ems_array_set_value_u32(EMSArray *array, uint32_t index, uint32_t value);
void ems_array_set_value_i32(EMSArray *array, uint32_t index, int32_t value);
void ems_array_set_value_u64(EMSArray *array, uint32_t index, uint64_t value);
void ems_array_set_value_i64(EMSArray *array, uint32_t index, int64_t value);
void ems_array_set_value_double(EMSArray *array, uint32_t index, double value);
void ems_array_set_value_pointer(EMSArray *array, uint32_t index, void *value);

uint32_t ems_array_get_value_u32(EMSArray *array, uint32_t index);
int32_t ems_array_get_value_i32(EMSArray *array, uint32_t index);
uint64_t ems_array_get_value_u64(EMSArray *array, uint32_t index);
int64_t ems_array_get_value_i64(EMSArray *array, uint32_t index);
double ems_array_get_value_double(EMSArray *array, uint32_t index);
void *ems_array_get_value_pointer(EMSArray *array, uint32_t index);
