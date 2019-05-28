#include "ems-memory.h"
#include "ems-types.h"
#include <memory.h>

static size_t _ems_element_sizes[] = {
    0, /* EMS_TYPE_UNKNOWN */
    sizeof(uint32_t),
    sizeof(int32_t),
    sizeof(uint64_t),
    sizeof(int64_t),
    sizeof(double),
    sizeof(void *),
    sizeof(char *),
    sizeof(char *),
    sizeof(EMSArray)
};

void ems_array_init(EMSArray *array, EMSType type, uint32_t length)
{
    if (array && type <= EMS_TYPE_ARRAY) {
        array->member_type = type;
        array->length = length;
        array->data = ems_alloc0(length * _ems_element_sizes[type]);
    }
}

void ems_array_copy(EMSArray *dst, EMSArray *src)
{
    if (dst && src) {
        ems_free(dst->data);
        dst->member_type = src->member_type;
        dst->length = src->length;
        dst->data = ems_alloc(dst->length * _ems_element_sizes[dst->member_type]);
        memcpy(dst->data, src->data, dst->length * _ems_element_sizes[dst->member_type]);
    }
    else if (dst) {
        ems_array_init(dst, 0, 0);
    }
}

void ems_array_clear(EMSArray *array)
{
    if (array) {
        ems_free(array->data);
        memset(array, 0, sizeof(EMSArray));
    }
}

void ems_array_set_value_u32(EMSArray *array, uint32_t index, uint32_t value)
{
    if (array && array->member_type == EMS_TYPE_UINT && index < array->length)
        ((uint32_t *)array->data)[index] = value;
}

void ems_array_set_value_i32(EMSArray *array, uint32_t index, int32_t value)
{
    if (array && array->member_type == EMS_TYPE_INT && index < array->length)
        ((int32_t *)array->data)[index] = value;
}

void ems_array_set_value_u64(EMSArray *array, uint32_t index, uint64_t value)
{
    if (array && array->member_type == EMS_TYPE_UINT64 && index < array->length)
        ((uint64_t *)array->data)[index] = value;
}

void ems_array_set_value_i64(EMSArray *array, uint32_t index, int64_t value)
{
    if (array && array->member_type == EMS_TYPE_INT64 && index < array->length)
        ((int64_t *)array->data)[index] = value;
}

void ems_array_set_value_double(EMSArray *array, uint32_t index, double value)
{
    if (array && array->member_type == EMS_TYPE_DOUBLE && index < array->length)
        ((double *)array->data)[index] = value;
}

void ems_array_set_value_pointer(EMSArray *array, uint32_t index, void *value)
{
    if (array && array->member_type == EMS_TYPE_POINTER && index < array->length)
        ((void **)array->data)[index] = value;
}

uint32_t ems_array_get_value_u32(EMSArray *array, uint32_t index)
{
    if (array && array->member_type == EMS_TYPE_UINT && index < array->length)
        return ((uint32_t *)array->data)[index];
    return 0;
}

int32_t ems_array_get_value_i32(EMSArray *array, uint32_t index)
{
    if (array && array->member_type == EMS_TYPE_INT && index < array->length)
        return ((int32_t *)array->data)[index];
    return 0;
}

uint64_t ems_array_get_value_u64(EMSArray *array, uint32_t index)
{
    if (array && array->member_type == EMS_TYPE_UINT64 && index < array->length)
        return ((uint64_t *)array->data)[index];
    return 0;
}

int64_t ems_array_get_value_i64(EMSArray *array, uint32_t index)
{
    if (array && array->member_type == EMS_TYPE_INT64 && index < array->length)
        return ((int64_t *)array->data)[index];
    return 0;
}

double ems_array_get_value_double(EMSArray *array, uint32_t index)
{
    if (array && array->member_type == EMS_TYPE_DOUBLE && index < array->length)
        return ((double *)array->data)[index];
    return 0;
}

void *ems_array_get_value_pointer(EMSArray *array, uint32_t index)
{
    if (array && array->member_type == EMS_TYPE_POINTER && index < array->length)
        return ((void **)array->data)[index];
    return NULL;
}
