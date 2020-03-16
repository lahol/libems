#include "ems-msg-queue.h"
#include <memory.h>
#include "ems-memory.h"
#include "ems-util.h"
#include "ems-messages-internal.h"

struct _EMSMessageQueueEntry {
    EMSMessage *data;
    struct _EMSMessageQueueEntry *prev;
    struct _EMSMessageQueueEntry *next;
};

void ems_message_queue_init(EMSMessageQueue *mq)
{
    memset(mq, 0, sizeof(EMSMessageQueue));

    mq->filter_max = 32;
    mq->filters = ems_alloc(sizeof(uint32_t) * mq->filter_max);

    pthread_mutex_init(&mq->queue_lock, NULL);
}

void ems_message_queue_clear(EMSMessageQueue *mq)
{
    if (mq) {
        ems_free(mq->filters);
        pthread_mutex_destroy(&mq->queue_lock);

        EMSMessageQueueEntry *tmp;;
        while (mq->head) {
            tmp = mq->head->next;
            ems_message_unref(mq->head->data);
            ems_message_unref(mq->priv);
            ems_free(mq->head);
            mq->head = tmp;
        }

        memset(mq, 0, sizeof(EMSMessageQueue));
    }
}

void ems_message_queue_add_filter(EMSMessageQueue *mq, uint32_t msgtype)
{
    if (ems_unlikely(!mq))
        return;

    pthread_mutex_lock(&mq->queue_lock);
    int j;
    for (j = 0; j < mq->filter_count; ++j) {
        if (mq->filters[j] == msgtype) /* Filter is already in list. */
            goto done;
    }
    if (mq->filter_count == mq->filter_max) {
        mq->filter_max += 32;
        mq->filters = ems_realloc(mq->filters, sizeof(uint32_t) * mq->filter_max);
    }
    mq->filters[mq->filter_count++] = msgtype;

done:
    pthread_mutex_unlock(&mq->queue_lock);
}

void ems_message_queue_clear_filter(EMSMessageQueue *mq)
{
    if (ems_unlikely(!mq))
        return;
    pthread_mutex_lock(&mq->queue_lock);
    mq->filter_count = 0;
    pthread_mutex_unlock(&mq->queue_lock);
}

void ems_message_queue_push_tail(EMSMessageQueue *mq, EMSMessage *msg)
{
    if (ems_unlikely(!mq))
        return;
    EMSMessageQueueEntry *entry = ems_alloc(sizeof(EMSMessageQueueEntry));
    entry->data = msg;
    entry->next = NULL;

    pthread_mutex_lock(&mq->queue_lock);

    entry->prev = mq->tail;
    if (mq->tail)
        mq->tail->next = entry;
    else
        mq->head = entry;
    mq->tail = entry;

    ++mq->count;

    pthread_mutex_unlock(&mq->queue_lock);
}

void ems_message_queue_push_head(EMSMessageQueue *mq, EMSMessage *msg)
{
    if (ems_unlikely(!mq))
        return;
    EMSMessageQueueEntry *entry = ems_alloc(sizeof(EMSMessageQueueEntry));
    entry->data = msg;
    entry->prev = NULL;

    pthread_mutex_lock(&mq->queue_lock);

    entry->next = mq->head;
    if (mq->head)
        mq->head->prev = entry;
    else
        mq->tail = entry;
    mq->head = entry;

    ++mq->count;

    pthread_mutex_unlock(&mq->queue_lock);
}

static inline
EMSMessage *_ems_message_queue_pop_head_unsafe(EMSMessageQueue *mq)
{
    EMSMessage *msg = NULL;
    EMSMessageQueueEntry *tmp;
    if (mq->head) {
        tmp = mq->head->next;
        msg = mq->head->data;
        ems_free(mq->head);
        mq->head = tmp;
        if (mq->head)
            mq->head->prev = NULL;
        else
            mq->tail = NULL;
        --mq->count;
    }
    return msg;
}

static inline
void _ems_message_queue_remove_entry_unsafe(EMSMessageQueue *mq, EMSMessageQueueEntry *entry)
{
    if (entry->next)
        entry->next->prev = entry->prev;
    else
        mq->tail = entry->prev;
    if (entry->prev)
        entry->prev->next = entry->next;
    else
        mq->head = entry->next;
    ems_free(entry);
    --mq->count;
}

EMSMessage *ems_message_queue_pop_head(EMSMessageQueue *mq)
{
    if (ems_unlikely(!mq))
        return NULL;

    EMSMessage *msg = NULL;

    pthread_mutex_lock(&mq->queue_lock);
    msg = _ems_message_queue_pop_head_unsafe(mq);

    if (!msg) {
        ems_message_ref(mq->priv);
        msg = mq->priv;
    }

    pthread_mutex_unlock(&mq->queue_lock);

    return msg;
}

EMSMessage *ems_message_queue_peek_head(EMSMessageQueue *mq)
{
    if (ems_unlikely(!mq))
        return NULL;

    EMSMessage *msg = NULL;

    pthread_mutex_lock(&mq->queue_lock);
    if (mq->head)
        msg = mq->head->data;
    else
        msg = mq->priv;
    pthread_mutex_unlock(&mq->queue_lock);
    
    return msg;
}

EMSMessage *ems_message_queue_pop_filtered(EMSMessageQueue *mq)
{
    if (ems_unlikely(!mq))
        return NULL;
    EMSMessage *msg = NULL;
    EMSMessageQueueEntry *tmp;
    size_t j;

    pthread_mutex_lock(&mq->queue_lock);
    if (!mq->filter_count) {
        msg = _ems_message_queue_pop_head_unsafe(mq);
    }
    else {
        for (tmp = mq->head; tmp; tmp = tmp->next) {
            for (j = 0; j < mq->filter_count; ++j) {
                if (tmp->data->type == mq->filters[j]) {
                    goto found;
                }
            }
        }
        goto done;
found:
        msg = tmp->data;
        _ems_message_queue_remove_entry_unsafe(mq, tmp);
    }

done:
    if (!msg) {
        ems_message_ref(mq->priv);
        msg = mq->priv;
    }
    pthread_mutex_unlock(&mq->queue_lock);

    return msg;
}

EMSMessage *ems_message_queue_pop_matching(EMSMessageQueue *mq, EMSMessageFilterFunc filter, void *userdata)
{
    if (ems_unlikely(!mq))
        return NULL;

    EMSMessage *msg = NULL;
    EMSMessageQueueEntry *tmp;

    pthread_mutex_lock(&mq->queue_lock);
    if (ems_unlikely(!filter)) {
        msg = _ems_message_queue_pop_head_unsafe(mq);
    }
    else {
        for (tmp = mq->head; tmp; tmp = tmp->next) {
            if (filter(tmp->data, userdata) == 0)
                goto found;
        }
        goto done;
found:
        msg = tmp->data;
        _ems_message_queue_remove_entry_unsafe(mq, tmp);
    }

done:
    if (!msg) {
        ems_message_ref(mq->priv);
        msg = mq->priv;
    }
    pthread_mutex_unlock(&mq->queue_lock);

    return msg;
}

EMSMessage *ems_message_queue_peek_filtered(EMSMessageQueue *mq)
{
    if (ems_unlikely(!mq))
        return NULL;

    EMSMessage *msg = NULL;
    EMSMessageQueueEntry *tmp;
    size_t j;

    pthread_mutex_lock(&mq->queue_lock);
    if (!mq->filter_count)
        msg = mq->head ? mq->head->data : NULL;
    else {
        for (tmp = mq->head; tmp; tmp = tmp->next) {
            for (j = 0; j < mq->filter_count; ++j) {
                if (tmp->data->type == mq->filters[j]) {
                    msg = tmp->data;
                    goto done;
                }
            }
        }
    }
done:
    if (!msg) {
        msg = mq->priv;
    }
    pthread_mutex_unlock(&mq->queue_lock);

    return msg;
}

void ems_message_queue_enable(EMSMessageQueue *mq, int enable)
{
    if (ems_unlikely(!mq))
        return;

    pthread_mutex_lock(&mq->queue_lock);

    if (enable) {
        ems_message_unref(mq->priv);
        mq->priv = NULL;
    }
    else {
        if (!mq->priv)
            mq->priv = ems_message_new(__EMS_MESSAGE_QUEUE_DISABLED,
                                       EMS_MESSAGE_RECIPIENT_ALL,
                                       EMS_MESSAGE_RECIPIENT_ALL,
                                       NULL, NULL);
    }

    pthread_mutex_unlock(&mq->queue_lock);
}
