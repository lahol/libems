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

EMSList *ems_list_reverse(EMSList *list)
{
    EMSList *tmp;
    EMSList *new_head = list;
    while (list) {
        new_head = list;
        tmp = list->next;
        list->next = list->prev;
        list->prev = tmp;
        list = list->prev;
    }
    return new_head;
}

void ems_list_free_full(EMSList *list, EMSDestroyNotifyFunc notify)
{
    EMSList *tmp;
    while (list) {
        tmp = list->next;
        if (notify)
            notify(list->data);
        ems_free(list);
        list = tmp;
    }
}

EMSList *ems_list_delete_link(EMSList *list, EMSList *link)
{
    if (!link || !list)
        return list;

    if (link->prev)
        link->prev->next = link->next;
    else
        list = link->next;
    if (link->next)
        link->next->prev = link->prev;

    ems_free(link);

    return list;
}
