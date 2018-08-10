#include "ems-util-list.h"
#include "ems-memory.h"

EMSList *ems_list_prepend(EMSList *list, void *data)
{
    EMSList *entry = ems_alloc(sizeof(EMSList));
    entry->data = data;
    entry->next = list;
    entry->prev = NULL;
    if (list)
        list->prev = entry;
    return entry;
}
