#pragma once

typedef struct _EMSList EMSList;

struct _EMSList {
    void *data;
    EMSList *prev;
    EMSList *next;
};

EMSList *ems_list_prepend(EMSList *list, void *data);
EMSList *ems_list_reverse(EMSList *list);
EMSList *ems_list_delete_link(EMSList *list, EMSList *link);

typedef void (*EMSDestroyNotifyFunc)(void *);
void ems_list_free_full(EMSList *list, EMSDestroyNotifyFunc notify);
