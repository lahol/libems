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
} EMSMessageQueue;

void ems_message_queue_init(EMSMessageQueue *mq);
void ems_message_queue_clear(EMSMessageQueue *mq);
void ems_message_queue_add_filter(EMSMessageQueue *mq, uint32_t msgtype);
void ems_message_queue_clear_filter(EMSMessageQueue *mq);

void ems_message_queue_push_tail(EMSMessageQueue *mq, EMSMessage *msg);
void ems_message_queue_push_head(EMSMessageQueue *mq, EMSMessage *msg);

EMSMessage *ems_message_queue_pop_head(EMSMessageQueue *mq);
EMSMessage *ems_message_queue_peek_head(EMSMessageQueue *mq);
EMSMessage *ems_message_queue_pop_filtered(EMSMessageQueue *mq);
EMSMessage *ems_message_queue_peek_filtered(EMSMessageQueue *mq);
