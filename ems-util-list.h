#pragma once

typedef struct _EMSList EMSList;

struct _EMSList {
    void *data;
    EMSList *prev;
    EMSList *next;
};

EMSList *ems_list_prepend(EMSList *list, void *data);
