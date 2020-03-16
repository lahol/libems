/* A thread aware message queue.
 * A message queue may be used savely by multiple threads.
 * We could have done this in a more generic way. However, we
 * will only ever use it here for messages.
 * It is possible to add a filter for specific message types
 * so that other messages in the queue are ignored. This may be
 * used to get some form of priority queue.
 */
#pragma once

#include "ems-message.h"
#include <pthread.h>

typedef struct _EMSMessageQueueEntry EMSMessageQueueEntry;

typedef struct {
    EMSMessageQueueEntry *head;
    EMSMessageQueueEntry *tail;
    uint64_t count;              /* number of elements in queue */
    uint32_t *filters;           /* array of msgtypes */
    size_t filter_count;         /* number of active filters */
    size_t filter_max;           /* maximal number of filters */
    pthread_mutex_t queue_lock;  /* lock the queue */
    void *priv;                  /* private, do not read or write here */
} EMSMessageQueue;

/* Initialize the queue. */
void ems_message_queue_init(EMSMessageQueue *mq);

/* Free used resources. */
void ems_message_queue_clear(EMSMessageQueue *mq);

/* Add a filter for some message type. */
void ems_message_queue_add_filter(EMSMessageQueue *mq, uint32_t msgtype);

/* Clear all filters. */
void ems_message_queue_clear_filter(EMSMessageQueue *mq);

/* Push a message to the end of the queue. */
void ems_message_queue_push_tail(EMSMessageQueue *mq, EMSMessage *msg);

/* Push a message to the start of the queue. */
void ems_message_queue_push_head(EMSMessageQueue *mq, EMSMessage *msg);

/* Get a message from the start of the head. */
EMSMessage *ems_message_queue_pop_head(EMSMessageQueue *mq);

/* Check the next message, but do not remove it from the queue. */
EMSMessage *ems_message_queue_peek_head(EMSMessageQueue *mq);

/* Get the next message matching the specified filter. */
EMSMessage *ems_message_queue_pop_filtered(EMSMessageQueue *mq);

/* Check if there is a message matching the specified filter. */
EMSMessage *ems_message_queue_peek_filtered(EMSMessageQueue *mq);

/* If the queue is disabled and the queue is empty, the special message EMSMessageQueueDisabled
 * will be returned. Other messages still on the queue will be delivered as usual. */
void ems_message_queue_enable(EMSMessageQueue *mq, int enable);

/* This function gets a message and user-defined data.
 * It shall return 0 if the message matches the filter criteria.
 */
typedef int (*EMSMessageFilterFunc)(EMSMessage *, void *);
EMSMessage *ems_message_queue_pop_matching(EMSMessageQueue *mq, EMSMessageFilterFunc filter, void *userdata);
