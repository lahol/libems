/* A generic doubly-linked list. */
#pragma once

typedef struct _EMSList EMSList;

struct _EMSList {
    void *data;
    EMSList *prev;
    EMSList *next;
};

/* Add data to the beginning of list and return the new head of the list. */
EMSList *ems_list_prepend(EMSList *list, void *data);

/* Reverse list and return the new head. */
EMSList *ems_list_reverse(EMSList *list);

/* Delete some link from list and return the new head. */
EMSList *ems_list_delete_link(EMSList *list, EMSList *link);

/* Free the whole list. Call notify for each data element. */
typedef void (*EMSDestroyNotifyFunc)(void *);
void ems_list_free_full(EMSList *list, EMSDestroyNotifyFunc notify);
